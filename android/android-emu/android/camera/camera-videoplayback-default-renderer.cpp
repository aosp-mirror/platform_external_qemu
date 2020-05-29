/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/camera/camera-videoplayback-default-renderer.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "offworld.pb.h"

namespace android {
namespace videoplayback {

class DefaultFrameRendererImpl {
public:
    DefaultFrameRendererImpl(const GLESv2Dispatch* gles2,
                             int width,
                             int height,
                             base::Looper* looper);
    ~DefaultFrameRendererImpl() = default;

    bool initialize();
    void render();
    int64_t nowNs();
private:
    const GLESv2Dispatch* const mGles2;
    const int mRenderWidth;
    const int mRenderHeight;
    base::Looper* const mLooper;
};

DefaultFrameRendererImpl::DefaultFrameRendererImpl(const GLESv2Dispatch* gles2,
                                                   int width,
                                                   int height,
                                                   base::Looper* looper)
    : mGles2(gles2), mRenderWidth(width), mRenderHeight(height), mLooper(looper) {
}

bool DefaultFrameRendererImpl::initialize() {
    return true;
}

void DefaultFrameRendererImpl::render() {
    mGles2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mGles2->glViewport(0, 0, mRenderWidth, mRenderHeight);
    mGles2->glClearColor(1.0f, 0.6f, 0.0f, 1.0f);
    mGles2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGles2->glFrontFace(GL_CW);
    mGles2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGles2->glBindTexture(GL_TEXTURE_2D, 0);
    mGles2->glUseProgram(0);
}

int64_t DefaultFrameRendererImpl::nowNs() {
  return mLooper->nowNs(base::Looper::ClockType::kVirtual);
}


DefaultFrameRenderer::DefaultFrameRenderer() {}
DefaultFrameRenderer::~DefaultFrameRenderer() {}

bool DefaultFrameRenderer::initialize(const GLESv2Dispatch* gles2,
                                      int width,
                                      int height) {
    mImpl = std::unique_ptr<DefaultFrameRendererImpl>(
            new DefaultFrameRendererImpl(gles2, width, height, base::ThreadLooper::get()));
    return mImpl->initialize();
}

void DefaultFrameRenderer::uninitialize() {
    mImpl.reset();
}

int64_t DefaultFrameRenderer::render() {
    mImpl->render();
    return mImpl->nowNs();
}

}  // namespace videoplayback
}  // namespace android
