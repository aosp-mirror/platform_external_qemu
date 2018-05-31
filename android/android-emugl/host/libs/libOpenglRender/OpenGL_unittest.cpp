// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"
#include "android/snapshot/TextureSaver.h"
#include "android/snapshot/TextureLoader.h"

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"
#include "GLESv2Decoder.h"
#include "GLSnapshot.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

namespace emugl {

using android::base::LazyInstance;
using android::base::Lock;
using android::base::AutoLock;
using android::base::PathUtils;
using android::base::System;
using android::base::StdioStream;
using android::snapshot::TextureSaver;
using android::snapshot::TextureLoader;

using namespace gltest;

class GLTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
        EXPECT_TRUE(egl != nullptr);
        EXPECT_TRUE(gl != nullptr);

        m_display = getDisplay();
        m_config = createConfig(m_display, 8, 8, 8, 8, 24, 8, 0);
        m_surface = pbufferSurface(m_display, m_config, 32, 32);
        m_context = createContext(m_display, m_config, 3, 0);

        egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }

    virtual void TearDown() {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        destroyContext(m_display, m_context);
        destroySurface(m_display, m_surface);
        destroyDisplay(m_display);
    }

    EGLDisplay m_display;
    EGLConfig m_config;
    EGLSurface m_surface;
    EGLContext m_context;
};

class SnapshotTest : public GLTest {
public:
    void saveSnap(const std::string streamFile, const std::string textureFile) {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

        auto m_stream = std::make_unique<StdioStream>(
                fopen(streamFile.c_str(), "wb"), StdioStream::kOwner);
        auto egl_stream = static_cast<EGLStream>(m_stream.get());
        auto m_texture_saver = std::make_unique<TextureSaver>(StdioStream(
                fopen(textureFile.c_str(), "wb"), StdioStream::kOwner));

        // attempting to mimic FrameBuffer.onSave
        egl->eglPreSaveContext(m_display, m_context, egl_stream);
        egl->eglSaveAllImages(m_display, egl_stream, &m_texture_saver);

        egl->eglSaveContext(m_display, m_context, egl_stream);

        // FrameBuffer saves a bunch of its fields
        // we're lower level here and don't necessarily need such info

        // FrameBuffer saves colorbuffers
        // FrameBuffer saves window surfaces

        egl->eglSaveConfig(m_display, m_config, egl_stream);

        // FrameBuffer saves a bunch of proc-owned collections

        egl->eglPostSaveContext(m_display, m_context, egl_stream);

        m_stream->close();
        m_texture_saver->done();
    }

    void loadSnap(const std::string streamFile, const std::string textureFile) {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

        // "teardown" of existing context and surface
        egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        destroyContext(m_display, m_context);
        destroySurface(m_display, m_surface);

        auto m_stream = std::make_unique<StdioStream>(
                fopen(streamFile.c_str(), "rb"), StdioStream::kOwner);
        auto egl_stream = static_cast<EGLStream>(m_stream.get());
        auto m_texture_loader = std::make_shared<TextureLoader>(StdioStream(
                fopen(textureFile.c_str(), "rb"), StdioStream::kOwner));

        egl->eglLoadAllImages(m_display, egl_stream, &m_texture_loader);

        EGLint contextAttribs[5] = {
            EGL_CONTEXT_CLIENT_VERSION, 3, /* do we need to tell this again? */
            EGL_CONTEXT_MINOR_VERSION_KHR, 0,
            EGL_NONE
        };

        m_context = egl->eglLoadContext(m_display, &contextAttribs[0], egl_stream);
        m_config = egl->eglLoadConfig(m_display, egl_stream);
        m_surface = pbufferSurface(m_display, m_config, 32, 32);
        egl->eglPostLoadAllImages(m_display, egl_stream);

        m_stream->close();
        m_texture_loader->join();
        egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }

    void doSnapshot() {
        // TODO: use Translator snapshot instead of libGLSnapshot
        // const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        // mSnap.save();
        // mEGL->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        // destroyContext(m_display, m_context);
        // destroySurface(m_display, m_surface);
        // m_surface = pbufferSurface(m_display, m_config, mWidth, mHeight);
        // m_context = createContext(m_display, m_config, mMajorVersion, mMinorVersion);
        // mEGL->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
        // mSnap.restore();

        // Translator snapshot, taking similar approach to FrameBuffer
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();

        saveSnap("glunittest_s.snap", "glunittest_texture.snap");

        void * prev_display = m_display;
        void * prev_context = m_context;
        void * prev_surface = m_surface;
        fprintf(stderr, "pdisp ptr %p\n", prev_display);
        fprintf(stderr, "pcntx ptr %p\n", prev_context);
        fprintf(stderr, "psurf ptr %p\n", prev_surface);

        fprintf(stderr, "========================== GL_BLEND is enabled? %d\n",
                gl->glIsEnabled(GL_BLEND) );
        loadSnap("glunittest_s.snap", "glunittest_texture.snap");

        EXPECT_TRUE(m_context != EGL_NO_CONTEXT);
        EXPECT_TRUE(m_surface != EGL_NO_SURFACE);
        EXPECT_TRUE(m_display != EGL_NO_DISPLAY);

        fprintf(stderr, "mdisp ptr %p\n", m_display);
        fprintf(stderr, "mcntx ptr %p\n", m_context);
        fprintf(stderr, "msurf ptr %p\n", m_surface);

        EXPECT_FALSE(m_context == prev_context);
        EXPECT_TRUE(m_display == prev_display);
        EXPECT_FALSE(m_surface == prev_surface);

    }
};

TEST_F(GLTest, InitDestroy) {
}

TEST_F(SnapshotTest, InitDestroy) {
}

TEST_F(SnapshotTest, EnableBlend) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    fprintf(stderr, "========================== GL_BLEND is enabled? %d\n", gl->glIsEnabled(GL_BLEND) );
    gl->glEnable(GL_BLEND);
    fprintf(stderr, "========================== GL_BLEND is enabled? %d\n",
            gl->glIsEnabled(GL_BLEND) );
    doSnapshot();
    fprintf(stderr, "========================== GL_BLEND is enabled? %d\n",
            gl->glIsEnabled(GL_BLEND) );
}

}  // namespace emugl
