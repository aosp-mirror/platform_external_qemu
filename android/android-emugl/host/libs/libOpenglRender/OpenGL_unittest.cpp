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
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/threads/Thread.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include <gtest/gtest.h>

#include "GLESv2Decoder.h"
#include "GLSnapshot.h"
#include "OpenGLTestContext.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <memory>

namespace emugl {

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::PathUtils;
using android::base::StdioStream;
using android::base::System;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

using namespace gltest;

// Capabilities which, according to the GLES2 spec, start disabled.
static const GLenum kGLES2CanBeEnabled[] = {GL_BLEND,
                                            GL_CULL_FACE,
                                            GL_DEPTH_TEST,
                                            GL_POLYGON_OFFSET_FILL,
                                            GL_SAMPLE_ALPHA_TO_COVERAGE,
                                            GL_SAMPLE_COVERAGE,
                                            GL_SCISSOR_TEST,
                                            GL_STENCIL_TEST};

// Capabilities which, according to the GLES2 spec, start enabled.
static const GLenum kGLES2CanBeDisabled[] = {GL_DITHER};

// Modes for CullFace
static const GLenum kGLES2CullFaceModes[] = {GL_BACK, GL_FRONT,
                                             GL_FRONT_AND_BACK};

// Dimensions for test surface
static const int kSurfaceSize[] = {32, 32};

// Viewport settings to attempt
static const GLint kGLES2ViewportValues[] = {10, 10, 100, 100};

// Depth range settings to attempt
static const GLclampf kGLES2DepthRangeValues[] = {0.2f, 0.8f};

class GLTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
        EXPECT_TRUE(egl != nullptr);
        EXPECT_TRUE(gl != nullptr);

        m_display = getDisplay();
        m_config = createConfig(m_display, 8, 8, 8, 8, 24, 8, 0);
        m_surface = pbufferSurface(m_display, m_config, kSurfaceSize[0],
                                   kSurfaceSize[1]);
        egl->eglSetMaxGLESVersion(3);
        m_context = createContext(m_display, m_config, 3, 0);
        egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }

    virtual void TearDown() {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                            EGL_NO_CONTEXT);
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
    SnapshotTest()
        : mTestSystem(PATH_SEP "progdir",
                      System::kProgramBitness,
                      PATH_SEP "homedir",
                      PATH_SEP "appdir") {}

    void SetUp() override {
        GLTest::SetUp();
        mTestSystem.getTempRoot()->makeSubDir("Snapshots");
        mSnapshotPath = mTestSystem.getTempRoot()->makeSubPath("Snapshots");
    }

    // Mimics FrameBuffer.onSave, with fewer objects to manage.
    // |streamFile| is a filename into which the snapshot will be saved.
    // |textureFile| is a filename into which the textures will be saved.
    void saveSnapshot(const std::string streamFile,
                      const std::string textureFile) {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                fopen(streamFile.c_str(), "wb"), StdioStream::kOwner));
        auto egl_stream = static_cast<EGLStream>(m_stream.get());
        std::unique_ptr<TextureSaver> m_texture_saver(
                new TextureSaver(StdioStream(fopen(textureFile.c_str(), "wb"),
                                             StdioStream::kOwner)));

        egl->eglPreSaveContext(m_display, m_context, egl_stream);
        egl->eglSaveAllImages(m_display, egl_stream, &m_texture_saver);

        egl->eglSaveContext(m_display, m_context, egl_stream);

        // Skip saving a bunch of FrameBuffer's fields
        // Skip saving colorbuffers
        // Skip saving window surfaces

        egl->eglSaveConfig(m_display, m_config, egl_stream);

        // Skip saving a bunch of process-owned objects

        egl->eglPostSaveContext(m_display, m_context, egl_stream);

        m_stream->close();
        m_texture_saver->done();
    }

    // Mimics FrameBuffer.onLoad, with fewer objects to manage.
    // Assumes that a valid display is present.
    // |streamFile| is a filename from which the snapshot will be loaded.
    // |textureFile| is a filename from which the textures will be loaded.
    void loadSnapshot(const std::string streamFile,
                      const std::string textureFile) {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                fopen(streamFile.c_str(), "rb"), StdioStream::kOwner));
        auto egl_stream = static_cast<EGLStream>(m_stream.get());
        std::shared_ptr<TextureLoader> m_texture_loader(
                new TextureLoader(StdioStream(fopen(textureFile.c_str(), "rb"),
                                              StdioStream::kOwner)));

        egl->eglLoadAllImages(m_display, egl_stream, &m_texture_loader);

        EGLint contextAttribs[5] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                    EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE};

        m_context =
                egl->eglLoadContext(m_display, &contextAttribs[0], egl_stream);
        m_config = egl->eglLoadConfig(m_display, egl_stream);
        m_surface = pbufferSurface(m_display, m_config, 32, 32);
        egl->eglPostLoadAllImages(m_display, egl_stream);

        m_stream->close();
        m_texture_loader->join();
        egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }

    // Performs a teardown and reset of graphics objects in preparation for
    // a snapshot load.
    void preloadReset() {
        GLTest::TearDown();
        GLTest::SetUp();
    }

    // Mimics saving and then loading a graphics snapshot.
    // To verify that state has been reset to some default before the load,
    // assertions can be performed in |preloadCheck|.
    void doSnapshot(std::function<void()> preloadCheck = [] {}) {
        std::string timeStamp =
                std::to_string(android::base::System::get()->getUnixTime());
        std::string snapshotFile =
                mSnapshotPath + PATH_SEP "snapshot_" + timeStamp + ".snap";
        std::string textureFile =
                mSnapshotPath + PATH_SEP "textures_" + timeStamp + ".stex";

        saveSnapshot(snapshotFile, textureFile);

        preloadReset();
        preloadCheck();

        loadSnapshot(snapshotFile, textureFile);

        EXPECT_NE(m_context, EGL_NO_CONTEXT);
        EXPECT_NE(m_surface, EGL_NO_SURFACE);
    }

