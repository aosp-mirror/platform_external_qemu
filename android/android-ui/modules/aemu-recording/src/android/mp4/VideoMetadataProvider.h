// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>                              // for int32_t, uint64_t
#include <memory>                                // for unique_ptr

#include "offworld.pb.h"  // for DatasetInfo

extern "C" {
#include "libavcodec/avcodec.h"                  // for AVPacket
}

namespace android {
namespace mp4 {

typedef ::offworld::DatasetInfo DatasetInfo;

struct VideoMetadata {
    int32_t frameNumber;
    uint64_t timestamp;
};

class VideoMetadataProvider {
public:
    virtual ~VideoMetadataProvider(){};
    static std::unique_ptr<VideoMetadataProvider> create(
            const DatasetInfo& datasetInfo);
    virtual int createVideoMetadata(const AVPacket* packet) = 0;
    virtual bool hasNextFrameMetadata() = 0;
    virtual VideoMetadata getNextFrameMetadata() = 0;
    virtual VideoMetadata peekNextFrameMetadata() = 0;

protected:
    VideoMetadataProvider() = default;
};

}  // namespace mp4
}  // namespace android
