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

struct GlSampleCoverage {
    GLclampf value;
    GLboolean invert;
};
using GlSampleCoverage = struct GlSampleCoverage;

struct GlStencilFunc {
    GLenum func;
    GLint ref;
    GLuint mask;
};
using GlStencilFunc = struct GlStencilFunc;

struct GlStencilOp {
    GLenum sfail;
    GLenum dpfail;
    GLenum dppass;
};
using GlStencilOp = struct GlStencilOp;

static const int kRandomSeed = 0;

// Dimensions for test surface
static const int kSurfaceSize[] = {32, 32};

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

// Viewport settings to attempt
static const GLint kGLES2TestViewport[] = {10, 10, 100, 100};

// Depth range settings to attempt
static const GLclampf kGLES2TestDepthRange[] = {0.2f, 0.8f};

// Line width settings to attempt
static const GLfloat kGLES2TestLineWidths[] = {2.0f};

// Modes for CullFace
static const GLenum kGLES2CullFaceModes[] = {GL_BACK, GL_FRONT,
                                             GL_FRONT_AND_BACK};

// Modes for FrontFace
static const GLenum kGLES2FrontFaceModes[] = {GL_CCW, GL_CW};

// Polygon offset settings to attempt
static const GLfloat kGLES2TestPolygonOffset[] = {0.5f, 0.5f};

// Sample coverage settings to attempt
static const GlSampleCoverage kGLES2TestSampleCoverages[] = {{0.3, true},
                                                             {0, false}};

// Scissor box settings to attempt
static const GLint kGLES2TestScissorBox[] = {2, 3, 10, 20};

// Valid Stencil test functions
static const GLenum kGLES2StencilFuncs[] = {GL_NEVER,   GL_ALWAYS,  GL_LESS,
                                            GL_LEQUAL,  GL_EQUAL,   GL_GEQUAL,
                                            GL_GREATER, GL_NOTEQUAL};
// Valid Stencil test result operations
static const GLenum kGLES2StencilOps[] = {GL_KEEP,      GL_ZERO,     GL_REPLACE,
                                          GL_INCR,      GL_DECR,     GL_INVERT,
                                          GL_INCR_WRAP, GL_DECR_WRAP};
// Stencil reference value to attempt
static const GLint kGLES2StencilRef = 1;
// Stencil default mask; max GLuint is clamped to a signed GLint by GetInteger?
static const GLuint kGLES2StencilDefaultMask = 0x7FFFFFFF;
// Stencil mask values to attempt
static const GLuint kGLES2StencilMasks[] = {0, 1, 0x1000000, 0x7FFFFFFF};

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

// Abstract, for granular testing of snapshot's ability to preserve parts of GL
// state.
class SnapshotPreserveTest : public SnapshotTest {
public:
    // Asserts that we are working from a clean starting state.
    virtual void defaultStateCheck(const GLESv2Dispatch* gl) {
        ADD_FAILURE() << "Snapshot test needs a default state check function.";
    }

    // Asserts that any expected changes to state have occurred.
    virtual void changedStateCheck(const GLESv2Dispatch* gl) {
        ADD_FAILURE()
                << "Snapshot test needs a post-change state check function.";
    }

    // Modifies the state.
    virtual void stateChange(const GLESv2Dispatch* gl) {
        ADD_FAILURE() << "Snapshot test needs a state-changer function.";
    }

    // Sets up a non-default state and asserts that a snapshot preserves it.
    virtual void doCheckedSnapshot() {
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
        defaultStateCheck(gl);
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
        stateChange(gl);
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
        changedStateCheck(gl);
        SnapshotTest::doSnapshot([this, gl] {
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
            defaultStateCheck(gl);
        });
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        changedStateCheck(gl);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    }
};

// For testing preservation of pieces of GL state that can be checked via
// comparison(s) which are the same for both a known default and after
// predictable changes.
template <class T>
class SnapshotSetValueTest : public SnapshotPreserveTest {
public:
    // Configures the test to assert against values which it should consider
    // default and values which it should expect after changes.
    void setExpectedValues(T defaultValue, T changedValue) {
        m_default_value = std::unique_ptr<T>(new T(defaultValue));
        m_changed_value = std::unique_ptr<T>(new T(changedValue));
    }

    // Checks part of state against an expected value.
    virtual void stateCheck(const GLESv2Dispatch* gl, T expected) {
        ADD_FAILURE() << "Snapshot test needs a state-check function.";
    };

    void defaultStateCheck(const GLESv2Dispatch* gl) override {
        stateCheck(gl, *m_default_value);
    }
    void changedStateCheck(const GLESv2Dispatch* gl) override {
        stateCheck(gl, *m_changed_value);
    }

