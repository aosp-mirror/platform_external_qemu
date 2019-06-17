#pragma once

#include <chrono>

#include "android/automation/proto/automation.pb.h"

namespace pb = emulator_automation;

namespace android {
namespace automation {

// An EventSource provides a series of RecordedEvent's to AutomationController
class EventSource {
public:
    virtual ~EventSource() = default;

    // Return the next command from the source.
    virtual pb::RecordedEvent consumeNextCommand() = 0;

    // Get the next command delay from the event source.  Returns false if there
    // are no events remaining.
    virtual bool getNextCommandDelay(DurationNs* outDelay) = 0;
};

}  // namespace automation
}  // namespace android