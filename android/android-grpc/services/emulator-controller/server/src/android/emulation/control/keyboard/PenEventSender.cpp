#include "android/emulation/control/keyboard/PenEventSender.h"

#include <functional>
#include <vector>

#include "aemu/base/Log.h"

#include "aemu/base/async/ThreadLooper.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/hw-events.h"
#include "android/hw-sensors.h"
#include "android/multitouch-screen.h"
#include "android/skin/generic-event-buffer.h"
#include "android/skin/linux_keycodes.h"
#include "emulator_controller.pb.h"
#include "google/protobuf/repeated_field.h"

namespace android {
namespace emulation {
namespace control {

#define EV_ABS_MIN 0x0000
#define EV_ABS_MAX 0x7FFF

/* Special tracking ID for the pen pointer, the next one after the fingers.*/
#define MTS_POINTER_PEN         MTS_POINTERS_NUM

// Scales an axis to the proper EVDEV value..
static int scaleAxis(int value, int min_in, int max_in) {
    constexpr int64_t min_out = EV_ABS_MIN;
    constexpr int64_t max_out = EV_ABS_MAX;
    constexpr int64_t range_out = (int64_t)max_out - min_out;

    int64_t range_in = (int64_t)max_in - min_in;
    if (range_in < 1) {
        return min_out + range_out / 2;
    }
    return ((int64_t)value - min_in) * range_out / range_in + min_out;
}

void PenEventSender::doSend(const PenEvent request) {
    // Obtain display width, height for the given display id.
    auto displayId = request.display();
    if (android_foldable_is_folded() && android_foldable_is_pixel_fold()) {
        displayId = android_foldable_pixel_fold_second_display_id();
    }
    uint32_t w = 0;
    uint32_t h = 0;
    if (!mAgents->emu->getMultiDisplay(displayId, NULL, NULL, &w, &h, NULL,
                                       NULL, NULL)) {
        mAgents->display->getFrameBuffer((int*)&w, (int*)&h, NULL, NULL, NULL);
    }

    // Sends a sequence of touch events to the linux kernel using "Protocol B"
    std::vector<SkinGenericEventCode> events;

    // Let's expunge expired events..
    // This alleviates problems where a developer never closes out a slot.
    auto now = std::chrono::system_clock::now();
    std::chrono::seconds epoch =
            std::chrono::duration_cast<std::chrono::seconds>(
                    now.time_since_epoch());

    if (mIdLastUsedEpoch < epoch) {
        // First create an up event, otherwise android kernel might get
        // confused
        events.push_back(
                {EV_ABS, LINUX_ABS_MT_SLOT, MTS_POINTER_PEN, displayId});
        events.push_back(
                {EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP, displayId});
        mIdLastUsedEpoch = std::chrono::seconds(LONG_MAX);
    }

    for (auto event : request.events()) {
        auto touch = event.location();

        int slot = MTS_POINTER_PEN;

        if (touch.expiration() == Touch::NEVER_EXPIRE) {
            mIdLastUsedEpoch = std::chrono::seconds(LONG_MAX);
        } else {
            mIdLastUsedEpoch = epoch + kPEN_EXPIRE_AFTER_120S;
        }

        events.push_back({EV_ABS, LINUX_ABS_MT_SLOT, slot, displayId});

        // Only register slot if needed.
        if (touch.pressure() != 0) {
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot, displayId});
        }

        // Scale to proper evdev values.
        int dx = scaleAxis(touch.x(), 0, w);
        int dy = scaleAxis(touch.y(), 0, h);

        if (touch.orientation()) {
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_ORIENTATION, touch.orientation()});
        }

        auto ev_dev_pressed =
                event.button_pressed() ? MTS_BTN_DOWN : MTS_BTN_UP;
        if (event.rubber_pointer()) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TOOL_TYPE, MT_TOOL_MAX});
            events.push_back({EV_KEY, LINUX_BTN_TOOL_RUBBER, ev_dev_pressed});
        } else {
            events.push_back({EV_ABS, LINUX_ABS_MT_TOOL_TYPE, MT_TOOL_PEN});
        }

        events.push_back({EV_KEY, LINUX_BTN_STYLUS, ev_dev_pressed});
        events.push_back(
                {EV_ABS, LINUX_ABS_MT_PRESSURE, touch.pressure(), displayId});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_X, dx, displayId});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_Y, dy, displayId});

        // Clean up slot if pressure is 0 (implies finger has been lifted)
        if (touch.pressure() == 0) {
            events.push_back({EV_ABS, LINUX_ABS_MT_PRESSURE, 0});
            events.push_back({EV_ABS, LINUX_ABS_MT_ORIENTATION, 0});
            events.push_back({EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP,
                              displayId});
            mIdLastUsedEpoch = std::chrono::seconds(LONG_MAX);
        }
    }

    // Submit the events to kernel.
    events.push_back({EV_SYN, LINUX_SYN_REPORT, 0, displayId});
    mAgents->user_event->sendGenericEvents(events.data(), events.size());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
