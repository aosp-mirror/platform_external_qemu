// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GLSnapshotTesting.h"
#include "Standalone.h"
#include "samples/HelloTriangle.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include <gtest/gtest.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace emugl {

using android::base::LazyInstance;
using android::base::StdioStream;
using android::base::System;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

static const GLESv2Dispatch* getSnapshotTestDispatch();

// static
class SnapshotTestDispatch : public GLESv2Dispatch {
public:
    SnapshotTestDispatch() : mTestSystem(PATH_SEP "progdir",
                      android::base::System::kProgramBitness,
                      PATH_SEP "homedir",
                      PATH_SEP "appdir") {
        mTestSystem.getTempRoot()->makeSubDir("SampleSnapshots");
        mSnapshotPath = mTestSystem.getTempRoot()->makeSubPath("SampleSnapshots");

        mValid = gles2_dispatch_init(this);
        if (mValid) {
            overrideFunctions();
            fprintf(stderr, "SnapshotTestDispatch initialized.\n");
        }
        else {
            fprintf(stderr, "SnapshotTestDispatch failed to initialize.\n");
        }

    }

    void overrideFunctions() {
        this->glEnable = test_glEnable;
        this->glDrawArrays = test_glDrawArrays;
    }

    void saveSnapshot() {
        fprintf(stderr, "snapshot dispatch save snapshot\n");
        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            FAIL() << "Could not get FrameBuffer during snapshot test.";
        }

        std::string timeStamp =
                std::to_string(android::base::System::get()->getUnixTime())
                + "-" + std::to_string(saveCount);
        mSnapshotFile =
                mSnapshotPath + PATH_SEP "snapshot_" + timeStamp + ".snap";
        mTextureFile =
                mSnapshotPath + PATH_SEP "textures_" + timeStamp + ".stex";

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                fopen(mSnapshotFile.c_str(), "wb"), StdioStream::kOwner));
        auto a_stream = static_cast<android::base::Stream*>(m_stream.get());
        std::shared_ptr<TextureSaver> m_texture_saver(
                new TextureSaver(StdioStream(fopen(mTextureFile.c_str(), "wb"),
                                             StdioStream::kOwner)));

        fb->onSave(a_stream, m_texture_saver);
        fprintf(stderr, "Saved snapshot.\n");
        saveCount ++;

        m_stream->close();
        m_texture_saver->done();
    }

    void loadSnapshot() {
        fprintf(stderr, "snapshot dispatch load snapshot\n");
        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            FAIL() << "Could not get FrameBuffer during snapshot test.";
        }

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                fopen(mSnapshotFile.c_str(), "rb"), StdioStream::kOwner));
        std::shared_ptr<TextureLoader> m_texture_loader(
                new TextureLoader(StdioStream(fopen(mTextureFile.c_str(), "rb"),
                                              StdioStream::kOwner)));
        fb->onLoad(m_stream.get(), m_texture_loader);
        fprintf(stderr, "Loaded snapshot.\n");

        m_stream->close();
        m_texture_loader->join();
    }

private:
    static void test_glEnable(GLenum capability) {
        fprintf(stderr, "SnapshotTestDispatch's glEnable reached.\n");
        LazyLoadedGLESv2Dispatch::get()->glEnable(capability);
    }

    static void test_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
        fprintf(stderr, "SnapshotTestDispatch's glDrawArrays reached.\n");

        // take a snapshot
        ((SnapshotTestDispatch*)getSnapshotTestDispatch())->saveSnapshot();
        LazyLoadedGLESv2Dispatch::get()->glDrawArrays(mode, first, count);
        // save the framebuffer contents

        // load the snapshot
        ((SnapshotTestDispatch*)getSnapshotTestDispatch())->loadSnapshot();
        LazyLoadedGLESv2Dispatch::get()->glDrawArrays(mode, first, count);
        // compare the framebuffer contents
    }

    bool mValid = false;

    int saveCount = 0;

    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath = {};

    std::string mSnapshotFile = {};
    std::string mTextureFile = {};
};

static LazyInstance<SnapshotTestDispatch> sSnapshotTestDispatch =
        LAZY_INSTANCE_INIT;

// static
const GLESv2Dispatch* getSnapshotTestDispatch() {
    return sSnapshotTestDispatch.ptr();
}

TEST_F(GLTest, OverrideDispatch) {
    FrameBuffer::initialize(256, 256, true /* useSubWindow */,
                            true /* egl2egl */);

    struct VertexAttributes {
        float position[2];
        float color[3];
    };

    EXPECT_TRUE(gl != nullptr);
    gl = getSnapshotTestDispatch();
    gl->glEnable(GL_BLEND);

    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(1);

    const VertexAttributes vertexAttrs[] = {
            {
                    {
                            -0.5f,
                            -0.5f,
                    },
                    {
                            0.2,
                            0.1,
                            0.9,
                    },
            },
            {
                    {
                            0.5f,
                            -0.5f,
                    },
                    {
                            0.8,
                            0.3,
                            0.1,
                    },
            },
            {
                    {
                            0.0f,
                            0.5f,
                    },
                    {
                            0.1,
                            0.9,
                            0.6,
                    },
            },
    };

    GLuint buffer;
    gl->glGenBuffers(1, &buffer);
    gl->glBindBuffer(GL_ARRAY_BUFFER, buffer);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                     GL_STATIC_DRAW);

    gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes), 0);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes),
                              (GLvoid*)offsetof(VertexAttributes, color));

    gl->glDrawArrays(GL_TRIANGLES, 0, 3);

    delete FrameBuffer::getFB();
}

class SnapshotTestTriangle : public HelloTriangle {
protected:
    const GLESv2Dispatch* getGlDispatch() { return getSnapshotTestDispatch(); }
};

TEST(SnapshotGlRenderingSampleTest, DrawTriangle) {
    SnapshotTestTriangle app;
    app.drawOnce();
}

}  // namespace emugl
