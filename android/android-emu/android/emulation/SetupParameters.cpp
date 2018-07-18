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

#include "android/emulation/SetupParameters.h"

namespace android {

// static
void setupVirtualSerialPorts(ParameterList* kernelParams,
                             ParameterList* qemuParams,
                             int apiLevel,
                             const char* targetArch,
                             const char* kernelSerialPrefix,
                             bool isQemu2,
                             bool optionShowKernel,
                             bool hasShellConsole,
                             const char* shellSerial) {
    bool isX86ish = !strcmp(targetArch, "x86") || !strcmp(targetArch, "x86_64");

    bool isARMish = !strcmp(targetArch, "arm") || !strcmp(targetArch, "arm64");

    // Each virtual serial port device is connected to a QEMU 'chardev'
    // backend which can be 'null', 'stdio', 'con:', 'android-kmsg' or
    // 'android-qemud' in case of QEMU1. See docs/CHAR-DEVICES.TXT.
    //
    // CRITICAL: To properly support snapshotting, we need to ensure that the
    // set of virtual devices created for an AVD is consistent with its
    // hardware settings but does _not_ depend on command-line options. The
    // associated chardev can be different from one run to the other though.
    // A single virtual tty is used for -show-kernel, -shell and -logcat
    // and the -shell-serial option will affect all three of them.
    // TODO(digit): Ensure -shell-serial only works on -shell output.
    if (kernelParams) {
        if (optionShowKernel) {
            kernelParams->addFormat("console=%s0,38400", kernelSerialPrefix);
        }

        if (isARMish) {
            kernelParams->add("keep_bootcon");
            kernelParams->addFormat("earlyprintk=%s0", kernelSerialPrefix);
        }

        if (hasShellConsole) {
            kernelParams->addFormat("androidboot.console=%s0",
                                    kernelSerialPrefix);
        }

        // The rild daemon, used for GSM emulation, checks for qemud, just
        // set it to a dummy value instead of a serial port.
        kernelParams->add("android.qemud=1");
    }

    if (qemuParams) {
        bool useFirstSerial = optionShowKernel || hasShellConsole;
        qemuParams->add2("-serial", useFirstSerial ? shellSerial : "null");
    }

    if (kernelParams && !optionShowKernel) {
        // Required to prevent kernel messages to be sent to framebuffer
        // (through 'vc0', i.e. virtual console 0).
        kernelParams->add("console=0");
    }
}

}  // namespace android
