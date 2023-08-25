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

#include <stdio.h>                                       // for fflush, stdout
#include <stdlib.h>                                      // for free
#include <string.h>                                      // for strcmp, strlen
#include <time.h>                                        // for ctime, time
#include <algorithm>                                     // for max
#include <atomic>                                        // for atomic, __at...
#include <cstdint>                                       // for uint8_t, int...
#include <fstream>                                       // for basic_istream
#include <memory>                                        // for make_unique
#include <random>                                        // for mt19937, ran...
#include <string>                                        // for allocator
#include <thread>                                        // for thread
#include <vector>                                        // for vector

#include "host-common/hw-config.h"                       // for AndroidHwConfig
#include "android/avd/info.h"                            // for avdInfo_getA...
#include "aemu/base/ProcessControl.h"                 // for restartEmulator

#include "aemu/base/files/MemStream.h"                   // for MemStream
#include "aemu/base/files/PathUtils.h"                   // for PathUtils
#include "aemu/base/misc/StringUtils.h"                  // for Split
#include "aemu/base/threads/Thread.h"                    // for Thread
#include "android/base/system/System.h"                  // for System
#include "android/console.h"                             // for getConsoleAg...
#include "android/emulation/AndroidMessagePipe.h"        // for AndroidMessa...
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbInterface
#include "android/emulation/control/globals_agent.h"     // for LanguageSett...
#include "android/emulation/resizable_display_config.h"  // for getResizable...
#include "android/hw-sensors.h"                          // for FoldableHing...
#include "android/metrics/MetricsReporter.h"             // for MetricsReporter
#include "android/physics/FoldableModel.h"
#include "android/user-config.h"
#include "android/utils/aconfig-file.h"  // for aconfig_find
#include "android/utils/debug.h"         // for dinfo
#include "android/utils/path.h"          // for path_exists
#include "android/utils/system.h"        // for get_uptime_ms
#include "host-common/AndroidPipe.h"     // for AndroidPipe:...
#include "host-common/FeatureControl.h"  // for isEnabled
#include "host-common/Features.h"        // for DeviceSkinOv...
#include "host-common/vm_operations.h"   // for QAndroidVmOp...
#include "host-common/window_agent.h"    // for QAndroidEmul...
#include "studio_stats.pb.h"             // for EmulatorBoot...

// This indicates the number of heartbeats from guest
static std::atomic<int> guest_heart_beat_count {};

static std::atomic<int> restart_when_stalled {};

static std::atomic<int> num_watchdog {};
static std::atomic<int> num_hostcts_watchdog {};

static int64_t s_reset_request_uptime_ms;

extern "C" int get_host_cts_heart_beat_count(void);
extern "C" bool android_op_wipe_data;