    void doCheckedSnapshot() override {
        if (m_default_value == nullptr || m_changed_value == nullptr) {
            ADD_FAILURE() << "Snapshot test not provided expected values.";
        }
        SnapshotPreserveTest::doCheckedSnapshot();
    }

protected:
    std::unique_ptr<T> m_default_value;
    std::unique_ptr<T> m_changed_value;
};

TEST_F(GLTest, InitDestroy) {}

TEST_F(SnapshotTest, InitDestroy) {}

class SnapshotGlEnableTest : public SnapshotPreserveTest,
                             public ::testing::WithParamInterface<GLenum> {
    void defaultStateCheck(const GLESv2Dispatch* gl) override {
        EXPECT_FALSE(gl->glIsEnabled(GetParam()));
    }
    void changedStateCheck(const GLESv2Dispatch* gl) override {
        EXPECT_TRUE(gl->glIsEnabled(GetParam()));
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glEnable(GetParam());
    }
};

TEST_P(SnapshotGlEnableTest, PreserveEnable) {
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotCapability,
                        SnapshotGlEnableTest,
                        ::testing::ValuesIn(kGLES2CanBeEnabled));

class SnapshotGlDisableTest : public SnapshotPreserveTest,
                              public ::testing::WithParamInterface<GLenum> {
    void defaultStateCheck(const GLESv2Dispatch* gl) override {
        EXPECT_TRUE(gl->glIsEnabled(GetParam()));
    }
    void changedStateCheck(const GLESv2Dispatch* gl) override {
        EXPECT_FALSE(gl->glIsEnabled(GetParam()));
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glDisable(GetParam());
    }
};

TEST_P(SnapshotGlDisableTest, PreserveDisable) {
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotCapability,
                        SnapshotGlDisableTest,
                        ::testing::ValuesIn(kGLES2CanBeDisabled));

class SnapshotGlViewportTest
    : public SnapshotPreserveTest,
      public ::testing::WithParamInterface<const GLint*> {
    void defaultStateCheck(const GLESv2Dispatch* gl) override {
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        // initial viewport should match surface size
        EXPECT_EQ(kSurfaceSize[0], viewport[2]);
        EXPECT_EQ(kSurfaceSize[1], viewport[3]);
    }
    void changedStateCheck(const GLESv2Dispatch* gl) override {
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(GetParam()[i], viewport[i]);
        }
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glViewport(GetParam()[0], GetParam()[1], GetParam()[2],
                       GetParam()[3]);
    }
};

TEST_P(SnapshotGlViewportTest, SetViewport) {
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotTransformation,
                        SnapshotGlViewportTest,
                        ::testing::Values(kGLES2TestViewport));

class SnapshotGlDepthRangeTest
    : public SnapshotSetValueTest<GLclampf*>,
      public ::testing::WithParamInterface<const GLclampf*> {
    void stateCheck(const GLESv2Dispatch* gl, GLclampf* expected) override {
        GLfloat depthRange[2] = {};
        gl->glGetFloatv(GL_DEPTH_RANGE, depthRange);
        EXPECT_EQ(expected[0], depthRange[0]);
        EXPECT_EQ(expected[1], depthRange[1]);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glDepthRangef(GetParam()[0], GetParam()[1]);
    }
};

TEST_P(SnapshotGlDepthRangeTest, SetDepthRange) {
    GLclampf defaultRange[2] = {0.0f, 1.0f};
    GLclampf testRange[2] = {GetParam()[0], GetParam()[1]};
    setExpectedValues(defaultRange, testRange);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotTransformation,
                        SnapshotGlDepthRangeTest,
                        ::testing::Values(kGLES2TestDepthRange));

class SnapshotGlLineWidthTest : public SnapshotSetValueTest<GLfloat>,
                                public ::testing::WithParamInterface<GLfloat> {
    void stateCheck(const GLESv2Dispatch* gl, GLfloat expected) {
        GLfloat lineWidth;
        gl->glGetFloatv(GL_LINE_WIDTH, &lineWidth);
        EXPECT_EQ(expected, lineWidth);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glLineWidth(GetParam());
    }
};

