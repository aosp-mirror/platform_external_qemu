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

#include "SampleApplication.h"

#include "android/base/GLObjectCounter.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/threads/FunctorThread.h"

#include "Standalone.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::MessageChannel;
using android::base::System;
using android::base::TestSystem;

namespace emugl {

// Class holding the persistent test window.
class TestWindow {
public:
    TestWindow() {
        window = CreateOSWindow();
    }

    ~TestWindow() {
        if (window) {
            window->destroy();
        }
    }

    // Check on initialization if windows are available.
    bool initializeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window->initialize("libOpenglRender test", width, height)) {
            window->destroy();
            window = nullptr;
            return false;
        }
        window->setVisible(true);
        window->setPosition(xoffset, yoffset);
        window->messageLoop();
        return true;
    }

    void resizeWithRect(int xoffset, int yoffset, int width, int height) {
        if (!window) return;

        window->setPosition(xoffset, yoffset);
        window->resize(width, height);
        window->messageLoop();
    }

    OSWindow* window = nullptr;
};

static LazyInstance<TestWindow> sTestWindow = LAZY_INSTANCE_INIT;

bool shouldUseHostGpu() {
    bool useHost = TestSystem::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_HOST_GPU") == "1";

    // Also set the global emugl renderer accordingly.
    if (useHost) {
        emugl::setRenderer(SELECTED_RENDERER_HOST);
    } else {
        emugl::setRenderer(SELECTED_RENDERER_SWIFTSHADER_INDIRECT);
    }

    return useHost;
}

bool shouldUseWindow() {
    bool useWindow = TestSystem::getEnvironmentVariable("ANDROID_EMU_TEST_WITH_WINDOW") == "1";
    return useWindow;
}

OSWindow* createOrGetTestWindow(int xoffset, int yoffset, int width, int height) {
    if (!shouldUseWindow()) return nullptr;

    if (!sTestWindow.hasInstance()) {
        sTestWindow.get();
        sTestWindow->initializeWithRect(xoffset, yoffset, width, height);
    } else {
        sTestWindow->resizeWithRect(xoffset, yoffset, width, height);
    }
    return sTestWindow->window;
}

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

// app -> SF queue: separate storage, bindTexture blits
// SF queue -> HWC: shared storage
class ColorBufferQueue { // Note: we could have called this BufferQueue but there is another
                         // class of name BufferQueue that does something totally different

public:
    static constexpr int kCapacity = 3;
    class Item {
    public:
        Item(unsigned int cb = 0, FenceSync* s = nullptr) : colorBuffer(cb), sync(s) { }
        unsigned int colorBuffer = 0;
        FenceSync* sync = nullptr;
    };

    ColorBufferQueue() = default;

    void queueBuffer(const Item& item) {
        mQueue.send(item);
    }

    void dequeueBuffer(Item* outItem) {
        mQueue.receive(outItem);
    }

private:
    MessageChannel<Item, kCapacity> mQueue;
};

class AutoComposeDevice {
public:
    AutoComposeDevice(uint32_t targetCb, uint32_t layerCnt = 2) :
      mData(sizeof(ComposeDevice) + layerCnt * sizeof(ComposeLayer))
    {
        mComposeDevice = reinterpret_cast<ComposeDevice*>(mData.data());
        mComposeDevice->version = 1;
        mComposeDevice->targetHandle = targetCb;
        mComposeDevice->numLayers = layerCnt;
    }

    ComposeDevice* get() {
        return mComposeDevice;
    }

    uint32_t getSize() {
        return mData.size();
    }

