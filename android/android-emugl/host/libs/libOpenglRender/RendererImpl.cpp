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
#include "RenderThread.h"

#include "android/base/system/System.h"
#include "android/snapshot/Snapshotter.h"
#include "android/utils/debug.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/logging.h"
#include "FenceSync.h"
#include "FrameBuffer.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/DispatchTables.h"

#include <algorithm>
#include <utility>

#include <assert.h>

using namespace gfxstream;
using namespace gfxstream::gl;

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

// This object manages the cleanup of guest process resources when the process
// exits. It runs the cleanup in a separate thread to never block the main
// render thread for a low-priority task.
class RendererImpl::ProcessCleanupThread {
public:
    ProcessCleanupThread()
        : mCleanupThread([this]() {
              while (const auto id = mCleanupProcessIds.receive()) {
                  FrameBuffer::getFB()->cleanupProcGLObjects(*id);
              }
          }) {
        mCleanupThread.start();
    }

    ~ProcessCleanupThread() {
        mCleanupProcessIds.stop();
        mCleanupThread.wait();
    }

    void cleanup(uint64_t processId) {
        mCleanupProcessIds.send(processId);
    }

    void stop() {
        mCleanupProcessIds.stop();
    }

    void waitForCleanup() {
        mCleanupProcessIds.waitForEmpty();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ProcessCleanupThread);

    android::base::MessageChannel<uint64_t, 64> mCleanupProcessIds;
    android::base::FunctorThread mCleanupThread;
};

RendererImpl::RendererImpl() {
    mCleanupThread.reset(new ProcessCleanupThread());
}

RendererImpl::~RendererImpl() {
    stop(true);
    mRenderWindow.reset();
}

bool RendererImpl::initialize(int width, int height, bool useSubWindow, bool egl2egl) {
    if (android::base::System::get()->envGet("ANDROID_EMUGL_VERBOSE") == "1") {
        base_enable_verbose_logs();
    }

    // NOTE: vulkan-cereal initializes the following inside of FrameBuffer.

    //
    // Load EGL Plugin
    //
    if (!emugl::LazyLoadedEGLDispatch::get()) {
        printf("Failed to init_egl_dispatch\n");
        return false;
    }

    //
    // Load GLES Plugin
    //
    if (!emugl::LazyLoadedGLESv1Dispatch::get()) {
        ERR("Failed to gles1_dispatch_init\n");
        return false;
    }

    //
    // Load GLES 2 Plugin
    //
    if (!emugl::LazyLoadedGLESv2Dispatch::get()) {
        ERR("Failed to gles2_dispatch_init\n");
        return false;
    }

    if (mRenderWindow) {
        return false;
    }

    std::unique_ptr<RenderWindow> renderWindow(new RenderWindow(
            width, height, kUseSubwindowThread, useSubWindow, egl2egl));
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

    // This render thread won't do anything but will only preload resources
    // for the real threads to start faster.
    mLoaderRenderThread.reset(new RenderThread(nullptr));
    mLoaderRenderThread->start();

    return true;
}

void RendererImpl::stop(bool wait) {
    android::base::AutoLock lock(mChannelsLock);
    mStopped = true;
    auto channels = std::move(mChannels);
    lock.unlock();

    if (const auto fb = FrameBuffer::getFB()) {
        fb->setShuttingDown();
    }
    for (const auto& c : channels) {
        c->stopFromHost();
    }
    // We're stopping the renderer, so there's no need to clean up resources
    // of some pending processes: we'll destroy everything soon.
    mCleanupThread->stop();

    mStoppedChannels.insert(mStoppedChannels.end(),
                            std::make_move_iterator(channels.begin()),
                            std::make_move_iterator(channels.end()));

    if (!wait) {
        return;
    }

    // Each render channel is referenced in the corresponing pipe object, so
    // even if we clear the |channels| vector they could still be alive
    // for a while. This means we need to make sure to wait for render thread
    // exit explicitly.
    for (const auto& c : mStoppedChannels) {
        c->renderThread()->wait();
    }
    mStoppedChannels.clear();
}

