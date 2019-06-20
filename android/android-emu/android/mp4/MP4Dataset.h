#pragma once

#include <memory>
#include <string>

#include "android/hw-sensors.h"
#include "android/utils/compiler.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace android {
namespace mp4 {

// A wrapper around an MP4 dataset file that provides access to
// information about this dataset.
class Mp4Dataset {
public:
    virtual ~Mp4Dataset() {};
    static std::unique_ptr<Mp4Dataset> create(std::string filepath);
    virtual int init() = 0;
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