TEST_P(SnapshotGlLineWidthTest, SetLineWidth) {
    setExpectedValues(1.0f, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlLineWidthTest,
                        ::testing::ValuesIn(kGLES2TestLineWidths));

class SnapshotGlCullFaceTest : public SnapshotSetValueTest<GLenum>,
                               public ::testing::WithParamInterface<GLenum> {
    void stateCheck(const GLESv2Dispatch* gl, GLenum expected) {
        GLint mode;
        gl->glGetIntegerv(GL_CULL_FACE_MODE, &mode);
        EXPECT_EQ(expected, mode);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glCullFace(GetParam());
    }
};

TEST_P(SnapshotGlCullFaceTest, SetCullFaceMode) {
    setExpectedValues(GL_BACK, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlCullFaceTest,
                        ::testing::ValuesIn(kGLES2CullFaceModes));

class SnapshotGlFrontFaceTest : public SnapshotSetValueTest<GLenum>,
                                public ::testing::WithParamInterface<GLenum> {
    void stateCheck(const GLESv2Dispatch* gl, GLenum expected) {
        GLint mode;
        gl->glGetIntegerv(GL_FRONT_FACE, &mode);
        EXPECT_EQ(expected, mode);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glFrontFace(GetParam());
    }
};

TEST_P(SnapshotGlFrontFaceTest, SetFrontFaceMode) {
    setExpectedValues(GL_CCW, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlFrontFaceTest,
                        ::testing::ValuesIn(kGLES2FrontFaceModes));

class SnapshotGlPolygonOffsetTest
    : public SnapshotSetValueTest<GLfloat*>,
      public ::testing::WithParamInterface<const GLfloat*> {
    void stateCheck(const GLESv2Dispatch* gl, GLfloat* expected) {
        GLfloat factor, units;
        gl->glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
        gl->glGetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
        EXPECT_EQ(expected[0], factor);
        EXPECT_EQ(expected[1], units);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glPolygonOffset(GetParam()[0], GetParam()[1]);
    }
};

TEST_P(SnapshotGlPolygonOffsetTest, SetPolygonOffset) {
    GLfloat defaultOffset[2] = {0.0f, 0.0f};
    GLfloat testOffset[2] = {GetParam()[0], GetParam()[1]};
    setExpectedValues(defaultOffset, testOffset);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlPolygonOffsetTest,
                        ::testing::Values(kGLES2TestPolygonOffset));

class SnapshotGlSampleCoverageTest
    : public SnapshotSetValueTest<GlSampleCoverage>,
      public ::testing::WithParamInterface<GlSampleCoverage> {
    void stateCheck(const GLESv2Dispatch* gl, GlSampleCoverage expected) {
        GLfloat value;
        gl->glGetFloatv(GL_SAMPLE_COVERAGE_VALUE, &value);
        EXPECT_EQ(expected.value, value);
        GLboolean invert;
        gl->glGetBooleanv(GL_SAMPLE_COVERAGE_INVERT, &invert);
        EXPECT_EQ(expected.invert, invert);
    }
    void stateChange(const GLESv2Dispatch* gl) {
        gl->glSampleCoverage(GetParam().value, GetParam().invert);
    }
};

TEST_P(SnapshotGlSampleCoverageTest, SetSampleCoverage) {
    GlSampleCoverage defaultCoverage = {1.0f, false};
    setExpectedValues(defaultCoverage, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotMultisampling,
                        SnapshotGlSampleCoverageTest,
                        ::testing::ValuesIn(kGLES2TestSampleCoverages));

class SnapshotGlScissorBoxTest
    : public SnapshotSetValueTest<GLint*>,
      public ::testing::WithParamInterface<const GLint*> {
    void stateCheck(const GLESv2Dispatch* gl, GLint* expected) override {
        GLint box[4] = {};
        gl->glGetIntegerv(GL_SCISSOR_BOX, box);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(expected[i], box[i]);
        }
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        gl->glScissor(GetParam()[0], GetParam()[1], GetParam()[2],
                      GetParam()[3]);
    }
};

TEST_P(SnapshotGlScissorBoxTest, SetScissorBox) {
    GLint defaultViewport[] = {0, 0, kSurfaceSize[0], kSurfaceSize[1]};
    GLint testViewport[] = {GetParam()[0], GetParam()[1], GetParam()[2],
                            GetParam()[3]};
    setExpectedValues(defaultViewport, testViewport);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlScissorBoxTest,
                        ::testing::Values(kGLES2TestScissorBox));

// Tests preservation of stencil test conditional state, set by glStencilFunc..
class SnapshotGlStencilConditionsTest
    : public SnapshotSetValueTest<GlStencilFunc> {
    void stateCheck(const GLESv2Dispatch* gl, GlStencilFunc expected) {
        GLint frontFunc, frontRef, frontMask, backFunc, backRef, backMask;
        gl->glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);
        gl->glGetIntegerv(GL_STENCIL_REF, &frontRef);
        gl->glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontMask);
        gl->glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);
        gl->glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
        gl->glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backMask);
        EXPECT_EQ(expected.func, frontFunc);
        EXPECT_EQ(expected.ref, frontRef);
        EXPECT_EQ(expected.mask, frontMask);
        EXPECT_EQ(expected.func, backFunc);
        EXPECT_EQ(expected.ref, backRef);
        EXPECT_EQ(expected.mask, backMask);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        GlStencilFunc sFunc = *m_changed_value;
        gl->glStencilFunc(sFunc.func, sFunc.ref, sFunc.mask);
    }
};