    void configureLayer(uint32_t layerId, unsigned int cb,
                        hwc2_composition_t composeMode,
                        hwc_rect_t displayFrame,
                        hwc_frect_t crop,
                        hwc2_blend_mode_t blendMode,
                        float alpha,
                        hwc_color_t color
                        ) {
        mComposeDevice->layer[layerId].cbHandle = cb;
        mComposeDevice->layer[layerId].composeMode = composeMode;
        mComposeDevice->layer[layerId].displayFrame = displayFrame;
        mComposeDevice->layer[layerId].crop = crop;
        mComposeDevice->layer[layerId].blendMode = blendMode;
        mComposeDevice->layer[layerId].alpha = alpha;
        mComposeDevice->layer[layerId].color = color;
        mComposeDevice->layer[layerId].transform = HWC_TRANSFORM_FLIP_H;
    }

private:
    std::vector<uint8_t> mData;
    ComposeDevice* mComposeDevice;
};

// SampleApplication implementation/////////////////////////////////////////////
SampleApplication::SampleApplication(int windowWidth, int windowHeight, int refreshRate, GLESApi glVersion, bool compose) :
    mWidth(windowWidth), mHeight(windowHeight), mRefreshRate(refreshRate), mIsCompose(compose) {

    setupStandaloneLibrarySearchPaths();
    emugl::setGLObjectCounter(android::base::GLObjectCounter::get());
    LazyLoadedEGLDispatch::get();
    if (glVersion == GLESApi_CM) LazyLoadedGLESv1Dispatch::get();
    LazyLoadedGLESv2Dispatch::get();

    bool useHostGpu = shouldUseHostGpu();
    mWindow = createOrGetTestWindow(mXOffset, mYOffset, mWidth, mHeight);
    mUseSubWindow = mWindow != nullptr;

    FrameBuffer::initialize(
            mWidth, mHeight,
            mUseSubWindow,
            !useHostGpu /* egl2egl */);
    mFb.reset(FrameBuffer::getFB());

    if (mUseSubWindow) {
        mFb->setupSubWindow(
            (FBNativeWindowType)(uintptr_t)
            mWindow->getFramebufferNativeWindow(),
            0, 0,
            mWidth, mHeight, mWidth, mHeight,
            mWindow->getDevicePixelRatio(), 0, false);
        mWindow->messageLoop();
    }

    mRenderThreadInfo.reset(new RenderThreadInfo());

    mColorBuffer = mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    mContext = mFb->createRenderContext(0, 0, glVersion);
    mSurface = mFb->createWindowSurface(0, mWidth, mHeight);

    mFb->bindContext(mContext, mSurface, mSurface);
    mFb->setWindowSurfaceColorBuffer(mSurface, mColorBuffer);

    if (mIsCompose && mTargetCb == 0) {
        mTargetCb = mFb->createColorBuffer(mFb->getWidth(),
                                           mFb->getHeight(),
                                           GL_RGBA,
                                           FRAMEWORK_FORMAT_GL_COMPATIBLE);
        mFb->openColorBuffer(mTargetCb);
    }
 }

SampleApplication::~SampleApplication() {
    if (mFb) {
        if (mTargetCb) {
            mFb->closeColorBuffer(mTargetCb);
        }
        mFb->bindContext(0, 0, 0);
        mFb->closeColorBuffer(mColorBuffer);
        mFb->DestroyWindowSurface(mSurface);
        mFb->finalize();
    }
}

void SampleApplication::rebind() {
    mFb->bindContext(mContext, mSurface, mSurface);
}

void SampleApplication::drawLoop() {
    this->initialize();

    Vsync vsync(mRefreshRate);

    while (true) {
        this->draw();
        mFb->flushWindowSurfaceColorBuffer(mSurface);
        vsync.waitUntilNextVsync();
        if (mUseSubWindow) {
            mFb->post(mColorBuffer);
            mWindow->messageLoop();
        }
    }
}

FenceSync* SampleApplication::getFenceSync() {
    auto gl = getGlDispatch();
    FenceSync* sync = new FenceSync(false, false);
    gl->glFlush();
    return sync;
}

