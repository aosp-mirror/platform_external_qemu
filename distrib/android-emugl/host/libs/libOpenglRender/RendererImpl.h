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

#include "RenderThread.h"

#include <memory>
#include <utility>
#include <vector>

namespace emugl {

class RendererImpl final : public Renderer,
                           public std::enable_shared_from_this<RendererImpl> {
public:
    RendererImpl() = default;
    ~RendererImpl();

    bool initialize(int width, int height, bool useSubWindow);

    void stop();

public:
    virtual RenderChannelPtr createRenderChannel() override final;
    virtual HardwareStrings getHardwareStrings() override final;
    virtual void setPostCallback(OnPostCallback onPost,
                                 void* context) override final;
    virtual bool showOpenGLSubwindow(FBNativeWindowType window,
                                     int wx,
                                     int wy,
                                     int ww,
                                     int wh,
                                     int fbw,
                                     int fbh,
                                     float dpr,
                                     float zRot) override final;
    virtual bool destroyOpenGLSubwindow() override final;
    virtual void setOpenGLDisplayRotation(float zRot) override final;
    virtual void setOpenGLDisplayTranslation(float px, float py) override final;
    virtual void repaintOpenGLDisplay() override final;

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RendererImpl);

private:
    std::unique_ptr<RenderWindow> mRenderWindow;

    android::base::Lock mRenderThreadSharedLock;
    android::base::Lock mThreadVectorLock;

    using ThreadWithChannel = std::pair<std::unique_ptr<RenderThread>,
                                        std::weak_ptr<RenderChannelImpl>>;

    std::vector<ThreadWithChannel> mThreads;
};

}  // namespace emugl
