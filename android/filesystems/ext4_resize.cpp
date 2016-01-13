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

#include "android/base/String.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include "android/base/system/Win32Utils.h"
#include "android/utils/win32_cmdline_quote.h"
#include <windows.h>

using android::base::Win32Utils;
#else
#include <sys/wait.h>
#endif

using android::base::RunOptions;
using android::base::String;
using android::base::System;

static unsigned convertBytesToMB(uint64_t size) {
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
    fprintf(stderr, "ERROR: %s - %s\n", msg, pstr);
    LocalFree(pstr);
#else
    fprintf(stderr, "ERROR: %s - %s\n", msg, strerror(errno));
#endif
}

int resizeExt4Partition(const char* partitionPath, int64_t newByteSize) {
    // sanity checks
    if (partitionPath == NULL || !checkExt4PartitionSize(newByteSize)) {
        return -1;
    }

    // format common arguments once
    String executable = System::get()->findBundledExecutable("resize2fs");
    if (executable.empty()) {
        fprintf(stderr, "ERROR: couldn't get path to resize2fs binary\n");
        return -1;
    }

    char size_in_MB[50];
    int copied = snprintf(size_in_MB, sizeof(size_in_MB), "%uM",
                          convertBytesToMB(newByteSize));
    size_in_MB[sizeof(size_in_MB) - 1] = '\0';
    if (copied < 0 || static_cast<size_t>(copied) >= sizeof(size_in_MB)) {
        fprintf(stderr, "ERROR: failed to format size in resize2fs command\n");
        return -1;
    }

    auto& sys = *System::get();
    auto exitCode = sys.runCommand({executable, "-f", partitionPath, size_in_MB}, RunOptions::WaitForCompletion
                                  | RunOptions::ReturnExitCode);

    if (exitCode != 0) {
        fprintf(stderr, "ERROR: resizing partition failed with exit code %d\n",
                (int)exitCode);
        return exitCode;
    }
    return 0;
}

bool checkExt4PartitionSize(int64_t byteSize) {
    uint64_t maxSizeMB =
            16 * 1024 * 1024;  // (16 TiB) * (1024 GiB / TiB) * (1024 MiB / GiB)
    uint64_t minSizeMB = 128;
    uint64_t sizeMB = convertBytesToMB(byteSize);

    // compiler converts signed to unsigned
    return (sizeMB >= minSizeMB) && (sizeMB <= maxSizeMB);
}
