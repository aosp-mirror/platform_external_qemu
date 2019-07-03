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

#include <memory>
#include <string>

#include "android/hw-sensors.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/utils/compiler.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace android {
namespace mp4 {

typedef ::offworld::DatasetInfo DatasetInfo;

// A wrapper around an MP4 dataset file that provides access to
// information about this dataset.
class Mp4Dataset {
public:
    virtual ~Mp4Dataset(){};
    static std::unique_ptr<Mp4Dataset> create(std::string filepath,
                                              DatasetInfo* datasetInfo);
    virtual int getAudioStreamIndex() = 0;
    virtual int getVideoStreamIndex() = 0;
    virtual int getSensorDataStreamIndex(AndroidSensor sensor) = 0;
    virtual int getLocationDataStreamIndex() = 0;
    virtual AVFormatContext* getFormatContext() = 0;
    virtual void clearFormatContext() = 0;

protected:
    Mp4Dataset() = default;
};

}  // namespace mp4
}  // namespace android
