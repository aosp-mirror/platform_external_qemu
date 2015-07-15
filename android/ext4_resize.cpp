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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include "android/base/String.h"
#include "android/ext4_resize.h"
#include "android/utils/path.h"
#include "base/system/System.h"
#include "main-common.h"
#include "utils/win32_cmdline_quote.h"

using android::base::String;
using android::base::System;

// Convenience function for formatting and printing errors that show up
// regardless of host platform
void printErrors (const char * msg) {
#ifdef _WIN32
    char *pstr = NULL;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL,
        GetLastError(),
        0,
        (LPTSTR) &pstr,
        2,
        NULL);
    fprintf(stderr, "ERROR: %s - %s\n", msg, pstr);
    LocalFree(pstr);
#else
    fprintf(stderr, "ERROR: %s - %s\n", msg, strerror(errno));
#endif
}

void resizeExt4Partition (char * partitionPath, int64_t newSize) {
    // sanity checks
    if (partitionPath == NULL || newSize < 0) {
        return;
    }

    // format common arguments once
    String executable = System::get()->findBundledExecutable("resize2fs");
    if(executable.empty()) {
        fprintf(stderr, "ERROR: couldn't get path to resize2fs binary\n");
        return;
    }

    char size_in_MB[50];
    int copied = snprintf(size_in_MB, sizeof(size_in_MB),
        "%uM", convertBytesToMB(newSize));
    size_in_MB[sizeof(size_in_MB) - 1] = '\0';
    if (copied < 0 || copied >= sizeof(size_in_MB)) {
        fprintf(stderr, "ERROR: failed to format size in resize2fs command\n");
        return;
    }

#ifdef _WIN32
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;
    DWORD                 exitCode;

    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&pinfo, sizeof(pinfo));
    startup.cb = sizeof(startup);

    char args[PATH_MAX * 2 + 1];
    copied = snprintf(args, sizeof(args), "resize2fs.exe -f %s %s",
                 win32_cmdline_quote(partitionPath),
                 size_in_MB);
    args[sizeof(args) - 1] = '\0';
    if (copied < 0 || copied >= sizeof(args)) {
        fprintf(stderr, "ERROR: failed to format resize2fs command\n");
        return;
    }

    BOOL success = CreateProcess(
        win32_cmdline_quote(executable.c_str()),      /* program path */
        args,                                    /* command line args */
        NULL,                    /* process handle is not inheritable */
        NULL,                     /* thread handle is not inheritable */
        FALSE,                       /* no, don't inherit any handles */
        CREATE_NO_WINDOW,   /* the new process doesn't have a console */
        NULL,                       /* use parent's environment block */
        NULL,                      /* use parent's starting directory */
        &startup,                   /* startup info, i.e. std handles */
        &pinfo);
    if (!success) {
        printErrors("failed to create process while resizing partition");
        return;
    }

    WaitForSingleObject(pinfo.hProcess, INFINITE);
    if (!GetExitCodeProcess(pinfo.hProcess, &exitCode)) {
        printErrors("couldn't get exit code from resizing partition process");
    }

    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);
#else
    int32_t exitCode = 0;
    pid_t pid;
    pid_t child = fork();

    if (child < 0) {
        printErrors("couldn't create a child process to resize the partition");
        return;
    }else if (child == 0) {
        execlp(executable.c_str(), executable.c_str(), "-f", partitionPath,
            size_in_MB, NULL);
        exit(-1);
    }

    while ((pid = waitpid(-1, &exitCode, 0)) != child) {
        if (pid == -1) {
            printErrors("resizing partition waitpid failed");
            return;
        }
    }
#endif
    if(exitCode != 0) {
        fprintf(stderr, "ERROR: resizing partition failed with exit code %d\n",
            exitCode);
    }
}
