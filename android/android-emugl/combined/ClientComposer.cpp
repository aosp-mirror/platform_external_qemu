// Copyright 2018 The Android Open Source Project
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
#include "ClientComposer.h"

#include "AndroidBufferQueue.h"
#include "AndroidWindow.h"

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "android/base/synchronization/Lock.h"
#include "android/base/threads/WorkerThread.h"

#include <atomic>
#include <unordered_map>

#include <stdio.h>

#define E(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

using android::base::AutoLock;
using android::base::WorkerProcessingResult;
using android::base::WorkerThread;
using android::base::Lock;

namespace aemu {

class ClientComposer::Impl {
public:
    enum Command {
        Stop = 0,
        AdvanceFrame = 1,
    };

    Impl(AndroidWindow* composeWindow,
         AndroidBufferQueue* fromApp,
         AndroidBufferQueue* toApp)
        : mWorkerThread([this, composeWindow, fromApp, toApp](
                                Command&& command) {
              Command cmd = command;
              return this->composeLoop(composeWindow, fromApp, toApp, cmd);
          }) {
        mWorkerThread.start();
    }

    void join() {
        mWorkerThread.enqueue(Command::Stop);
        mWorkerThread.join();
    }

    ~Impl() {
        join();
    }

    void advanceFrame() {
        mWorkerThread.enqueue(Command::AdvanceFrame);
    }

private:

    void initialize(AndroidWindow* composeWindow) {
        mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EGLint maj, min;
        eglInitialize(mDisplay, &maj, &min);

        eglBindAPI(EGL_OPENGL_ES_API);
        EGLint total_num_configs = 0;
        eglGetConfigs(mDisplay, 0, 0, &total_num_configs);

        EGLint wantedRedSize = 8;
        EGLint wantedGreenSize = 8;
        EGLint wantedBlueSize = 8;

        const GLint configAttribs[] = {
            EGL_RED_SIZE, wantedRedSize,
            EGL_GREEN_SIZE, wantedGreenSize,
            EGL_BLUE_SIZE, wantedBlueSize,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };

        EGLint total_compatible = 0;

        std::vector<EGLConfig> configs(total_num_configs);
        eglChooseConfig(mDisplay, configAttribs, configs.data(), total_num_configs,
                        &total_compatible);

        EGLint exact_match_index = -1;

        for (EGLint i = 0; i < total_compatible; i++) {
            EGLint r, g, b;
            EGLConfig c = configs[i];
            eglGetConfigAttrib(mDisplay, c, EGL_RED_SIZE, &r);
            eglGetConfigAttrib(mDisplay, c, EGL_GREEN_SIZE, &g);
            eglGetConfigAttrib(mDisplay, c, EGL_BLUE_SIZE, &b);

            if (r == wantedRedSize && g == wantedGreenSize &&
                b == wantedBlueSize) {
                exact_match_index = i;
                break;
            }
        }

        EGLConfig match = configs[exact_match_index];

        static const GLint glesAtt[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                        EGL_NONE};

        mContext = eglCreateContext(mDisplay, match, EGL_NO_CONTEXT, glesAtt);

        mSurface = eglCreateWindowSurface(
                mDisplay, match, (EGLNativeWindowType)composeWindow, 0);

        eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);

        static constexpr char blitVshaderSrc[] = R"(
        precision highp float;

        attribute vec2 pos;
        attribute vec2 texcoord;

        varying vec2 texcoord_varying;

        void main() {
            gl_Position = vec4(pos, 0.0, 1.0);
            texcoord_varying = texcoord;
        })";
         
        static constexpr char blitFshaderSrc[] = R"(
        precision highp float;

        uniform sampler2D tex;

        varying vec2 texcoord_varying;

