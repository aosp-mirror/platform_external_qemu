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
#include "android/base/GLObjectCounter.h"
#include "android/base/perflogger/BenchmarkLibrary.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include "GLSnapshotTesting.h"
#include "GLTestUtils.h"
#include "Standalone.h"

#include <gtest/gtest.h>
#include <memory>


#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

using android::base::System;
using android::base::StdioStream;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

namespace emugl {

class FrameBufferTest : public ::testing::Test {
public:
    FrameBufferTest() : mTestSystem(PATH_SEP "progdir",
                                    android::base::System::kProgramBitness,
                                    PATH_SEP "homedir",
                                    PATH_SEP "appdir") { }

protected:

    virtual void SetUp() override {
        setupStandaloneLibrarySearchPaths();
        emugl::setGLObjectCounter(android::base::GLObjectCounter::get());

        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        ASSERT_NE(nullptr, egl);
        ASSERT_NE(nullptr, LazyLoadedGLESv2Dispatch::get());

        bool useHostGpu = shouldUseHostGpu();
        mWindow = createOrGetTestWindow(mXOffset, mYOffset, mWidth, mHeight);
        mUseSubWindow = mWindow != nullptr;

        if (mUseSubWindow) {
            ASSERT_NE(nullptr, mWindow->getFramebufferNativeWindow());

            EXPECT_TRUE(
                FrameBuffer::initialize(
                    mWidth, mHeight,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */));
            mFb = FrameBuffer::getFB();
            EXPECT_NE(nullptr, mFb);

            mFb->setupSubWindow(
                (FBNativeWindowType)(uintptr_t)
                mWindow->getFramebufferNativeWindow(),
                0, 0,
                mWidth, mHeight, mWidth, mHeight,
                mWindow->getDevicePixelRatio(), 0, false);
            mWindow->messageLoop();
        } else {
            EXPECT_TRUE(
                FrameBuffer::initialize(
                    mWidth, mHeight,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */));
            mFb = FrameBuffer::getFB();
            ASSERT_NE(nullptr, mFb);
        }
        EXPECT_EQ(EGL_SUCCESS, egl->eglGetError());

        mRenderThreadInfo = new RenderThreadInfo();

        // Snapshots
        mTestSystem.getTempRoot()->makeSubDir("Snapshots");
        mSnapshotPath = mTestSystem.getTempRoot()->makeSubPath("Snapshots");
        mTimeStamp = std::to_string(android::base::System::get()->getUnixTime());
        mSnapshotFile = mSnapshotPath + PATH_SEP "snapshot_" + mTimeStamp + ".snap";
        mTextureFile = mSnapshotPath + PATH_SEP "textures_" + mTimeStamp + ".stex";
    }

    virtual void TearDown() override {
        if (mFb) {
            delete mFb;  // destructor calls finalize
        }
        delete mRenderThreadInfo;
        EXPECT_EQ(EGL_SUCCESS, LazyLoadedEGLDispatch::get()->eglGetError())
                << "FrameBufferTest TearDown found EGL error";
    }

    void saveSnapshot() {
        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                    android_fopen(mSnapshotFile.c_str(), "wb"), StdioStream::kOwner));
        std::shared_ptr<TextureSaver> m_texture_saver(new TextureSaver(StdioStream(
                        android_fopen(mTextureFile.c_str(), "wb"), StdioStream::kOwner)));
        mFb->onSave(m_stream.get(), m_texture_saver);

        m_stream->close();
        m_texture_saver->done();
    }

    void loadSnapshot() {
        // unbind so load will destroy previous objects
        mFb->bindContext(0, 0, 0);

        std::unique_ptr<StdioStream> m_stream(new StdioStream(
                    android_fopen(mSnapshotFile.c_str(), "rb"), StdioStream::kOwner));
        std::shared_ptr<TextureLoader> m_texture_loader(
                new TextureLoader(StdioStream(android_fopen(mTextureFile.c_str(), "rb"),
                        StdioStream::kOwner)));
        mFb->onLoad(m_stream.get(), m_texture_loader);
        m_stream->close();
        m_texture_loader->join();
    }

    bool mUseSubWindow = false;
    OSWindow* mWindow = nullptr;
    FrameBuffer* mFb = nullptr;
    RenderThreadInfo* mRenderThreadInfo = nullptr;

    int mWidth = 256;
    int mHeight = 256;
    int mXOffset= 400;
    int mYOffset= 400;

    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath;
    std::string mTimeStamp;
    std::string mSnapshotFile;
    std::string mTextureFile;
};

// Tests that framebuffer initialization and finalization works.
TEST_F(FrameBufferTest, FrameBufferBasic) {
}

// Tests the creation of a single color buffer for the framebuffer.
TEST_F(FrameBufferTest, CreateColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    // FramBuffer::finalize handles color buffer destruction here
}