void SampleApplication::drawWorkerWithCompose(ColorBufferQueue& app2sfQueue,
                                              ColorBufferQueue& sf2appQueue) {
    ColorBufferQueue::Item appItem = {};
    AutoComposeDevice autoComposeDevice(mTargetCb);
    hwc_rect_t displayFrame = {0, mHeight/2, mWidth, mHeight};
    hwc_frect_t crop = {0.0, 0.0, 0.0, 0.0};
    hwc_color_t color = {200, 0, 0, 255};
    autoComposeDevice.configureLayer(0, 0,
                                     HWC2_COMPOSITION_SOLID_COLOR,
                                     displayFrame,
                                     crop,
                                     HWC2_BLEND_MODE_NONE,
                                     1.0,
                                     color);

    while (true) {
        app2sfQueue.dequeueBuffer(&appItem);
        if (appItem.sync) { appItem.sync->wait(EGL_FOREVER_KHR); }

        hwc_rect_t displayFrame = {0, 0, mWidth, mHeight/2};
        hwc_frect_t crop = {0.0, 0.0, (float)mWidth, (float)mHeight};
        hwc_color_t color = {0, 0, 0, 0};
        autoComposeDevice.configureLayer(1,
                                         appItem.colorBuffer,
                                         HWC2_COMPOSITION_DEVICE,
                                         displayFrame,
                                         crop,
                                         HWC2_BLEND_MODE_PREMULTIPLIED,
                                         0.8,
                                         color);
        mFb->compose(autoComposeDevice.getSize(), autoComposeDevice.get());

        if (appItem.sync) { appItem.sync->decRef(); }
        sf2appQueue.queueBuffer(ColorBufferQueue::Item(appItem.colorBuffer, getFenceSync()));
    }
}

void SampleApplication::drawWorker(ColorBufferQueue& app2sfQueue,
                                   ColorBufferQueue& sf2appQueue,
                                   ColorBufferQueue& sf2hwcQueue,
                                   ColorBufferQueue& hwc2sfQueue) {
    RenderThreadInfo* tInfo = new RenderThreadInfo;
    unsigned int sfContext = mFb->createRenderContext(0, 0, GLESApi_3_0);
    unsigned int sfSurface = mFb->createWindowSurface(0, mWidth, mHeight);
    mFb->bindContext(sfContext, sfSurface, sfSurface);

    auto gl = getGlDispatch();

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

    GLint samplerLoc = gl->glGetUniformLocation(blitProgram, "tex");

    GLuint blitVbo;
    gl->glGenBuffers(1, &blitVbo);
    gl->glBindBuffer(GL_ARRAY_BUFFER, blitVbo);
    const float attrs[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 0.0f,
    };
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);
    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(1);

    gl->glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    gl->glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
        (GLvoid*)(uintptr_t)(2 * sizeof(GLfloat)));

    GLuint blitTexture;
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glGenTextures(1, &blitTexture);
    gl->glBindTexture(GL_TEXTURE_2D, blitTexture);

    gl->glUseProgram(blitProgram);
    gl->glUniform1i(samplerLoc, 0);

    ColorBufferQueue::Item appItem = {};
    ColorBufferQueue::Item hwcItem = {};

    while (true) {
        hwc2sfQueue.dequeueBuffer(&hwcItem);
        if (hwcItem.sync) { hwcItem.sync->wait(EGL_FOREVER_KHR); }

        mFb->setWindowSurfaceColorBuffer(sfSurface, hwcItem.colorBuffer);

        {
            app2sfQueue.dequeueBuffer(&appItem);

            mFb->bindColorBufferToTexture(appItem.colorBuffer);

            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            if (appItem.sync) { appItem.sync->wait(EGL_FOREVER_KHR); }

            gl->glDrawArrays(GL_TRIANGLES, 0, 6);

            if (appItem.sync) { appItem.sync->decRef(); }
            sf2appQueue.queueBuffer(ColorBufferQueue::Item(appItem.colorBuffer, getFenceSync()));
        }

        mFb->flushWindowSurfaceColorBuffer(sfSurface);

        if (hwcItem.sync) { hwcItem.sync->decRef(); }
        sf2hwcQueue.queueBuffer(ColorBufferQueue::Item(hwcItem.colorBuffer, getFenceSync()));
    }
    delete tInfo;
}

