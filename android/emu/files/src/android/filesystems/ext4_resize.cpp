// Copyright (C) 2015 The Android Open Source Project
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

#include "android/filesystems/ext4_resize.h"

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/Log.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

#include <string>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef _WIN32
#include "aemu/base/system/Win32Utils.h"
#include "android/utils/win32_cmdline_quote.h"
#include <windows.h>

using android::base::Win32Utils;
#else
#include <sys/wait.h>
#endif

using android::base::c_str;
using android::base::RunOptions;
using android::base::System;


static auto convertBytesToMB(uint64_t size) -> unsigned {
    if (size == 0) {
        return 0;
    }
    size = (size + (1ULL << 20) - 1ULL) >> 20;
    if (size > UINT_MAX) {
        size = UINT_MAX;
    }
    return static_cast<unsigned>(size);
}

// Convenience function for formatting and printing system call/library
// function errors that show up regardless of host platform. Equivalent
// to printing the stringified error code from errno or GetLastError()
// (for windows).
void explainSystemErrors(const char* msg) {
#ifdef _WIN32
    char* pstr = NULL;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  NULL, GetLastError(), 0, (LPTSTR)&pstr, 2, NULL);
    derror("%s - %s\n", msg, pstr);
    LocalFree(pstr);
#else
    derror("%s - %s\n", msg, strerror(errno));
#endif
}

static auto runExt4Program(const std::string_view& program,
                           std::initializer_list<std::string> params) -> int {
    std::string executable = System::get()->findBundledExecutable(program);
    if (executable.empty()) {
        derror("couldn't get path to %s binary",
                c_str(program).get());
        return -1;
    }

    std::vector<std::string> commandLine{executable};
    commandLine.insert(commandLine.end(), params);

    System::ProcessExitCode exitCode = 1;
    auto success = System::get()->runCommand(commandLine,
                                             RunOptions::WaitForCompletion,
                                             System::kInfinite, &exitCode);

    if (!success) {
        derror("resizing partition failed to launch %s",
                executable.c_str());
        return -1;
    }
    if (exitCode != 0) {
        derror("resizing partition %s failed with exit code %d",
                c_str(program).get(), (int)exitCode);
        return exitCode;
    }
    return 0;
}

auto resizeExt4Partition(const char* partitionPath, int64_t newByteSize) -> int {
    // sanity checks
    if (partitionPath == nullptr || !checkExt4PartitionSize(newByteSize)) {
        return -1;
    }

    // resize2fs requires that we run e2fsck first in order to make sure that
    // the filesystem is in good shape. If we resize without first running this
    // the guest kernel could decide to replay the journal and end up in a state
    // before the resize took place. This is something that frequently happened
    // and caused the resize to not be visible in the guest system.
    int fsckReturnCode = runExt4Program("e2fsck", {"-y", partitionPath});
    if (fsckReturnCode != 0) {
        return fsckReturnCode;
    }

    char size_in_MB[50];
    int copied = snprintf(size_in_MB, sizeof(size_in_MB), "%uM",
                          convertBytesToMB(newByteSize));
    size_in_MB[sizeof(size_in_MB) - 1] = '\0';
    if (copied < 0 || static_cast<size_t>(copied) >= sizeof(size_in_MB)) {
        derror("failed to format size in resize2fs command");
        return -1;
    }

    return runExt4Program("resize2fs", {"-f", partitionPath, size_in_MB});
}

auto checkExt4PartitionSize(int64_t byteSize) -> bool {
    uint64_t maxSizeMB =
            16 * 1024 * 1024;  // (16 TiB) * (1024 GiB / TiB) * (1024 MiB / GiB)
    uint64_t minSizeMB = 128;
    uint64_t sizeMB = convertBytesToMB(byteSize);

    // compiler converts signed to unsigned
    return (sizeMB >= minSizeMB) && (sizeMB <= maxSizeMB);
}