// Tests both creation and closing a color buffer.
TEST_F(FrameBufferTest, CreateCloseColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    mFb->closeColorBuffer(handle);
}

// Tests create, open, and close color buffer.
TEST_F(FrameBufferTest, CreateOpenCloseColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));
    mFb->closeColorBuffer(handle);
}

// Tests that the color buffer can be update with a test pattern and that
// the test pattern can be read back from the color buffer.
TEST_F(FrameBufferTest, CreateOpenUpdateCloseColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));

    TestTexture forUpdate = createTestPatternRGBA8888(mWidth, mHeight);
    mFb->updateColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forUpdate.data());

    TestTexture forRead = createTestTextureRGBA8888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 4, mWidth, forUpdate.data(), forRead.data()));

    mFb->closeColorBuffer(handle);
}

// bug: 110105029
// Tests that color buffer updates should not fail if there is a format change.
// Needed to accomodate format-changing behavior from the guest gralloc.
TEST_F(FrameBufferTest, CreateOpenUpdateCloseColorBuffer_FormatChange) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));

    TestTexture forUpdate = createTestPatternRGB888(mWidth, mHeight);
    mFb->updateColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, forUpdate.data());

    TestTexture forRead = createTestTextureRGB888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 3, mWidth, forUpdate.data(),
                             forRead.data()));

    mFb->closeColorBuffer(handle);
}

// Tests obtaining EGL configs from FrameBuffer.
TEST_F(FrameBufferTest, Configs) {
    const FbConfigList* configs = mFb->getConfigs();
    EXPECT_GE(configs->size(), 0);
}

// Tests creating GL context from FrameBuffer.
TEST_F(FrameBufferTest, CreateRenderContext) {
    HandleType handle = mFb->createRenderContext(0, 0, GLESApi_3_0);
    EXPECT_NE(0, handle);
}

// Tests creating window surface from FrameBuffer.
TEST_F(FrameBufferTest, CreateWindowSurface) {
    HandleType handle = mFb->createWindowSurface(0, mWidth, mHeight);
    EXPECT_NE(0, handle);
}

// Tests eglMakeCurrent from FrameBuffer.
TEST_F(FrameBufferTest, CreateBindRenderContext) {
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);
    EXPECT_TRUE(mFb->bindContext(context, surface, surface));
}

// A basic blit test that simulates what the guest system does in one pass
// of draw + eglSwapBuffers:
// 1. Draws in OpenGL with glClear.
// 2. Calls flushWindowSurfaceColorBuffer(), which is the "backing operation" of
// ANativeWindow::queueBuffer in the guest.
// 3. Calls post() with the resulting color buffer, the backing operation of fb device "post"
// in the guest.
TEST_F(FrameBufferTest, BasicBlit) {
    auto gl = LazyLoadedGLESv2Dispatch::get();

    HandleType colorBuffer =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);

    EXPECT_TRUE(mFb->bindContext(context, surface, surface));
    EXPECT_TRUE(mFb->setWindowSurfaceColorBuffer(surface, colorBuffer));

    float colors[3][4] = {
        { 1.0f, 0.0f, 0.0f, 1.0f},
        { 0.0f, 1.0f, 0.0f, 1.0f},
        { 0.0f, 0.0f, 1.0f, 1.0f},
    };

    for (int i = 0; i < 3; i++) {
        float* color = colors[i];

        gl->glClearColor(color[0], color[1], color[2], color[3]);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mFb->flushWindowSurfaceColorBuffer(surface);

        TestTexture targetBuffer =
            createTestTextureRGBA8888SingleColor(
                mWidth, mHeight, color[0], color[1], color[2], color[3]);

        TestTexture forRead =
            createTestTextureRGBA8888SingleColor(
                mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);

        mFb->readColorBuffer(
            colorBuffer, 0, 0, mWidth, mHeight,
            GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

        EXPECT_TRUE(
            ImageMatches(
                mWidth, mHeight, 4, mWidth,
                targetBuffer.data(), forRead.data()));

        if (mUseSubWindow) {
            mFb->post(colorBuffer);
            mWindow->messageLoop();
        }
    }

    EXPECT_TRUE(mFb->bindContext(0, 0, 0));
    mFb->closeColorBuffer(colorBuffer);
    mFb->closeColorBuffer(colorBuffer);
    mFb->DestroyWindowSurface(surface);
}

// Tests that snapshot works with an empty FrameBuffer.
TEST_F(FrameBufferTest, SnapshotSmokeTest) {
    saveSnapshot();
    loadSnapshot();
}

