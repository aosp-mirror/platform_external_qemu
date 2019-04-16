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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH1 "virtio-input-multi-touch-device1"
#define VIRTIO_INPUT_MULTI_TOUCH1(obj) \
    OBJECT_CHECK(VirtIOInputMultiTouch1, (obj), TYPE_VIRTIO_INPUT_MULTI_TOUCH1)

typedef struct VirtIOInputMultiTouch1 VirtIOInputMultiTouch1;

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI1 "virtio-input-multi-touch-pci1"
#define VIRTIO_INPUT_MULTI_TOUCH_PCI1(obj)         \
    OBJECT_CHECK(VirtIOInputMultiTouchPCI1, (obj), \
                 TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI1)

typedef struct VirtIOInputMultiTouchPCI1 VirtIOInputMultiTouchPCI1;

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH2 "virtio-input-multi-touch-device2"
#define VIRTIO_INPUT_MULTI_TOUCH2(obj) \
    OBJECT_CHECK(VirtIOInputMultiTouch2, (obj), TYPE_VIRTIO_INPUT_MULTI_TOUCH2)

typedef struct VirtIOInputMultiTouch2 VirtIOInputMultiTouch2;

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI2 "virtio-input-multi-touch-pci2"
#define VIRTIO_INPUT_MULTI_TOUCH_PCI2(obj)         \
    OBJECT_CHECK(VirtIOInputMultiTouchPCI2, (obj), \
                 TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI2)

typedef struct VirtIOInputMultiTouchPCI2 VirtIOInputMultiTouchPCI2;

extern int android_virtio_input_send(int type, int code, int value);
extern void android_virtio_kbd_mouse_event(int dx,
                                           int dy,
                                           int dz,
                                           int buttonsState,
                                           int displayId);
ANDROID_END_HEADER