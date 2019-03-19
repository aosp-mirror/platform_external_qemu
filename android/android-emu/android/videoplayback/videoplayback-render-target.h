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
#include "android/recording/video/player/VideoPlayerRenderTarget.h"

#include <memory>

namespace android {
namespace videoplayback {

class VideoplaybackRenderTarget : public VideoPlayerRenderTarget {
 public:
  VideoplaybackRenderTarget(const GLESv2Dispatch* gles2,
                            int width,
                            int height);
  void getRenderTargetSize(float sampleAspectRatio,
                           int video_width,
                           int video_height,
                           int* resultRenderTargetWidth,
                           int* resultRenderTargetHeight) override;
  void setPixelBuffer(const unsigned char* buf, size_t len) override;

  void renderBuffer();
 private:
  const unsigned char* mBuffer = nullptr;
  size_t mBufferLen;
  const GLESv2Dispatch* const mGles2;
  int mRenderWidth;
  int mRenderHeight;
};

}  // namespace videoplayback
}  // namespace android
