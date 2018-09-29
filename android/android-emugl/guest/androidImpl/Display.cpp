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
// limitations under the License.#pragma once
#define __ANDROID__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include "GoldfishOpenGLDispatch.h"
#include "GrallocDispatch.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

#include "emugl/common/OpenGLDispatchLoader.h"

#include <unordered_map>

#include <system/window.h>
#include <stdio.h>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::MessageChannel;
using android::base::System;

namespace aemu {

class BQ {
public:
    static constexpr int kCapacity = 3;

    class Item {
    public:
        Item(ANativeWindowBuffer* buf = 0, int fd = -1) :
            buffer(buf), fenceFd(fd) { }
        ANativeWindowBuffer* buffer = 0;
        int fenceFd = -1;
    };

    void queueBuffer(const Item& item) {
        mQueue.send(item);
    }

    void dequeueBuffer(Item* outItem) {
        mQueue.receive(outItem);
    }

private:
    MessageChannel<Item, kCapacity> mQueue;
};



static int hook_setSwapInterval(struct ANativeWindow* window, int interval);
static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer);
static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_query(const struct ANativeWindow* window, int what, int* value);
static int hook_perform(struct ANativeWindow* window, int operation, ... );
static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd);
static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd);
static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd);
static void hook_incRef(struct android_native_base_t* common);
static void hook_decRef(struct android_native_base_t* common);

class VirtualSurface : public ANativeWindow {
public:
    VirtualSurface() {
        // Initialize the ANativeWindow function pointers.
        ANativeWindow::setSwapInterval  = hook_setSwapInterval;
        ANativeWindow::dequeueBuffer    = hook_dequeueBuffer;
        ANativeWindow::cancelBuffer     = hook_cancelBuffer;
        ANativeWindow::queueBuffer      = hook_queueBuffer;
        ANativeWindow::query            = hook_query;
        ANativeWindow::perform          = hook_perform;

        ANativeWindow::dequeueBuffer_DEPRECATED = hook_dequeueBuffer_DEPRECATED;
        ANativeWindow::cancelBuffer_DEPRECATED  = hook_cancelBuffer_DEPRECATED;
        ANativeWindow::lockBuffer_DEPRECATED    = hook_lockBuffer_DEPRECATED;
        ANativeWindow::queueBuffer_DEPRECATED   = hook_queueBuffer_DEPRECATED;

        const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
        const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;

        common.incRef = hook_incRef;
        common.decRef = hook_decRef;
    }

    void setProducer(BQ* fromSf, BQ* toSf) {
        mFromSf = fromSf;
        mToSf = toSf;
    }

    int dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd) {
        assert(mFromSf);
        BQ::Item item;
        mFromSf->dequeueBuffer(&item);
        *buffer = item.buffer;
        if (fenceFd) *fenceFd = item.fenceFd;
        return 0;
    }

    int queueBuffer(ANativeWindowBuffer* buffer, int fenceFd) {
        assert(mToSf);
        mToSf->queueBuffer({ buffer, fenceFd });
        return 0;
    }

    int query(int what, int* value) const {
        switch (what) {
            case NATIVE_WINDOW_WIDTH:
                *value = 256;
                break;
            case NATIVE_WINDOW_HEIGHT:
                *value = 256;
                break;
            default:
                break;
        }
        return 0;
    }

    BQ* mFromSf = nullptr;
    BQ* mToSf = nullptr;

    int swapInterval = 1;

    static VirtualSurface* getSelf(ANativeWindow* window) { return (VirtualSurface*)(window); }
    static const VirtualSurface* getSelfConst(const ANativeWindow* window) { return (const VirtualSurface*)(window); }
};

// Android native window implementation
static int hook_setSwapInterval(struct ANativeWindow* window, int interval) {
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    surface->swapInterval = interval;
    return 0;
}

static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer) {
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    return surface->dequeueBuffer(buffer, nullptr);
}

static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_query(const struct ANativeWindow* window, int what, int* value) {
    const VirtualSurface* surface = VirtualSurface::getSelfConst(window);
    return surface->query(what, value);
}

static int hook_perform(struct ANativeWindow* window, int operation, ... ) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd) {
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    return surface->dequeueBuffer(buffer, fenceFd);
}

static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    VirtualSurface* surface = VirtualSurface::getSelf(window);
    return surface->queueBuffer(buffer, fenceFd);
}

