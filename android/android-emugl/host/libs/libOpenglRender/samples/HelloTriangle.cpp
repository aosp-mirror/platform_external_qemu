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

#include "FrameBuffer.h"

#include "android/base/system/System.h"

#include "RenderThreadInfo.h"

#include "GLTestGlobals.h"
#include "GLTestUtils.h"
#include "OpenGLTestContext.h"
#include "OSWindow.h"

#include <functional>

using android::base::System;

namespace emugl {

using namespace gltest;

class SampleWindow {
public:
    SampleWindow() {
        LazyLoadedEGLDispatch::get();
        LazyLoadedGLESv2Dispatch::get();

        bool useHostGpu = shouldUseHostGpu();
        mWindow = createOrGetTestWindow(mXOffset, mYOffset, mWidth, mHeight);
        mUseSubWindow = mWindow != nullptr;

        FrameBuffer::initialize(
                mWidth, mHeight,
                mUseSubWindow,
                !useHostGpu /* egl2egl */);
        if (mUseSubWindow) {
            mFb = FrameBuffer::getFB();

            mFb->setupSubWindow(
                (FBNativeWindowType)(uintptr_t)
                mWindow->getFramebufferNativeWindow(),
                0, 0,
                mWidth, mHeight, mWidth, mHeight,
                mWindow->getDevicePixelRatio(), 0, false);
            mWindow->messageLoop();
        } else {
                FrameBuffer::initialize(
                    mWidth, mHeight,
                    mUseSubWindow,
                    !useHostGpu /* egl2egl */);
            mFb = FrameBuffer::getFB();
        }

        mRenderThreadInfo = new RenderThreadInfo();

        mColorBuffer = mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
        mContext = mFb->createRenderContext(0, 0, GLESApi_3_0);
        mSurface = mFb->createWindowSurface(0, mWidth, mHeight);

        mFb->bindContext(mContext, mSurface, mSurface);
        mFb->setWindowSurfaceColorBuffer(mSurface, mColorBuffer);
    }

    ~SampleWindow() {
        if (mFb) {
            mFb->bindContext(0, 0, 0);
            mFb->closeColorBuffer(mColorBuffer);
            mFb->DestroyWindowSurface(mSurface);
            mFb->finalize();
            delete mFb;
        }
        delete mRenderThreadInfo;
    }

    void post() {
        mFb->flushWindowSurfaceColorBuffer(mSurface);
        if (mUseSubWindow) {
            mFb->post(mColorBuffer);
            mWindow->messageLoop();
        }
    }

    void initialize(const std::function<void()>& initCmds) {
        initCmds();
        mWindow->messageLoop();
    }

    void drawLoop(const std::function<void()>& drawCmds) {
        while (true) {
            drawCmds();
            post();
        }
    }

    bool mUseSubWindow = false;
    OSWindow* mWindow = nullptr;
    FrameBuffer* mFb = nullptr;
    RenderThreadInfo* mRenderThreadInfo = nullptr;

    int mWidth = 256;
    int mHeight = 256;
    int mXOffset= 400;
    int mYOffset= 400;

    HandleType mColorBuffer = 0;
    HandleType mContext = 0;
    HandleType mSurface = 0;
};

} // namespace emugl

GLuint compileShader(GLenum shaderType, const char* src) {
    auto gl = LazyLoadedGLESv2Dispatch::get();

    GLuint shader = gl->glCreateShader(shaderType);
    gl->glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
    gl->glCompileShader(shader);

    GLint compileStatus;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        gl->glGetShaderInfoLog(shader, infoLogLength, nullptr, &infoLog[0]);
        fprintf(stderr, "%s: fail to compile. infolog %s\n", __func__, &infoLog[0]);
    }

    return shader;
}

GLint compileAndLinkShaderProgram(const char* vshaderSrc, const char* fshaderSrc) {
    auto gl = LazyLoadedGLESv2Dispatch::get();

    GLuint vshader = compileShader(GL_VERTEX_SHADER, vshaderSrc);
    GLuint fshader = compileShader(GL_FRAGMENT_SHADER, fshaderSrc);

    GLuint program = gl->glCreateProgram();
    gl->glAttachShader(program, vshader);
    gl->glAttachShader(program, fshader);
    gl->glLinkProgram(program);

    GLint linkStatus;
    gl->glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    gl->glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

    if (linkStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        gl->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        gl->glGetProgramInfoLog(program, infoLogLength, nullptr, &infoLog[0]);

        fprintf(stderr, "%s: fail to link program. infolog: %s\n", __func__,
                &infoLog[0]);
    }

    return program;
}

int main(int argc, char** argv) {

    emugl::SampleWindow window;

    window.initialize([] {
        constexpr char vshaderSrc[] = R"(#version 300 es
        precision highp float;
        layout (location = 0) in vec2 pos;
        
        void main() {
            gl_Position = vec4(pos, 0.0, 1.0);
        }
        )";
        
        constexpr char fshaderSrc[] = R"(#version 300 es
        precision highp float;
        
        out vec4 fragColor;
        
        void main() {
            fragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
        )";

        GLint program = compileAndLinkShaderProgram(
                vshaderSrc,
                fshaderSrc
        );

        auto gl = LazyLoadedGLESv2Dispatch::get();

        gl->glEnableVertexAttribArray(0);

        const GLfloat vertexAttrs[] = {
            -0.5f, -0.5f,
            0.5f, -0.5f,
            0.0f, 0.5f,
        };

        gl->glVertexAttribPointerWithDataSize(
            0, 2, GL_FLOAT, GL_FALSE, 0, vertexAttrs, sizeof(vertexAttrs));
        gl->glUseProgram(program);
    });
  
    window.drawLoop([] {
        auto gl = LazyLoadedGLESv2Dispatch::get();
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    }); 

    return 0;
}
