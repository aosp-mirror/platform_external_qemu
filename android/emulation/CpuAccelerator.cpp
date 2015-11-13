// Copyright (C) 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#define CPU_ACCELERATOR_PRIVATE
#include "android/emulation/CpuAccelerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winioctl.h>
#else
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <stdio.h>

#include "android/base/Compiler.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/cpu_accelerator.h"
#include "android/utils/file_data.h"
#include "android/utils/path.h"
#include "android/utils/x86_cpuid.h"
#include "target-i386/hax-interface.h"

#ifdef _WIN32
#include "android/base/files/ScopedHandle.h"
#include "android/windows_installer.h"
#endif

#ifdef __APPLE__
#include "android/emulation/internal/CpuAccelerator.h"
#endif

// NOTE: This source file must be independent of the rest of QEMU, as such
//       it should not include / reuse any QEMU source file or function
//       related to KVM or HAX.

#ifdef __linux__
#  define  HAVE_KVM  1
#  define  HAVE_HAX  0
#elif defined(_WIN32) || defined(__APPLE__)
#  define HAVE_KVM 0
#  define HAVE_HAX 1
#else
#  error "Unsupported host platform!"
#endif

namespace android {

using base::String;
using base::StringAppendFormat;
using base::ScopedFd;

namespace {

struct GlobalState {
    bool probed;
    bool testing;
    CpuAccelerator accel;
    char status[256];
    AndroidCpuAcceleration status_code;
};

GlobalState gGlobals = {false,
                        false,
                        CPU_ACCELERATOR_NONE,
                        {'\0'},
                        ANDROID_CPU_ACCELERATION_ERROR};

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////   Linux KVM support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if HAVE_KVM

#include <linux/kvm.h>

// Return true iff KVM is installed and usable on this machine.
// |*status| will be set to a small status string explaining the
// status of KVM on success or failure.
AndroidCpuAcceleration ProbeKVM(String* status) {
    // Check that /dev/kvm exists.
    if (::access("/dev/kvm", F_OK)) {
        // /dev/kvm does not exist
        bool cpu_ok =
                android_get_x86_cpuid_vmx_support() ||
                android_get_x86_cpuid_svm_support();
        if (!cpu_ok) {
            status->assign(
                "KVM requires a CPU that supports vmx or svm");
            return ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT;
        }

        status->assign(
            "/dev/kvm is not found: VT disabled in BIOS or KVM kernel module not loaded");
        return ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND;
    }

    // Check that /dev/kvm can be opened.
    if (::access("/dev/kvm", R_OK)) {
        status->assign(
            "This user doesn't have permissions to use KVM (/dev/kvm).");
        return ANDROID_CPU_ACCELERATION_DEV_PERMISSION;
    }

    // Open the file.
    ScopedFd fd(TEMP_FAILURE_RETRY(open("/dev/kvm", O_RDWR)));
    if (!fd.valid()) {
        status->assign("Could not open /dev/kvm :");
        status->append(strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED;
    }

    // Extract KVM version number.
    int version = ::ioctl(fd.get(), KVM_GET_API_VERSION, 0);
    if (version < 0) {
        status->assign("Could not extract KVM version: ");
        status->append(strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    // Compare to minimum supported version
    status->clear();

    if (version < KVM_API_VERSION) {
        StringAppendFormat(status,
                           "KVM version too old: %d (expected at least %d)\n",
                           version,
                           KVM_API_VERSION);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    // Profit!
    StringAppendFormat(status,
                       "KVM (version %d) is installed and usable.",
                       version);
    return ANDROID_CPU_ACCELERATION_READY;
}

#endif  // HAVE_KVM


#if HAVE_HAX

#define HAXM_INSTALLER_VERSION_MINIMUM 0x01001005

std::string cpuAcceleratorFormatVersion(int32_t version) {
    if (version < 0) {
        return "<invalid>";
    }
    char buf[16];  // strlen("127.255.65535")+1 = 14
    int32_t revision = version & 0xffff;
    version >>= 16;
    int32_t minor = version & 0xff;
    version >>= 8;
    int32_t major = version & 0x7f;
    snprintf(buf, sizeof(buf), "%i.%i.%i", major, minor, revision);
    return buf;
}

#ifdef __APPLE__

}  // namespace

int32_t cpuAcceleratorParseVersionScript(const std::string& version_script) {
    int32_t result = 0;
    const char ver[] = "VERSION=";
    const size_t ver_len = sizeof(ver) - 1U;

    size_t offset = version_script.find(ver);
    if (offset == std::string::npos) {
        return -1;
    }
    const char* pos = version_script.c_str() + ver_len;

    const int kValueCountMax = 3;  // support 3 numbers max major.minor.rev
    const int bit_offset[kValueCountMax] = {24, 16, 0};
    const int value_max[kValueCountMax] = {127, 255, 65535};
    for (int i = 0; i < kValueCountMax; i++) {
        const char* end = pos;
        unsigned long value = strtoul(pos, (char**)&end, 10);
        if (pos == end) {
            // no number was found, error if there was not at least one.
            if (i == 0) {
                result = -1;
            }
            break;
        }

        if (value > value_max[i]) {
            // invalid number was found
            return -1;
        }

        result |= (value << bit_offset[i]);

        if (*end == '.') {
            // skip delimiter
            end += 1;
        } else {
            // we're done parsing
            break;
        }
        // advance to next number
        pos = end;
    }
    // 0 is an invalid version number.
    if (result == 0) {
        result = -1;
    }
    return result;
}

int32_t cpuAcceleratorGetHaxVersion(const char* kext_dir[],
                                    const size_t kext_dir_count,
                                    const char* version_file) {
    bool found_haxm_kext = false;

    for (size_t i = 0; i < kext_dir_count; i++) {
        struct stat s;
        int err = stat(kext_dir[i], &s);
        if (err < 0 || !S_ISDIR(s.st_mode)) {
            // dir not found
            continue;
        }
        // At this point, we're certain that haxm is installed,
        // but might be broken
        found_haxm_kext = true;

        std::string version_file_abs =
                std::string(kext_dir[i]) + "/" + version_file;
        FileData fd;
        if (fileData_initFromFile(&fd, version_file_abs.c_str()) < 0) {
            // let's try the next directory, just in case
            continue;
        }

        // File data contains a bash variable assignment
        // e.g. "VERSION=1.1.4"
        std::string version_script(fd.data, fd.data + fd.size);
        int32_t result = cpuAcceleratorParseVersionScript(version_script);
        if (result == 0) {
            // This function uses a return value of zero to indicate "not
            // installed"
            // So I'm declaring version "0.0.0" to be invalid
            result = -1;
        }
        if (result <= -1) {
            // let's try the next directory, just in case
            continue;
        }
        return result;
    }

    if (found_haxm_kext) {
        // found haxm kext but couldn't parse version file
        return -1;
    }
    // not installed
    return 0;
}

namespace {

#endif  // __APPLE__

// Version numbers for the HAX kernel module.
// |compat_version| is the minimum API version supported by the module.
// |current_version| is its current API version.
struct HaxModuleVersion {
    uint32_t compat_version;
    uint32_t current_version;
};

/*
 * ProbeHaxCpu: returns ANDROID_CPU_ACCELERATION_READY if the CPU supports
 * HAXM requirements.
 *
 * Otherwise returns some other AndroidCpuAcceleration status and sets
 * |status| to a user-understandable error string
 */
AndroidCpuAcceleration ProbeHaxCpu(String* status) {
    char vendor_id[16];
    android_get_x86_cpuid_vendor_id(vendor_id, sizeof(vendor_id));

    if (android_get_x86_cpuid_vendor_id_is_vmhost(vendor_id)) {

        StringAppendFormat(status,
                           "Android Emulator does not support nested virtualization.  "
                           "Your CPU: '%s'",
                           vendor_id);
        return ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED;
    }

    /* HAXM only supports GenuineIntel processors */
    if (strcmp(vendor_id, "GenuineIntel") != 0) {
        StringAppendFormat(status,
                           "Android Emulator requires an Intel processor with "
                           "VT-x and NX support.  Your CPU: '%s'",
                           vendor_id);
        return ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT;
    }

    if (!android_get_x86_cpuid_vmx_support()) {
        if (android_get_x86_cpuid_is_vcpu()) {
#ifdef _WIN32
            // The vcpu bit is set but your vendor id is not one of the known VM ids
            // You are probably running under Hyper-V
            status->assign("Please disable Hyper-V before using the Android Emulator.  "
                           "Start a command prompt as Administrator, run 'bcdedit /set "
                           "hypervisorlaunchtype off', reboot.");
            return ANDROID_CPU_ACCELERATION_HYPERV_ENABLED;
#else // OSX
            StringAppendFormat(status,
                               "Android Emulator does not support nested virtualization.  "
                               "Your CPU: '%s'",
                               vendor_id);
            return ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED;
#endif
        }
        status->assign(
            "Android Emulator requires an Intel processor with VT-x and NX support.  "
            "(VT-x is not supported)");
        return ANDROID_CPU_ACCELERATION_NO_CPU_VTX_SUPPORT;
    }

    if (!android_get_x86_cpuid_nx_support()) {
        status->assign(
            "Android Emulator requires an Intel processor with VT-x and NX support.  "
            "(NX is not supported)");
        return ANDROID_CPU_ACCELERATION_NO_CPU_NX_SUPPORT;
    }

    return ANDROID_CPU_ACCELERATION_READY;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////  Windows HAX support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

using base::ScopedHandle;
using namespace android;

// Windows IOCTL code to extract HAX kernel module version.
#define HAX_DEVICE_TYPE 0x4000
#define HAX_IOCTL_VERSION       \
    CTL_CODE(HAX_DEVICE_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CAPABILITY    \
    CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

// The minimum API version supported by the Android emulator.
#define HAX_MIN_VERSION  1

AndroidCpuAcceleration ProbeHAX(String* status) {
    status->clear();

    AndroidCpuAcceleration cpu = ProbeHaxCpu(status);
    if (cpu != ANDROID_CPU_ACCELERATION_READY)
        return cpu;

    const char* productDisplayName = u8"Intel® Hardware Accelerated Execution Manager";
    int32_t haxm_installer_version = WindowsInstaller::getVersion(productDisplayName);
    if (haxm_installer_version == 0) {
        status->assign(
            "HAXM is not installed on this machine");
        return ANDROID_CPU_ACCELERATION_ACCEL_NOT_INSTALLED;
    }

    if (haxm_installer_version < HAXM_INSTALLER_VERSION_MINIMUM) {
        StringAppendFormat(
                status, "HAXM must be updated (version %s < %s).",
                cpuAcceleratorFormatVersion(haxm_installer_version).c_str(),
                cpuAcceleratorFormatVersion(HAXM_INSTALLER_VERSION_MINIMUM)
                        .c_str());
        return ANDROID_CPU_ACCELERATION_ACCEL_OBSOLETE;
    }

    // 1) Try to find the HAX kernel module.
    ScopedHandle hax(CreateFile("\\\\.\\HAX",
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL));
    if (!hax.valid()) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            status->assign(
                "Unable to open HAXM device: ERROR_FILE_NOT_FOUND");
            return ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND;
        } else if (err == ERROR_ACCESS_DENIED) {
            status->assign(
                "Unable to open HAXM device: ERROR_ACCESS_DENIED");
            return ANDROID_CPU_ACCELERATION_DEV_PERMISSION;
        }
        StringAppendFormat(status,
                           "Opening HAX kernel module failed: %u",
                           err);
        return ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED;
    }

    // 2) Extract the module's version.
    HaxModuleVersion hax_version;

    DWORD dSize = 0;
    BOOL ret = DeviceIoControl(hax.get(),
                               HAX_IOCTL_VERSION,
                               NULL, 0,
                               &hax_version, sizeof(hax_version),
                               &dSize,
                               (LPOVERLAPPED) NULL);
    if (!ret) {
        DWORD err = GetLastError();
        StringAppendFormat(status,
                            "Could not extract HAX module version: %u",
                            err);
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    // 3) Check that it is the right version.
    if (hax_version.current_version < HAX_MIN_VERSION) {
        StringAppendFormat(status,
                           "HAX version (%d) is too old (need at least %d).",
                           hax_version.current_version,
                           HAX_MIN_VERSION);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    hax_capabilityinfo cap;
    ret = DeviceIoControl(hax.get(),
                          HAX_IOCTL_CAPABILITY,
                          NULL, 0,
                          &cap, sizeof(cap),
                          &dSize,
                          (LPOVERLAPPED) NULL);

    if (!ret) {
        DWORD err = GetLastError();
        StringAppendFormat(status,
                            "Could not extract HAX capability: %u",
                            err);
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    if ( (cap.wstatus & HAX_CAP_WORKSTATUS_MASK) == HAX_CAP_STATUS_NOTWORKING )
    {
        if (cap.winfo & HAX_CAP_FAILREASON_VT) {
            status->assign("VT feature disabled in BIOS/UEFI");
            return ANDROID_CPU_ACCELERATION_VT_DISABLED;
        }
        else if (cap.winfo & HAX_CAP_FAILREASON_NX) {
            status->assign("NX feature disabled in BIOS/UEFI");
            return ANDROID_CPU_ACCELERATION_NX_DISABLED;
        }
    }

    // 4) Profit!
    StringAppendFormat(
            status, "HAXM version %s (%d) is installed and usable.",
            cpuAcceleratorFormatVersion(haxm_installer_version).c_str(),
            hax_version.current_version);
    return ANDROID_CPU_ACCELERATION_READY;
}

#elif defined(__APPLE__)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////  Darwin HAX support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

// An IOCTL command number used to retrieve the HAX kernel module version.
#define HAX_IOCTL_VERSION _IOWR(0, 0x20, HaxModuleVersion)

// The minimum API version supported by the Android emulator.
#define HAX_MIN_VERSION  1

AndroidCpuAcceleration ProbeHAX(String* status) {
    AndroidCpuAcceleration cpu = ProbeHaxCpu(status);
    if (cpu != ANDROID_CPU_ACCELERATION_READY)
        return cpu;

    const char* kext_dir[] = {
        "/Library/Extensions/intelhaxm.kext", // current
        "/System/Library/Extensions/intelhaxm.kext", // old
    };
    size_t kext_dir_count = sizeof(kext_dir)/sizeof(kext_dir[0]);

    const char* version_file = "Contents/Resources/support.txt";
    int32_t version = cpuAcceleratorGetHaxVersion(
        kext_dir,
        kext_dir_count,
        version_file
        );
    if (version == 0) {
        status->assign(
            "HAXM is not installed on this machine");
        return ANDROID_CPU_ACCELERATION_ACCEL_NOT_INSTALLED;
    }

    if (version < HAXM_INSTALLER_VERSION_MINIMUM) {
        // HAXM was found but version number was too old or missing
        StringAppendFormat(
                status, "HAXM must be updated (version %s < %s).",
                cpuAcceleratorFormatVersion(version).c_str(),
                cpuAcceleratorFormatVersion(HAXM_INSTALLER_VERSION_MINIMUM)
                        .c_str());
        return ANDROID_CPU_ACCELERATION_ACCEL_OBSOLETE;
    }

    // 1) Check that /dev/HAX exists.
    if (::access("/dev/HAX", F_OK)) {
        status->assign(
            "HAXM is not installed on this machine (/dev/HAX is missing).");
        return ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND;
    }

    // 2) Check that /dev/HAX can be opened.
    if (::access("/dev/HAX", R_OK)) {
        status->assign(
            "This user doesn't have permission to use HAX (/dev/HAX).");
        return ANDROID_CPU_ACCELERATION_DEV_PERMISSION;
    }

    // 3) Open the file.
    ScopedFd fd(open("/dev/HAX", O_RDWR));
    if (!fd.valid()) {
        status->assign("Could not open /dev/HAX: ");
        status->append(strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED;
    }

    // 4) Extract HAX version number.
    status->clear();

    HaxModuleVersion hax_version;
    if (::ioctl(fd.get(), HAX_IOCTL_VERSION, &hax_version) < 0) {
        StringAppendFormat(status,
                           "Could not extract HAX version: %s",
                           strerror(errno));
        return ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED;
    }

    if (hax_version.current_version < hax_version.compat_version) {
        StringAppendFormat(
                status,
                "Malformed HAX version numbers (current=%d, compat=%d)\n",
                hax_version.current_version,
                hax_version.compat_version);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    // 5) Compare to minimum supported version.

    if (hax_version.current_version < HAX_MIN_VERSION) {
        StringAppendFormat(status,
                           "HAX API version too old: %d (expected at least %d)\n",
                           hax_version.current_version,
                           HAX_MIN_VERSION);
        return ANDROID_CPU_ACCELERATION_DEV_OBSOLETE;
    }

    // 6) Profit!
    StringAppendFormat(status, "HAXM version %s (%d) is installed and usable.",
                       cpuAcceleratorFormatVersion(version).c_str(),
                       hax_version.current_version);
    return ANDROID_CPU_ACCELERATION_READY;
}

#else   // !_WIN32 && !__APPLE__
#error "Unsupported HAX host platform"
#endif  // !_WIN32 && !__APPLE__

#endif  // HAVE_HAX

}  // namespace

CpuAccelerator GetCurrentCpuAccelerator() {
    GlobalState* g = &gGlobals;

    if (g->probed || g->testing) {
        return g->accel;
    }

    String status;
#if HAVE_KVM
    AndroidCpuAcceleration status_code = ProbeKVM(&status);
    if (status_code == ANDROID_CPU_ACCELERATION_READY) {
        g->accel = CPU_ACCELERATOR_KVM;
    }
#elif HAVE_HAX
    AndroidCpuAcceleration status_code = ProbeHAX(&status);
    if (status_code == ANDROID_CPU_ACCELERATION_READY) {
        g->accel = CPU_ACCELERATOR_HAX;
    }
#else
    status = "This system does not support CPU acceleration.";
#endif

    // cache status
    g->probed = true;
    g->status_code = status_code;
    ::snprintf(g->status, sizeof(g->status), "%s", status.c_str());

    return g->accel;
}

String GetCurrentCpuAcceleratorStatus() {
    GlobalState *g = &gGlobals;

    if (!g->probed && !g->testing) {
        // Force detection of the current CPU accelerator.
        GetCurrentCpuAccelerator();
    }

    return String(g->status);
}

AndroidCpuAcceleration GetCurrentCpuAcceleratorStatusCode() {
    GlobalState *g = &gGlobals;

    if (!g->probed && !g->testing) {
        // Force detection of the current CPU accelerator.
        GetCurrentCpuAccelerator();
    }

    return g->status_code;
}

void SetCurrentCpuAcceleratorForTesting(CpuAccelerator accel,
                                        AndroidCpuAcceleration status_code,
                                        const char* status) {
    GlobalState *g = &gGlobals;

    g->testing = true;
    g->accel = accel;
    g->status_code = status_code;
    ::snprintf(g->status, sizeof(g->status), "%s", status);
}

}  // namespace android