static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static void hook_incRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p call\n", __func__, common);
}

static void hook_decRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p call\n", __func__, common);
}

class SurfaceFlinger {
public:
    SurfaceFlinger() = default;

    void setComposeWindow(ANativeWindow* w) {
        mWindow = w;
    }

    void setAppProducer(BQ* app2sf, BQ* sf2App) {
        AutoLock lock(mLock);
        mApp2SfQueue = app2sf;
        mSf2AppQueue = sf2App;

        if (!mThread) {
            composeThread();
        }
    }

    void composeThread() {
        mThread = new FunctorThread([this]() {
            auto& egl = emugl::loadGoldfishOpenGL()->egl;
            auto& gl = emugl::loadGoldfishOpenGL()->gles2;

            EGLDisplay d = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
            EGLint maj, min;
            egl.eglInitialize(d, &maj, &min);

            egl.eglBindAPI(EGL_OPENGL_ES_API);
            EGLint total_num_configs = 0;
            egl.eglGetConfigs(d, 0, 0, &total_num_configs);

            EGLint wantedRedSize = 8;
            EGLint wantedGreenSize = 8;
            EGLint wantedBlueSize = 8;

            const GLint configAttribs[] = {
                    EGL_RED_SIZE,       wantedRedSize,
                    EGL_GREEN_SIZE, wantedGreenSize,    EGL_BLUE_SIZE, wantedBlueSize,
                    EGL_SURFACE_TYPE,   EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
                    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
            EGLint total_compatible = 0;

            std::vector<EGLConfig> configs(total_num_configs);
            egl.eglChooseConfig(d, configAttribs, configs.data(), total_num_configs, &total_compatible);

            EGLint exact_match_index = -1;
            for (EGLint i = 0; i < total_compatible; i++) {
                EGLint r, g, b;
                EGLConfig c = configs[i];
                egl.eglGetConfigAttrib(d, c, EGL_RED_SIZE, &r);
                egl.eglGetConfigAttrib(d, c, EGL_GREEN_SIZE, &g);
                egl.eglGetConfigAttrib(d, c, EGL_BLUE_SIZE, &b);

                if (r == wantedRedSize && g == wantedGreenSize && b == wantedBlueSize) {
                    exact_match_index = i;
                    break;
                }
            }

            EGLConfig match = configs[exact_match_index];

            static const GLint glesAtt[] =
            { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

            EGLContext c = egl.eglCreateContext(d, match, EGL_NO_CONTEXT, glesAtt);

            assert(mWindow);

            EGLSurface s = egl.eglCreateWindowSurface(d, match, mWindow, 0);
            
            egl.eglMakeCurrent(d, s, s, c);

            fprintf(stderr, "%s: extensionStr %s\n", __func__, gl.glGetString(GL_EXTENSIONS));

            static constexpr char blitVshaderSrc[] = R"(#version 300 es
            precision highp float;
            layout (location = 0) in vec2 pos;
            layout (location = 1) in vec2 texcoord;
            out vec2 texcoord_varying;
            void main() {
                gl_Position = vec4(pos, 0.0, 1.0);
                texcoord_varying = texcoord;
            })";

            static constexpr char blitFshaderSrc[] = R"(#version 300 es
            precision highp float;
            uniform sampler2D tex;
            in vec2 texcoord_varying;
            out vec4 fragColor;
            void main() {
                fragColor = texture(tex, texcoord_varying);
            })";

            GLint blitProgram =
                compileAndLinkShaderProgram(
                    blitVshaderSrc, blitFshaderSrc);

            GLint samplerLoc = gl.glGetUniformLocation(blitProgram, "tex");

            GLuint blitVbo;
            gl.glGenBuffers(1, &blitVbo);
            gl.glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
            const float attrs[] = {
                -1.0f, -1.0f, 0.0f, 1.0f,
                1.0f, -1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f,
                -1.0f, -1.0f, 0.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f,
                -1.0f, 1.0f, 0.0f, 0.0f,
            };
            gl.glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);
            gl.glEnableVertexAttribArray(0);
            gl.glEnableVertexAttribArray(1);

            gl.glVertexAttribPointer(
                0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
            gl.glVertexAttribPointer(
                1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                (GLvoid*)(uintptr_t)(2 * sizeof(GLfloat)));

            GLuint blitTexture;
            gl.glActiveTexture(GL_TEXTURE0);
            gl.glGenTextures(1, &blitTexture);
            gl.glBindTexture(GL_TEXTURE_2D, blitTexture);

            gl.glUseProgram(blitProgram);
            gl.glUniform1i(samplerLoc, 0);

            BQ::Item appItem = {};

            while (true) {
                AutoLock lock(mLock);

                mApp2SfQueue->dequeueBuffer(&appItem);

                EGLImageKHR image = getEGLImage(appItem.buffer);

                gl.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

                gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                gl.glDrawArrays(GL_TRIANGLES, 0, 6);
                mSf2AppQueue->queueBuffer(BQ::Item(appItem.buffer, -1));

                egl.eglSwapBuffers(d, s);
            }
        });

        mThread->start();
    }

private:

