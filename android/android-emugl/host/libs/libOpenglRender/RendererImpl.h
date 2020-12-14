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
#pragma once

#include "OpenglRender/Renderer.h"

#include "RenderWindow.h"

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/FunctorThread.h"
#include "android/snapshot/common.h"

#include "RenderThread.h"

#include <memory>
#include <utility>
#include <vector>

namespace android_studio {
    class EmulatorGLESUsages;
}

namespace emugl {

class RendererImpl final : public Renderer {
public:
    RendererImpl();
    ~RendererImpl();

    bool initialize(int width, int height, bool useSubWindow, bool egl2egl);
    void stop(bool wait) override;
    void finish() override;

public:
    RenderChannelPtr createRenderChannel(android::base::Stream* loadStream) final;

    void* addressSpaceGraphicsConsumerCreate(
        struct asg_context,
        android::emulation::asg::ConsumerCallbacks) override final;
    void addressSpaceGraphicsConsumerDestroy(void*) override final;

    HardwareStrings getHardwareStrings() final;
    void setPostCallback(OnPostCallback onPost,
                         void* context,
                         bool useBgraReadback,
                         uint32_t displayId) final;
    bool asyncReadbackSupported() final;
    ReadPixelsCallback getReadPixelsCallback() final;
    FlushReadPixelPipeline getFlushReadPixelPipeline() final;
    bool showOpenGLSubwindow(FBNativeWindowType window,
                             int wx,
                             int wy,
                             int ww,
                             int wh,
                             int fbw,
                             int fbh,
                             float dpr,
                             float zRot,
                             bool deleteExisting,
                             bool hideWindow) final;
    bool destroyOpenGLSubwindow() final;
    void setOpenGLDisplayRotation(float zRot) final;
    void setOpenGLDisplayTranslation(float px, float py) final;
    void repaintOpenGLDisplay() final;

    bool hasGuestPostedAFrame() final;
    void resetGuestPostedAFrame() final;

    void setScreenMask(int width, int height, const unsigned char* rgbaData) final;
    void setMultiDisplay(uint32_t id,
                         int32_t x,
                         int32_t y,
                         uint32_t w,
                         uint32_t h,
                         uint32_t dpi,
                         bool add) final;
    void setMultiDisplayColorBuffer(uint32_t id, uint32_t cb) override;
    void cleanupProcGLObjects(uint64_t puid) final;
    void waitForProcessCleanup() final;
    struct AndroidVirtioGpuOps* getVirtioGpuOps() final;

    void pauseAllPreSave() final;
    void resumeAll() final;

    void save(android::base::Stream* stream,
              const android::snapshot::ITextureSaverPtr& textureSaver) final;
    bool load(android::base::Stream* stream,
              const android::snapshot::ITextureLoaderPtr& textureLoader) final;
    void fillGLESUsages(android_studio::EmulatorGLESUsages*) final;
    void getScreenshot(unsigned int nChannels, unsigned int* width,
            unsigned int* height, std::vector<unsigned char>& pixels,
            int displayId, int desiredWidth, int desiredHeight,
            SkinRotation desiredRotation) final;
    void snapshotOperationCallback(
            android::snapshot::Snapshotter::Operation op,
            android::snapshot::Snapshotter::Stage stage) final;

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RendererImpl);

private:
    // Stop all render threads and wait until they exit,
    // and also delete them.
    void cleanupRenderThreads();

    std::unique_ptr<RenderWindow> mRenderWindow;

    android::base::Lock mChannelsLock;

    std::vector<std::shared_ptr<RenderChannelImpl>> mChannels;
    std::vector<std::shared_ptr<RenderChannelImpl>> mStoppedChannels;
    bool mStopped = false;

    class ProcessCleanupThread;
    std::unique_ptr<ProcessCleanupThread> mCleanupThread;

    std::unique_ptr<RenderThread> mLoaderRenderThread;
};

}  // namespace emugl
