#include "android/emulation/control/keyboard/TouchEventSender.h"

#include <functional>  // for __base
#include <vector>      // for vector

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
#define MTS_POINTERS_NUM 10
/* Signals that pointer is not tracked (or is "up"). */
#define MTS_POINTER_UP -1

#define EV_ABS_MIN 0x0000
#define EV_ABS_MAX 0x7FFF

TouchEventSender::TouchEventSender(
        const AndroidConsoleAgents* const consoleAgents)
    : mAgents(consoleAgents) {
    mLooper = android::base::ThreadLooper::get();
}

TouchEventSender::~TouchEventSender(){};

bool TouchEventSender::send(const TouchEvent* request) {
    // Send the event on the background looper.
    TouchEvent event = *request;
    mLooper->scheduleCallback([this, event]() { this->doSend(event); });
    return true;
}

bool TouchEventSender::sendOnThisThread(const TouchEvent* request) {
    // Send the event on the current thread, not the background looper.
    TouchEvent event = *request;
    doSend(event);
    return true;
}

// Scales an axis to the proper EVDEV value..
static int scaleAxis(int value,
                     int min_in,
                     int max_in) {
    constexpr int64_t min_out = EV_ABS_MIN;
    constexpr int64_t max_out = EV_ABS_MAX;
    constexpr int64_t range_out = (int64_t)max_out - min_out;

    int64_t range_in = (int64_t)max_in - min_in;
    if (range_in < 1) {
        return min_out + range_out / 2;
    }
    return ((int64_t)value - min_in) * range_out / range_in + min_out;
}

void TouchEventSender::doSend(const TouchEvent request) {

    // Obtain display width, height for the given display id.
    auto displayId = request.device();
    uint32_t w = 0;
    uint32_t h = 0;
    if (!mAgents->emu->getMultiDisplay(displayId, NULL, NULL, &w, &h, NULL,
                                       NULL, NULL)) {
        mAgents->display->getFrameBuffer((int*)&w, (int*)&h, NULL, NULL, NULL);
    }

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
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot, displayId});
            mUsedSlots.insert(slot);
        }

        // Scale to proper evdev values.
        int dx = scaleAxis(touch.x(), 0, w);
        int dy = scaleAxis(touch.y(), 0, h);

        events.push_back({EV_ABS, LINUX_ABS_MT_SLOT, slot, displayId});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_X, dx, displayId});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_Y, dy, displayId});

        if (touch.touch_major()) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TOUCH_MAJOR,
                              touch.touch_major(), displayId});
        }

        if (touch.touch_minor()) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TOUCH_MINOR,
                              touch.touch_minor(), displayId});
        }

        // Clean up slot if pressure is 0 (implies finger has been lifted)
        if (touch.pressure() == 0) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP,
                              displayId});
            mUsedSlots.erase(slot);
        } else {
            events.push_back({EV_ABS, LINUX_ABS_MT_PRESSURE, touch.pressure(),
                              displayId});
        }
    }

    // Submit the events to kernel.
    events.push_back({EV_SYN, LINUX_SYN_REPORT, 0, displayId});
    mAgents->user_event->sendGenericEvents(events.data(), events.size());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