// Tests that the snapshot restores the clear color state, by changing the clear
// color in between save and load. If this fails, it means failure to restore a
// number of different states from GL contexts.
TEST_F(FrameBufferTest, SnapshotPreserveColorClear) {
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);
    EXPECT_TRUE(mFb->bindContext(context, surface, surface));

    auto gl = LazyLoadedGLESv2Dispatch::get();
    gl->glClearColor(1, 1, 1, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, {1, 1, 1, 1}));

    saveSnapshot();

    gl->glClearColor(0.5, 0.5, 0.5, 0.5);
    EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE,
                                      {0.5, 0.5, 0.5, 0.5}));

    loadSnapshot();
    EXPECT_TRUE(mFb->bindContext(context, surface, surface));

    EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, {1, 1, 1, 1}));
}

// Tests that snapshot works to save the state of a single ColorBuffer; we
// upload a test pattern to the ColorBuffer, take a snapshot, load it, and
// verify that the contents are the same.
TEST_F(FrameBufferTest, SnapshotSingleColorBuffer) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);

    TestTexture forUpdate = createTestPatternRGBA8888(mWidth, mHeight);
    mFb->updateColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forUpdate.data());

    saveSnapshot();
    loadSnapshot();

    TestTexture forRead = createTestTextureRGBA8888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 4, mWidth, forUpdate.data(), forRead.data()));

    mFb->closeColorBuffer(handle);
}

// bug: 111360779
// Tests that the ColorBuffer is successfully updated even if a reformat happens
// on restore; the reformat may mess up the texture restore logic.
// In ColorBuffer::subUpdate, this test is known to fail if touch() is moved after the reformat.
TEST_F(FrameBufferTest, SnapshotColorBufferSubUpdateRestore) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);

    saveSnapshot();
    loadSnapshot();

    TestTexture forUpdate = createTestPatternRGBA8888(mWidth, mHeight);
    mFb->updateColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forUpdate.data());

    TestTexture forRead = createTestTextureRGBA8888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 4, mWidth, forUpdate.data(), forRead.data()));

    mFb->closeColorBuffer(handle);
}

// bug: 111558407
// Tests that ColorBuffer's blit path is retained on save/restore.
TEST_F(FrameBufferTest, SnapshotFastBlitRestore) {
    HandleType handle = mFb->createColorBuffer(mWidth, mHeight, GL_RGBA,
                                               FRAMEWORK_FORMAT_GL_COMPATIBLE);

    EXPECT_TRUE(mFb->isFastBlitSupported());

    mFb->lock();
    EXPECT_EQ(mFb->isFastBlitSupported(),
              mFb->getColorBuffer_locked(handle)->isFastBlitSupported());
    mFb->unlock();

    saveSnapshot();
    loadSnapshot();

    mFb->lock();
    EXPECT_EQ(mFb->isFastBlitSupported(),
              mFb->getColorBuffer_locked(handle)->isFastBlitSupported());
    mFb->unlock();

    mFb->closeColorBuffer(handle);
}

// Tests the API to completely replace a ColorBuffer.
TEST_F(FrameBufferTest, ReplaceContentsTest) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);

    EXPECT_NE(0, handle);
    EXPECT_EQ(0, mFb->openColorBuffer(handle));

    TestTexture forUpdate = createTestPatternRGBA8888(mWidth, mHeight);
    mFb->replaceColorBufferContents(handle, forUpdate.data(), mWidth * mHeight * 4);

    TestTexture forRead = createTestTextureRGBA8888SingleColor(mWidth, mHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    mFb->readColorBuffer(handle, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, forRead.data());

    EXPECT_TRUE(ImageMatches(mWidth, mHeight, 4, mWidth, forUpdate.data(), forRead.data()));

    mFb->closeColorBuffer(handle);
}

// Tests rate of draw calls with no guest/host communication, but with translator.
static constexpr uint32_t kDrawCallLimit = 50000;

