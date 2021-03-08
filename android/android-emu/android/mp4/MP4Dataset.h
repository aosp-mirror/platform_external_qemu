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

#include <memory>                                // for unique_ptr
#include <string>                                // for string

#include "android/hw-sensors.h"                  // for AndroidSensor
#include "offworld.pb.h"  // for DatasetInfo

extern "C" {
#include "libavformat/avformat.h"                // for AVFormatContext
}

namespace android {
namespace mp4 {

typedef ::offworld::DatasetInfo DatasetInfo;

// A wrapper around an MP4 dataset file that provides access to
// information about this dataset.
class Mp4Dataset {
public:
    virtual ~Mp4Dataset() {};
    static std::unique_ptr<Mp4Dataset> create(const std::string filepath,
                                              const DatasetInfo& datasetInfo);
    virtual int getAudioStreamIndex() = 0;
    virtual int getVideoStreamIndex() = 0;
    virtual int getSensorDataStreamIndex(AndroidSensor sensor) = 0;
    virtual int getLocationDataStreamIndex() = 0;
    virtual int getVideoMetadataStreamIndex() = 0;
    virtual AVFormatContext* getFormatContext() = 0;
    virtual void clearFormatContext() = 0;

protected:
    Mp4Dataset() = default;
};

}  // namespace mp4
}  // namespace android
