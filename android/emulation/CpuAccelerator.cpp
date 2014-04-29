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

#include "android/emulation/CpuAccelerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winioctl.h>
#else
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <stdio.h>

#include "android/utils/path.h"

#include "android/base/Compiler.h"
#include "android/base/files/ScopedFd.h"
#ifdef _WIN32
#include "android/base/files/ScopedHandle.h"
#endif
#include "android/base/Log.h"
#include "android/base/StringFormat.h"

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
};

GlobalState gGlobals = { false, false, CPU_ACCELERATOR_NONE, { '\0' } };

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
bool ProbeKVM(String *status) {
    // 1) Check that /dev/kvm exists.
    if (::access("/dev/kvm", F_OK)) {
        status->assign(
            "KVM is not installed on this machine (/dev/kvm is missing).");
        return false;
    }
    // 2) Check that /dev/kvm can be opened.
    if (::access("/dev/kvm", R_OK)) {
        status->assign(
            "This user doesn't have permissions to use KVM (/dev/kvm).");
        return false;
    }
    // 3) Open the file.
    ScopedFd fd(TEMP_FAILURE_RETRY(open("/dev/kvm", O_RDWR)));
    if (!fd.valid()) {
        status->assign("Could not open /dev/kvm :");
        status->append(strerror(errno));
        return false;
    }

    // 4) Extract KVM version number.
    int version = ::ioctl(fd.get(), KVM_GET_API_VERSION, 0);
    if (version < 0) {
        status->assign("Could not extract KVM version: ");
        status->append(strerror(errno));
        return false;
    }

    // 5) Compare to minimum supported version
    status->clear();

    if (version < KVM_API_VERSION) {
        StringAppendFormat(status,
                           "KVM version too old: %d (expected at least %d)\n",
                           version,
                           KVM_API_VERSION);
        return false;
    }

    // 6) Profit!
    StringAppendFormat(status,
                       "KVM (version %d) is installed and usable.",
                       version);
    return true;
}

#endif  // HAVE_KVM


#if HAVE_HAX

// Version numbers for the HAX kernel module.
// |compat_version| is the minimum API version supported by the module.
// |current_version| is its current API version.
struct HaxModuleVersion {
    uint32_t compat_version;
    uint32_t current_version;
};


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////
/////  Windows HAX support.
/////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

using base::ScopedHandle;

// Windows IOCTL code to extract HAX kernel module version.
#define HAX_DEVICE_TYPE 0x4000
#define HAX_IOCTL_VERSION       \
    CTL_CODE(HAX_DEVICE_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

// The minimum API version supported by the Android emulator.
#define HAX_MIN_VERSION  1

bool ProbeHAX(String* status) {
    status->clear();
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
            status->assign("HAX kernel module is not installed!");
        } else {
            StringAppendFormat(status,
                               "Opening HAX kernel module failed: %u",
                               err);
        }
        return false;
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
        return false;
    }

    // 3) Check that it is the right version.
    if (hax_version.current_version < HAX_MIN_VERSION) {
        StringAppendFormat(status,
                           "HAX version (%d) is too old (need at least %d).",
                           hax_version.current_version,
                           HAX_MIN_VERSION);
        return false;
    }

    // 4) Profit!
    StringAppendFormat(status,
                       "HAX (version %d) is installed and usable.",
                       hax_version.current_version);
    return true;
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

bool ProbeHAX(String* status) {
    // 1) Check that /dev/HAX exists.
    if (::access("/dev/HAX", F_OK)) {
        status->assign(
            "HAX is not installed on this machine (/dev/HAX is missing).");
        return false;
    }
    // 2) Check that /dev/HAX can be opened.
    if (::access("/dev/HAX", R_OK)) {
        status->assign(
            "This user doesn't have permission to use HAX (/dev/HAX).");
        return false;
    }
    // 3) Open the file.
    ScopedFd fd(open("/dev/HAX", O_RDWR));
    if (!fd.valid()) {
        status->assign("Could not open /dev/HAX: ");
        status->append(strerror(errno));
        return false;
    }

    // 4) Extract HAX version number.
    status->clear();

    HaxModuleVersion hax_version;
    if (::ioctl(fd.get(), HAX_IOCTL_VERSION, &hax_version) < 0) {
        StringAppendFormat(status,
                           "Could not extract HAX version: %s",
                           strerror(errno));
        return false;
    }

    if (hax_version.current_version < hax_version.compat_version) {
        StringAppendFormat(
                status,
                "Malformed HAX version numbers (current=%d, compat=%d)\n",
                hax_version.current_version,
                hax_version.compat_version);
        return false;
    }

    // 5) Compare to minimum supported version.

    if (hax_version.current_version < HAX_MIN_VERSION) {
        StringAppendFormat(status,
                           "HAX version too old: %d (expected at least %d)\n",
                           hax_version.current_version,
                           HAX_MIN_VERSION);
        return false;
    }

    // 6) Profit!
    StringAppendFormat(status,
                       "HAX (version %d) is installed and usable.",
                       hax_version.current_version);
    return true;
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
    if (ProbeKVM(&status)) {
        g->accel = CPU_ACCELERATOR_KVM;
    }
#elif HAVE_HAX
    if (ProbeHAX(&status)) {
        g->accel = CPU_ACCELERATOR_HAX;
    }
#else
    status = "This system does not support CPU acceleration.";
#endif
    ::snprintf(g->status, sizeof(g->status), "%s", status.c_str());

    g->probed = true;
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

void SetCurrentCpuAcceleratorForTesting(CpuAccelerator accel,
                                        const char* status) {
    GlobalState *g = &gGlobals;

    g->testing = true;
    g->accel = accel;
    ::snprintf(g->status, sizeof(g->status), "%s", status);
}

}  // namespace android
