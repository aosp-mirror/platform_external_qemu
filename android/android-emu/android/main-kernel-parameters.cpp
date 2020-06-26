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
#include "android/emulation/GoldfishDma.h"
#include "android/emulation/ParameterList.h"
#include "android/emulation/SetupParameters.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/kernel/kernel_utils.h"
#include "android/utils/debug.h"
#include "android/utils/dns.h"
#include "android/version.h"

#include <algorithm>
#include <memory>

#include <inttypes.h>
#include <string.h>

using android::base::StringFormat;

// Note: The ACPI _HID that follows devices/ must match the one defined in the
// ACPI tables (hw/i386/acpi_build.c)
static const char kSysfsAndroidDtDir[] =
        "/sys/bus/platform/devices/ANDR0001:00/properties/android/";
static const char kSysfsAndroidDtDirDtb[] =
        "/proc/device-tree/firmware/android/";

// Note: defined in platform/system/vold/model/Disk.cpp
static const unsigned int kMajorBlockLoop = 7;

char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   int apiLevel,
                                   const char* kernelSerialPrefix,
                                   const char* avdKernelParameters,
                                   const char* kernelPath,
                                   const std::vector<std::string>* verifiedBootParameters,
                                   AndroidGlesEmulationMode glesMode,
                                   int bootPropOpenglesVersion,
                                   uint64_t glFramebufferSizeBytes,
                                   mem_map ramoops,
                                   const int vm_heapSize,
                                   bool isQemu2,
                                   bool isCros,
                                   uint32_t lcd_width,
                                   uint32_t lcd_height,
                                   uint32_t lcd_vsync,
                                   const char* gltransport,
                                   uint32_t gltransport_drawFlushInterval) {
    android::ParameterList params;
    if (isCros) {
      std::string cmdline(StringFormat(
          "ro noresume noswap loglevel=7 noinitrd root=/dev/sda3 no_timer_check"
          "cros_legacy cros_debug console=%s",
          opts->show_kernel ? "ttyS0" : ""));
      return strdup(cmdline.c_str());
    }

    bool isX86ish = !strcmp(targetArch, "x86") || !strcmp(targetArch, "x86_64");

    // We always force qemu=1 when running inside QEMU.
    params.add("qemu=1");

    // Disable apic timer check. b/33963880
    params.add("no_timer_check");

    params.addFormat("androidboot.hardware=%s",
                     isQemu2 ? "ranchu" : "goldfish");

    {
        std::string myserialno(EMULATOR_VERSION_STRING);
        std::replace(myserialno.begin(), myserialno.end(), '.', 'X');
        params.addFormat("androidboot.serialno=EMULATOR%s", myserialno.c_str());
    }

    // TODO: enable this with option
    // params.addFormat("androidboot.logcat=*:D");

    if (isX86ish) {
        params.add("clocksource=pit");

        KernelVersion kernelVersion = KERNEL_VERSION_0;
        if (!android_getKernelVersion(kernelPath, &kernelVersion)) {
            derror("Can't get kernel version from the kernel image file: '%s'",
                   kernelPath);
        }

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

    // To save battery, set the screen off timeout to a high value.
    // Using int32_max here. The unit is milliseconds.
    params.addFormat("qemu.settings.system.screen_off_timeout=2147483647"); // 596 hours

    if (isQemu2 && android::featurecontrol::isEnabled(android::featurecontrol::EncryptUserData)) {
        params.add("qemu.encrypt=1");
    }

    // If qemu1, make sure GLDMA is disabled.
    if (!isQemu2)
        android::featurecontrol::setEnabledOverride(
                android::featurecontrol::GLDMA, false);

    // Android media profile selection
    // 1. If the SelectMediaProfileConfig is on, then select
    // <media_profile_name> if the resolution is above 1080p (1920x1080).
    if (isQemu2 && android::featurecontrol::isEnabled(android::featurecontrol::DynamicMediaProfile)) {
        if ((lcd_width > 1920 && lcd_height > 1080) ||
            (lcd_width > 1080 && lcd_height > 1920)) {
            fprintf(stderr, "Display resolution > 1080p. Using different media profile.\n");
            params.addFormat("qemu.mediaprofile.video=%s", "/data/vendor/etc/media_codecs_google_video_v2.xml");
        }
    }

    // Set vsync rate
    params.addFormat("qemu.vsync=%u", lcd_vsync);

    // Set gl transport props
    params.addFormat("qemu.gltransport=%s", gltransport);
    params.addFormat("qemu.gltransport.drawFlushInterval=%u", gltransport_drawFlushInterval);

    // OpenGL ES related setup
    // 1. Set opengles.version and set Skia as UI renderer if
    // GLESDynamicVersion = on (i.e., is a reasonably good driver)
    params.addFormat("qemu.opengles.version=%d", bootPropOpenglesVersion);

    if (android::featurecontrol::isEnabled(
            android::featurecontrol::GLESDynamicVersion)) {
        params.addFormat("qemu.uirenderer=%s", "skiagl");
    }

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
        SockAddress ips[ANDROID_MAX_DNS_SERVERS];
        int dnsCount = android_dns_get_servers(opts->dns_server, ips);
        if (dnsCount > 1) {
            params.addFormat("ndns=%d", dnsCount);
        }
    }

    if (isQemu2 &&
            android::featurecontrol::isEnabled(android::featurecontrol::Wifi)) {
        params.add("qemu.wifi=1");
        // Enable multiple channels so the kernel can scan on one channel while
        // communicating the other. This speeds up scanning significantly. This
        // does not work if WiFi Direct is enabled starting with Q images so
        // only set this option if WiFi Direct support is not enabled.
        if (apiLevel < 29) {
            params.add("mac80211_hwsim.channels=2");
        } else if (!opts->wifi_client_port && !opts->wifi_server_port) {
            params.add("mac80211_hwsim.channels=2");
        }
    }
#ifndef AEMU_GFXSTREAM_BACKEND
    if (isQemu2 &&
            android::featurecontrol::isEnabled(android::featurecontrol::VirtioWifi)) {
        params.add("qemu.virtiowifi=1");
        // Turn off hwsim when virtio wifi is in use.
        params.add("mac80211_hwsim.radios=0");
    }
#endif
    const bool isDynamicPartition = android::featurecontrol::isEnabled(android::featurecontrol::DynamicPartition);
    if (isQemu2 && isX86ish && !isDynamicPartition) {
        // x86 and x86_64 platforms use an alternative Android DT directory that
        // mimics the layout of /proc/device-tree/firmware/android/
        params.addFormat("androidboot.android_dt_dir=%s",
            (android::featurecontrol::isEnabled(android::featurecontrol::KernelDeviceTreeBlobSupport) ?
                kSysfsAndroidDtDirDtb : kSysfsAndroidDtDir));
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

    if (verifiedBootParameters) {
        for (const std::string& param : *verifiedBootParameters) {
            params.add(param);
        }
    }

    // Configure the ramoops module, and mark the region where ramoops lives as
    // unusable. This will prevent anyone else from using this memory region.
    if (ramoops.size > 0 && ramoops.start > 0) {
      params.addFormat("ramoops.mem_address=0x%" PRIx64, ramoops.start);
      params.addFormat("ramoops.mem_size=0x%" PRIx64, ramoops.size);
      params.addFormat("memmap=0x%" PRIx64 "$0x%" PRIx64,  ramoops.size, ramoops.start);
    }

    if (vm_heapSize > 0) {
        char  temp[64];
        snprintf(temp, sizeof(temp), "%dm", vm_heapSize);
        params.addFormat("qemu.dalvik.vm.heapsize=%s", temp);
    }

    if (opts->legacy_fake_camera) {
        params.addFormat("qemu.legacy_fake_camera=1");
    }

    if (apiLevel > 29) {
        params.addFormat("qemu.camera_protocol_ver=1");
    }

    // User entered parameters are space separated. Passing false here to prevent
    // parameters from being surrounded by quotes.
    return params.toCStringCopy(false);
}
