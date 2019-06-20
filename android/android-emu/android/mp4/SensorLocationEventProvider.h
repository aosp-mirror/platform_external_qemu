#pragma once

#include <memory>

#include "android/automation/AutomationController.h"
#include "android/automation/EventSource.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace android {
namespace mp4 {

// SensorLocationEventProvider turns packets containing sensor/location event
// information into actual event objects that could be consumed by
// AutomationController.
class SensorLocationEventProvider : public automation::EventSource {
public:
    virtual ~SensorLocationEventProvider() = default;
    static std::unique_ptr<SensorLocationEventProvider> create();
    // Create a RecordedEvent from the packet
    virtual int createEvent(AVPacket* packet) { return -1; };
    pb::RecordedEvent consumeNextCommand();
    bool getNextCommandDelay(automation::DurationNs* outDelay);

protected:
    SensorLocationEventProvider() = default;
};

}  // namespace mp4
}  // namespace android