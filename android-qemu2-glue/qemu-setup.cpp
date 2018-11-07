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

#include "android-qemu2-glue/qemu-setup.h"

#include "android/android.h"
#include "android/base/Log.h"
#include "android/base/files/MemStream.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/console.h"
#include "android/cmdline-option.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/crash-handler.h"
#include "android/snapshot/interface.h"

#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/skin/LibuiAgent.h"
#include "android/skin/winsys.h"
#include "android-qemu2-glue/emulation/android_pipe_device.h"
#include "android-qemu2-glue/emulation/charpipe.h"
#include "android-qemu2-glue/emulation/DmaMap.h"
#include "android-qemu2-glue/emulation/goldfish_sync.h"
#include "android-qemu2-glue/emulation/VmLock.h"
#include "android-qemu2-glue/base/async/CpuLooper.h"
#include "android-qemu2-glue/base/async/Looper.h"
#include "android-qemu2-glue/looper-qemu.h"
#include "android-qemu2-glue/android_qemud.h"
#include "android-qemu2-glue/audio-capturer.h"
#include "android-qemu2-glue/audio-output.h"
#include "android-qemu2-glue/net-android.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android-qemu2-glue/qemu-control-impl.h"
#include "android-qemu2-glue/snapshot_compression.h"

#include <random>

extern "C" {

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/abort.h"
#include "qemu/main-loop.h"
#include "qemu/thread.h"
#include "sysemu/device_tree.h"
#include "sysemu/ranchu.h"
#include "sysemu/rng-random-generic.h"

// TODO: Remove op_http_proxy global variable.
extern char* op_http_proxy;

}  // extern "C"

using android::VmLock;
using android::DmaMap;

extern "C" void ranchu_device_tree_setup(void *fdt) {
    /* fstab */
    qemu_fdt_add_subnode(fdt, "/firmware/android/fstab");
    qemu_fdt_setprop_string(fdt, "/firmware/android/fstab", "compatible", "android,fstab");

    char* system_path = avdInfo_getSystemImageDevicePathInGuest(android_avdInfo);
    if (system_path) {
        qemu_fdt_add_subnode(fdt, "/firmware/android/fstab/system");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "compatible", "android,system");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "dev", system_path);
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "fsmgr_flags", "wait");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "mnt_flags", "ro");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "type", "ext4");
        free(system_path);
    }

    char* vendor_path = avdInfo_getVendorImageDevicePathInGuest(android_avdInfo);
    if (vendor_path) {
        qemu_fdt_add_subnode(fdt, "/firmware/android/fstab/vendor");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "compatible", "android,vendor");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "dev", vendor_path);
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "fsmgr_flags", "wait");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "mnt_flags", "ro");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "type", "ext4");
        free(vendor_path);
    }
}

extern "C" void rng_random_generic_read_random_bytes(void *buf, int size) {
    if (size <= 0) return;

    android::base::MemStream stream;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(0,255); //fill a byte

    for (int i=0; i < size; ++i) {
        stream.putByte(uniform_dist(gen));
    }
    stream.read(buf, size);
}

bool qemu_android_emulation_early_setup() {
    // Ensure that the looper is set for the main thread and for any
    // future thread created by QEMU.
    qemu_looper_setForThread();
    qemu_thread_register_setup_callback(qemu_looper_setForThread);

    qemu_abort_set_handler((QemuAbortHandler)&crashhandler_die_format_v);
    qemu_crash_dump_message_func_set(
        (QemuCrashDumpMessageFunc)&crashhandler_append_message_format_v);

    // Make sure we override the ctrl-C handler as soon as possible.
    qemu_set_ctrlc_handler(&skin_winsys_quit_request);

    // Ensure charpipes i/o are handled properly.
    main_loop_register_poll_callback(qemu_charpipe_poll);

    // Register qemud-related snapshot callbacks.
    android_qemu2_qemud_init();

    // Ensure the VmLock implementation is setup.
    VmLock* vmLock = new qemu2::VmLock();
    VmLock* prevVmLock = VmLock::set(vmLock);
    CHECK(prevVmLock == nullptr) << "Another VmLock was already installed!";

    // Ensure the DmaMap implementation is setup.
    DmaMap* dmaMap = new qemu2::DmaMap();
    DmaMap* prevDmaMap = DmaMap::set(dmaMap);
    CHECK(prevDmaMap == nullptr) << "Another DmaMap was already installed!";

    // Initialize host pipe service.
    if (!qemu_android_pipe_init(vmLock)) {
        return false;
    }

    // Initialize host sync service.
    if (!qemu_android_sync_init(vmLock)) {
        return false;
    }

    androidSnapshot_initialize(gQAndroidVmOperations,
                               gQAndroidEmulatorWindowAgent);
    qemu_snapshot_compression_setup();

    android::emulation::AudioCaptureEngine::set(
                new android::qemu::QemuAudioCaptureEngine());

    android::emulation::AudioOutputEngine::set(
                new android::qemu::QemuAudioOutputEngine());

    return true;
}

static const AndroidConsoleAgents* getConsoleAgents() {
    // Initialize UI/console agents.
    static const AndroidConsoleAgents consoleAgents = {
            gQAndroidBatteryAgent,        gQAndroidDisplayAgent,
            gQAndroidEmulatorWindowAgent, gQAndroidFingerAgent,
            gQAndroidLocationAgent,       gQAndroidHttpProxyAgent,
            gQAndroidRecordScreenAgent,   gQAndroidTelephonyAgent,
            gQAndroidUserEventAgent,      gQAndroidVmOperations,
            gQAndroidNetAgent,            gQAndroidLibuiAgent,
            gQCarDataAgent,
    };
    return &consoleAgents;
}

bool qemu_android_ports_setup() {
    return android_ports_setup(getConsoleAgents(), true);
}

bool qemu_android_emulation_setup() {
    android_qemu_init_slirp_shapers();

    if (!qemu_android_setup_http_proxy(op_http_proxy)) {
        return false;
    }

    if (!android_emulation_setup(getConsoleAgents(), true)) {
        return false;
    }

    android::base::ScopedCPtr<const char> arch(
                avdInfo_getTargetCpuArch(android_avdInfo));
    const bool isX86 =
            arch && (strstr(arch.get(), "x86") || strstr(arch.get(), "i386"));
    const int nCores = isX86 ? android_hw->hw_cpu_ncore : 1;
    for (int i = 0; i < nCores; ++i) {
        android::crashreport::CrashReporter::get()->hangDetector().
                addWatchedLooper(new android::qemu::CpuLooper(i));
    }

    if (android_cmdLineOptions && android_cmdLineOptions->detect_image_hang) {
        dwarning("Enabled event overflow detection.\n");
        const int kMaxEventsDropped = 1024;
        android::crashreport::CrashReporter::get()
                ->hangDetector()
                .addPredicateCheck(
                        [] {
                            return gQAndroidUserEventAgent->eventsDropped() >
                                    kMaxEventsDropped;
                        },
                        "The goldfish event queue is overflowing, the system is no longer responding.");
    }

    return true;
}

void qemu_android_emulation_teardown() {
    android::qemu::skipTimerOps();
    androidSnapshot_finalize();
    android_emulation_teardown();
}
