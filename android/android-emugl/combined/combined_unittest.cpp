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
#include <gtest/gtest.h>

#include "android/base/GLObjectCounter.h"
#include "android/base/files/PathUtils.h"
#include "android/base/perflogger/BenchmarkLibrary.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/opengles.h"

#include "AndroidBufferQueue.h"
#include "AndroidWindow.h"
#include "AndroidWindowBuffer.h"
#include "ClientComposer.h"
#include "Display.h"
#include "GoldfishOpenglTestEnv.h"
#include "GrallocDispatch.h"

#include <android/hardware_buffer.h>
#include <hardware/gralloc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <sstream>
#include <string>

#include <stdio.h>
#include <sys/time.h>

using aemu::AndroidBufferQueue;
using aemu::AndroidWindow;
using aemu::AndroidWindowBuffer;
using aemu::ClientComposer;
using aemu::Display;

using android::base::FunctorThread;
using android::base::pj;
using android::base::System;

static constexpr int kWindowSize = 256;

class CombinedGoldfishOpenglTest : public ::testing::Test {
protected:

    static GoldfishOpenglTestEnv* testEnv;

    static void SetUpTestCase() {
        testEnv = new GoldfishOpenglTestEnv;
    }

    static void TearDownTestCase() {
        delete testEnv;
        testEnv = nullptr;
    }

    struct EGLState {
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLConfig config;
        EGLSurface surface = EGL_NO_SURFACE;
        EGLContext context = EGL_NO_CONTEXT;
    };

    void SetUp() override {
        setupGralloc();
        setupEGLAndMakeCurrent();
        mBeforeTest = android::base::GLObjectCounter::get()->getCounts();
    }

    void TearDown() override {
        if (eglGetCurrentContext() != EGL_NO_CONTEXT) {
            glFinish(); // sync the pipe
        }
        mAfterTest = android::base::GLObjectCounter::get()->getCounts();
        for (int i = 0; i < mBeforeTest.size(); i++) {
            EXPECT_TRUE(mBeforeTest[i] >= mAfterTest[i]) <<
                "Leaked objects of type " << i;
        }
        teardownEGL();
        teardownGralloc();
    }

    void setupGralloc() {
        auto grallocPath = pj(System::get()->getProgramDirectory(), "lib64",
                              "gralloc.ranchu" LIBSUFFIX);

        load_gralloc_module(grallocPath.c_str(), &mGralloc);
        set_global_gralloc_module(&mGralloc);

        EXPECT_NE(nullptr, mGralloc.fb_dev);
        EXPECT_NE(nullptr, mGralloc.alloc_dev);
        EXPECT_NE(nullptr, mGralloc.fb_module);
        EXPECT_NE(nullptr, mGralloc.alloc_module);
    }

    void teardownGralloc() { unload_gralloc_module(&mGralloc); }