using android::base::PathUtils;
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

    avdInfo_getSkinInfo(getConsoleAgents()->settings->avdInfo(), &skinName, &skinDir);
    std::string layoutPath = android::base::PathUtils::join(
            skinDir ? skinDir : "", skinName ? skinName : "", "layout");
    AFREE(skinName);
    AFREE(skinDir);

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
        avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) >= 28) {
        char* skinName = nullptr;
        char* skinDir = nullptr;
        avdInfo_getSkinInfo(getConsoleAgents()->settings->avdInfo(), &skinName, &skinDir);
        auto myhw = getConsoleAgents()->settings->hw();
        const char* const overlayName = avdInfo_skinHasOverlay(skinName)
                                                ? skinName
                                                : myhw->hw_device_name;
        if (avdInfo_skinHasOverlay(overlayName)) {
            std::string androidOverlay("com.android.internal.emulation.");
            std::string systemUIOverlay("com.android.systemui.emulation.");
            androidOverlay += overlayName;
            systemUIOverlay += overlayName;
            adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                           "enable-exclusive", "--category",
                                           androidOverlay.c_str() });
            adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                           "enable-exclusive", "--category",
                                           systemUIOverlay.c_str() });
            dprint("applying overlayName %s\n", overlayName);
            AFREE(skinName);
            AFREE(skinDir);
            return;
        }
        AFREE(skinName);
        AFREE(skinDir);
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
        std::ifstream inFile(PathUtils::asUnicodePath(filepath.data()).c_str(), std::ios_base::in);
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
        // bug: 152636877
        auto bootTimeInMs = (long long)(get_uptime_ms() - s_reset_request_uptime_ms);
        dinfo("Boot completed in %lld ms",  bootTimeInMs);
        android::metrics::MetricsReporter::get().report(
                [=](android_studio::AndroidStudioEvent* event) {
                    auto boot_info = event->mutable_emulator_details()
                                             ->mutable_boot_info();
                    boot_info->set_boot_status(
                            android_studio::EmulatorBootInfo::BOOT_COMPLETED);
                    boot_info->set_duration_ms(bootTimeInMs);
                });
        fflush(stdout);

        if (getConsoleAgents()->settings->avdInfo()) {
            const char* avd_dir = avdInfo_getContentPath(
                    getConsoleAgents()->settings->avdInfo());
            if (avd_dir) {
                const auto bootcomplete_ini =
                        std::string{android::base::PathUtils::join(
                                avd_dir, "bootcompleted.ini")};
                path_empty_file(bootcomplete_ini.c_str());
            }
        }
        getConsoleAgents()->settings->set_guest_boot_completed(true);
        if (getConsoleAgents()->settings->hw()->test_quitAfterBootTimeOut > 0) {
            getConsoleAgents()->vm->vmShutdown();
        } else {
            auto adbInterface = emulation::AdbInterface::getGlobal();
            if (!adbInterface) return;

            dinfo("Increasing screen off timeout, "
                    "logcat buffer size to 2M.");

            const char* pTimeout = avdInfo_screen_off_timeout(avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()));
            adbInterface->enqueueCommand(
                { "shell", "settings", "put", "system",
                  "screen_off_timeout", pTimeout});
            adbInterface->enqueueCommand(
                { "shell", "logcat", "-G", "2M" });

            if (resizableEnabled()) {
                // adb interface may not be ready when initializing resizable configs,
                // resend resizable large screen settings to window manager.
                updateAndroidDisplayConfigPath(getResizableActiveConfigId());
            } else {
                // If hw.display.settings.xml set as freeform, config guest
                if ((android_op_wipe_data || !path_exists(getConsoleAgents()->settings->hw()->disk_dataPartition_path))) {
                    if (!strcmp(getConsoleAgents()->settings->hw()->display_settings_xml, "freeform")) {
                        adbInterface->enqueueCommand(
                            { "shell", "settings", "put", "global", "sf", "1" });
                        adbInterface->enqueueCommand(
                            { "shell", "settings", "put", "global",
                              "enable_freeform_support", "1" });
                        adbInterface->enqueueCommand(
                            { "shell", "settings", "put", "global",
                              "force_resizable_activities", "1" });
                        restartFrameWork = true;
                    }
                }
            }

            miscPipeSetAndroidOverlay(adbInterface);

            if (restart_when_stalled > 0 && num_watchdog == 0) {
                std::thread{watchDogFunction, 1}.detach();
            }

            if (getConsoleAgents()->settings->hw()->test_monitorAdb && num_hostcts_watchdog == 0) {
                std::thread{watchHostCtsFunction, 30}.detach();
            }

            if (getConsoleAgents()->settings->avdInfo()) {
#ifdef __linux__
                char* datadir = avdInfo_getDataInitDirPath(getConsoleAgents()->settings->avdInfo());
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
                         ".MultiDisplayServiceReceiver",
                         "--user 0"});
            }

            // Foldable hinge area and posture update. Called when cold boot or
            // Android reboot itself note: this does not apply to pixel_fold
            const bool not_pixel_fold = !android_foldable_is_pixel_fold();
            if (android_foldable_hinge_configured() && not_pixel_fold) {
                int xOffset = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_xOffset;
                int yOffset = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_yOffset;
                int width = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_width;
                int height = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_height;
                char foldedArea[64];
                sprintf(foldedArea, "%d,%d,%d,%d", xOffset, yOffset,
                        xOffset + width, yOffset + height);
                adbInterface->enqueueCommand({ "shell", "wm", "folded-area",
                                foldedArea});

                FoldableState state;
                if (!android_foldable_get_state(&state)) {
                    android::physics::FoldableModel::sendPostureToSystem(state.currentPosture);
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
                auto userConfig = getConsoleAgents()->settings->userConfig();
                int posture = auserConfig_getPosture(userConfig);
                if (posture > POSTURE_UNKNOWN && posture < POSTURE_MAX) {
                    adbInterface->enqueueCommand(
                            {"emu", "posture", std::to_string(posture)});
                }
            } else {
                auto isTablet = [&]() -> bool {
                    if (resizableEnabled()) {
                        return false;
                    }
                    auto myhw = getConsoleAgents()->settings->hw();
                    if (!myhw) {
                        return false;
                    }
                    auto isTabletDeviceName =
                            [&](std::string_view view) -> bool {
                        if (view == std::string_view("Nexus 10"))
                            return true;
                        if (std::string::npos != view.find("Tablet"))
                            return true;
                        if (std::string::npos != view.find("tablet"))
                            return true;
                        return false;
                    };
                    if (myhw->hw_initialOrientation &&
                        std::string_view(myhw->hw_initialOrientation) ==
                                std::string_view("landscape") &&
                        myhw->hw_device_name &&
                        isTabletDeviceName(myhw->hw_device_name)) {
                        if (myhw->hw_lcd_width > myhw->hw_lcd_height) {
                            dprint("Width %d > Height %d, assume this is a "
                                   "tablet",
                                   static_cast<int>(myhw->hw_lcd_width),
                                   static_cast<int>(myhw->hw_lcd_height));
                            return true;
                        }
                    }
                    return false;
                };
                if (isTablet()) {
                    dprint("Ignore orientation request for tablet");
                    adbInterface->enqueueCommand(
                            {"shell", "cmd", "window",
                             "set-ignore-orientation-request", "true"});
                }
            }
            if (getConsoleAgents()->settings->language()->changing_language_country_locale) {
                dinfo("Changing language, country or locale...");
                auto language = getConsoleAgents()->settings->language();
                if (language->language) {
                    dinfo("Changing language to %s", language->language);
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.language", language->language });
                }

                if (language->country) {
                    dinfo("Changing country to %s", language->country);
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.country", language->country });
                }

                if (language->locale) {
                    dinfo("Changing locale to %s", language->locale);
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.locale", language->locale });
                }

                // On changing language or country, we need to set locale as well,
                // otherwise the 2nd time and later after it's done once,
                // the option has no effect!
                if (!language->locale &&
                    !language->country &&
                    language->language) {
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.locale", language->language });
                }

                if (!language->locale &&
                    language->country &&
                    language->language) {
                    std::string languageCountry(language->language);
                    languageCountry += "-";
                    languageCountry += language->country;
                    adbInterface->enqueueCommand(
                        { "shell", "su", "0",
                          "setprop", "persist.sys.locale", languageCountry.c_str() });
                }
                restartFrameWork = true;
                getConsoleAgents()->settings->inject_language(nullptr, nullptr, nullptr);
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

extern "C" int64_t get_uptime_since_reset(void) {
  return s_reset_request_uptime_ms;
}