void RendererImpl::finish() {
    {
        android::base::AutoLock lock(mChannelsLock);
        mRenderWindow->setPaused(true);
    }
    cleanupRenderThreads();
    {
        android::base::AutoLock lock(mChannelsLock);
        mRenderWindow->setPaused(false);
    }
}

void RendererImpl::cleanupRenderThreads() {
    android::base::AutoLock lock(mChannelsLock);
    const auto channels = std::move(mChannels);
    assert(mChannels.empty());
    lock.unlock();
    for (const auto& c : channels) {
        // Please DO NOT notify the guest about this event (DO NOT call
        // stopFromHost() ), because this is used to kill old threads when
        // loading from a snapshot, and the newly loaded guest should not
        // be notified for those behavior.
        c->stop();
    }
    for (const auto& c : channels) {
        c->renderThread()->wait();
    }
}

void RendererImpl::waitForProcessCleanup() {
    mCleanupThread->waitForCleanup();
    // Recreate it to make sure we've started from scratch and that we've
    // finished all in-progress cleanups as well.
    mCleanupThread.reset(new ProcessCleanupThread());
}

RenderChannelPtr RendererImpl::createRenderChannel(
        android::base::Stream* loadStream, uint32_t virtioGpuContextId) {
    (void)virtioGpuContextId;

    const auto channel = std::make_shared<RenderChannelImpl>(loadStream);
    {
        android::base::AutoLock lock(mChannelsLock);

        if (mStopped) {
            return nullptr;
        }

        // Clean up the stopped channels.
        mChannels.erase(
                std::remove_if(mChannels.begin(), mChannels.end(),
                               [](const std::shared_ptr<RenderChannelImpl>& c) {
                                   return c->renderThread()->isFinished();
                               }),
                mChannels.end());
        mChannels.emplace_back(channel);

        // Take the time to check if our loader thread is done as well.
        if (mLoaderRenderThread && mLoaderRenderThread->isFinished()) {
            mLoaderRenderThread->wait();
            mLoaderRenderThread.reset();
        }

        GL_LOG("Started new RenderThread (total %" PRIu64 ") @%p",
               static_cast<uint64_t>(mChannels.size()), channel->renderThread());
    }

    return channel;
}

 void RendererImpl::addListener(FrameBufferChangeEventListener* listener) {
    mRenderWindow->addListener(listener);
 }

void RendererImpl::removeListener(FrameBufferChangeEventListener* listener) {
    mRenderWindow->removeListener(listener);
}

void* RendererImpl::addressSpaceGraphicsConsumerCreate(
    struct asg_context context,
    android::base::Stream* loadStream,
    android::emulation::asg::ConsumerCallbacks callbacks,
    uint32_t virtioGpuContextId, uint32_t virtioGpuCapsetId,
    std::optional<std::string> nameOpt) {
    auto thread = new RenderThread(context, loadStream, callbacks, virtioGpuContextId,
                                   virtioGpuCapsetId, std::move(nameOpt));
    thread->start();
    return (void*)thread;
}

void RendererImpl::addressSpaceGraphicsConsumerDestroy(void* consumer) {
    RenderThread* thread = (RenderThread*)consumer;
    thread->notifyTeardown();
    thread->wait();
    delete thread;
}

void RendererImpl::addressSpaceGraphicsConsumerPreSave(void* consumer) {
    RenderThread* thread = (RenderThread*)consumer;
    thread->pausePreSnapshot();
}

void RendererImpl::addressSpaceGraphicsConsumerSave(void* consumer, android::base::Stream* stream) {
    RenderThread* thread = (RenderThread*)consumer;
    thread->save(stream);
}

void RendererImpl::addressSpaceGraphicsConsumerPostSave(void* consumer) {
    RenderThread* thread = (RenderThread*)consumer;
    thread->resume();
}

void RendererImpl::addressSpaceGraphicsConsumerRegisterPostLoadRenderThread(void* consumer) {
    RenderThread* thread = (RenderThread*)consumer;
    mAdditionalPostLoadRenderThreads.push_back(thread);
}

void RendererImpl::pauseAllPreSave() {
    android::base::AutoLock lock(mChannelsLock);
    if (mStopped) {
        return;
    }
    for (const auto& c : mChannels) {
        c->renderThread()->pausePreSnapshot();
    }
    lock.unlock();
    waitForProcessCleanup();
}

