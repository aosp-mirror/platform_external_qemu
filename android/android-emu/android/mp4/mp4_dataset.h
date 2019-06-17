#pragma once

#include "android/hw-sensors.h"
#include "android/utils/compiler.h"

#include <memory>

extern "C" {
#include "libavformat/avformat.h"
}

namespace android {
namespace mp4 {

class Mp4Dataset {
public:
    virtual ~Mp4Dataset() = default;
    static std::unique_ptr<Mp4Dataset> create(std::string filepath);
    virtual int init();
    virtual int getAudioStreamIndex();
    virtual int getVideoStreamIndex();
    virtual int getSensorDataStreamIndex(AndroidSensor sensor);
    virtual int getLocationDataStreamIndex();
    virtual AVFormatContext* getFormatContext();
    virtual void clearFormatContext();

protected:
    Mp4Dataset() = default;
};

}
}