    void setupEGLAndMakeCurrent() {
        int maj, min;

        mEGL.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EXPECT_EQ(EGL_TRUE, eglInitialize(mEGL.display, &maj, &min));

        eglBindAPI(EGL_OPENGL_ES_API);

        static const EGLint configAttribs[] = {
                EGL_SURFACE_TYPE,   EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
                EGL_OPENGL_ES2_BIT, EGL_NONE,
        };

        int numConfigs;

        EXPECT_EQ(EGL_TRUE, eglChooseConfig(mEGL.display, configAttribs,
                                            &mEGL.config, 1, &numConfigs))
                << "Could not find GLES2 config!";

        EXPECT_GT(numConfigs, 0);

        static const EGLint pbufAttribs[] = {EGL_WIDTH, kWindowSize, EGL_HEIGHT,
                                             kWindowSize, EGL_NONE};

        mEGL.surface =
                eglCreatePbufferSurface(mEGL.display, mEGL.config, pbufAttribs);

        EXPECT_NE(EGL_NO_SURFACE, mEGL.surface)
                << "Could not create pbuffer surface!";

        static const EGLint contextAttribs[] = {
                EGL_CONTEXT_CLIENT_VERSION,
                2,
                EGL_NONE,
        };

        mEGL.context = eglCreateContext(mEGL.display, mEGL.config,
                                        EGL_NO_CONTEXT, contextAttribs);

        EXPECT_NE(EGL_NO_CONTEXT, mEGL.context) << "Could not create context!";

        EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, mEGL.surface,
                                           mEGL.surface, mEGL.context))
                << "eglMakeCurrent failed!";
    }

    void teardownEGL() {
        eglRelease();

        // Cancel all host threads as well
        android_finishOpenglesRenderer();
    }

    void eglRelease() {
        if (mEGL.display != EGL_NO_DISPLAY) {
            EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, EGL_NO_SURFACE,
                                               EGL_NO_SURFACE, EGL_NO_CONTEXT));
            EXPECT_EQ(EGL_TRUE, eglDestroyContext(mEGL.display, mEGL.context));
            EXPECT_EQ(EGL_TRUE, eglDestroySurface(mEGL.display, mEGL.surface));
            EXPECT_EQ(EGL_TRUE, eglTerminate(mEGL.display));
            EXPECT_EQ(EGL_TRUE, eglReleaseThread());
        }

        mEGL.display = EGL_NO_DISPLAY;
        mEGL.context = EGL_NO_CONTEXT;
        mEGL.surface = EGL_NO_SURFACE;
    }

    buffer_handle_t createTestGrallocBuffer(
            int usage = GRALLOC_USAGE_HW_RENDER,
            int format = HAL_PIXEL_FORMAT_RGBA_8888,
            int width = kWindowSize,
            int height = kWindowSize) {
        buffer_handle_t buffer;
        int stride;

        mGralloc.alloc(width, height, format, usage, &buffer, &stride);
        mGralloc.registerBuffer(buffer);

        (void)stride;

        return buffer;
    }

    void destroyTestGrallocBuffer(buffer_handle_t buffer) {
        mGralloc.unregisterBuffer(buffer);
        mGralloc.free(buffer);
    }

    std::vector<AndroidWindowBuffer> primeWindowBufferQueue(AndroidWindow& window) {
        EXPECT_NE(nullptr, window.fromProducer);
        EXPECT_NE(nullptr, window.toProducer);

        std::vector<AndroidWindowBuffer> buffers;
        for (int i = 0; i < AndroidBufferQueue::kCapacity; i++) {
            buffers.push_back(AndroidWindowBuffer(
                    &window, createTestGrallocBuffer()));
        }

        for (int i = 0; i < AndroidBufferQueue::kCapacity; i++) {
            window.fromProducer->queueBuffer(AndroidBufferQueue::Item(
                    (ANativeWindowBuffer*)(buffers.data() + i)));
        }
        return buffers;
    }

    class WindowWithBacking {
    public:
        WindowWithBacking(CombinedGoldfishOpenglTest* _env)
            : env(_env), window(kWindowSize, kWindowSize) {
            window.setProducer(&toWindow, &fromWindow);
            buffers = env->primeWindowBufferQueue(window);
        }

        ~WindowWithBacking() {
            for (auto& buffer : buffers) {
                env->destroyTestGrallocBuffer(buffer.handle);
            }
        }

        CombinedGoldfishOpenglTest* env;
        AndroidWindow window;
        AndroidBufferQueue toWindow;
        AndroidBufferQueue fromWindow;
        std::vector<AndroidWindowBuffer> buffers;
    };

    struct gralloc_implementation mGralloc;
    EGLState mEGL;
    std::vector<size_t> mBeforeTest;
    std::vector<size_t> mAfterTest;
};

// static
GoldfishOpenglTestEnv* CombinedGoldfishOpenglTest::testEnv = nullptr;

// Tests that the pipe connection can be made and that
// gralloc/EGL can be set up.

TEST_F(CombinedGoldfishOpenglTest, Basic) { }

// Tests allocation and deletion of a gralloc buffer.
TEST_F(CombinedGoldfishOpenglTest, GrallocBuffer) {
    buffer_handle_t buffer = createTestGrallocBuffer();
    destroyTestGrallocBuffer(buffer);
}

// Tests basic GLES usage.
TEST_F(CombinedGoldfishOpenglTest, ViewportQuery) {
    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);

    EXPECT_EQ(0, viewport[0]);
    EXPECT_EQ(0, viewport[1]);
    EXPECT_EQ(kWindowSize, viewport[2]);
    EXPECT_EQ(kWindowSize, viewport[3]);
}