void RendererImpl::resumeAll(bool waitForSave) {
    {
        (void)waitForSave;
        android::base::AutoLock lock(mChannelsLock);
        if (mStopped) {
            return;
        }
        for (const auto& c : mChannels) {
            c->renderThread()->resume();
        }

        for (const auto t: mAdditionalPostLoadRenderThreads) {
            t->resume();
        }
        mAdditionalPostLoadRenderThreads.clear();
    }

    repaintOpenGLDisplay();
}

void RendererImpl::save(android::base::Stream* stream,
                        const android::snapshot::ITextureSaverPtr& textureSaver) {
    stream->putByte(mStopped);
    if (mStopped) {
        return;
    }
    auto fb = FrameBuffer::getFB();
    assert(fb);
    fb->onSave(stream, textureSaver);

    FenceSync::onSave(stream);
}

bool RendererImpl::load(android::base::Stream* stream,
                        const android::snapshot::ITextureLoaderPtr& textureLoader) {

#ifdef SNAPSHOT_PROFILE
    android::base::System::Duration startTime =
            android::base::System::get()->getUnixTimeUs();
#endif
    waitForProcessCleanup();
#ifdef SNAPSHOT_PROFILE
    printf("Previous session cleanup time: %lld ms\n",
           (long long)(android::base::System::get()
                               ->getUnixTimeUs() -
                       startTime) /
                   1000);
#endif

    mStopped = stream->getByte();
    if (mStopped) {
        return true;
    }
    auto fb = FrameBuffer::getFB();
    assert(fb);

    bool res = true;

    res = fb->onLoad(stream, textureLoader);
    FenceSync::onLoad(stream);

    return res;
}

void RendererImpl::fillGLESUsages(android_studio::EmulatorGLESUsages* usages) {
    auto fb = FrameBuffer::getFB();
    if (fb) fb->fillGLESUsages(usages);
}

int RendererImpl::getScreenshot(unsigned int nChannels,
                                unsigned int* width,
                                unsigned int* height,
                                uint8_t* pixels,
                                size_t* cPixels,
                                int displayId = 0,
                                int desiredWidth = 0,
                                int desiredHeight = 0,
                                int desiredRotation = SKIN_ROTATION_0,
                                Rect rect = {{0, 0}, {0, 0}}) {
    auto fb = FrameBuffer::getFB();
    if (fb) {
        return fb->getScreenshot(nChannels, width, height, pixels, cPixels,
                                 displayId, desiredWidth, desiredHeight,
                                 static_cast<SkinRotation>(desiredRotation), {{rect.pos.x, rect.pos.y}, {rect.size.w, rect.size.h}});
    }
    *cPixels = 0;
    return -1;
}

void RendererImpl::setMultiDisplay(uint32_t id,
                                   int32_t x,
                                   int32_t y,
                                   uint32_t w,
                                   uint32_t h,
                                   uint32_t dpi,
                                   bool add) {
    auto fb = FrameBuffer::getFB();
    if (fb) {
        if (add) {
            fb->createDisplay(&id);
            fb->setDisplayPose(id, x, y, w, h, dpi);
        } else {
            fb->destroyDisplay(id);
        }
    }
}

void RendererImpl::setMultiDisplayColorBuffer(uint32_t id, uint32_t cb) {
    auto fb = FrameBuffer::getFB();
    if (fb) {
        fb->setDisplayColorBuffer(id, cb);
    }
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
                                   void* context,
                                   bool useBgraReadback,
                                   uint32_t displayId) {
    assert(mRenderWindow);
    mRenderWindow->setPostCallback(onPost, context, displayId, useBgraReadback);
}

bool RendererImpl::asyncReadbackSupported() {
    assert(mRenderWindow);
    return mRenderWindow->asyncReadbackSupported();
}

RendererImpl::ReadPixelsCallback
RendererImpl::getReadPixelsCallback() {
    assert(mRenderWindow);
    return mRenderWindow->getReadPixelsCallback();
}

RendererImpl::FlushReadPixelPipeline
RendererImpl::getFlushReadPixelPipeline() {
    assert(mRenderWindow);
    return mRenderWindow->getFlushReadPixelPipeline();
}

