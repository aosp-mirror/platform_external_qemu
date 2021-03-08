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
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/cellular_agent.h"
#include "android/emulation/control/clipboard_agent.h"
#include "android/emulation/control/display_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/grpc_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/http_proxy_agent.h"
#include "android/emulation/control/net_agent.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/virtual_scene_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// TODO(jansene): This file can be safely deleted once all the include dependencies are fixed.

// This file contains handles to globally allocated objects implementing various
// interfaces required by AndroidEmu library in emulation/control/*.

// Defined in .../emulation/control/automation_agent.h
typedef struct QAndroidAutomationAgent QAndroidAutomationAgent;

ANDROID_END_HEADER
