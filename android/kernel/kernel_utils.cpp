// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/kernel/kernel_utils.h"

#include "android/base/Log.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/base/String.h"
#include "android/kernel/kernel_utils_testing.h"
#include "android/utils/path.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEBUG_KERNEL  0

#define KERNEL_LOG     LOG_IF(INFO, DEBUG_KERNEL)
#define KERNEL_PLOG    PLOG_IF(INFO, DEBUG_KERNEL)
#define KERNEL_ERROR   LOG_IF(ERROR, DEBUG_KERNEL)
#define KERNEL_PERROR  PLOG_IF(ERROR, DEBUG_KERNEL)

using android::base::String;

namespace {

#ifndef _WIN32
// Helper class to perform launch a command through popen() and call
// pclose() on destruction.
class ScopedPopenFile {
public:
    ScopedPopenFile(const char* command) {
        mFile = ::popen(command, "r");
    }

    FILE* get() const { return mFile; }

    ~ScopedPopenFile() {
        if (mFile) {
            ::pclose(mFile);
        }
    }

private:
    FILE* mFile;
};
#endif  // !_WIN32

bool getFileDescription(void* opaque, const char* filePath, String* text) {
    if (!filePath) {
        KERNEL_ERROR << "NULL path parameter";
        return false;
    }

    if (!path_exists(filePath)) {
        KERNEL_ERROR << "Kernel file doesn't exist: " << filePath;
        return false;
    }

#ifdef _WIN32
    // TODO(digit): Better/portable detection based on libmagic or something.
    KERNEL_ERROR << "Can't detect kernel version on Windows!";
    return false;
#else
    // NOTE: Use /usr/bin/file instead of 'file' because the latter can
    // be broken in certain environments (e.g. some versions of MacPorts).
    String command("/usr/bin/file ");
    command += filePath;

    ScopedPopenFile file(command.c_str());
    if (!file.get()) {
        KERNEL_PERROR << "Could not launch command: " << command.c_str();
        return false;
    }

    String result;
    const size_t kReserveSize = 256U;
    result.resize(kReserveSize);

    int ret = ::fread(&result[0], 1, kReserveSize, file.get());
    if (ret < static_cast<int>(kReserveSize) && ferror(file.get())) {
        KERNEL_ERROR << "Could not read file command output!?";
        return false;
    }
    result.resize(ret);
    text->assign(result);
    return true;
#endif
}

android::kernel::GetFileDescriptionFunction* sGetFileDescription =
        getFileDescription;

void* sGetFileDescriptionOpaque = NULL;

}  // namespace

namespace android {
namespace kernel {

void setFileDescriptionFunction(GetFileDescriptionFunction* file_func,
                                void* file_opaque) {
    sGetFileDescription = file_func ? file_func : &getFileDescription;
    sGetFileDescriptionOpaque = file_func ? file_opaque : NULL;
}

}  // namespace kernel
}  // namespace android

bool android_pathProbeKernelType(const char* kernelPath, KernelType* ktype) {
    String description;

    if (!sGetFileDescription(sGetFileDescriptionOpaque,
                             kernelPath,
                             &description)) {
        return false;
    }
    const char* bzImage = ::strstr(description.c_str(), "bzImage");
    if (!bzImage) {
        KERNEL_ERROR << "Not a compressed Linux kernel image!";
        return false;
    }
    const char* version = ::strstr(bzImage, "version ");
    if (!version) {
        KERNEL_ERROR << "Could not determine version!";
        return false;
    }
    version += ::strlen("version ");
    KERNEL_LOG << "Found kernel version " << version;

    char* end;
    unsigned long major = ::strtoul(version, &end, 10);
    if (end == version || *end != '.') {
        KERNEL_ERROR << "Could not find kernel major version!";
        return false;
    }
    KERNEL_LOG << "Kernel major version: " << major;
    if (major > 3) {
        *ktype = KERNEL_TYPE_3_10_OR_ABOVE;
    } else if (major < 3) {
        *ktype = KERNEL_TYPE_LEGACY;
    } else /* major == 3 */ {
        version = end + 1;
        unsigned long minor = ::strtoul(version, &end, 10);
        if (end == version) {
            KERNEL_ERROR << "Could not find kernel minor version!";
            return false;
        }
        KERNEL_LOG << "Kernel minor version: " << minor;

        *ktype = (minor >= 10)
                ? KERNEL_TYPE_3_10_OR_ABOVE : KERNEL_TYPE_LEGACY;
    }
    return true;
}

const char* android_kernelSerialDevicePrefix(KernelType ktype) {
    switch (ktype) {
        case KERNEL_TYPE_LEGACY: return "ttyS";
        case KERNEL_TYPE_3_10_OR_ABOVE: return "ttyGF";
        default: return "";
    }
}
