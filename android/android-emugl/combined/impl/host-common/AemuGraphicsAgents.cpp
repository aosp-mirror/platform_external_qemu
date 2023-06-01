// Copyright (C) 2022 The Android Open Source Project
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
#include "AemuGraphicsAgents.h"

#include "android/avd/info.h"                                // for avdInfo_...
#include "android/cmdline-option.h"                          // for AndroidO...
#include "android/console.h"
#include "android/emulation/control/AndroidAgentFactory.h"

#include <cassert>
#include <string>

AndroidOptions emptyOptions{};
AndroidOptions* sAndroid_cmdLineOptions = &emptyOptions;
AvdInfo* sAndroid_avdInfo = nullptr;
AndroidHwConfig s_hwConfig = {0};
AvdInfoParams sAndroid_avdInfoParams = {0};
std::string sCmdlLine;
LanguageSettings s_languageSettings = {0};
AUserConfig* s_userConfig = nullptr;
bool sKeyCodeForwarding = false;

// /* this indicates that guest has mounted data partition */
int s_guest_data_partition_mounted = 0;
// /* this indicates that guest has boot completed */
bool s_guest_boot_completed = 0;
bool s_arm_snapshot_save_completed = 0;
bool s_host_emulator_is_headless = 0;
// /* are we using the emulator in the android mode or plain qemu? */
bool s_android_qemu_mode = true;
// /* are we using android-emu libraries for a minimal configuration? */
// Min config mode and fuchsia mode are equivalent, at least for now.
bool s_min_config_qemu_mode = false;
// /* is android-emu running Fuchsia? */
int s_android_snapshot_update_timer = 0;

static const QAndroidGlobalVarsAgent gMockAndroidGlobalVarsAgent = {
        .avdParams = []() { return &sAndroid_avdInfoParams; },
        .avdInfo =
                []() {
                    // Do not access the info before it is injected!
                    assert(sAndroid_avdInfo != nullptr);
                    return sAndroid_avdInfo;
                },
        .hw = []() { return &s_hwConfig; },
        // /* this indicates that guest has mounted data partition */
        .guest_data_partition_mounted =
                []() { return s_guest_data_partition_mounted; },

        // /* this indicates that guest has boot completed */
        .guest_boot_completed = []() { return s_guest_boot_completed; },

        .arm_snapshot_save_completed =
                []() { return s_arm_snapshot_save_completed; },

        .host_emulator_is_headless =
                []() { return s_host_emulator_is_headless; },

        // /* are we using the emulator in the android mode or plain qemu? */
        .android_qemu_mode = []() { return s_android_qemu_mode; },

        // /* are we using android-emu libraries for a minimal configuration? */
        .min_config_qemu_mode = []() { return s_min_config_qemu_mode; },

        // /* is android-emu running Fuchsia? */
        .is_fuchsia = []() { return s_min_config_qemu_mode; },

        .android_snapshot_update_timer =
                []() { return s_android_snapshot_update_timer; },
        .language = []() { return &s_languageSettings; },
        .use_keycode_forwarding = []() { return sKeyCodeForwarding; },
        .userConfig = []() { return s_userConfig; },
        .android_cmdLineOptions = []() { return sAndroid_cmdLineOptions; },
        .inject_cmdLineOptions =
                [](AndroidOptions* opts) { sAndroid_cmdLineOptions = opts; },
        .has_cmdLineOptions =
                []() {
                    return gMockAndroidGlobalVarsAgent.android_cmdLineOptions() != nullptr;
                },
        .android_cmdLine = []() { return (const char*)sCmdlLine.c_str(); },
        .inject_android_cmdLine =
                [](const char* cmdline) { sCmdlLine = cmdline; },
        .inject_language =
                [](char* language, char* country, char* locale) {
                    s_languageSettings.language = language;
                    s_languageSettings.country = country;
                    s_languageSettings.locale = locale;
                    s_languageSettings.changing_language_country_locale =
                            language || country || locale;
                },
        .inject_userConfig = [](AUserConfig* config) { s_userConfig = config; },
        .set_keycode_forwarding =
                [](bool enabled) { sKeyCodeForwarding = enabled; },
        .inject_AvdInfo = [](AvdInfo* avd) { sAndroid_avdInfo = avd; },

        // /* this indicates that guest has mounted data partition */
        .set_guest_data_partition_mounted =
                [](int guest_data_partition_mounted) {
                    s_guest_data_partition_mounted =
                            guest_data_partition_mounted;
                },

        // /* this indicates that guest has boot completed */
        .set_guest_boot_completed =
                [](bool guest_boot_completed) {
                    s_guest_boot_completed = guest_boot_completed;
                },

        .set_arm_snapshot_save_completed =
                [](bool arm_snapshot_save_completed) {
                    s_arm_snapshot_save_completed = arm_snapshot_save_completed;
                },

        .set_host_emulator_is_headless =
                [](bool host_emulator_is_headless) {
                    s_host_emulator_is_headless = host_emulator_is_headless;
                },

        // /* are we using the emulator in the android mode or plain qemu? */
        .set_android_qemu_mode =
                [](bool android_qemu_mode) {
                    s_android_qemu_mode = android_qemu_mode;
                },

        // /* are we using android-emu libraries for a minimal configuration? */
        .set_min_config_qemu_mode =
                [](bool min_config_qemu_mode) {
                    s_min_config_qemu_mode = min_config_qemu_mode;
                },

        // /* is android-emu running Fuchsia? */
        .set_is_fuchsia = [](bool is_fuchsia) { s_min_config_qemu_mode = is_fuchsia; },

        .set_android_snapshot_update_timer =
                [](int android_snapshot_update_timer) {
                    s_android_snapshot_update_timer =
                            android_snapshot_update_timer;
                }

};

