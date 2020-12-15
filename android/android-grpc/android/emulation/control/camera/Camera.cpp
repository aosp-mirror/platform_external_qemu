
// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/camera/Camera.h"

#include "android/camera/camera-service.h"

namespace android {
namespace emulation {
namespace control {

static Camera* sCamera = nullptr;

Camera* Camera::getCamera() {
    if (!sCamera) {
        sCamera = new Camera();
    }
}

Camera::Camera() : mConnected(false) {
    register_camera_status_change_callback(&Camera::guestCameraCallback, this);
}

void Camera::guestCameraCallback(void* context, bool connected) {
    auto self = static_cast<Camera*>(context);
    self->mConnected = connected;
    self->mEventWaiter.newEvent();
}

EventWaiter* Camera::eventWaiter() {
    return &mEventWaiter;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
