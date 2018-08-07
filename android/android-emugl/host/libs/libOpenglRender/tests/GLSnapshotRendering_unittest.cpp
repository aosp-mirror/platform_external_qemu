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

#include "GLSnapshotTestStateUtils.h"
#include "GLSnapshotTesting.h"
#include "GLTestUtils.h"
#include "OpenglCodecCommon/glUtils.h"
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

namespace emugl {

using android::base::LazyInstance;
using android::base::StdioStream;
using android::base::System;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

// Global dispatch object with functions overridden for snapshot testing
static const GLESv2Dispatch* getSnapshotTestDispatch();

// Holds handles for context that needs to be bound
struct SnapshotHandles {
    HandleType drawSurface;
    HandleType readSurface;
    HandleType context;
    HandleType colorBuffer;
};

// SnapshotTestDispatch - a GL dispatcher with some of its functions overridden
// that can act as a drop-in replacement for GLESv2Dispatch. These functions are
// given wrappers that perform a test of the GL snapshot when they are called.
//
// It uses the FrameBuffer to perform the snapshot; thus will only work in an
// environment where the FrameBuffer exists.
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
        }
        else {
            fprintf(stderr, "SnapshotTestDispatch failed to initialize.\n");
            ADD_FAILURE() << "SnapshotTestDispatch could not initialize.";
        }
    }

    void captureCurrentHandles(SnapshotHandles handles) {
        //mSaveDisplay = display;
        mHandles = handles;
        fprintf(stderr, "Handles captured\n");
        // mSaveDrawSurface = handles.drawSurface;
        // mSaveReadSurface = handles.readSurface;
        // mSaveContext = handles.context;
        // const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        // mSaveDisplay = egl->eglGetCurrentDisplay();
        // mSaveDrawSurface = egl->eglGetCurrentSurface(EGL_DRAW);
        // mSaveReadSurface = egl->eglGetCurrentSurface(EGL_READ);
        // mSaveContext = egl->eglGetCurrentContext();
        // fprintf(stderr, "Tracking EGL context %p\n", mSaveContext);
    }