// Tests eglCreateWindowSurface.
TEST_F(CombinedGoldfishOpenglTest, CreateWindowSurface) {
    AndroidWindow testWindow(kWindowSize, kWindowSize);

    AndroidBufferQueue toWindow;
    AndroidBufferQueue fromWindow;

    testWindow.setProducer(&toWindow, &fromWindow);

    AndroidWindowBuffer testBuffer(&testWindow, createTestGrallocBuffer());

    toWindow.queueBuffer(AndroidBufferQueue::Item(&testBuffer));

    static const EGLint attribs[] = {EGL_WIDTH, kWindowSize, EGL_HEIGHT,
                                     kWindowSize, EGL_NONE};
    EGLSurface testEGLWindow =
            eglCreateWindowSurface(mEGL.display, mEGL.config,
                                   (EGLNativeWindowType)(&testWindow), attribs);

    EXPECT_NE(EGL_NO_SURFACE, testEGLWindow);
    EXPECT_EQ(EGL_TRUE, eglDestroySurface(mEGL.display, testEGLWindow));

    destroyTestGrallocBuffer(testBuffer.handle);
}

// Starts a client composer, but doesn't do anything with it.
TEST_F(CombinedGoldfishOpenglTest, ClientCompositionSetup) {

    WindowWithBacking composeWindow(this);
    WindowWithBacking appWindow(this);

    ClientComposer composer(&composeWindow.window, &appWindow.fromWindow,
                            &appWindow.toWindow);
}

// Client composition, but also advances a frame.
TEST_F(CombinedGoldfishOpenglTest, ClientCompositionAdvanceFrame) {
    WindowWithBacking composeWindow(this);
    WindowWithBacking appWindow(this);

    AndroidBufferQueue::Item item;

    appWindow.toWindow.dequeueBuffer(&item);
    appWindow.fromWindow.queueBuffer(item);

    ClientComposer composer(&composeWindow.window, &appWindow.fromWindow,
                            &appWindow.toWindow);

    composer.advanceFrame();
    composer.join();
}

TEST_F(CombinedGoldfishOpenglTest, ShowWindow) {
    bool useWindow =
            System::get()->envGet("ANDROID_EMU_TEST_WITH_WINDOW") == "1";
    aemu::Display disp(useWindow, kWindowSize, kWindowSize);

    if (useWindow) {
        EXPECT_NE(nullptr, disp.getNative());
        android_showOpenglesWindow(disp.getNative(), 0, 0, kWindowSize,
                                   kWindowSize, kWindowSize, kWindowSize, 1.0,
                                   0, false);
    }

    for (int i = 0; i < 10; i++) {
        disp.loop();
        System::get()->sleepMs(1);
    }

    if (useWindow) android_hideOpenglesWindow();
}

TEST_F(CombinedGoldfishOpenglTest, DISABLED_ThreadCleanup) {
    // initial clean up.
    eglRelease();
    mBeforeTest = android::base::GLObjectCounter::get()->getCounts();
    for (int i = 0; i < 100; i++) {
        std::unique_ptr<FunctorThread> th(new FunctorThread([this]() {
            setupEGLAndMakeCurrent();
            eglRelease();
        }));
        ;
        th->start();
        th->wait();
    }
}

// Test case adapted from https://buganizer.corp.google.com/issues/111228391
TEST_F(CombinedGoldfishOpenglTest, SwitchContext) {
    EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, EGL_NO_SURFACE,
                                       EGL_NO_SURFACE, EGL_NO_CONTEXT));
    for (int i = 0; i < 100; i++) {
        std::unique_ptr<FunctorThread> th(new FunctorThread([this]() {
            EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, mEGL.surface,
                                               mEGL.surface, mEGL.context));
            EXPECT_EQ(EGL_TRUE, eglMakeCurrent(mEGL.display, EGL_NO_SURFACE,
                                               EGL_NO_SURFACE, EGL_NO_CONTEXT));
        }));
        th->start();
        th->wait();
    }
}

