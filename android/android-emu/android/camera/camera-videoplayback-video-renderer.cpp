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

#include "android/camera/camera-videoplayback-video-renderer.h"

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/camera/camera-virtualscene-utils.h"
#include "android/videoplayback/VideoplaybackRenderTarget.h"

namespace android {
namespace videoplayback {

VideoplaybackVideoRenderer::VideoplaybackVideoRenderer(): mLooper(base::ThreadLooper::get()){}
VideoplaybackVideoRenderer::~VideoplaybackVideoRenderer() {}

bool VideoplaybackVideoRenderer::initialize(const GLESv2Dispatch* gles2,
                                            int width,
                                            int height) {
    mRenderTarget = std::unique_ptr<VideoplaybackRenderTarget>(
            new VideoplaybackRenderTarget(gles2, width, height));
    return mRenderTarget->initialize();
}

void VideoplaybackVideoRenderer::uninitialize() {
    mRenderTarget->uninitialize();
    mRenderTarget.reset();
}

int64_t VideoplaybackVideoRenderer::render() {
  mRenderTarget->renderBuffer();
  return mLooper->nowNs(base::Looper::ClockType::kVirtual);
}

VideoplaybackRenderTarget* VideoplaybackVideoRenderer::renderTarget() {
  return mRenderTarget.get();
}

}  // namespace videoplayback
}  // namespace android