void SampleApplication::surfaceFlingerComposerLoop() {
    ColorBufferQueue app2sfQueue;
    ColorBufferQueue sf2appQueue;
    ColorBufferQueue sf2hwcQueue;
    ColorBufferQueue hwc2sfQueue;

    std::vector<unsigned int> sfColorBuffers;
    std::vector<unsigned int> hwcColorBuffers;

    for (int i = 0; i < ColorBufferQueue::kCapacity; i++) {
        sfColorBuffers.push_back(mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE));
        hwcColorBuffers.push_back(mFb->createColorBuffer(mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE));
    }

    for (int i = 0; i < ColorBufferQueue::kCapacity; i++) {
        mFb->openColorBuffer(sfColorBuffers[i]);
        mFb->openColorBuffer(hwcColorBuffers[i]);
    }

    // prime the queue
    for (int i = 0; i < ColorBufferQueue::kCapacity; i++) {
        sf2appQueue.queueBuffer(ColorBufferQueue::Item(sfColorBuffers[i], nullptr));
        hwc2sfQueue.queueBuffer(ColorBufferQueue::Item(hwcColorBuffers[i], nullptr));
    }

    FunctorThread appThread([&]() {
        RenderThreadInfo* tInfo = new RenderThreadInfo;
        unsigned int appContext = mFb->createRenderContext(0, 0, GLESApi_3_0);
        unsigned int appSurface = mFb->createWindowSurface(0, mWidth, mHeight);
        mFb->bindContext(appContext, appSurface, appSurface);

        ColorBufferQueue::Item sfItem = {};

        sf2appQueue.dequeueBuffer(&sfItem);
        mFb->setWindowSurfaceColorBuffer(appSurface, sfItem.colorBuffer);
        if (sfItem.sync) { sfItem.sync->wait(EGL_FOREVER_KHR); sfItem.sync->decRef(); }

        this->initialize();

        while (true) {
            this->draw();
            mFb->flushWindowSurfaceColorBuffer(appSurface);
            app2sfQueue.queueBuffer(ColorBufferQueue::Item(sfItem.colorBuffer, getFenceSync()));

            sf2appQueue.dequeueBuffer(&sfItem);
            mFb->setWindowSurfaceColorBuffer(appSurface, sfItem.colorBuffer);
            if (sfItem.sync) { sfItem.sync->wait(EGL_FOREVER_KHR); sfItem.sync->decRef(); }
        }

        delete tInfo;
    });

    FunctorThread sfThread([&]() {
        if (mIsCompose) {
            drawWorkerWithCompose(app2sfQueue, sf2appQueue);
        }
        else {
            drawWorker(app2sfQueue, sf2appQueue, sf2hwcQueue, hwc2sfQueue);
        }
    });

    sfThread.start();
    appThread.start();

    Vsync vsync(mRefreshRate);
    ColorBufferQueue::Item sfItem = {};
    if (!mIsCompose) {
        while (true) {
            sf2hwcQueue.dequeueBuffer(&sfItem);
            if (sfItem.sync) { sfItem.sync->wait(EGL_FOREVER_KHR); sfItem.sync->decRef(); }
            vsync.waitUntilNextVsync();
            mFb->post(sfItem.colorBuffer);
            if (mUseSubWindow) {
                mWindow->messageLoop();
            }
            hwc2sfQueue.queueBuffer(ColorBufferQueue::Item(sfItem.colorBuffer, getFenceSync()));
        }
    }

    appThread.wait();
    sfThread.wait();
}

void SampleApplication::drawOnce() {
    this->initialize();
    this->draw();
    mFb->flushWindowSurfaceColorBuffer(mSurface);
    if (mUseSubWindow) {
        mFb->post(mColorBuffer);
        mWindow->messageLoop();
    }
}

const GLESv2Dispatch* SampleApplication::getGlDispatch() {
    return LazyLoadedGLESv2Dispatch::get();
}

} // namespace emugl