class SnapshotGlStencilFuncTest : public SnapshotGlStencilConditionsTest,
                                  public ::testing::WithParamInterface<GLenum> {
};

class SnapshotGlStencilMaskTest : public SnapshotGlStencilConditionsTest,
                                  public ::testing::WithParamInterface<GLuint> {
};

TEST_P(SnapshotGlStencilFuncTest, SetStencilFunc) {
    GlStencilFunc defaultStencilFunc = {GL_ALWAYS, 0, kGLES2StencilDefaultMask};
    GlStencilFunc testStencilFunc = {GetParam(), kGLES2StencilRef, 0};
    setExpectedValues(defaultStencilFunc, testStencilFunc);
    doCheckedSnapshot();
}

TEST_P(SnapshotGlStencilMaskTest, SetStencilMask) {
    GlStencilFunc defaultStencilFunc = {GL_ALWAYS, 0, kGLES2StencilDefaultMask};
    GlStencilFunc testStencilFunc = {GL_ALWAYS, kGLES2StencilRef, GetParam()};
    setExpectedValues(defaultStencilFunc, testStencilFunc);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilFuncTest,
                        ::testing::ValuesIn(kGLES2StencilFuncs));

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilMaskTest,
                        ::testing::ValuesIn(kGLES2StencilMasks));

class SnapshotGlStencilConsequenceTest
    : public SnapshotSetValueTest<GlStencilOp> {
    void stateCheck(const GLESv2Dispatch* gl, GlStencilOp expected) override {
        GLint frontFail, frontDepthFail, frontDepthPass, backFail,
                backDepthFail, backDepthPass;
        gl->glGetIntegerv(GL_STENCIL_FAIL, &frontFail);
        gl->glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &frontDepthFail);
        gl->glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &frontDepthPass);
        gl->glGetIntegerv(GL_STENCIL_BACK_FAIL, &backFail);
        gl->glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &backDepthFail);
        gl->glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &backDepthPass);
        EXPECT_EQ(expected.sfail, frontFail);
        EXPECT_EQ(expected.dpfail, frontDepthFail);
        EXPECT_EQ(expected.dppass, frontDepthPass);
        EXPECT_EQ(expected.sfail, backFail);
        EXPECT_EQ(expected.dpfail, backDepthFail);
        EXPECT_EQ(expected.dppass, backDepthPass);
    }
    void stateChange(const GLESv2Dispatch* gl) override {
        GlStencilOp sOp = *m_changed_value;
        gl->glStencilOp(sOp.sfail, sOp.dpfail, sOp.dppass);
    }
};

class SnapshotGlStencilFailTest : public SnapshotGlStencilConsequenceTest,
                                  public ::testing::WithParamInterface<GLenum> {
};

class SnapshotGlStencilDepthFailTest
    : public SnapshotGlStencilConsequenceTest,
      public ::testing::WithParamInterface<GLenum> {};

class SnapshotGlStencilDepthPassTest
    : public SnapshotGlStencilConsequenceTest,
      public ::testing::WithParamInterface<GLenum> {};

TEST_P(SnapshotGlStencilFailTest, SetStencilOps) {
    GlStencilOp defaultStencilOp = {GL_KEEP, GL_KEEP, GL_KEEP};
    GlStencilOp testStencilOp = {GetParam(), GL_KEEP, GL_KEEP};
    setExpectedValues(defaultStencilOp, testStencilOp);
    doCheckedSnapshot();
}

TEST_P(SnapshotGlStencilDepthFailTest, SetStencilOps) {
    GlStencilOp defaultStencilOp = {GL_KEEP, GL_KEEP, GL_KEEP};
    GlStencilOp testStencilOp = {GL_KEEP, GetParam(), GL_KEEP};
    setExpectedValues(defaultStencilOp, testStencilOp);
    doCheckedSnapshot();
}

TEST_P(SnapshotGlStencilDepthPassTest, SetStencilOps) {
    GlStencilOp defaultStencilOp = {GL_KEEP, GL_KEEP, GL_KEEP};
    GlStencilOp testStencilOp = {GL_KEEP, GL_KEEP, GetParam()};
    setExpectedValues(defaultStencilOp, testStencilOp);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilFailTest,
                        ::testing::ValuesIn(kGLES2StencilOps));

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilDepthFailTest,
                        ::testing::ValuesIn(kGLES2StencilOps));

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilDepthPassTest,
                        ::testing::ValuesIn(kGLES2StencilOps));

}  // namespace emugl
