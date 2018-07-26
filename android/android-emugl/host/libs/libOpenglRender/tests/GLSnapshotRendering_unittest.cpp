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

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"

namespace emugl {

using android::base::LazyInstance;
using android::base::StdioStream;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

class SnapshotTestDispatch : public GLESv2Dispatch {
public:
    // TODO(benzene): make it a static singleton?
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

    static void test_glEnable(GLenum capability) {
        fprintf(stderr, "SnapshotTestDispatch's glEnable reached.\n");
        LazyLoadedGLESv2Dispatch::get()->glEnable(capability);
    }

    static void test_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
        fprintf(stderr, "SnapshotTestDispatch's glDrawArrays reached.\n");

        // take a snapshot
        saveSnapshot();
        LazyLoadedGLESv2Dispatch::get()->glDrawArrays(mode, first, count);
        // save the framebuffer contents

        // load the snapshot
        loadSnapshot();
        LazyLoadedGLESv2Dispatch::get()->glDrawArrays(mode, first, count);
        // compare the framebuffer contents
    }

    void overrideFunctions() {
        this->glEnable = test_glEnable;
        this->glDrawArrays = test_glDrawArrays;
    }

    void saveSnapshot() {
        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            FAIL() << "Could not get FrameBuffer during snapshot test.";
        }
        std::string timeStamp =
                std::to_string(android::base::System::get()->getUnixTime())
                + "-" + std::to_string(saveCount);
        mLastSnapshotFile =
                mSnapshotPath + PATH_SEP "snapshot_" + timeStamp + ".snap";
        mLastTextureFile =
                mSnapshotPath + PATH_SEP "textures_" + timeStamp + ".stex";

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                fopen(mLastSnapshotFile.c_str(), "wb"), StdioStream::kOwner));
        auto a_stream = static_cast<android::base::Stream*>(m_stream.get());
        std::shared_ptr<TextureSaver> m_texture_saver(new TextureSaver(StdioStream(
                fopen(mLastTextureFile.c_str(), "wb"), StdioStream::kOwner)));

        saveCount ++;
    }

    void loadSnapshot() {
        // yeah
    }

protected:
    bool mValid = false;

    int saveCount = 0;

    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath = {};

    std::string mLastSnapshotFile = {};
    std::string mLastTextureFile = {};
};

TEST_F(GLTest, OverrideDispatch) {
    EXPECT_TRUE(gl != nullptr);
    SnapshotTestDispatch snapgl;

    GLESv2Dispatch* snapptr = &snapgl;
    snapptr->glEnable(GL_BLEND);
}