    EGLImageKHR getEGLImage(ANativeWindowBuffer* buffer) {
        auto it = mEGLImages.find(buffer);
        if (it != mEGLImages.end()) return it->second;

        auto& egl = emugl::loadGoldfishOpenGL()->egl;
        EGLDisplay d = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);
        EGLImageKHR image = egl.eglCreateImageKHR(d, 0, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)buffer, 0);

        mEGLImages[buffer] = image;

        return image;
    }

    GLuint compileShader(GLenum shaderType, const char* src) {
        auto& gl = emugl::loadGoldfishOpenGL()->gles2;
   
        GLuint shader = gl.glCreateShader(shaderType);
        gl.glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
        gl.glCompileShader(shader);
    
        GLint compileStatus;
        gl.glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    
        if (compileStatus != GL_TRUE) {
            GLsizei infoLogLength = 0;
            gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<char> infoLog(infoLogLength + 1, 0);
            gl.glGetShaderInfoLog(shader, infoLogLength, nullptr, &infoLog[0]);
            fprintf(stderr, "%s: fail to compile. infolog %s\n", __func__, &infoLog[0]);
                fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
        }
    
        return shader;
    }
    
    GLint compileAndLinkShaderProgram(const char* vshaderSrc, const char* fshaderSrc) {
        auto& gl = emugl::loadGoldfishOpenGL()->gles2;
    
        GLuint vshader = compileShader(GL_VERTEX_SHADER, vshaderSrc);
        GLuint fshader = compileShader(GL_FRAGMENT_SHADER, fshaderSrc);
    
        GLuint program = gl.glCreateProgram();
        gl.glAttachShader(program, vshader);
        gl.glAttachShader(program, fshader);
        gl.glLinkProgram(program);
    
        GLint linkStatus;
        gl.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    
        gl.glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    
        if (linkStatus != GL_TRUE) {
            GLsizei infoLogLength = 0;
            gl.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<char> infoLog(infoLogLength + 1, 0);
            gl.glGetProgramInfoLog(program, infoLogLength, nullptr, &infoLog[0]);
    
            fprintf(stderr, "%s: fail to link program. infolog: %s\n", __func__,
                    &infoLog[0]);
        }
    
        return program;
    }

    BQ* mApp2SfQueue = nullptr;
    BQ* mSf2AppQueue = nullptr;

    ANativeWindow* mWindow = nullptr;

    std::unordered_map<ANativeWindowBuffer*, EGLImageKHR> mEGLImages;
    FunctorThread* mThread = nullptr;

    Lock mLock;
};

class VirtualDisplay {
public:
    class Vsync {
    public:
        Vsync(int refreshRate = 60) :
            mRefreshRate(refreshRate),
            mRefreshIntervalUs(1000000ULL / mRefreshRate),
            mThread([this] {
                while (true) {
                    if (mShouldStop) return 0;
                    System::get()->sleepUs(mRefreshIntervalUs);
                    AutoLock lock(mLock);
                    mSync = 1;
                    mCv.signal();
                }
                return 0;
            }) {
            mThread.start();
        }
    
        ~Vsync() {
            mShouldStop = true;
        }
    
        void waitUntilNextVsync() {
            AutoLock lock(mLock);
            mSync = 0;
            while (!mSync) {
                mCv.wait(&mLock);
            }
        }
    
    private:
        int mShouldStop = false;
        int mRefreshRate = 60;
        uint64_t mRefreshIntervalUs;
        volatile int mSync = 0;
    
