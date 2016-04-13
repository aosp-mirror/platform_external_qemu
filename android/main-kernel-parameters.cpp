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

#include <memory>

#include <string.h>

using android::base::StringFormat;

char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   const char* kernelSerialPrefix,
                                   const char* avdKernelParameters,
                                   AndroidGlesEmulationMode glesMode,
                                   bool isQemu2) {
    android::ParameterList params;
    bool isX86 = !strcmp(targetArch, "x86");

    // We always force qemu=1 when running inside QEMU.
    params.add("qemu=1");

    params.add("androidboot.hardware=goldfish");

    if (isX86) {
        params.add("clocksource=pit");
    }

    int lastSerial = 0;

    // The first virtual tty is reserved for receiving kernel messages.
    params.addFormat("console=%s%d", kernelSerialPrefix, lastSerial++);

    // The second virtual tty is reserved for the legacy QEMUD channel.
    params.addFormat("android.qemud=%s%d", kernelSerialPrefix, lastSerial++);

    if (opts->shell || opts->logcat) {
        // We need a new virtual tty to receive the logcat output and/or
        // the root console. Note however that on QEMU1/x86, the virtual
        // machine has a limited number of IRQs that doesn't allow more
        // than 2 ttys to be used at the same time.
        if (!isX86) {
            params.addFormat("androidboot.console=%s%d", kernelSerialPrefix,
                             lastSerial++);
        } else {
            dwarning("Sorry, but -logcat and -shell options are not "
                     "implemented for x86 AVDs running in the classic "
                     "emulator");
        }
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

    if (opts->logcat) {
        std::string param = StringFormat("androidboot.logcat=%s", opts->logcat);
        // Replace any space with a comma.
        for (size_t n = 0; n < param.size(); ++n) {
            if (param[n] == ' ') {
                param[n] = ',';
            }
        }
        params.add(std::move(param));
    }

    if (opts->bootchart) {
        params.addFormat("androidboot.bootchart=%s", opts->bootchart);
    }

    if (opts->selinux) {
        params.addFormat("androidboot.selinux=%s", opts->selinux);
    }

    if (avdKernelParameters && avdKernelParameters[0]) {
        params.add(avdKernelParameters);
    }

    return params.toCStringCopy();
}