bool RendererImpl::showOpenGLSubwindow(FBNativeWindowType window,
                                       int wx,
                                       int wy,
                                       int ww,
                                       int wh,
                                       int fbw,
                                       int fbh,
                                       float dpr,
                                       float zRot,
                                       bool deleteExisting,
                                       bool hideWindow) {
    assert(mRenderWindow);
    return mRenderWindow->setupSubWindow(window, wx, wy, ww, wh, fbw, fbh, dpr,
                                         zRot, deleteExisting, hideWindow);
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

bool RendererImpl::hasGuestPostedAFrame() {
    if (mRenderWindow) {
        return mRenderWindow->hasGuestPostedAFrame();
    }
    return false;
}

void RendererImpl::resetGuestPostedAFrame() {
    if (mRenderWindow) {
        mRenderWindow->resetGuestPostedAFrame();
    }
}

void RendererImpl::setScreenMask(int width, int height, const unsigned char* rgbaData) {
    assert(mRenderWindow);
    mRenderWindow->setScreenMask(width, height, rgbaData);
}

void RendererImpl::onGuestGraphicsProcessCreate(uint64_t puid) {
    // TODO(kaiyili): cherry-pick the entire https://r.android.com/c/2258774 change.
}

void RendererImpl::cleanupProcGLObjects(uint64_t puid) {
    mCleanupThread->cleanup(puid);
}

static struct AndroidVirtioGpuOps sVirtioGpuOps = {
        .create_color_buffer_with_handle =
                [](uint32_t width,
                   uint32_t height,
                   uint32_t format,
                   uint32_t fwkFormat,
                   uint32_t handle,
                   bool linear) {
                    FrameBuffer::getFB()->createColorBufferWithHandle(
                            width, height, (GLenum)format,
                            (FrameworkFormat)fwkFormat, handle);
                },
        .open_color_buffer =
                [](uint32_t handle) {
                    FrameBuffer::getFB()->openColorBuffer(handle);
                },
        .close_color_buffer =
                [](uint32_t handle) {
                    FrameBuffer::getFB()->closeColorBuffer(handle);
                },
        .update_color_buffer =
                [](uint32_t handle,
                   int x,
                   int y,
                   int width,
                   int height,
                   uint32_t format,
                   uint32_t type,
                   void* pixels) {
                    FrameBuffer::getFB()->updateColorBuffer(
                            handle, x, y, width, height, format, type, pixels);
                },
        .read_color_buffer =
                [](uint32_t handle,
                   int x,
                   int y,
                   int width,
                   int height,
                   uint32_t format,
                   uint32_t type,
                   void* pixels) {
                    FrameBuffer::getFB()->readColorBuffer(
                            handle, x, y, width, height, format, type, pixels);
                },
        .read_color_buffer_yuv =
                [](uint32_t handle,
                   int x,
                   int y,
                   int width,
                   int height,
                   void* pixels,
                   uint32_t pixels_size) {
                    FrameBuffer::getFB()->readColorBufferYUV(
                            handle, x, y, width, height, pixels, pixels_size);
                },
        .post_color_buffer =
                [](uint32_t handle) { FrameBuffer::getFB()->post(handle); },
        .repost = []() { FrameBuffer::getFB()->repost(); },
        .create_yuv_textures =
                [](uint32_t type,
                   uint32_t count,
                   int width,
                   int height,
                   uint32_t* output) {
                    FrameBuffer::getFB()->createYUVTextures(type, count, width,
                                                            height, output);
                },
        .destroy_yuv_textures =
                [](uint32_t type, uint32_t count, uint32_t* textures) {
                    FrameBuffer::getFB()->destroyYUVTextures(type, count,
                                                             textures);
                },
        .update_yuv_textures =
                [](uint32_t type,
                   uint32_t* textures,
                   void* privData,
                   void* func) {
                    FrameBuffer::getFB()->updateYUVTextures(type, textures,
                                                            privData, func);
                },
        .swap_textures_and_update_color_buffer =
                [](uint32_t colorbufferhandle,
                   int x,
                   int y,
                   int width,
                   int height,
                   uint32_t format,
                   uint32_t type,
                   uint32_t texture_type,
                   uint32_t* textures,
                   void* metadata) {
                    FrameBuffer::getFB()->swapTexturesAndUpdateColorBuffer(
                            colorbufferhandle, x, y, width, height, format,
                            type, texture_type, textures, metadata);
                },
        .get_last_posted_color_buffer =
                []() {
                    return FrameBuffer::getFB()->getLastPostedColorBuffer();
                },
        .bind_color_buffer_to_texture =
                [](uint32_t handle) {
                    FrameBuffer::getFB()->bindColorBufferToTexture2(handle);
                },
        .get_global_egl_context =
                []() { return FrameBuffer::getFB()->getGlobalEGLContext(); },
        .set_guest_managed_color_buffer_lifetime =
                [](bool guestManaged) {
                    FrameBuffer::getFB()->setGuestManagedColorBufferLifetime(
                            guestManaged);
                },
        .async_wait_for_gpu_with_cb =
                [](uint64_t eglsync, FenceCompletionCallback cb) {
                    FrameBuffer::getFB()->asyncWaitForGpuWithCb(eglsync, cb);
                },
        .async_wait_for_gpu_vulkan_with_cb =
                [](uint64_t device,
                   uint64_t fence,
                   FenceCompletionCallback cb) {
                    FrameBuffer::getFB()->asyncWaitForGpuVulkanWithCb(
                            device, fence, cb);
                },
        .async_wait_for_gpu_vulkan_qsri_with_cb =
                [](uint64_t image, FenceCompletionCallback cb) {
                    FrameBuffer::getFB()->asyncWaitForGpuVulkanQsriWithCb(image,
                                                                          cb);
                },
        .update_color_buffer_from_framework_format =
                [](uint32_t handle,
                   int x,
                   int y,
                   int width,
                   int height,
                   uint32_t fwkFormat,
                   uint32_t format,
                   uint32_t type,
                   void* pixels,
                   void* metadata) {
                    FrameBuffer::getFB()->updateColorBufferFromFrameworkFormat(
                            handle, x, y, width, height,
                            (FrameworkFormat)fwkFormat, format, type, pixels,
                            metadata);
                },
        .platform_create_shared_egl_context =
                []() {
                    return FrameBuffer::getFB()
                            ->platformCreateSharedEglContext();
                },
        .platform_destroy_shared_egl_context =
                [](void* context) {
                    return FrameBuffer::getFB()
                            ->platformDestroySharedEglContext(context);
                },
};

struct AndroidVirtioGpuOps* RendererImpl::getVirtioGpuOps() {
    return &sVirtioGpuOps;
}

void RendererImpl::snapshotOperationCallback(int op, int stage) {
    using namespace android::snapshot;
    switch (static_cast<Snapshotter::Operation>(op)) {
        case Snapshotter::Operation::Load:
            if (stage == Snapshotter::Stage::Start) {
#ifdef SNAPSHOT_PROFILE
             android::base::System::Duration startTime =
                     android::base::System::get()->getUnixTimeUs();
#endif
                mRenderWindow->setPaused(true);
                cleanupRenderThreads();
#ifdef SNAPSHOT_PROFILE
                printf("Previous session suspend time: %lld ms\n",
                       (long long)(android::base::System::get()
                                           ->getUnixTimeUs() -
                                   startTime) /
                               1000);
#endif
            }
            if (stage == Snapshotter::Stage::End) {
                mRenderWindow->setPaused(false);
            }
            break;
        default:
            break;
    }
}

void RendererImpl::setVsyncHz(int vsyncHz) {
    if (mRenderWindow) {
        mRenderWindow->setVsyncHz(vsyncHz);
    }
}

void RendererImpl::setDisplayConfigs(int configId,int w, int h,
                                     int dpiX, int dpiY) {
    if (mRenderWindow) {
        mRenderWindow->setDisplayConfigs(configId, w, h, dpiX, dpiY);
    }
}

void RendererImpl::setDisplayActiveConfig(int configId) {
    if (mRenderWindow) {
        mRenderWindow->setDisplayActiveConfig(configId);
    }
}

const void* RendererImpl::getEglDispatch() {
    return &s_egl;
}

const void* RendererImpl::getGles2Dispatch() {
    return &s_gles2;
}

}  // namespace emugl
