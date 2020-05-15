

// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/utils/ServiceUtils.h"

#include <istream>
#include <memory>
#include <string>
#include <unordered_map>

#include "android/avd/info.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/globals.h"

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::Lock;
namespace android {
namespace emulation {
namespace control {

std::unordered_map<std::string, std::string> getQemuConfig() {
    std::unordered_map<std::string, std::string> cfg;

    /* use the magic of macros to implement the hardware configuration loaded */
    AndroidHwConfig* config = android_hw;

#define HWCFG_BOOL(n, s, d, a, t) cfg[s] = config->n ? "true" : "false";
#define HWCFG_INT(n, s, d, a, t) cfg[s] = std::to_string(config->n);
#define HWCFG_STRING(n, s, d, a, t) cfg[s] = config->n;
#define HWCFG_DOUBLE(n, s, d, a, t) cfg[s] std::to_string(config->n);
#define HWCFG_DISKSIZE(n, s, d, a, t) cfg[s] = std::to_string(config->n);

#include "android/avd/hw-config-defs.h"

    return cfg;
}

struct BootCompletedSyncState {
    Lock lock;
    ConditionVariable cv;
    bool results = false;
};

bool bootCompleted() {
    auto state = std::make_shared<BootCompletedSyncState>();

    AutoLock aLock(state->lock);

    auto adbInterface = emulation::AdbInterface::getGlobal();
    if (!adbInterface) {
        return false;
    }

    // Older API levels do not have a qemu service reporting
    // to us about boot completion, so we will resort to
    // calling adb in those cases..
    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    const int k2SecondsUs = 2 * 1000 * 1000;
    if (!guest_boot_completed && apiLevel < 28) {
        adbInterface->enqueueCommand(
                {"shell", "getprop", "dev.bootcomplete"},
                [state](const OptionalAdbCommandResult& result) {
                    AutoLock aLock(state->lock);
                    if (result) {
                        std::string output(
                                std::istreambuf_iterator<char>(*result->output),
                                {});
                        guest_boot_completed =
                                guest_boot_completed ||
                                output.find("1") != std::string::npos;
                    }
                    state->results = true;
                    state->cv.signal();
                });
        if (!state->results) {
            state->cv.timedWait(
                &state->lock,
                base::System::get()->getUnixTimeUs() + k2SecondsUs);
        }
    }
    return guest_boot_completed;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
