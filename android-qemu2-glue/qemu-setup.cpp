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
#include "android/base/memory/ScopedPtr.h"
#include "android/console.h"
#include "android/crashreport/CrashReporter.h"
#include "android/globals.h"
#include "android/skin/LibuiAgent.h"
#include "android/skin/winsys.h"
#include "android-qemu2-glue/emulation/android_pipe_device.h"
#include "android-qemu2-glue/emulation/charpipe.h"
#include "android-qemu2-glue/emulation/DmaMap.h"
#include "android-qemu2-glue/emulation/goldfish_sync.h"
#include "android-qemu2-glue/emulation/VmLock.h"
#include "android-qemu2-glue/base/async/CpuLooper.h"
#include "android-qemu2-glue/looper-qemu.h"
#include "android-qemu2-glue/android_qemud.h"
#include "android-qemu2-glue/audio-capturer.h"
#include "android-qemu2-glue/net-android.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android-qemu2-glue/qemu-control-impl.h"
#include "android-qemu2-glue/snapshot_compression.h"

extern "C" {

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/main-loop.h"
#include "qemu/thread.h"

// TODO: Remove op_http_proxy global variable.
extern char* op_http_proxy;

}  // extern "C"

using android::VmLock;
using android::DmaMap;

bool qemu_android_emulation_early_setup() {
    // Ensure that the looper is set for the main thread and for any
    // future thread created by QEMU.
    qemu_looper_setForThread();
    qemu_thread_register_setup_callback(qemu_looper_setForThread);

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

    qemu_snapshot_compression_setup();

    android::emulation::AudioCaptureEngine::set(new android::qemu::QemuAudioCaptureEngine());

    return true;
}

bool qemu_android_emulation_setup() {
    android_qemu_init_slirp_shapers();

    // Initialize UI/console agents.
    static const AndroidConsoleAgents consoleAgents = {
            gQAndroidBatteryAgent,   gQAndroidEmulatorWindowAgent,
            gQAndroidFingerAgent,    gQAndroidLocationAgent,
            gQAndroidHttpProxyAgent,
            gQAndroidTelephonyAgent, gQAndroidUserEventAgent,
            gQAndroidVmOperations,   gQAndroidNetAgent,
            gQAndroidLibuiAgent,     gQCarDataAgent,
    };

    if (!qemu_android_setup_http_proxy(op_http_proxy)) {
        return false;
    }

    if (!android_emulation_setup(&consoleAgents, true)) {
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

    return true;
}

void qemu_android_emulation_teardown() {}