// Test that direct mapped memory works.
TEST_F(CombinedGoldfishOpenglTest, MappedMemory) {
    constexpr GLsizei kBufferSize = 64;

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, kBufferSize, 0, GL_DYNAMIC_DRAW);

    std::vector<uint8_t> data(kBufferSize);
    for (uint8_t i = 0; i < kBufferSize; ++i) {
        data[i] = i;
    }

    uint8_t* mapped =
        (uint8_t*)
            glMapBufferRange(
                GL_ARRAY_BUFFER, 0, kBufferSize, GL_MAP_WRITE_BIT);

    for (uint8_t i = 0; i < kBufferSize; ++i) {
        mapped[i] = data[i];
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    mapped =
        (uint8_t*)
            glMapBufferRange(
                GL_ARRAY_BUFFER, 0, kBufferSize, GL_MAP_READ_BIT);

    for (uint8_t i = 0; i < kBufferSize; ++i) {
        EXPECT_EQ(data[i], mapped[i]);
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &buffer);
}

// Test big buffer transfer
TEST_F(CombinedGoldfishOpenglTest, BigBuffer) {
    constexpr GLsizei kBufferSize = 4096;

    std::vector<uint8_t> buf(kBufferSize);

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glGetError();
    glBufferData(GL_ARRAY_BUFFER, kBufferSize, buf.data(), GL_DYNAMIC_DRAW);

    glGetError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &buffer);
}

static GLuint compileShader(GLenum shaderType, const char* src) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
    glCompileShader(shader);

    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
        fprintf(stderr, "Failed to compile shader. Info log: [%s]\n", infoLog.data());
    }

    return shader;
}

static GLint compileAndLinkShaderProgram(const char* vshaderSrc,
                                         const char* fshaderSrc) {
    GLuint vshader = compileShader(GL_VERTEX_SHADER, vshaderSrc);
    GLuint fshader = compileShader(GL_FRAGMENT_SHADER, fshaderSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    glDeleteShader(vshader);
    glDeleteShader(fshader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

    if (linkStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        glGetProgramInfoLog(program, infoLogLength, nullptr,
                            infoLog.data());
        fprintf(stderr, "Failed to link program. Info log: [%s]\n", infoLog.data());
    }

    return program;
}

// Test draw call rate. This is a perfgate benchmark
TEST_F(CombinedGoldfishOpenglTest, DrawCallRate) {
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

    GLint transformLoc = glGetUniformLocation(program, "transform");

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

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
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                     GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAttributes), 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes),
                              (GLvoid*)offsetof(VertexAttributes, color));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glUseProgram(program);

    glClearColor(0.2f, 0.2f, 0.3f, 0.0f);
    glViewport(0, 0, 1, 1);

    float matrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uint32_t drawCount = 0;
    static constexpr uint32_t kDrawCallLimit = 50000;

    auto cpuTimeStart = System::cpuTime();

    while (drawCount < kDrawCallLimit) {
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, matrix);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ++drawCount;
    }

    // Need a round trip at the end to get an accurate measurement
    glFinish();

    auto cpuTime = System::cpuTime() - cpuTimeStart;

    uint64_t duration_us = cpuTime.wall_time_us;
    uint64_t duration_cpu_us = cpuTime.usageUs();

    float ms = duration_us / 1000.0f;
    float sec = duration_us / 1000000.0f;

    float drawCallHz = kDrawCallLimit / sec;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &buffer);
    glUseProgram(0);
    glDeleteProgram(program);

    printf("Drew %u times in %f ms. Rate: %f Hz\n", kDrawCallLimit, ms,
           drawCallHz);

    android::perflogger::logDrawCallOverheadTest(
        (const char*)glGetString(GL_VENDOR),
        (const char*)glGetString(GL_RENDERER),
        (const char*)glGetString(GL_VERSION),
        "drawArrays", "fullStack",
        kDrawCallLimit,
        (long)drawCallHz,
        duration_us,
        duration_cpu_us);
}

extern "C" void glDrawArraysNullAEMU(GLenum mode, GLint first, GLsizei count);

