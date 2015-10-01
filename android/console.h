// Copyright 2015 The Android Open Source Project
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

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Generic entry point to start an android console.
// QEMU implementations should populate |vm_operations| with QEMU specific
// functions.
// Takes ownership of |vm_operations|.
extern int control_console_start(int port,
                                 const QAndroidBatteryAgent* battery_agent,
                                 const QAndroidFingerAgent* finger_agent,
                                 const QAndroidLocationAgent* location_agent,
                                 const QAndroidVmOperations* vm_operations);

ANDROID_END_HEADER
