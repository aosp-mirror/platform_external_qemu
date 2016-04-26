// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/main-kernel-parameters.h"

#include "android/base/StringFormat.h"
#include "android/emulation/ParameterList.h"
#include "android/utils/debug.h"
#include "android/utils/dns.h"

#include <algorithm>
#include <memory>

#include <inttypes.h>
#include <string.h>

using android::base::StringFormat;

char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   const char* kernelSerialPrefix,
                                   const char* avdKernelParameters,
                                   AndroidGlesEmulationMode glesMode,
                                   uint64_t glesGuestCmaMB,
                                   bool isQemu2) {
    android::ParameterList params;
    bool isX86 = !strcmp(targetArch, "x86");

    // We always force qemu=1 when running inside QEMU.
    params.add("qemu=1");

    params.addFormat("androidboot.hardware=%s",
                     isQemu2 ? "ranchu" : "goldfish");

    if (isX86) {
        params.add("clocksource=pit");
    }

    int lastSerial = 0;

    if (!isQemu2) {
        // The first virtual tty is reserved for receiving kernel messages.
        // In QEMU1, this is used to send the output to the android-kmsg
        // special chardev backend which will simply print the output to
        // stdout, but could be modified to store it in memory (e.g. to
        // display later in a special pane of the UI window).
        // TODO(digit): Either get rid of android-kmsg completely or
        // implement kernel log message storage + UI.
        params.addFormat("console=%s%d", kernelSerialPrefix, lastSerial++);

        // For QEMU1, the second virtual tty is reserved for the legacy QEMUD
        // channel. TODO(digit): System images with API level >= 14 use an
        // Android pipe instead for qemud-based communication, so don't
        // reserve the tty for them, freeing one slot to make -logcat and
        // -shell work on x86 with QEMU1.
        params.addFormat("android.qemud=%s%d", kernelSerialPrefix,
                         lastSerial++);

        if (opts->shell || opts->logcat) {
            // We need a new virtual tty to receive the logcat output and/or
            // the root console. Note however that on QEMU1/x86, the virtual
            // machine has a limited number of IRQs that doesn't allow more
            // than 2 ttys to be used at the same time.
            if (!isX86) {
                params.addFormat("androidboot.console=%s%d", kernelSerialPrefix,
                                 lastSerial++);
            } else {
                dwarning(
                        "Sorry, but -logcat and -shell options are not "
                        "implemented for x86 AVDs running in the classic "
                        "emulator");
            }
        }
    } else {  // isQemu2
        // A single virtual tty is used for -show-kernel, -shell and -logcat
        // and the -shell-serial option will affect all three of them.
        // TODO(digit): Ensure -shell-serial only works on -shell output.
        if (opts->show_kernel) {
            params.addFormat("console=%s%d,38400", kernelSerialPrefix,
                             lastSerial);
        }

        if (!strcmp(targetArch, "arm") || !strcmp(targetArch, "arm64")) {
            params.add("keep_bootcon");
            params.addFormat("earlyprintk=%s%d", kernelSerialPrefix,
                             lastSerial);
        }

        if (opts->shell || opts->logcat) {
            params.addFormat("androidboot.console=%s%d", kernelSerialPrefix,
                             lastSerial++);
        }

        // The rild daemon, used for GSM emulation, checks for qemud, just
        // set it to a dummy value instead of a serial port.
        params.add("android.qemud=1");
    }

    params.addIf("android.checkjni=1", !opts->no_jni);
    params.addIf("android.bootanim=0", opts->no_boot_anim);

    // qemu.gles is used to pass the GPU emulation mode to the guest
    // through kernel parameters. Note that the ro.opengles.version
    // boot property must also be defined for |gles > 0|, but this
    // is not handled here (see vl-android.c for QEMU1).
    {
        int gles;
        switch (glesMode) {
            case kAndroidGlesEmulationHost: gles = 1; break;
            case kAndroidGlesEmulationGuest: gles = 2; break;
            default: gles = 0;
        }
        params.addFormat("qemu.gles=%d", gles);
    }

    // Additional memory for -gpu guest software renderers (e.g., SwiftShader)
    if (glesMode == kAndroidGlesEmulationGuest && glesGuestCmaMB > 0) {
        params.addFormat("cma=%" PRIu64 "M", glesGuestCmaMB);
    }

    if (opts->logcat) {
        std::string param = opts->logcat;
        // Replace any space with a comma.
        std::replace(param.begin(), param.end(), ' ', ',');
        std::replace(param.begin(), param.end(), '\t', ',');
        params.addFormat("androidboot.logcat=%s", param);
    }

    if (opts->bootchart) {
        params.addFormat("androidboot.bootchart=%s", opts->bootchart);
    }

    if (opts->selinux) {
        params.addFormat("androidboot.selinux=%s", opts->selinux);
    }

    if (opts->dns_server) {
        uint32_t ips[ANDROID_MAX_DNS_SERVERS];
        int dnsCount = android_dns_get_servers(opts->dns_server, ips);
        if (dnsCount > 1) {
            params.addFormat("ndns=%d", dnsCount);
        }
    }

    if (avdKernelParameters && avdKernelParameters[0]) {
        params.add(avdKernelParameters);
    }

    return params.toCStringCopy();
}
