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
#include "android/base/files/MemStream.h"
#include "android/base/ProcessControl.h"
#include "android/emulation/control/AdbInterface.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/emulation/control/vm_operations.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#include "android/globals.h"
#include "android/hw-sensors.h"

#include <assert.h>

#include <atomic>
#include <memory>
#include <random>
#include <thread>
#include <vector>

// This indicates the number of heartbeats from guest
static std::atomic<int> guest_heart_beat_count {};

static std::atomic<int> restart_when_stalled {};

static std::atomic<int> num_watchdog {};
static std::atomic<int> num_hostcts_watchdog {};

extern "C" int get_host_cts_heart_beat_count(void);

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

    int current = guest_heart_beat_count.load();
    // guest does not have heartbeat, do nothing
    if (current <= 0) return;

    num_watchdog ++;
    while (1) {
        // sleep x minutes
        base::Thread::sleepMs(sleep_minutes * 60 * 1000);
        int now = guest_heart_beat_count.load();
        if (now <= current) {
            // reboot
            printf("emulator: Guest seems stalled, reboot now.\n");
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
        printf("emulator : cts heart beat %d at %s\n", now, ctime(&curtime));
        fflush(stdout);
        if (current > 0 && now <= current && restart_when_stalled) {
            // reboot
            printf("emulator: host cts seems stalled, reboot now.\n");
            fflush(stdout);
            android::base::restartEmulator();
            break;
        } else {
            current = now;
        }
    }
    num_hostcts_watchdog --;
}

static void qemuMiscPipeDecodeAndExecute(const std::vector<uint8_t>& input,
                                         std::vector<uint8_t>* outputp) {
    std::vector<uint8_t> & output = *outputp;
    if (beginWith(input, "heartbeat")) {
        fillWithOK(output);
        guest_heart_beat_count ++;
        return;
    } else if (beginWith(input, "bootcomplete")) {
        fillWithOK(output);
        printf("emulator: INFO: boot completed\n");
        printf("emulator: INFO: boot time %lld ms\n", (long long)get_uptime_ms());
        fflush(stdout);

        guest_boot_completed = 1;

        if (android_hw->test_quitAfterBootTimeOut > 0) {
            gQAndroidVmOperations->vmShutdown();
        } else {
            auto adbInterface = emulation::AdbInterface::getGlobal();
            if (!adbInterface) return;

            printf("emulator: Increasing screen off timeout, "
                    "logcat buffer size to 2M.\n");

            adbInterface->enqueueCommand(
                { "shell", "settings", "put", "system",
                  "screen_off_timeout", "2147483647" });
            adbInterface->enqueueCommand(
                { "shell", "logcat", "-G", "2M" });

            // If we allowed host audio, don't revoke
            // microphone perms.
            if (gQAndroidVmOperations->isRealAudioAllowed()) {
                printf("emulator: Not revoking microphone permissions "
                       "for Google App.\n");
                return;
            } else {
                printf("emulator: Revoking microphone permissions "
                       "for Google App.\n");
            }

            adbInterface->enqueueCommand(
                { "shell", "pm", "revoke",
                  "com.google.android.googlequicksearchbox",
                  "android.permission.RECORD_AUDIO" });

            if (restart_when_stalled > 0 && num_watchdog == 0) {
                std::thread{watchDogFunction, 1}.detach();
            }

            if (android_hw->test_monitorAdb && num_hostcts_watchdog == 0) {
                std::thread{watchHostCtsFunction, 10}.detach();
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
    android::AndroidPipe::Service::add(new android::AndroidMessagePipe::Service(
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