        void main() {
            gl_FragColor = texture2D(tex, texcoord_varying);
        })";

        mProgram =
            compileAndLinkShaderProgram(
                blitVshaderSrc, blitFshaderSrc);
        
        GLint samplerLoc = glGetUniformLocation(mProgram, "tex");

        glGenBuffers(1, &mVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mVbo);
        const float attrs[] = {
                -1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,
                1.0f,  1.0f,  1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f,
                1.0f,  1.0f,  1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);

        mPosLoc = glGetAttribLocation(mProgram, "pos");
        mTexCoordLoc = glGetAttribLocation(mProgram, "texcoord");

        glEnableVertexAttribArray(mPosLoc);
        glEnableVertexAttribArray(mTexCoordLoc);

        glVertexAttribPointer(mPosLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                                 0);
        glVertexAttribPointer(mTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                                 (GLvoid*)(uintptr_t)(2 * sizeof(GLfloat)));

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);

        glUseProgram(mProgram);
        glUniform1i(samplerLoc, 0);

        mInitialized = true;
    }

    WorkerProcessingResult composeLoop(AndroidWindow* composeWindow,
                                       AndroidBufferQueue* fromApp,
                                       AndroidBufferQueue* toApp,
                                       Command command) {
        if (!mInitialized)
            initialize(composeWindow);

        AndroidBufferQueue::Item appItem = {};

        switch (command) {
            default:
            case Command::Stop:
                teardown();
                return WorkerProcessingResult::Stop;
            case Command::AdvanceFrame:
                fromApp->dequeueBuffer(&appItem);

                glEGLImageTargetTexture2DOES(
                        GL_TEXTURE_2D, createOrGetEGLImage(appItem.buffer));

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);

                glDrawArrays(GL_TRIANGLES, 0, 6);
                toApp->queueBuffer(
                        AndroidBufferQueue::Item(appItem.buffer, -1));

                eglSwapBuffers(mDisplay, mSurface);
                return WorkerProcessingResult::Continue;
        }
    }

    void teardown() {
        for (auto it : mEGLImages) {
            eglDestroyImageKHR(mDisplay, it.second);
        }

        glDisableVertexAttribArray(mPosLoc);
        glDisableVertexAttribArray(mTexCoordLoc);

        glUseProgram(0);

        glDeleteProgram(mProgram);

        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &mTexture);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &mVbo);

        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        eglDestroySurface(mDisplay, mSurface);
        eglDestroyContext(mDisplay, mContext);
        eglReleaseThread();
    }

    EGLImageKHR createOrGetEGLImage(ANativeWindowBuffer* buffer) {
        auto it = mEGLImages.find(buffer);
        if (it != mEGLImages.end()) return it->second;

        EGLImageKHR image =
                eglCreateImageKHR(mDisplay, 0, EGL_NATIVE_BUFFER_ANDROID,
                                  (EGLClientBuffer)buffer, 0);

        mEGLImages[buffer] = image;

        return image;
    }

    GLuint compileShader(GLenum shaderType, const char* src) {
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
            E("Failed to compile shader. Info log: [%s]", infoLog.data());
        }

        return shader;
    }

    GLint compileAndLinkShaderProgram(const char* vshaderSrc,
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
            E("Failed to link program. Info log: [%s]\n", infoLog.data());
        }

        return program;
    }

    // EGL/GLES related state
    bool mInitialized = false;

    EGLDisplay mDisplay;
    EGLContext mContext;
    EGLSurface mSurface;
    GLuint mPosLoc;
    GLuint mTexCoordLoc;
    GLuint mProgram;
    GLuint mTexture;
    GLuint mVbo;

    std::unordered_map<ANativeWindowBuffer*, EGLImageKHR> mEGLImages;

    WorkerThread<Command> mWorkerThread;
};

ClientComposer::ClientComposer(AndroidWindow* composeWindow,
                               AndroidBufferQueue* fromApp,
                               AndroidBufferQueue* toApp)
    : mImpl(new ClientComposer::Impl(composeWindow, fromApp, toApp)) {}

ClientComposer::~ClientComposer() = default;

void ClientComposer::join() {
    mImpl->join();
}

void ClientComposer::advanceFrame() {
    mImpl->advanceFrame();
}

}  // namespace aemu