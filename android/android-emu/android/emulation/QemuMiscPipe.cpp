// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/QemuMiscPipe.h"

#include <stdio.h>                                       // for printf, fflush
#include <string.h>                                      // for strcmp, strlen
#include <time.h>                                        // for ctime, time
#include <algorithm>                                     // for uniform_int_...
#include <atomic>                                        // for atomic, __at...
#include <cstdint>                                       // for uint8_t, int...
#include <fstream>                                       // for file read
#include <functional>                                    // for __base
#include <random>                                        // for mt19937, ran...
#include <string>                                        // for string, to_s...
#include <thread>                                        // for thread
#include <vector>                                        // for vector

#include "android/avd/info.h"                            // for avdInfo_getS...
#include "android/base/ProcessControl.h"                 // for restartEmulator
#include "android/base/files/MemStream.h"                // for MemStream
#include "android/base/files/PathUtils.h"                // for PathUtils
#include "android/base/misc/StringUtils.h"               // for split strings
#include "android/base/threads/Thread.h"                 // for Thread
#include "android/console.h"                             // for getConsoleAg...
#include "android/emulation/AndroidMessagePipe.h"        // for AndroidMessa...
#include "android/emulation/AndroidPipe.h"               // for AndroidPipe
#include "android/emulation/resizable_display_config.h"
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbInterface
#include "android/emulation/control/vm_operations.h"     // for QAndroidVmOp...
#include "android/featurecontrol/FeatureControl.h"       // for isEnabled
#include "android/featurecontrol/Features.h"             // for MultiDisplay
#include "android/globals.h"                             // for to_set_language
#include "android/hw-sensors.h"                          // for FoldableHing...
#include "android/utils/aconfig-file.h"                  // for aconfig_find
#include "android/utils/path.h"                          // for path_exists
#include "android/utils/system.h"                        // for get_uptime_ms

// This indicates the number of heartbeats from guest
static std::atomic<int> guest_heart_beat_count {};

static std::atomic<int> restart_when_stalled {};

static std::atomic<int> num_watchdog {};
static std::atomic<int> num_hostcts_watchdog {};

static int64_t s_reset_request_uptime_ms;

extern "C" int get_host_cts_heart_beat_count(void);
extern "C" bool android_op_wipe_data;

namespace fc = android::featurecontrol;

