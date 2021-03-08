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

#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/camera/camera-virtualscene-utils.h"
#include "android/videoplayback/VideoplaybackRenderTarget.h"

namespace android {
namespace videoplayback {

class VideoplaybackVideoRenderer : public virtualscene::CameraRenderer {
 public:
  VideoplaybackVideoRenderer();
  ~VideoplaybackVideoRenderer();
  bool initialize(const GLESv2Dispatch* gles2,
                  int width,
                  int height) override;
  void uninitialize() override;
  int64_t render() override;
  VideoplaybackRenderTarget* renderTarget();
 private:
  std::unique_ptr<VideoplaybackRenderTarget> mRenderTarget;
  base::Looper* const mLooper;
};


}  // namespace videoplayback
}  // namespace android