protected:
    void overrideFunctions() {
        this->glDrawArrays = (glDrawArrays_t)test_glDrawArrays;
        this->glDrawElements = (glDrawElements_t)test_glDrawElements;
    }

    void saveSnapshot() {
        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            FAIL() << "Could not get FrameBuffer during snapshot test.";
        }

        std::string timeStamp =
                std::to_string(android::base::System::get()->getUnixTime())
                + "-" + std::to_string(loadCount);
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

        m_stream->close();
        m_texture_saver->done();
    }

    void loadSnapshot() {
        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            FAIL() << "Could not get FrameBuffer during snapshot test.";
        }

        //fb->bindContext(0, 0, 0);

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                fopen(mSnapshotFile.c_str(), "rb"), StdioStream::kOwner));
        std::shared_ptr<TextureLoader> m_texture_loader(
                new TextureLoader(StdioStream(fopen(mTextureFile.c_str(), "rb"),
                                              StdioStream::kOwner)));

        fb->onLoad(m_stream.get(), m_texture_loader);

        m_stream->close();
        m_texture_loader->join();

        // if (mSaveDisplay != EGL_NO_DISPLAY) {
            // const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
            // egl->eglMakeCurrent(mSaveDisplay, mSaveDrawSurface,
            //         mSaveReadSurface, mSaveContext);
        if (mHandles.context) {
            fprintf(stderr, "Binding context after load\n");
            fb->bindContext(mHandles.context, mHandles.drawSurface,
                     mHandles.readSurface);
            //fb->setWindowSurfaceColorBuffer(mHandles.drawSurface, mHandles.colorBuffer);
        }
        // }
        loadCount++;
    }

    static void testDraw(std::function<void()> doDraw) {
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        ASSERT_NE(nullptr, gl);
        ASSERT_NE(nullptr, egl);

        FrameBuffer* fb = FrameBuffer::getFB();
        if (!fb) {
            ADD_FAILURE() << "No framebuffer, test cannot snapshot.";
            doDraw();
            return;
        }

        // save then draw
        ((SnapshotTestDispatch*)getSnapshotTestDispatch())->saveSnapshot();
        doDraw();
        EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE,
                                          {0.2f, 0.2f, 0.3f, 0.0f}));

        // save the framebuffer contents
        GLuint width, height, bytesPerPixel;
        width = fb->getWidth();
        height = fb->getHeight();
        bytesPerPixel = glUtilsPixelBitSize(GL_RGBA, GL_UNSIGNED_BYTE) / 8;
        std::vector<GLubyte> prePixels = {};
        prePixels.resize(width * height * bytesPerPixel);
        gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                         prePixels.data());

        gl->glFinish();
        gl->glClearColor(0.5, 0.5, 0.5, 0.5);
        gl->glClear(GL_COLOR_BUFFER_BIT);
        gl->glFinish();
        EXPECT_TRUE(
                compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, {0.5, 0.5, 0.5, 0.5}));

        // load and redraw
        ((SnapshotTestDispatch*)getSnapshotTestDispatch())->loadSnapshot();
        //egl->eglMakeCurrent(getDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        // egl->eglMakeCurrent(fb->getDisplay(),
        //         fb->getWindowSurface(),
        //         fb->getWindowSurface(),
        //         fb->getContext());
        //fb->bindContext(fb->getContext(), fb->getWindowSurface(), fb->getWindowSurface());
        // fb->lock();
        // fb->bind_locked();
        // fb->unbind_locked();
        // fb->unlock();
        doDraw();

        // check that clear is restored
        EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE,
                                          {0.2f, 0.2f, 0.3f, 0.0f}));

        fprintf(stderr, "reached pre-read-pixels\n");

        // compare the framebuffer contents
        std::vector<GLubyte> postPixels = {};
        postPixels.resize(width * height * bytesPerPixel);
        gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                         postPixels.data());

        fprintf(stderr, "reached comparisons\n");

        EXPECT_EQ(prePixels, postPixels);
        EXPECT_TRUE(compareVector<GLubyte>(prePixels, postPixels));
        EXPECT_TRUE(ImageMatches(width, height, bytesPerPixel, width,
                                 prePixels.data(), postPixels.data()));

        fprintf(stderr, "ended test\n");
    }

    static void test_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
        testDraw([&] {
            LazyLoadedGLESv2Dispatch::get()->glDrawArrays(mode, first, count);
        });
    }

    static void test_glDrawElements(GLenum mode,
                                    GLsizei count,
                                    GLenum type,
                                    const GLvoid* indices) {
        testDraw([&] {
            LazyLoadedGLESv2Dispatch::get()->glDrawElements(mode, count, type,
                                                            indices);
        });
    }

    bool mValid = false;

    int loadCount = 0;

    SnapshotHandles mHandles;
    //HandleType mSaveDisplay;
    // HandleType mSaveDrawSurface;
    // HandleType mSaveReadSurface;
    // HandleType mSaveContext;
    // EGLDisplay mSaveDisplay = EGL_NO_DISPLAY;
    // EGLSurface mSaveDrawSurface = EGL_NO_SURFACE;
    // EGLSurface mSaveReadSurface = EGL_NO_SURFACE;
    // EGLContext mSaveContext = EGL_NO_CONTEXT;

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

TEST(SnapshotGlRenderingSampleTest, OverrideDispatch) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    const GLESv2Dispatch* testGl = getSnapshotTestDispatch();
    EXPECT_NE(nullptr, gl);
    EXPECT_NE(nullptr, testGl);
    EXPECT_NE(gl->glDrawArrays, testGl->glDrawArrays);
    EXPECT_NE(gl->glDrawElements, testGl->glDrawElements);
}

class SnapshotTestTriangle : public HelloTriangle {
public:
    void drawLoop() {
        this->initialize();
        while (mFrameCount < 5) {
            this->draw();
            mFb->flushWindowSurfaceColorBuffer(mSurface);
            if (mUseSubWindow) {
                mFb->post(mColorBuffer);
                mWindow->messageLoop();
            }
        }
    }

protected:
    const GLESv2Dispatch* getGlDispatch() { return getSnapshotTestDispatch(); }

    SnapshotHandles getHandles() {
        return { mSurface, mSurface, mContext, mColorBuffer };
    }

    void draw() override {
        ((SnapshotTestDispatch*)getSnapshotTestDispatch())->captureCurrentHandles(getHandles());
        HelloTriangle::draw();
        mFrameCount++;
    }

    int mFrameCount = 0;
};

TEST(SnapshotGlRenderingSampleTest, DrawTriangleOnce) {
    SnapshotTestTriangle app;
    app.drawOnce();
}

TEST(SnapshotGlRenderingSampleTest, DISABLED_DrawTriangleLoop) {
    SnapshotTestTriangle app;
    app.drawLoop();
}

}  // namespace emugl