namespace android {
static bool beginWith(const std::vector<uint8_t>& input, const char* keyword) {
    if (input.empty()) {
        return false;
    }
    int size = strlen(keyword);
    const char *mesg = (const char*)(&input[0]);
    return (input.size() >= size && strncmp(mesg, keyword, size) == 0);
}

static void getRandomBytes(int bytes, std::vector<uint8_t>* outputp) {
    android::base::MemStream stream;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(0,255); //fill a byte

    for (int i=0; i < bytes; ++i) {
        stream.putByte(uniform_dist(gen));
    }

    std::vector<uint8_t> & output = *outputp;
    output.resize(bytes);
    stream.read(&(output[0]), bytes);
}

static void fillWithOK(std::vector<uint8_t> &output) {
    output.resize(3);
    output[0]='O';
    output[1]='K';
    output[2]='\0';
}

static void watchDogFunction(int sleep_minutes) {
    if (sleep_minutes <= 0) return;

    int waitCount = 3;
    int current = guest_heart_beat_count.load();
    while (current <= 0) {
        base::Thread::sleepMs(sleep_minutes * 60 * 1000);
        current = guest_heart_beat_count.load();
        --waitCount;
        if (waitCount <= 0) {
            break;
        }
    }

    num_watchdog ++;
    while (1) {
        // sleep x minutes
        base::Thread::sleepMs(sleep_minutes * 60 * 1000);
        int now = guest_heart_beat_count.load();
        if (now <= current) {
            // reboot
            dinfo("Guest seems stalled, reboot now.");
            fflush(stdout);
            android::base::restartEmulator();
            break;
        } else {
            current = now;
        }
    }
    num_watchdog --;
}

static void watchHostCtsFunction(int sleep_minutes) {
    if (sleep_minutes <= 0) return;

    int current = get_host_cts_heart_beat_count();

    num_hostcts_watchdog ++;
    while (1) {
        // sleep x minutes
        base::Thread::sleepMs(sleep_minutes * 60 * 1000);
        int now = get_host_cts_heart_beat_count();
        if (now <= 0) continue;

        time_t curtime;
        time(&curtime);
        dinfo("cts heart beat %d at %s", now, ctime(&curtime));
        fflush(stdout);
        if (current > 0 && now <= current && restart_when_stalled) {
            // reboot
            dinfo("host cts seems stalled, reboot now.");
            fflush(stdout);
            android::base::restartEmulator();
            break;
        } else {
            current = now;
        }
    }
    num_hostcts_watchdog --;
}

static AConfig* miscPipeGetForegroundConfig() {
    char* skinName;
    char* skinDir;

    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    std::string layoutPath = android::base::PathUtils::join(skinDir, skinName, "layout");
    AConfig* rootConfig = aconfig_node("", "");
    aconfig_load_file(rootConfig, layoutPath.c_str());

    // Look for parts/portrait/foreground
    AConfig* nextConfig = aconfig_find(rootConfig, "parts");
    if (nextConfig == nullptr) {
        return nullptr;
    }
    nextConfig = aconfig_find(nextConfig, "portrait");
    if (nextConfig == nullptr) {
        return nullptr;
    }

    return aconfig_find(nextConfig, "foreground");
}

static void miscPipeSetPaddingAndCutout(emulation::AdbInterface* adbInterface, AConfig* config) {
    // Padding
    // If the AVD display has rounded corners, the items on the
    // top line of the device display need to be moved in, away
    // from the sides.
    //
    // Pading for Oreo:
    // Get the amount that the icons should be moved, and send
    // that number to the device. The layout has this number as
    // parts/portrait/foreground/padding
    const char* roundedContentPadding = aconfig_str(config, "padding", 0);
    if (roundedContentPadding && roundedContentPadding[0] != '\0') {
        adbInterface->enqueueCommand({ "shell", "settings", "put", "secure",
                                       "sysui_rounded_content_padding",
                                       roundedContentPadding });
    }

    // Padding for P and later:
    // The padding value is set by selecting a resource overlay. The
    // layout has the overlay name as parts/portrait/foreground/corner.
    const char* cornerKeyword = aconfig_str(config, "corner", 0);
    if (cornerKeyword && cornerKeyword[0] != '\0') {
        std::string cornerOverlayType("com.android.internal.display.corner.emulation.");
        cornerOverlayType += cornerKeyword;

        adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                       "enable-exclusive", "--category",
                                       cornerOverlayType.c_str() });
    }

    // Cutout
    // If the AVD display has a cutout, send a command to the device
    // to have it adjust the screen layout appropriately. The layout
    // has the emulated cutout name as parts/portrait/foreground/cutout.
    const char* cutoutKeyword = aconfig_str(config, "cutout", 0);
    if (cutoutKeyword && cutoutKeyword[0] != '\0') {
        std::string cutoutType("com.android.internal.display.cutout.emulation.");
        cutoutType += cutoutKeyword;
        adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                       "enable-exclusive", "--category",
                                       cutoutType.c_str() });
    }
}

void miscPipeSetAndroidOverlay(emulation::AdbInterface* adbInterface) {
    // overlay for pixels with system after O
    if (fc::isEnabled(fc::DeviceSkinOverlay) &&
        avdInfo_getApiLevel(android_avdInfo) >= 28) {
        char* skinName = nullptr;
        char* skinDir = nullptr;
        avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
        if (avdInfo_skinHasOverlay(skinName)) {
            std::string androidOverlay("com.android.internal.emulation.");
            std::string systemUIOverlay("com.android.systemui.emulation.");
            androidOverlay += skinName;
            systemUIOverlay += skinName;
            adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                           "enable-exclusive", "--category",
                                           androidOverlay.c_str() });
            adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                           "enable-exclusive", "--category",
                                           systemUIOverlay.c_str() });
            return;
        }
    }
    AConfig* foregroundConfig = miscPipeGetForegroundConfig();
    if (foregroundConfig != nullptr) {
        miscPipeSetPaddingAndCutout(adbInterface, foregroundConfig);
    }
}

