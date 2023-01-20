#include "android/emulation/control/keyboard/TouchEventSender.h"

#include <functional>  // for __base
#include <vector>      // for vector

#include "aemu/base/Log.h"  // for LogStream, LOG

#include "aemu/base/async/Looper.h"                   // for Looper
#include "aemu/base/async/ThreadLooper.h"             // for ThreadLooper
#include "android/emulation/control/user_event_agent.h"  // for QAndroidUser...
#include "android/hw-events.h"                           // for EV_ABS, EV_SYN
#include "android/multitouch-screen.h"                   // for MTS_*
#include "android/skin/generic-event-buffer.h"           // for SkinGenericE...
#include "android/skin/linux_keycodes.h"                 // for LINUX_ABS_MT...
#include "emulator_controller.pb.h"                      // for Touch, Touch...
#include "google/protobuf/repeated_field.h"              // for RepeatedPtrF...

namespace android {
namespace emulation {
namespace control {

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

int TouchEventSender::pickNextSlot() {
    static_assert(MTS_POINTERS_NUM < 20,
                  "Consider a better algorithm for finding an empty slot.");
    for (int i = 0; i < MTS_POINTERS_NUM; i++) {
        if (mUsedSlots.count(i) == 0) {
            return i;
        }
    }
    return -1;
}

void TouchEventSender::doSend(const TouchEvent request) {
    // Obtain display width, height for the given display id.
    auto displayId = request.display();
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

    for (auto it = mIdLastUsedEpoch.begin(); it != mIdLastUsedEpoch.end();) {
        if (it->second < epoch) {
            auto removeSlot = mIdMap[it->first];
            LOG(WARNING) << "Expiring outdated event id: " << it->first
                         << ", slot: " << removeSlot;

            // First create an up event, otherwise android kernel might get
            // confused
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_SLOT, removeSlot, displayId});
            events.push_back({EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP,
                              displayId});

            // Next remove the mappings from existence.
            mIdMap.erase(it->first);
            it = mIdLastUsedEpoch.erase(it);
            mUsedSlots.erase(removeSlot);
        } else {
            ++it;
        }
    }

    for (auto touch : request.touches()) {
        int slot = 0;
        if (mIdMap.count(touch.identifier()) == 0) {
            // pick next available slot.
            slot = pickNextSlot();
            if (slot < 0) {
                LOG(WARNING) << "No more slots available.. skipping";
                continue;
            }
        } else {
            slot = mIdMap[touch.identifier()];
        }
        if (touch.pressure() == 0 && mUsedSlots.count(slot) == 0) {
            LOG(WARNING) << "Ignorig closure of unused (closed) slot: " << slot;
            continue;
        }

        if (touch.expiration() == Touch::NEVER_EXPIRE) {
            mIdLastUsedEpoch[touch.identifier()] =
                    std::chrono::seconds(LONG_MAX);
        } else {
            mIdLastUsedEpoch[touch.identifier()] =
                    epoch + kTOUCH_EXPIRE_AFTER_120S;
        }

        events.push_back({EV_ABS, LINUX_ABS_MT_SLOT, slot, displayId});

        // Only register slot if needed.
        if (mUsedSlots.count(slot) == 0 && touch.pressure() != 0) {
            events.push_back(
                    {EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot, displayId});
            mUsedSlots.insert(slot);
            mIdMap[touch.identifier()] = slot;
            LOG(DEBUG) << "Registering " << touch.identifier() << " ->"
                         << slot;
        }

        // Scale to proper evdev values.
        int dx = scaleAxis(touch.x(), 0, w);
        int dy = scaleAxis(touch.y(), 0, h);

        if (touch.touch_major()) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TOUCH_MAJOR,
                              touch.touch_major(), displayId});
        }

        if (touch.touch_minor()) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TOUCH_MINOR,
                              touch.touch_minor(), displayId});
        }

        events.push_back(
                {EV_ABS, LINUX_ABS_MT_PRESSURE, touch.pressure(), displayId});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_X, dx, displayId});
        events.push_back({EV_ABS, LINUX_ABS_MT_POSITION_Y, dy, displayId});

        // Clean up slot if pressure is 0 (implies finger has been lifted)
        if (touch.pressure() == 0) {
            events.push_back({EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP,
                              displayId});
            mUsedSlots.erase(slot);
            mIdMap.erase(touch.identifier());
            mIdLastUsedEpoch.erase(touch.identifier());
        }
    }

    // Submit the events to kernel.
    events.push_back({EV_SYN, LINUX_SYN_REPORT, 0, displayId});
    mAgents->user_event->sendGenericEvents(events.data(), events.size());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
