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

#include "aemu/base/Debug.h"

#include "aemu/base/ArraySize.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#endif

namespace android {
namespace base {

#ifdef __linux__
static std::string readFile(std::string_view path) {
    std::ifstream is(c_str(path));

    if (!is) {
        return {};
    }

    std::ostringstream ss;
    ss << is.rdbuf();
    return ss.str();
}
#endif

bool IsDebuggerAttached() {
#ifdef _WIN32
    return ::IsDebuggerPresent() != 0;
#elif defined(__linux__)
    std::string procStatus = readFile("/proc/self/status");

    static constexpr std::string_view kTracerPidPrefix = "TracerPid:";
    const auto tracerPid = procStatus.find(kTracerPidPrefix.data());
    if (tracerPid == std::string::npos) {
        return false;
    }

    // If the tracer PID is parseable and not 0, there's a debugger attached.
    const bool debuggerAttached =
            atoi(procStatus.c_str() + tracerPid + kTracerPidPrefix.size()) != 0;
    return debuggerAttached;
#elif defined(__APPLE__)
    int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    struct kinfo_proc procInfo = {};
    size_t infoSize = sizeof(procInfo);
    const int res =
            sysctl(mib, arraySize(mib), &procInfo, &infoSize, nullptr, 0);
    if (res) {
        return false;
    }
    return (procInfo.kp_proc.p_flag & P_TRACED) != 0;
#else
#error Unsupported platform
#endif
}

bool WaitForDebugger(int64_t timeoutMs) {
    static const int64_t sleepTimeoutMs = 500;

    int64_t sleptForMs = 0;
    while (!IsDebuggerAttached()
           && (timeoutMs == -1 || sleptForMs < timeoutMs)) {
        System::get()->sleepMs(sleepTimeoutMs);
        sleptForMs += sleepTimeoutMs;
    }
    return IsDebuggerAttached();
}

void DebugBreak() {
#ifdef _WIN32
    ::DebugBreak();
#else
#ifdef __x86_64__
    asm("int $3");
#elif defined(__aarch64)
    asm("trap");
#endif
#endif
}

}  // namespace base
}  // namespace android
