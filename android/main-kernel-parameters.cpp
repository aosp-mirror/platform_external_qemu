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

#include <memory>

#include <string.h>

using android::base::StringFormat;

char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   const char* kernelSerialPrefix,
                                   bool is_qemu2) {
    android::ParameterList params;

    params.add("androidboot.hardware=goldfish");

    if (!strcmp(targetArch, "x86")) {
        params.add("clocksource=pit");
    }

    if (opts->shell || opts->logcat) {
        // Technical note: The first tty is reserved for kernel logs,
        // the second one is reserved for legacy QEMUD, so to get
        // -shell or -logcat working, we need to setup a new virtual tty
        // device (which is performed in main-qemu-parameters.cpp and
        // tell the kernel to use it to open a console, which is performed
        // here.
        int shell_serial = 2;
        std::string param = StringFormat("androidboot.console=%s%d",
                                         kernelSerialPrefix,
                                         shell_serial);
        params.add(std::move(param));
    }

    params.addIf("android.checkjni=1", !opts->no_jni);
    params.addIf("android.bootanim=0", opts->no_boot_anim);

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
        std::string param =
                StringFormat("androidboot.bootchart=%s", opts->bootchart);
        params.add(std::move(param));
    }

    if (opts->selinux) {
        std::string param =
                StringFormat("androidboot.selinux=%s", opts->selinux);
        params.add(std::move(param));
    }

    return params.toCStringCopy();
}