static GraphicsAgents sGraphicsAgents{0};

#define AEMU_TO_GRAPHICS_AGENT_SETTER(typ, name) \
    sGraphicsAgents.name = getConsoleAgents()->name;

const GraphicsAgents* getGraphicsAgents() {
    if (!agentsAvailable()) {
        // Let's not get involved with undefined behavior, if this happens the
        // developer is not calling injectGraphicsAgents early enough.
        fprintf(stderr, "Accessing graphics agents before injecting them.\n");
        *(uint32_t*)(1234) = 5;
    }
    GRAPHICS_AGENTS_LIST(AEMU_TO_GRAPHICS_AGENT_SETTER);
    return &sGraphicsAgents;
}

namespace android {
namespace emulation {

#define DEFINE_AEMU_MOCK_CONSOLE_GETTER(typ, name)         \
    const typ* const android_get_##typ() const {           \
        return mGraphicsAgentFactory->android_get_##typ();  \
    };
class AemuMockConsoleFactory : public android::emulation::AndroidConsoleFactory {
public:
    AemuMockConsoleFactory(const GraphicsAgentFactory* f) : mGraphicsAgentFactory(f) { }

    GRAPHICS_AGENTS_LIST(DEFINE_AEMU_MOCK_CONSOLE_GETTER)
    const QAndroidGlobalVarsAgent* const android_get_QAndroidGlobalVarsAgent()
            const {
        return &gMockAndroidGlobalVarsAgent;
    }
private:
    const GraphicsAgentFactory* mGraphicsAgentFactory;
};

#define DEFINE_GRAPHICS_AGENT_GETTER_IMPL(typ, name)                   \
    const typ* const GraphicsAgentFactory::android_get_##typ() const { \
        return sGraphicsAgents.name;                                   \
    };

GRAPHICS_AGENTS_LIST(DEFINE_GRAPHICS_AGENT_GETTER_IMPL)

#define GRAPHICS_AGENT_SETTER(typ, name) \
    sGraphicsAgents.name = factory.android_get_##typ();

// This should only be used in vkcereal unittests
void injectGraphicsAgents(const GraphicsAgentFactory& factory) {
    if (::agentsAvailable()) {
        // This is not necessarily an error, as there are cases where this might
        // be acceptable.
        fprintf(stderr, "Warning: Console agents were already injected!\n");
    }
    AemuMockConsoleFactory f(&factory);
    injectConsoleAgents(f);
}

const QAndroidEmulatorWindowAgent* const
AemuGraphicsAgentFactory::android_get_QAndroidEmulatorWindowAgent()
        const {
    return getConsoleAgents()->emu;
}

const QAndroidDisplayAgent* const
AemuGraphicsAgentFactory::android_get_QAndroidDisplayAgent() const {
    return getConsoleAgents()->display;
}

const QAndroidRecordScreenAgent* const
AemuGraphicsAgentFactory::android_get_QAndroidRecordScreenAgent() const {
    return getConsoleAgents()->record;
}

const QAndroidMultiDisplayAgent* const
AemuGraphicsAgentFactory::android_get_QAndroidMultiDisplayAgent() const {
    return getConsoleAgents()->multi_display;
}

const QAndroidVmOperations* const
AemuGraphicsAgentFactory::android_get_QAndroidVmOperations() const {
    return getConsoleAgents()->vm;
}

}  // namespace emulation
}  // namespace android
