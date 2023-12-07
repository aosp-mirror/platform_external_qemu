// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/emulation/control/notifications/NotificationStream.h"

#include <assert.h>
#include <cstdint>

#include "aemu/base/EventNotificationSupport.h"
#include "android/avd/info.h"
#include "android/emulation/control/BootCompletionHandler.h"
#include "android/hw-sensors.h"
#include "android/utils/debug.h"
#include "host-common/MultiDisplay.h"

#define DEBUG 0
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace control {

NotificationStream::NotificationStream(VirtualSceneCamera* camera,
                                       const AndroidConsoleAgents* agents)
    : mCamera(camera), mAgents(agents) {}

std::optional<Notification> NotificationStream::getDisplayNotificationEvent() {
    Notification event;
    event.set_event(Notification::DISPLAY_CONFIGURATIONS_CHANGED_UI);

    auto eventDetails =
            event.mutable_displayconfigurationschangednotification();
    auto displayConfig = eventDetails->mutable_displayconfigurations();

    for (int i = 0; i < MultiDisplay::s_maxNumMultiDisplay; i++) {
        uint32_t width = 0, height = 0, dpi = 0, flags = 0;
        bool enabled = false;
        bool receivedDisplayStatus = mAgents->multi_display->getMultiDisplay(
                i, nullptr, nullptr, &width, &height, &dpi, &flags, &enabled);

        // Workaround for the fact that MultiDisplay is currently not atomic.
        // This will make sure we will not include partially created/configured
        // displays in the event (See b/290831895).
        bool isPartiallyCreatedEvent = width == 0 && height == 0 && dpi == 0;
        if (receivedDisplayStatus && !isPartiallyCreatedEvent) {
            auto cfg = displayConfig->add_displays();
            cfg->set_width(width);
            cfg->set_height(height);
            cfg->set_dpi(dpi);
            cfg->set_display(i);
            cfg->set_flags(flags);

            const bool is_pixel_fold = android_foldable_is_pixel_fold();
            if (is_pixel_fold)
                break;
        }
    }

    displayConfig->set_maxdisplays(android::MultiDisplay::s_maxNumMultiDisplay);
    displayConfig->set_userconfigurable(avdInfo_maxMultiDisplayEntries());

    DD("Display notification event: %s", event.ShortDebugString().c_str());
    return event;
}

std::optional<Notification> NotificationStream::getBootedNotificationEvent() {
    Notification event;
    if (!BootCompletionHandler::get()->hasBooted()) {
        DD("Not yet booted");
        return std::nullopt;
    }

    auto eventDetails = event.mutable_booted();
    eventDetails->set_time(BootCompletionHandler::get()->bootTime().count());

    DD("Booted notification event: %s", event.ShortDebugString().c_str());
    return event;
}

std::optional<Notification> NotificationStream::getCameraNotificationEvent() {
    Notification event;
    auto eventDetails = event.mutable_cameranotification();

    // Camera is currently always associated with display 0
    eventDetails->set_display(0);
    if (mCamera->isConnected()) {
        event.set_event(Notification::VIRTUAL_SCENE_CAMERA_ACTIVE);
        eventDetails->set_active(true);
    } else {
        event.set_event(Notification::VIRTUAL_SCENE_CAMERA_INACTIVE);
        eventDetails->set_active(false);
    }

    assert(event.has_cameranotification());

    DD("Camera notification event: %s", event.ShortDebugString().c_str());
    return event;
}

std::optional<Notification> NotificationStream::getPostureNotificationEvent() {
    Notification event;
    static_assert((int)POSTURE_MAX ==
                  ::android::emulation::control::Posture_PostureValue::
                          Posture_PostureValue_POSTURE_MAX);
    auto currentPosture =
            static_cast<FoldablePostures>(android_foldable_get_posture());

    if (currentPosture == POSTURE_UNKNOWN || currentPosture == POSTURE_MAX) {
        VERBOSE_INFO(grpc,
                     "Received bad foldable posture when trying to send "
                     "gRPC notification, posture "
                     "value: %d",
                     currentPosture);
        return std::nullopt;
        return std::nullopt;
    }
    event.mutable_posture()->set_value(
            static_cast< ::android::emulation::control::Posture_PostureValue>(
                    currentPosture));

    DD("Posture notification event: %s", event.ShortDebugString().c_str());
    return event;
}

NotificationStreamWriter* NotificationStream::notificationStream() {
    if (!mRegisteredListeners.test_and_set()) {
        registerListeners();
    }

    auto stream = new NotificationStreamWriter(&mNotificationListeners);
    stream->eventArrived(getCameraNotificationEvent());
    stream->eventArrived(getPostureNotificationEvent());
    stream->eventArrived(getBootedNotificationEvent());
    return stream;
}

static void brightness_forwarder(void* opaque,
                                 const char* light,
                                 int brightness) {
    NotificationEventChangeSupport* notificationListeners =
            reinterpret_cast<NotificationEventChangeSupport*>(opaque);
    Notification event;
    auto bv = event.mutable_brightness();

    std::string name = light;
    if (name == "lcd_backlight") {
        bv->set_target(BrightnessValue::LCD);
    } else if (name == "keyboard_backlight") {
        bv->set_target(BrightnessValue::KEYBOARD);
    } else if (name == "button_backlight") {
        bv->set_target(BrightnessValue::BUTTON);
    }

    bv->set_value(brightness);
    notificationListeners->fireEvent(event);
}

void NotificationStream::registerListeners() {
    // Register event listeners.
    mCamera->registerOnce([&](auto state) {
        mNotificationListeners.fireEvent(getCameraNotificationEvent());
    });

    // TODO(jansene): This assumes we close down the event handler system after
    // qemu stops!
    const static AndroidHwControlFuncs sCallbacks = {
            .light_brightness = brightness_forwarder};
    mAgents->hw_control->setCallbacks(&mNotificationListeners, &sCallbacks);

    MultiDisplay::getInstance()->registerOnce(
            [&](const android::DisplayChangeEvent state) {
                DD("Displaychange event: %d", state.change);
                if (state.change == DisplayChange::DisplayAdded) {
                    return;
                }
                mNotificationListeners.fireEvent(getDisplayNotificationEvent());
            });
    auto foldableListener =
            static_cast<base::EventNotificationSupport<FoldablePostures>*>(
                    android_get_posture_listener());
    foldableListener->registerOnce([&](auto state) {
        mNotificationListeners.fireEvent(getPostureNotificationEvent());
    });
    BootCompletionHandler::get()->registerOnce([&](auto completed) {
        mNotificationListeners.fireEvent(getBootedNotificationEvent());
    });
}
}  // namespace control
}  // namespace emulation
}  // namespace android