        Lock mLock;

        ConditionVariable mCv;
    
        FunctorThread mThread;
    };

    VirtualDisplay(int width = 256, int height = 256, int refreshRate = 60) : mWidth(width), mHeight(height), mRefreshRate(refreshRate) {
        mGralloc = *emugl::loadGoldfishGralloc();
        mFb0Device = mGralloc.open_device("fb0");
        mGpu0Device = mGralloc.open_device("gpu0");

        primeQueue(&mHwc2SfQueue, GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_FB);

        // initialize SurfaceFlinger
        //
        VirtualSurface* flingerWindow = new VirtualSurface;
        flingerWindow->setProducer(&mHwc2SfQueue, &mSf2HwcQueue);
        mFlinger.setComposeWindow((ANativeWindow*)flingerWindow);

        hwcThread();
        mFlinger.composeThread();
    }

    void primeQueue(BQ* queue, int usage) {
        bool useFbDevice = usage & GRALLOC_USAGE_HW_FB;
        for (int i = 0; i < BQ::kCapacity; i++) {
            queue->queueBuffer(
                { allocateWindowBuffer(
                        useFbDevice,
                      mWidth, mHeight,
                      HAL_PIXEL_FORMAT_RGBA_8888 /* rgba_8888 */,
                      usage), -1 });
        }
    }

    ANativeWindowBuffer* allocateWindowBuffer(bool forFb, int width, int height, int format, int usage) {
        buffer_handle_t* handle = new buffer_handle_t;
        int stride;
        mGralloc.alloc(forFb ? mFb0Device : mGpu0Device, width, height, format, usage, handle, &stride);

        ANativeWindowBuffer* buffer = new ANativeWindowBuffer;

        buffer->width = width;
        buffer->height = height;
        buffer->stride = stride;
        buffer->format = format;
        buffer->usage_deprecated = usage;
        buffer->usage = usage;

        buffer->handle = *handle;

        buffer->common.incRef = hook_incRef;
        buffer->common.decRef = hook_decRef;

        return buffer;
    }

    int createAppWindowAndSetCurrent() {
        int nextUnusedWindowId = 0;

        for (auto it : mWindows) {
            if (it.first >= nextUnusedWindowId) {
                nextUnusedWindowId = it.first + 1;
            }
        }

        Window w = { new VirtualSurface, new BQ, new BQ };
        mWindows[nextUnusedWindowId] = w;

        auto window = mWindows[nextUnusedWindowId];

        window.surface->setProducer(window.sf2appQueue, window.app2sfQueue);
        primeQueue(window.sf2appQueue, GRALLOC_USAGE_HW_RENDER);

        mFlinger.setAppProducer(window.app2sfQueue, window.sf2appQueue);

        mCurrentWindow = nextUnusedWindowId;
        return nextUnusedWindowId;
    }

    ANativeWindow* getCurrentWindow() {
        return (ANativeWindow*)(mWindows[mCurrentWindow].surface);
    }

    void hwcThread() {
        mThread = new FunctorThread([this]() {
            Vsync vsync(mRefreshRate);
            BQ::Item sfItem = {};
            while (true) {
                mSf2HwcQueue.dequeueBuffer(&sfItem);
                vsync.waitUntilNextVsync();
                mGralloc.fbpost(mFb0Device, sfItem.buffer->handle);
                mHwc2SfQueue.queueBuffer(sfItem);
            }
        });

        mThread->start();
    }

private:
    int mWidth;
    int mHeight;
    int mRefreshRate;

    GrallocDispatch mGralloc;
    void* mFb0Device;
    void* mGpu0Device;

    struct Window {
        VirtualSurface* surface;
        BQ* sf2appQueue;
        BQ* app2sfQueue;
    };

    std::unordered_map<int, Window> mWindows;
    int mCurrentWindow = -1;

    SurfaceFlinger mFlinger;
    BQ mSf2HwcQueue;
    BQ mHwc2SfQueue;

    FunctorThread* mThread = nullptr;
};

static LazyInstance<VirtualDisplay> sDisplay = LAZY_INSTANCE_INIT;

void initVirtualDisplay() {
    sDisplay.get();
}

ANativeWindow* createWindow() {
    sDisplay->createAppWindowAndSetCurrent();
    return sDisplay->getCurrentWindow();
}

} // namespace aemu
