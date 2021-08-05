// Copyright 2019 The Android Open Source Project
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

#include "android/skin/event.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* Maximum number of virtio input devices*/
#define VIRTIO_INPUT_MAX_NUM 11

extern int android_virtio_input_send(int type,
                                     int code,
                                     int value,
                                     int displayId);

extern void android_virtio_kbd_mouse_event(int dx,
                                           int dy,
                                           int dz,
                                           int buttonsState,
                                           int displayId);

extern void android_virtio_pen_event(int dx,
                                     int dy,
                                     const SkinEvent* ev,
                                     int buttonsState,
                                     int displayId);

extern void android_virtio_touch_event(const SkinEvent* const data,
                                       int displayId);
ANDROID_END_HEADER
