#include "android/emulation/control/keyboard/TouchEventSender.h"

#include <functional>                                    // for __base
#include <vector>                                        // for vector

#include "android/base/Log.h"                            // for LogStream, LOG
#include "android/base/async/Looper.h"                   // for Looper
#include "android/base/async/ThreadLooper.h"             // for ThreadLooper
#include "android/emulation/control/user_event_agent.h"  // for QAndroidUser...
#include "android/hw-events.h"                           // for EV_ABS, EV_SYN
#include "android/skin/generic-event-buffer.h"           // for SkinGenericE...
#include "android/skin/linux_keycodes.h"                 // for LINUX_ABS_MT...
#include "emulator_controller.pb.h"                      // for Touch, Touch...
#include "google/protobuf/repeated_field.h"              // for RepeatedPtrF...

namespace android {
namespace emulation {
namespace control {

/* Maximum number of pointers, supported by multi-touch emulation. */
#define MTS_POINTERS_NUM    10
/* Signals that pointer is not tracked (or is "up"). */
#define MTS_POINTER_UP      -1

TouchEventSender::TouchEventSender(
        const AndroidConsoleAgents* const consoleAgents)
    : mAgents(consoleAgents) {
    mLooper = android::base::ThreadLooper::get();
}

TouchEventSender::~TouchEventSender(){};

bool TouchEventSender::send(const TouchEvent* request) {
    // Send the event on the background looper.
    TouchEvent event = *request;
    mLooper->scheduleCallback([this, event]() {
        this->doSend(event); });
    return true;
}

void TouchEventSender::doSend(const TouchEvent request) {
    // Sends a sequence of touch events to the linux kernel using "Protocol B"
    std::vector<SkinGenericEventCode> events;
    for (auto touch : request.touches()) {
        int slot = touch.identifier();
        if (slot < 0 || slot > MTS_POINTERS_NUM) {
            LOG(WARNING) << "Skipping identifier " << slot
                         << " that is out of range.";
            continue;
        }
        // Only register slot if needed.
        if (mUsedSlots.count(slot) == 0 && touch.pressure() != 0) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot});
            mUsedSlots.insert(slot);
        }

        events.push_back({EV_ABS, LINUX_ABS_MT_SLOT, slot});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_X, touch.x()});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_Y, touch.y()});

        if (touch.touch_major()) {
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_TOUCH_MAJOR, touch.touch_major()});
        }

        if (touch.touch_minor()) {
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_TOUCH_MINOR, touch.touch_minor()});
        }

        // Clean up slot if pressure is 0 (implies finger has been lifted)
        if (touch.pressure() == 0) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP});
            mUsedSlots.erase(slot);
        } else {
            events.push_back({EV_ABS, LINUX_ABS_MT_PRESSURE, touch.pressure()});
        }
    }

    // Submit the events to kernel.
    events.push_back({EV_SYN, LINUX_SYN_REPORT, 0});
    mAgents->user_event->sendGenericEvents(events.data(), events.size());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