protected:
    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath = {};
};

// For granular testing of snapshot's ability to preserve parts of GL state that
// can be changed from their default values GL function calls.
class SnapshotPreserveTest : public SnapshotTest {
public:
    void setDefaultStateCheck(std::function<void()> checkFunc) {
        m_default_state_check = checkFunc;
    }

    void setChangedStateCheck(std::function<void()> checkFunc) {
        m_changed_state_check = checkFunc;
    }

    void setStateChanger(std::function<void()> changeFunc) {
        m_state_changer = changeFunc;
    }

    void doCheckedSnapshot() {
        m_default_state_check();
        m_state_changer();
        m_changed_state_check();
        SnapshotTest::doSnapshot(m_default_state_check);
        m_changed_state_check();
    }

protected:
    std::function<void()> m_default_state_check = [] {};
    std::function<void()> m_changed_state_check = [] {};
    std::function<void()> m_state_changer = [] {};
};

TEST_F(GLTest, InitDestroy) {}

TEST_F(SnapshotTest, InitDestroy) {}

class SnapshotGlEnableTest : public SnapshotPreserveTest,
                             public ::testing::WithParamInterface<GLenum> {};

TEST_P(SnapshotGlEnableTest, PreserveEnable) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    GLenum capability = GetParam();
    setDefaultStateCheck([gl, capability] {
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_FALSE(gl->glIsEnabled(capability));
    });
    setStateChanger([gl, capability] { gl->glEnable(capability); });
    setChangedStateCheck([gl, capability] {
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_TRUE(gl->glIsEnabled(capability));
    });
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPreserve,
                        SnapshotGlEnableTest,
                        ::testing::ValuesIn(kGLES2CanBeEnabled));

class SnapshotGlDisableTest : public SnapshotPreserveTest,
                              public ::testing::WithParamInterface<GLenum> {};

TEST_P(SnapshotGlDisableTest, PreserveDisable) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    GLenum capability = GetParam();
    setDefaultStateCheck([gl, capability] {
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_TRUE(gl->glIsEnabled(capability));
    });
    setStateChanger([gl, capability] { gl->glDisable(capability); });
    setChangedStateCheck([gl, capability] {
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_FALSE(gl->glIsEnabled(capability));
    });
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPreserve,
                        SnapshotGlDisableTest,
                        ::testing::ValuesIn(kGLES2CanBeDisabled));

class SnapshotGlViewportTest
    : public SnapshotPreserveTest,
      public ::testing::WithParamInterface<const GLint*> {};

TEST_P(SnapshotGlViewportTest, SetViewport) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    GLint testViewport[4] = {GetParam()[0], GetParam()[1], GetParam()[2],
                             GetParam()[3]};
    setDefaultStateCheck([gl] {
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        // initial viewport should match surface size
        EXPECT_EQ(kSurfaceSize[0], viewport[2]);
        EXPECT_EQ(kSurfaceSize[1], viewport[3]);
    });
    setStateChanger([gl, testViewport] {
        gl->glViewport(testViewport[0], testViewport[1], testViewport[2],
                       testViewport[3]);
    });
    setChangedStateCheck([gl, testViewport] {
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(testViewport[i], viewport[i]);
        }
    });
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPreserve,
                        SnapshotGlViewportTest,
                        ::testing::Values(kGLES2ViewportValues));

class SnapshotGlDepthRangeTest
    : public SnapshotPreserveTest,
      public ::testing::WithParamInterface<const GLclampf*> {};

TEST_P(SnapshotGlDepthRangeTest, SetDepthRange) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    GLclampf testDepthRange[2] = {GetParam()[0], GetParam()[1]};
    setDefaultStateCheck([gl] {
        GLfloat depthRange[2] = {};
        gl->glGetFloatv(GL_DEPTH_RANGE, depthRange);
        EXPECT_EQ(0.0f, depthRange[0]);
        EXPECT_EQ(1.0f, depthRange[1]);
    });
    setStateChanger([gl, testDepthRange] {
        gl->glDepthRangef(testDepthRange[0], testDepthRange[1]);
    });
    setChangedStateCheck([gl, testDepthRange] {
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        GLfloat depthRange[2] = {};
        gl->glGetFloatv(GL_DEPTH_RANGE, depthRange);
        EXPECT_EQ(testDepthRange[0], depthRange[0]);
        EXPECT_EQ(testDepthRange[1], depthRange[1]);
    });
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPreserve,
                        SnapshotGlDepthRangeTest,
                        ::testing::Values(kGLES2DepthRangeValues));

class SnapshotGlCullFaceTest : public SnapshotPreserveTest,
                               public ::testing::WithParamInterface<GLenum> {};

TEST_P(SnapshotGlCullFaceTest, SetCullFaceMode) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    const GLenum defaultMode = GL_BACK;
    GLenum testMode = GetParam();
    setDefaultStateCheck([gl, defaultMode] {
        GLint mode;
        gl->glGetIntegerv(GL_CULL_FACE_MODE, &mode);
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_EQ(defaultMode, mode);
    });
    setStateChanger([gl, testMode] { gl->glCullFace(testMode); });
    setChangedStateCheck([gl, testMode] {
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        GLint mode;
        gl->glGetIntegerv(GL_CULL_FACE_MODE, &mode);
        EXPECT_EQ(testMode, mode);
    });
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPreserve,
                        SnapshotGlCullFaceTest,
                        ::testing::ValuesIn(kGLES2CullFaceModes));

}  // namespace emugl
