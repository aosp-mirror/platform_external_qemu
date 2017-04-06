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

#include "android-qemu1-glue/qemu-setup.h"

#include "android-qemu1-glue/emulation/VmLock.h"
#include "android-qemu1-glue/qemu-control-impl.h"
#include "android/android.h"
#include "android/base/Log.h"
#include "android/console.h"
#include "android/emulation/AndroidPipe.h"
#include "android/skin/LibuiAgent.h"

extern "C" {
#include "qemu-common.h"
}  // extern "C"

using android::VmLock;

bool qemu_android_emulation_setup() {
  static const AndroidConsoleAgents consoleAgents = {
      gQAndroidBatteryAgent,   gQAndroidEmulatorWindowAgent,
      gQAndroidFingerAgent,    gQAndroidLocationAgent,
      gQAndroidHttpProxyAgent,
      gQAndroidTelephonyAgent, gQAndroidUserEventAgent,
      gQAndroidVmOperations,   gQAndroidNetAgent,
      gQAndroidLibuiAgent,     gQCarDataAgent,
  };

  VmLock* vmLock = new qemu::VmLock();
  VmLock* prevVmLock = android::VmLock::set(vmLock);
  CHECK(prevVmLock == nullptr) << "Another VmLock was already installed!";

  android::AndroidPipe::initThreading(vmLock);

  return android_emulation_setup(&consoleAgents, false);
}
