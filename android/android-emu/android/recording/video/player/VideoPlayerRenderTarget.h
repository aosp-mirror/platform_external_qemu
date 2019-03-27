// Copyright 2019 The Android Open Source Project
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

#include <stdlib.h>

// Abstract interface-only "class" that can do stuff upon
// changes in the video dimensions.
class VideoPlayerRenderTarget {
public:
    // Metadata for the decoded frame being passed to setPixelBuffer.
    struct FrameInfo {
      size_t headerlen;
      size_t width;
      size_t height;
    };

    // An interaction between the render target (whether it is a raw buffer, Qt
    // widget, or GL texture, VkImage, or whatever) and the video player.
    //
    // The video player calls getRenderTargetSize passing in the raw video
    // dimensions, then gets the render target's size as the output.
    //
    // It is possible that the render target size differs from the input size,
    // and that the render target itself may change to accommodate the video.
    virtual void getRenderTargetSize(float sampleAspectRatio,
                                     int video_width,
                                     int video_height,
                                     int* resultRenderTargetWidth,
                                     int* resultRenderTargetHeight) = 0;

    // The video player calls setPixelBuffer to communicate the actual decoded
    // result to the render target.
    // TODO: An interface that doesn't
    // necessarily have to use CPU memory transfers.
    virtual void setPixelBuffer(
            const FrameInfo& frameInfo,
            const unsigned char* buf,
            size_t len) = 0;

};
