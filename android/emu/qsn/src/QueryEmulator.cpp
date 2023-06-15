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
#include <stdio.h>   // for printf
#include <stdlib.h>  // for exit, EXIT_FAILURE, NULL
#include <iostream>  // for operator<<, endl, basic_ost...
#include <string>    // for string
#include <vector>    // for vector

#ifdef _MSC_VER
#include "msvc-getopt.h"
#include "msvc-posix.h"
#else
#include <getopt.h>  // for no_argument, getopt_long
#endif

#include <google/protobuf/text_format.h>  // for TextFormat

#include "android-qemu2-glue/qemu-console-factory.h"
#include "android/cmdline-definitions.h"
#include "android/avd/info.h"         // for avdInfo_new

#include "android/emulation/control/AndroidAgentFactory.h"
#include "android/console.h"  // for getConsoleAgents()->settings->avdInfo(), android_av...
#include "android/snapshot/Snapshot.h"  // for Snapshot
#include "snapshot.pb.h"                // for Snapshot
#include "snapshot_service.pb.h"        // for SnapshotDetails, SnapshotList

using android::emulation::control::SnapshotDetails;
using android::emulation::control::SnapshotList;
using android::snapshot::Snapshot;

static struct option long_options[] = {{"valid-only", no_argument, 0, 'l'},
                                       {"invalid-only", no_argument, 0, 'i'},
                                       {"no-size", no_argument, 0, 's'},
                                       {"human", no_argument, 0, 'u'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

static void printUsage() {
    printf("usage: qsn [--valid-only] [--invalid-only] [--no-size] [--help] "
           "avd\n"
           "\n"
           "Lists available snapshots for the given avd in protobuf text "
           "format. The protobuf specification can be found\n"
           "in $ANDROID_SDK_ROOT/emulator/lib/snapshot_service.proto\n"
           "\n"
           "positional arguments:\n"
           "  avd             Name of the avd to query.\n"
           "\n"
           "optional arguments:\n"
           "  --valid-only    only valid snapshots are returned\n"
           "  --invalid-only  only invalid snapshots are returned\n"
           "  --no-size       size on disk is not included in the output\n"
           "  --help          show this help message and exit\n");
}

bool FLAG_valid_only = false, FLAG_invalid_only = false, FLAG_size = true;
std::string FLAG_avd = "";

static void parseArgs(int argc, char** argv) {
    int long_index = 0;
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) !=
           -1) {
        switch (opt) {
            case 'l':
                FLAG_valid_only = true;
                break;
            case 'i':
                FLAG_invalid_only = true;
                break;
            case 's':
                FLAG_size = false;
                break;
            case 'h':
            default:
                printUsage();
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        FLAG_avd = argv[optind];
    } else {
        printUsage();
        exit(EXIT_FAILURE);
    }
}

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

static const QAndroidGlobalVarsAgent globalVarsAgent = {
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
                    return globalVarsAgent.android_cmdLineOptions() != nullptr;
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



namespace android::emulation {
class MockAndroidConsoleFactory : public AndroidConsoleFactory {
public:
    const QAndroidGlobalVarsAgent* const android_get_QAndroidGlobalVarsAgent()
            const override {
        return &globalVarsAgent;
    };
};
}  // namespace android::emulation

int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
   android::emulation::injectConsoleAgents(
            android::emulation::MockAndroidConsoleFactory());
    // Initialize the avd
    getConsoleAgents()->settings->inject_AvdInfo(avdInfo_new(
            FLAG_avd.c_str(), getConsoleAgents()->settings->avdParams(),
            getConsoleAgents()->settings->android_cmdLineOptions()->sysdir));
    if (getConsoleAgents()->settings->avdInfo() == NULL) {
        std::cerr << "Could not find virtual device named: " << FLAG_avd
                  << std::endl;
        exit(1);
    }

    // And list the snapshots.
    SnapshotList list;
    for (auto snapshot : Snapshot::getExistingSnapshots()) {
        auto protobuf = snapshot.getGeneralInfo();

        bool keep = (FLAG_valid_only && snapshot.checkOfflineValid()) ||
                    (FLAG_invalid_only && !snapshot.checkOfflineValid()) ||
                    (!FLAG_valid_only && !FLAG_invalid_only);
        if (protobuf && keep) {
            auto details = list.add_snapshots();
            details->set_snapshot_id(snapshot.name().data());
            if (FLAG_size)
                details->set_size(snapshot.folderSize());
            // We only need to check for snapshot validity once.
            // Invariant: FLAG_valid_only -> snapshot.checkOfflineValid()
            if (FLAG_valid_only || snapshot.checkOfflineValid()) {
                details->set_status(SnapshotDetails::Compatible);
            } else {
                details->set_status(SnapshotDetails::Incompatible);
            }

            *details->mutable_details() = *protobuf;
        }
    }

    std::string asText;
    google::protobuf::TextFormat::PrintToString(list, &asText);

    std::cout << asText << std::endl;
}