// Test draw call rate without drawing on the host driver, which gives numbers
// closer to pure emulator overhead. This is a perfgate benchmark
TEST_F(CombinedGoldfishOpenglTest, DrawCallRateOverheadOnly) {
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

    GLint transformLoc = glGetUniformLocation(program, "transform");

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

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
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                     GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAttributes), 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes),
                              (GLvoid*)offsetof(VertexAttributes, color));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glUseProgram(program);

    glClearColor(0.2f, 0.2f, 0.3f, 0.0f);
    glViewport(0, 0, 1, 1);

    float matrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uint32_t drawCount = 0;
    static constexpr uint32_t kDrawCallLimit = 2000000;

    auto cpuTimeStart = System::cpuTime();

    while (drawCount < kDrawCallLimit) {
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, matrix);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glDrawArraysNullAEMU(GL_TRIANGLES, 0, 3);
        ++drawCount;
    }

    // Need a round trip at the end to get an accurate measurement
    glFinish();

    auto cpuTime = System::cpuTime() - cpuTimeStart;

    uint64_t duration_us = cpuTime.wall_time_us;
    uint64_t duration_cpu_us = cpuTime.usageUs();

    float ms = duration_us / 1000.0f;
    float sec = duration_us / 1000000.0f;

    float drawCallHz = kDrawCallLimit / sec;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &buffer);
    glUseProgram(0);
    glDeleteProgram(program);

    printf("Drew %u times in %f ms. Rate: %f Hz\n", kDrawCallLimit, ms,
           drawCallHz);

    android::perflogger::logDrawCallOverheadTest(
        (const char*)glGetString(GL_VENDOR),
        (const char*)glGetString(GL_RENDERER),
        (const char*)glGetString(GL_VERSION),
        "drawArrays", "fullStackOverheadOnly",
        kDrawCallLimit,
        (long)drawCallHz,
        duration_us,
        duration_cpu_us);
}

// Test uniform upload rate. This is a perfgate benchmark
TEST_F(CombinedGoldfishOpenglTest, UniformUploadRate) {
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

    GLint transformLoc = glGetUniformLocation(program, "transform");

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

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
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                     GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAttributes), 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(VertexAttributes),
                              (GLvoid*)offsetof(VertexAttributes, color));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glUseProgram(program);

    glClearColor(0.2f, 0.2f, 0.3f, 0.0f);
    glViewport(0, 0, 1, 1);

    float matrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uint32_t uploadCount = 0;
    static constexpr uint32_t kUploadLimit = 8000000;

    auto cpuTimeStart = System::cpuTime();

    while (uploadCount < kUploadLimit) {
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, matrix);
        ++uploadCount;
    }

    // Need a round trip at the end to get an accurate measurement
    glFinish();

    auto cpuTime = System::cpuTime() - cpuTimeStart;

    uint64_t duration_us = cpuTime.wall_time_us;
    uint64_t duration_cpu_us = cpuTime.usageUs();

    float ms = duration_us / 1000.0f;
    float sec = duration_us / 1000000.0f;

    float drawCallHz = kUploadLimit / sec;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &buffer);
    glUseProgram(0);
    glDeleteProgram(program);

    printf("glUniformMatrix4fv %u times in %f ms. Rate: %f Hz\n", kUploadLimit, ms,
           drawCallHz);

    android::perflogger::logDrawCallOverheadTest(
        (const char*)glGetString(GL_VENDOR),
        (const char*)glGetString(GL_RENDERER),
        (const char*)glGetString(GL_VERSION),
        "uniformUpload", "fullStack",
        kUploadLimit,
        (long)drawCallHz,
        duration_us,
        duration_cpu_us);
}

TEST_F(CombinedGoldfishOpenglTest, AndroidHardwareBuffer) {
    uint64_t usage =
        AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
        AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
        AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    uint64_t cpuUsage =
        AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
        AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;

    AHardwareBuffer_Desc desc = {
        kWindowSize, kWindowSize, 1,
        AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
        usage,
        4, // stride ignored for allocate; don't check this
    };

    AHardwareBuffer* buf;
    AHardwareBuffer_allocate(&desc, &buf);

    AHardwareBuffer_Desc outDesc;
    AHardwareBuffer_describe(buf, &outDesc);

    EXPECT_EQ(desc.width, outDesc.width);
    EXPECT_EQ(desc.height, outDesc.height);
    EXPECT_EQ(desc.layers, outDesc.layers);
    EXPECT_EQ(desc.format, outDesc.format);
    EXPECT_EQ(desc.usage, outDesc.usage);

    void* outVirtualAddress;

    ARect rect = { 0, 0, kWindowSize, kWindowSize, };

    EXPECT_EQ(0, AHardwareBuffer_lock(
        buf, cpuUsage, -1, &rect, &outVirtualAddress));
    EXPECT_EQ(0, AHardwareBuffer_unlock(buf, nullptr));

    AHardwareBuffer_release(buf);
}