static void runAdbScripts(emulation::AdbInterface* adbInterface,
                          std::string dataDir,
                          std::string adbScriptsDir) {
    // the scripts contains multiple lines of adb command
    // fileds in each line are tab separated, they are feed to adb command
    // one bye one; right now, only push is implemented, and push's first
    // argument will have dataDir prefixed to it. e.g., push
    // media/test/file.mp4 /sdcard/test/media/file.mp4

    auto list = base::System::get()->scanDirEntries(adbScriptsDir, true);
    for (auto filepath : list) {
        std::ifstream inFile(filepath, std::ios_base::in);
        if (!inFile.is_open()) {
            continue;
        }
        std::string line;
        while (std::getline(inFile, line)) {
            std::vector<std::string> commands = base::Split(line, "\t");
            if (commands.size() != 3) {
                continue;
            }
            if (commands[0] != "push")
                continue;
            adbInterface->enqueueCommand(
                    {"push", dataDir + "/" + commands[1], commands[2]});
        }
    }
}

static void qemuMiscPipeDecodeAndExecute(const std::vector<uint8_t>& input,
                                         std::vector<uint8_t>* outputp) {
    std::vector<uint8_t> & output = *outputp;
    bool restartFrameWork = false;

    if (beginWith(input, "heartbeat")) {
        fillWithOK(output);
        guest_heart_beat_count ++;
        return;
    } else if (beginWith(input, "bootcomplete")) {
        fillWithOK(output);
        dinfo("boot completed");
        // bug: 152636877
#ifndef _WIN32
        dinfo("boot time %lld ms",
            (long long)(get_uptime_ms() - s_reset_request_uptime_ms));
#endif
        fflush(stdout);

        guest_boot_completed = 1;

        if (android_hw->test_quitAfterBootTimeOut > 0) {
            getConsoleAgents()->vm->vmShutdown();
        } else {
            auto adbInterface = emulation::AdbInterface::getGlobal();
            if (!adbInterface) return;

            dinfo("Increasing screen off timeout, "
                    "logcat buffer size to 2M.");

            adbInterface->enqueueCommand(
                { "shell", "settings", "put", "system",
                  "screen_off_timeout", "2147483647" });
            adbInterface->enqueueCommand(
                { "shell", "logcat", "-G", "2M" });

            if (resizableEnabled()) {
                // adb interface may not be ready when initializing resizable configs,
                // resend resizable large screen settings to window manager.
                updateAndroidDisplayConfigPath(getResizableActiveConfigId());
            } else {
                // If hw.display.settings.xml set as freeform, config guest
                if ((android_op_wipe_data || !path_exists(android_hw->disk_dataPartition_path))) {
                    if (!strcmp(android_hw->display_settings_xml, "freeform")) {
                        adbInterface->enqueueCommand(
                            { "shell", "settings", "put", "global", "sf", "1" });
                        adbInterface->enqueueCommand(
                            { "shell", "settings", "put", "global",
                              "enable_freeform_support", "1" });
                        adbInterface->enqueueCommand(
                            { "shell", "settings", "put", "global",
                              "force_resizable_activities", "1" });
                    }
                }
            }

            // If we allowed host audio, don't revoke
            // microphone perms.
            if (getConsoleAgents()->vm->isRealAudioAllowed()) {
                dinfo("Not revoking microphone permissions "
                       "for Google App.");
                return;
            } else {
                dinfo("Revoking microphone permissions "
                       "for Google App.");
            }

            adbInterface->enqueueCommand(
                { "shell", "pm", "revoke",
                  "com.google.android.googlequicksearchbox",
                  "android.permission.RECORD_AUDIO" });

            miscPipeSetAndroidOverlay(adbInterface);

            if (restart_when_stalled > 0 && num_watchdog == 0) {
                std::thread{watchDogFunction, 1}.detach();
            }

            if (android_hw->test_monitorAdb && num_hostcts_watchdog == 0) {
                std::thread{watchHostCtsFunction, 30}.detach();
            }

            if (android_avdInfo) {
#ifdef __linux__
                char* datadir = avdInfo_getDataInitDirPath(android_avdInfo);
                if (datadir) {
                    std::string adbscriptsdir = android::base::PathUtils::join(
                            datadir, "adbscripts");
                    if (path_exists(adbscriptsdir.c_str())) {
                        // check adb path
                        if (adbInterface->adbPath().empty()) {
                            // try "/opt/bin/adb"
                            constexpr char GCE_PRESUBMIT_ADB_PATH[] =
                                    "/opt/bin/adb";
                            if (path_exists(GCE_PRESUBMIT_ADB_PATH)) {
                                adbInterface->setCustomAdbPath(
                                        GCE_PRESUBMIT_ADB_PATH);
                            }
                        }
                        if (!adbInterface->adbPath().empty()) {
                            std::thread{runAdbScripts, adbInterface, datadir,
                                        adbscriptsdir}
                                    .detach();
                        }
                    }
                    free(datadir);
                }
#endif
            }

            // start multi-display service after boot completion
            if (fc::isEnabled(fc::MultiDisplay) &&
                !fc::isEnabled(fc::Minigbm)) {
                adbInterface->enqueueCommand(
                        {"shell", "am", "broadcast", "-a",
                         "com.android.emulator.multidisplay.START", "-n",
                         "com.android.emulator.multidisplay/"
                         ".MultiDisplayServiceReceiver"});
            }

            // Foldable hinge area and posture update. Called when cold boot or Android reboot itself
            if (android_foldable_hinge_configured()) {
                FoldableState state;
                if (!android_foldable_get_state(&state)) {
                    if (!featurecontrol::isEnabled(android::featurecontrol::DeviceStateOnBoot)) {
                        adbInterface->enqueueCommand({ "shell", "settings", "put",
                                                       "global", "device_posture",
                                                       std::to_string((int)state.currentPosture).c_str() });
                    }
                    // Android accepts only one hinge area currently
                    char hingeArea[128];
                    snprintf(hingeArea, 128, "%s-[%d,%d,%d,%d]",
                             (state.config.hingesSubType == ANDROID_FOLDABLE_HINGE_FOLD) ? "fold" : "hinge",
                             state.config.hingeParams[0].x,
                             state.config.hingeParams[0].y,
                             state.config.hingeParams[0].x + state.config.hingeParams[0].width,
                             state.config.hingeParams[0].y + state.config.hingeParams[0].height);
                    adbInterface->enqueueCommand({ "shell", "settings", "put",
                                                   "global", "display_features",
                                                   hingeArea });
                }
            }
            // Sync folded area and LID open/close for foldable hinge/rollable/legacyFold
            if (android_foldable_hinge_configured() ||
                android_foldable_rollable_configured() ||
                android_foldable_folded_area_configured(0)) {
                getConsoleAgents()->emu->updateFoldablePostureIndicator(true);
            }

            if (changing_language_country_locale) {
                dinfo("Changing language, country or locale...");
                if (to_set_language) {
                    dinfo("Changing language to %s", to_set_language);
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.language", to_set_language });
                }

                if (to_set_country) {
                    dinfo("Changing country to %s", to_set_country);
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.country", to_set_country });
                }

                if (to_set_locale) {
                    dinfo("Changing locale to %s", to_set_locale);
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.locale", to_set_locale });
                }

                // On changing language or country, we need to set locale as well,
                // otherwise the 2nd time and later after it's done once,
                // the option has no effect!
                if (!to_set_locale &&
                    !to_set_country &&
                    to_set_language) {
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.locale", to_set_language });
                }

                if (!to_set_locale &&
                    to_set_country &&
                    to_set_language) {
                    std::string languageCountry(to_set_language);
                    languageCountry += "-";
                    languageCountry += to_set_country;
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.locale", languageCountry.c_str() });
                }
                restartFrameWork = true;
                changing_language_country_locale = 0;
            }

            if (restartFrameWork) {
                dinfo("Restarting framework.");
                adbInterface->enqueueCommand(
                    { "shell", "su", "0",
                      "setprop", "ctl.restart", "zygote" });
            }
        }

        return;
    } else if (beginWith(input, "get_random=")) {
        // need to parse the value after =
        int bytes=0;
        const char *mesg = (const char*)(&input[0]);
        sscanf(mesg, "get_random=%d", &bytes);
        if (bytes > 0) {
            getRandomBytes(bytes, outputp);
            return;
        }
    }

    //not implemented Kock out
    output.resize(3);
    output[0]='K';
    output[1]='O';
    output[2]='\0';
}

void registerQemuMiscPipeServicePipe() {
    AndroidPipe::Service::add(std::make_unique<AndroidMessagePipe::Service>(
            "QemuMiscPipe", qemuMiscPipeDecodeAndExecute));
}
}  // namespace android

extern "C" void android_init_qemu_misc_pipe(void) {
    android::registerQemuMiscPipeServicePipe();
}

extern "C" int get_guest_heart_beat_count(void) {
    return guest_heart_beat_count.load();
}

extern "C" void set_restart_when_stalled() {
    restart_when_stalled = 1;
}

extern "C" int is_restart_when_stalled(void) {
    return restart_when_stalled.load();
}

extern "C" void signal_system_reset_was_requested(void) {
  s_reset_request_uptime_ms = get_uptime_ms();
}
