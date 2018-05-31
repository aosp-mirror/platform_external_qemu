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
#include "android/base/containers/SmallVector.h"
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
        //egl->eglSetMaxGLESVersion(3); /* GLES_3_0 = 3 */
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

        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();

        m_stream = std::make_unique<StdioStream>(fopen("glunittest_s.snap", "wb"),
                StdioStream::kOwner);
        EGLStream egl_stream = static_cast<EGLStream>(m_stream.get());
        m_texture_saver = std::make_unique<TextureSaver>(StdioStream(
                fopen("glunittest_texture.snap", "wb"), StdioStream::kOwner));

        // attempting to mimic FrameBuffer.onSave
        // (what exactly is stopping us from making our own FrameBuffer for these tests?)
        // FrameBuffer.initialize takes some size parameters? we could do that.
        egl->eglPreSaveContext(m_display, m_context, egl_stream);
        egl->eglSaveAllImages(m_display, egl_stream, &m_texture_saver);

        // this is the wrong kind of context?
        // What are we actually supposed to save?
        // RenderContexts get saved with RenderContext::onSave
        egl->eglSaveContext(m_display, m_context, egl_stream);

        // here the framebuffer saves a bunch of its fields

        // here the colorbuffers get saved with ColorBuffer::onSave
        // where would we get colorbuffers from?

        // here the window surfaces get saved with WindowSurface::onSave
        egl->eglSaveConfig(m_display, m_config, egl_stream);

        // bunch of proc-owned collections get saved
        egl->eglPostSaveContext(m_display, m_context, egl_stream);

        m_stream->close();
        m_texture_saver->done();

        // what about m_surface? what is the pbuffer?

        void * prev_display = m_display;
        void * prev_context = m_context;
        void * prev_surface = m_surface;
        fprintf(stderr, "pdisp ptr %p\n", prev_display);
        fprintf(stderr, "pcntx ptr %p\n", prev_context);
        fprintf(stderr, "psurf ptr %p\n", prev_surface);


        // "teardown"
        egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        destroyContext(m_display, m_context);
        destroySurface(m_display, m_surface);


        fprintf(stderr, "========================== GL_BLEND is enabled? %d\n", gl->glIsEnabled(GL_BLEND) );


        // onLoad

        m_stream = std::make_unique<StdioStream>(fopen("glunittest_s.snap", "rb"),
                StdioStream::kOwner);
        egl_stream = static_cast<EGLStream>(m_stream.get());

        m_texture_loader = std::make_shared<TextureLoader>(StdioStream(
                fopen("glunittest_texture.snap", "rb"), StdioStream::kOwner));

        // do we need to re-get any of context,surface,display?

        egl->eglLoadAllImages(m_display, egl_stream, &m_texture_loader);

        android::base::SmallFixedVector<EGLint, 5> contextAttribs = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_CONTEXT_MINOR_VERSION_KHR, 0,
            EGL_NONE
        };

        m_surface = pbufferSurface(m_display, m_config, 32, 32);
        m_context = egl->eglLoadContext(m_display, &contextAttribs[0], egl_stream);
        m_config = egl->eglLoadConfig(m_display, egl_stream);
        egl->eglPostLoadAllImages(m_display, egl_stream);

        EXPECT_TRUE(m_context != EGL_NO_CONTEXT);
        EXPECT_TRUE(m_surface != EGL_NO_SURFACE);
        EXPECT_TRUE(m_display != EGL_NO_DISPLAY);

        fprintf(stderr, "mdisp ptr %p\n", m_display);
        fprintf(stderr, "mcntx ptr %p\n", m_context);
        fprintf(stderr, "msurf ptr %p\n", m_surface);

        m_stream->close();
        m_texture_loader->join();
        egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }
protected:
    std::unique_ptr<StdioStream> m_stream;
    std::unique_ptr<TextureSaver> m_texture_saver;
    std::shared_ptr<TextureLoader> m_texture_loader;
};

TEST_F(GLTest, InitDestroy) {
}

TEST_F(SnapshotTest, InitDestroy) {
}

TEST_F(SnapshotTest, EnableBlend) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    fprintf(stderr, "========================== GL_BLEND is enabled? %d\n", gl->glIsEnabled(GL_BLEND) );
    gl->glEnable(GL_BLEND);
    fprintf(stderr, "========================== GL_BLEND is enabled? %d\n", gl->glIsEnabled(GL_BLEND) );
    doSnapshot();
    fprintf(stderr, "========================== GL_BLEND is enabled? %d\n", gl->glIsEnabled(GL_BLEND) );
}

}  // namespace emugl
