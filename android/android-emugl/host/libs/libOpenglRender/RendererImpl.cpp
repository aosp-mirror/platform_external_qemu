// Copyright (C) 2016 The Android Open Source Project
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
#include "RendererImpl.h"

#include "RenderChannelImpl.h"

#include "emugl/common/logging.h"
#include "ErrorLog.h"
#include "FrameBuffer.h"

#include <algorithm>
#include <utility>

#include <assert.h>

namespace emugl {

// kUseSubwindowThread is used to determine whether the RenderWindow should use
// a separate thread to manage its subwindow GL/GLES context.
// For now, this feature is disabled entirely for the following
// reasons:
//
// - It must be disabled on Windows at all times, otherwise the main window
//   becomes unresponsive after a few seconds of user interaction (e.g. trying
//   to move it over the desktop). Probably due to the subtle issues around
//   input on this platform (input-queue is global, message-queue is
//   per-thread). Also, this messes considerably the display of the
//   main window when running the executable under Wine.
//
// - On Linux/XGL and OSX/Cocoa, this used to be necessary to avoid corruption
//   issues with the GL state of the main window when using the SDL UI.
//   After the switch to Qt, this is no longer necessary and may actually cause
//   undesired interactions between the UI thread and the RenderWindow thread:
//   for example, in a multi-monitor setup the context might be recreated when
//   dragging the window between monitors, triggering a Qt-specific callback
//   in the context of RenderWindow thread, which will become blocked on the UI
//   thread, which may in turn be blocked on something else.
static const bool kUseSubwindowThread = false;

RendererImpl::RendererImpl()
    : mCleanupThread([this]() {
          while (const auto id = mCleanupProcessIds.receive()) {
              FrameBuffer::getFB()->cleanupProcGLObjects(*id);
          }
      }) {
    mCleanupThread.start();
}

RendererImpl::~RendererImpl() {
    stop();
    mRenderWindow.reset();
}

bool RendererImpl::initialize(int width, int height, bool useSubWindow) {
    if (mRenderWindow) {
        return false;
    }

    std::unique_ptr<RenderWindow> renderWindow(
            new RenderWindow(width, height, kUseSubwindowThread, useSubWindow));
    if (!renderWindow) {
        ERR("Could not create rendering window class\n");
        GL_LOG("Could not create rendering window class");
        return false;
    }
    if (!renderWindow->isValid()) {
        ERR("Could not initialize emulated framebuffer\n");
        return false;
    }

    mRenderWindow = std::move(renderWindow);
    GL_LOG("OpenGL renderer initialized successfully");
    return true;
}

void RendererImpl::stop() {
    android::base::AutoLock lock(mThreadVectorLock);
    mStopped = true;
    auto threads = std::move(mThreads);
    lock.unlock();

    if (const auto fb = FrameBuffer::getFB()) {
        fb->setShuttingDown();
    }
    for (const auto& t : threads) {
        if (const auto channel = t.second.lock()) {
            channel->stopFromHost();
        }
    }
    // We're stopping the renderer, so there's no need to clean up resources
    // of some pending processes: we'll destroy everything soon.
    mCleanupProcessIds.stop();

    for (const auto& t : threads) {
        t.first->wait();
    }
    mCleanupThread.wait();
}

RenderChannelPtr RendererImpl::createRenderChannel() {
    const auto channel = std::make_shared<RenderChannelImpl>();

    std::unique_ptr<RenderThread> rt(RenderThread::create(
            shared_from_this(), channel));
    if (!rt) {
        fprintf(stderr, "Failed to create RenderThread\n");
        return nullptr;
    }

    {
        android::base::AutoLock lock(mThreadVectorLock);

        if (mStopped) return nullptr;

        if (!rt->start()) {
            fprintf(stderr, "Failed to start RenderThread\n");
            return nullptr;
        }

        // clean up the threads that are no longer running
        mThreads.erase(std::remove_if(mThreads.begin(), mThreads.end(),
                                      [](const ThreadWithChannel& t) {
                                          return t.second.expired() ||
                                                 t.first->isFinished();
                                      }),
                       mThreads.end());

        DBG("Started new RenderThread (total %" PRIu64 ") @%p\n",
            static_cast<uint64_t>(mThreads.size() + 1), rt.get());

        mThreads.emplace_back(std::move(rt), channel);
    }

    return channel;
}

RendererImpl::HardwareStrings RendererImpl::getHardwareStrings() {
    assert(mRenderWindow);

    const char* vendor = nullptr;
    const char* renderer = nullptr;
    const char* version = nullptr;
    if (!mRenderWindow->getHardwareStrings(&vendor, &renderer, &version)) {
        return {};
    }
    HardwareStrings res;
    res.vendor = vendor ? vendor : "";
    res.renderer = renderer ? renderer : "";
    res.version = version ? version : "";
    return res;
}

void RendererImpl::setPostCallback(RendererImpl::OnPostCallback onPost,
                                   void* context) {
    assert(mRenderWindow);
    mRenderWindow->setPostCallback(onPost, context);
}

bool RendererImpl::showOpenGLSubwindow(FBNativeWindowType window,
                                       int wx,
                                       int wy,
                                       int ww,
                                       int wh,
                                       int fbw,
                                       int fbh,
                                       float dpr,
                                       float zRot) {
    assert(mRenderWindow);
    return mRenderWindow->setupSubWindow(window, wx, wy, ww, wh, fbw, fbh, dpr,
                                         zRot);
}

bool RendererImpl::destroyOpenGLSubwindow() {
    assert(mRenderWindow);
    return mRenderWindow->removeSubWindow();
}

void RendererImpl::setOpenGLDisplayRotation(float zRot) {
    assert(mRenderWindow);
    mRenderWindow->setRotation(zRot);
}

void RendererImpl::setOpenGLDisplayTranslation(float px, float py) {
    assert(mRenderWindow);
    mRenderWindow->setTranslation(px, py);
}

void RendererImpl::repaintOpenGLDisplay() {
    assert(mRenderWindow);
    mRenderWindow->repaint();
}

void RendererImpl::cleanupProcGLObjects(uint64_t puid) {
    mCleanupProcessIds.send(puid);
}

}  // namespace emugl