TEST_F(FrameBufferTest, DrawCallRate) {
    HandleType colorBuffer =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);

    EXPECT_TRUE(mFb->bindContext(context, surface, surface));
    EXPECT_TRUE(mFb->setWindowSurfaceColorBuffer(surface, colorBuffer));

    auto gl = LazyLoadedGLESv2Dispatch::get();

    constexpr char vshaderSrc[] = R"(#version 300 es
    precision highp float;

    layout (location = 0) in vec2 pos;
    layout (location = 1) in vec3 color;

    uniform mat4 transform;

    out vec3 color_varying;

    void main() {
        gl_Position = transform * vec4(pos, 0.0, 1.0);
        color_varying = (transform * vec4(color, 1.0)).xyz;
    }
    )";
    constexpr char fshaderSrc[] = R"(#version 300 es
    precision highp float;

    in vec3 color_varying;

    out vec4 fragColor;

    void main() {
        fragColor = vec4(color_varying, 1.0);
    }
    )";

    GLuint program = compileAndLinkShaderProgram(vshaderSrc, fshaderSrc);

    GLint transformLoc = gl->glGetUniformLocation(program, "transform");

    struct VertexAttributes {
        float position[2];
        float color[3];
    };

    const VertexAttributes vertexAttrs[] = {
        { { -0.5f, -0.5f,}, { 0.2, 0.1, 0.9, }, },
        { { 0.5f, -0.5f,}, { 0.8, 0.3, 0.1,}, },
        { { 0.0f, 0.5f,}, { 0.1, 0.9, 0.6,}, },
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
    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(1);

    gl->glUseProgram(program);

    gl->glClearColor(0.2f, 0.2f, 0.3f, 0.0f);
    gl->glViewport(0, 0, 1, 1);

    float matrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uint32_t drawCount = 0;

    auto cpuTimeStart = System::cpuTime();

fprintf(stderr, "%s: transform loc %d\n", __func__, transformLoc);

    while (drawCount < kDrawCallLimit) {
        gl->glUniformMatrix4fv(transformLoc, 1, GL_FALSE, matrix);
        gl->glBindBuffer(GL_ARRAY_BUFFER, buffer);
        gl->glDrawArrays(GL_TRIANGLES, 0, 3);
        ++drawCount;
    }

    gl->glFinish();

    auto cpuTime = System::cpuTime() - cpuTimeStart;

    uint64_t duration_us = cpuTime.wall_time_us;
    uint64_t duration_cpu_us = cpuTime.usageUs();

    float ms = duration_us / 1000.0f;
    float sec = duration_us / 1000000.0f;
    float drawCallHz = (float)kDrawCallLimit / sec;

    printf("Drew %u times in %f ms. Rate: %f Hz\n", kDrawCallLimit, ms, drawCallHz);

    android::perflogger::logDrawCallOverheadTest(
        (const char*)gl->glGetString(GL_VENDOR),
        (const char*)gl->glGetString(GL_RENDERER),
        (const char*)gl->glGetString(GL_VERSION),
        "drawArrays", "decoder",
        kDrawCallLimit,
        (long)drawCallHz,
        duration_us,
        duration_cpu_us);

    EXPECT_TRUE(mFb->bindContext(0, 0, 0));
    mFb->closeColorBuffer(colorBuffer);
    mFb->closeColorBuffer(colorBuffer);
    mFb->DestroyWindowSurface(surface);
}

// Tests rate of draw calls with only the host driver and no translator.
TEST_F(FrameBufferTest, HostDrawCallRate) {
    HandleType colorBuffer =
        mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    HandleType context = mFb->createRenderContext(0, 0, GLESApi_3_0);
    HandleType surface = mFb->createWindowSurface(0, mWidth, mHeight);

    EXPECT_TRUE(mFb->bindContext(context, surface, surface));
    EXPECT_TRUE(mFb->setWindowSurfaceColorBuffer(surface, colorBuffer));

    auto gl = LazyLoadedGLESv2Dispatch::get();

    uint64_t duration_us, duration_cpu_us;
    gl->glTestHostDriverPerformance(kDrawCallLimit, &duration_us, &duration_cpu_us);

    float ms = duration_us / 1000.0f;
    float sec = duration_us / 1000000.0f;
    float drawCallHz = kDrawCallLimit / sec;

    printf("Drew %u times in %f ms. Rate: %f Hz\n", kDrawCallLimit, ms, drawCallHz);

    android::perflogger::logDrawCallOverheadTest(
        (const char*)gl->glGetString(GL_VENDOR),
        (const char*)gl->glGetString(GL_RENDERER),
        (const char*)gl->glGetString(GL_VERSION),
        "drawArrays", "host",
        kDrawCallLimit,
        (long)drawCallHz,
        duration_us,
        duration_cpu_us);

    EXPECT_TRUE(mFb->bindContext(0, 0, 0));
    mFb->closeColorBuffer(colorBuffer);
    mFb->closeColorBuffer(colorBuffer);
    mFb->DestroyWindowSurface(surface);
}

// Tests Vulkan interop query.
TEST_F(FrameBufferTest, VulkanInteropQuery) {
    auto egl = LazyLoadedEGLDispatch::get();

    EXPECT_NE(nullptr, egl->eglQueryVulkanInteropSupportANDROID);

    EGLBoolean supported =
        egl->eglQueryVulkanInteropSupportANDROID();

    // Disregard the result for now 
    (void)supported;
}

// Tests ColorBuffer with GL_BGRA input.
TEST_F(FrameBufferTest, CreateColorBufferBGRA) {
    HandleType handle =
        mFb->createColorBuffer(mWidth, mHeight, GL_BGRA_EXT, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    EXPECT_NE(0, handle);
    // FramBuffer::finalize handles color buffer destruction here
}

}  // namespace emugl
