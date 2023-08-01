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
#pragma once

#include <atomic>
#include <optional>
#include "android/console.h"
#include "android/emulation/control/camera/VirtualSceneCamera.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

// EventChangeSupport which can use optional values for the event.
class NotificationEventChangeSupport : public EventChangeSupport<Notification> {
public:
    void fireEvent(std::optional<Notification> event) {
        if (event.has_value()) {
            EventChangeSupport<Notification>::fireEvent(*event);
        }
    }
};

// A filtered stream writer that will only write unique notifications
class NotificationStreamWriter : public UniqueEventStreamWriter<Notification> {
public:
    NotificationStreamWriter(NotificationEventChangeSupport* listener)
        : UniqueEventStreamWriter<Notification>(listener) {}

    // Dispatch an event if it is actually there.
    void eventArrived(const std::optional<Notification> event) {
        if (event.has_value()) {
            UniqueEventStreamWriter<Notification>::eventArrived(*event);
        }
    }
};

// This class implements a factory that can produce a NoticationStreamImpl
// A NotificationStreamImpl is an async grpc implementation for delivering
// notification events.
//
class NotificationStream {
public:
    NotificationStream(VirtualSceneCamera* camera,
                       const AndroidConsoleAgents* agents);
    ~NotificationStream() = default;

    // Produce an aysnchronous handler for the following gRPC method:
    //
    //
    // Notifies client of the following changes:
    //
    // - Virtual scene camera status change.
    // - Boot complete status change.
    // - Display configuration changes from extended ui. This will only be fired
    //   if the user makes modifications the extended displays through the
    //   extended control tab.
    //
    // Note that this method will send the initial virtual scene state
    // immediately.
    // rpc streamNotification(google.protobuf.Empty)
    //         returns (stream Notification) {}
    //
    NotificationStreamWriter* notificationStream();

private:
    void registerListeners();
    std::optional<Notification> getCameraNotificationEvent();
    std::optional<Notification> getPostureNotificationEvent();
    std::optional<Notification> getDisplayNotificationEvent();
    std::optional<Notification> getBootedNotificationEvent();

    NotificationEventChangeSupport mNotificationListeners;
    VirtualSceneCamera* mCamera;
    const AndroidConsoleAgents* mAgents;
    std::atomic_flag mRegisteredListeners = ATOMIC_FLAG_INIT;
};
}  // namespace control
}  // namespace emulation
}  // namespace android