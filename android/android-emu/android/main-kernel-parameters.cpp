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

#include "aemu/base/StringFormat.h"
#include "host-common/GoldfishDma.h"
#include "android/emulation/ParameterList.h"
#include "android/emulation/SetupParameters.h"
#include "host-common/FeatureControl.h"
#include "android/kernel/kernel_utils.h"
#include "android/utils/debug.h"
#include "android/utils/dns.h"

#include <algorithm>
#include <memory>

#include <inttypes.h>
#include <string.h>

using android::base::StringFormat;

// Note: defined in platform/system/vold/model/Disk.cpp
static const unsigned int kMajorBlockLoop = 7;

std::string emulator_getKernelParameters(const AndroidOptions* opts,
                                         const char* targetArch,
                                         int apiLevel,
                                         const char* kernelSerialPrefix,
                                         const char* avdKernelParameters,
                                         const char* kernelPath,
                                         const std::vector<std::string>* verifiedBootParameters,
                                         uint64_t glFramebufferSizeBytes,
                                         mem_map ramoops,
                                         bool isQemu2,
                                         bool isCros,
                                         std::vector<std::string> userspaceBootProps) {
    if (isCros) {
      std::string cmdline(StringFormat(
          "ro noresume noswap loglevel=7 noinitrd root=/dev/sda3 no_timer_check"
          "cros_legacy cros_debug console=%s",
          opts->show_kernel ? "ttyS0" : ""));
      return strdup(cmdline.c_str());
    }

    KernelVersion kernelVersion = KERNEL_VERSION_0;
    if (!android_getKernelVersion(kernelPath, &kernelVersion)) {
        derror("Can't get kernel version from the kernel image file: '%s'",
               kernelPath);
    }

    bool isX86ish = !strcmp(targetArch, "x86") || !strcmp(targetArch, "x86_64");

    android::ParameterList params;
    // Disable apic timer check. b/33963880
    params.add("no_timer_check");

    if (kernelVersion >= KERNEL_VERSION_5_4_0) {
        params.add("8250.nr_uarts=1");  // disabled by default for security reasons
    }

    // TODO: enable this with option
    // params.addFormat("androidboot.logcat=*:D");

    if (isX86ish) {
        params.add("clocksource=pit");
        // b/67565886, when cpu core is set to 2, clock_gettime() function hangs
        // in goldfish kernel which caused surfaceflinger hanging in the guest
        // system. To workaround, start the kernel with no kvmclock. Currently,
        // only API 24 and API 25 have kvm clock enabled in goldfish kernel.
        //
        // kvm-clock seems to be stable for >= 5.4.
        if (kernelVersion < KERNEL_VERSION_5_4_0) {
            params.add("no-kvmclock");
        }
    }

    android::setupVirtualSerialPorts(
            &params, nullptr, apiLevel, targetArch, kernelSerialPrefix, isQemu2,
            opts->show_kernel, opts->logcat || opts->shell, opts->shell_serial);

    // If qemu1, make sure GLDMA is disabled.
    if (!isQemu2)
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLDMA, false);

    // 2. Calculate additional memory for software renderers (e.g., SwiftShader)
    const uint64_t one_MB = 1024ULL * 1024ULL;
    int numBuffers = 2; /* double buffering */
    uint64_t glEstimatedFramebufferMemUsageMB =
        (numBuffers * glFramebufferSizeBytes + one_MB - 1) / one_MB;

    // 3. Additional contiguous memory reservation for DMA and software framebuffers,
    // specified in MB
    const int Cma =
        2 * glEstimatedFramebufferMemUsageMB +
        (isQemu2 && android::featurecontrol::isEnabled(android::featurecontrol::GLDMA) ? 256 : 0);
    if (Cma) {
        params.addFormat("cma=%" PRIu64 "M@0-4G", Cma);
    }

    if (opts->dns_server) {
        SockAddress ips[ANDROID_MAX_DNS_SERVERS];
        int dnsCount = android_dns_get_servers(opts->dns_server, ips);
        if (dnsCount > 1) {
            params.addFormat("ndns=%d", dnsCount);
        }
    }
    // mac80211_hwsim
    if (isQemu2) {
        if (android::featurecontrol::isEnabled(
                                     android::featurecontrol::VirtioWifi)) {
            android::featurecontrol::setIfNotOverriden(
                    android::featurecontrol::Wifi, false);

            // For API 30, we still use mac80211_hwsim.mac_prefix within kernel
            // driver to configure mac address. For API 31  and above, we cannot
            // make assumption about mac80211_hwsim.mac_prefix in kernel.
            if (apiLevel <= 30) {
                params.add("mac80211_hwsim.radios=1");
                android::featurecontrol::setIfNotOverriden(
                        android::featurecontrol::Mac80211hwsimUserspaceManaged,
                        false);
            }
        } else if (android::featurecontrol::isEnabled(
                           android::featurecontrol::Wifi)) {
            if (!android::featurecontrol::isEnabled(
                        android::featurecontrol::
                                Mac80211hwsimUserspaceManaged)) {
                params.add("mac80211_hwsim.radios=2");
            }
            // Enable multiple channels so the kernel can scan on one channel
            // while communicating the other. This speeds up scanning
            // significantly. This does not work if WiFi Direct is enabled
            // starting with Q images so only set this option if WiFi Direct
            // support is not enabled.
            if (apiLevel < 29) {
                params.add("mac80211_hwsim.channels=2");
            } else if (!opts->wifi_client_port && !opts->wifi_server_port) {
                params.add("mac80211_hwsim.channels=2");
            }
        }
    }

    if (isQemu2) {
        if (android::featurecontrol::isEnabled(android::featurecontrol::SystemAsRoot)) {
            params.add("skip_initramfs");
            params.add("rootwait");
            params.add("ro");
            params.add("init=/init");
            // If verifiedBootParameters were added, they will provide the root
            // argument which corresponds to a mapped device.
            if (!verifiedBootParameters || verifiedBootParameters->empty()) {
                params.add("root=/dev/vda1");
            }
        }
    }

    // Enable partitions on loop devices.
    // This is used by the new "virtual disk" feature used by vold to help
    // debug and test storage code on devices without physical media.
    params.addFormat("loop.max_part=%u", kMajorBlockLoop);

    if (avdKernelParameters && avdKernelParameters[0]) {
        params.add(avdKernelParameters);
    }

    // Configure the ramoops module, and mark the region where ramoops lives as
    // unusable. This will prevent anyone else from using this memory region.
    if (ramoops.size > 0 && ramoops.start > 0) {
      params.addFormat("ramoops.mem_address=0x%" PRIx64, ramoops.start);
      params.addFormat("ramoops.mem_size=0x%" PRIx64, ramoops.size);
      params.addFormat("memmap=0x%" PRIx64 "$0x%" PRIx64,  ramoops.size, ramoops.start);
    }

    if (opts->shell || opts->shell_serial || opts->show_kernel) {
        // The default value for printk.devkmsg is "ratelimit",
        // causing only a few logs from the android init
        // executable to be printed.
        params.addFormat("printk.devkmsg=on");
    }

    for (std::string& prop : userspaceBootProps) {
        params.add(std::move(prop));
    }

    // User entered parameters are space separated. Passing false here to prevent
    // parameters from being surrounded by quotes.
    return params.toString(false);
}
