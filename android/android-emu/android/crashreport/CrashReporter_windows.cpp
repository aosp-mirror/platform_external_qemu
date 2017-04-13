// Copyright 2015 The Android Open Source Project
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

#include "android/crashreport/CrashReporter.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/utils/debug.h"
#include "client/windows/handler/exception_handler.h"

#include <psapi.h>

#include <memory>

#include <inttypes.h>
#include <stdint.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

namespace android {
namespace crashreport {

namespace {

class HostCrashReporter : public CrashReporter {
public:
    bool attachCrashHandler(const CrashSystem::CrashPipe& crashpipe) override {
        if (mHandler) {
            return false;
        }

        ::android::base::Win32UnicodeString dumpDir(getDumpDir().c_str(),
                                                    getDumpDir().length());
        std::wstring dumpDir_wstr(dumpDir.c_str());

        ::android::base::Win32UnicodeString crashPipe(crashpipe.mClient.c_str(),
                                                    crashpipe.mClient.length());

        // google_breakpad::ExceptionHandler makes a local copy of dumpDir arg.
        // crashPipe arg is copied locally during ExceptionHandler's construction
        // of CrashGenerationClient.
        mHandler.reset(new google_breakpad::ExceptionHandler(
                dumpDir_wstr, &HostCrashReporter::exceptionFilterCallback,
                nullptr, nullptr,
                google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal,
                crashPipe.c_str(), nullptr));
        return mHandler != nullptr;
    }

    bool waitServicePipeReady(const std::string& pipename,
                              int timeout_ms) override {
        static_assert(kWaitIntervalMS > 0, "kWaitIntervalMS must be greater than 0");
        ::android::base::Win32UnicodeString pipename_unicode(pipename.c_str(),
                                                             pipename.length());
        for (; timeout_ms > 0; timeout_ms -= kWaitIntervalMS) {
            if (WaitNamedPipeW(pipename_unicode.c_str(), timeout_ms) != 0) {
                return true;
            }

            if (GetLastError() == ERROR_SEM_TIMEOUT) {
                // Pipe exists but timed out waiting to become ready
                return false;
            } else {
                // Pipe does not exist yet, sleep and try again
                ::android::base::System::get()->sleepMs(kWaitIntervalMS);
            }

        }
        return false;
    }

    void setupChildCrashProcess(int pid) override {}

    void writeDump() override { mHandler->WriteMinidump(); }

private:
    static bool exceptionFilterCallback(
        void*,
        EXCEPTION_POINTERS*,
        MDRawAssertionInfo*);

    bool onCrashPlatformSpecific() override;

    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler;
};

::android::base::LazyInstance<HostCrashReporter> sCrashReporter =
        LAZY_INSTANCE_INIT;

bool HostCrashReporter::exceptionFilterCallback(
    void*,
    EXCEPTION_POINTERS*,
    MDRawAssertionInfo*) {
    return CrashReporter::get()->onCrash();
}

static uint64_t toKB(uint64_t value) {
    return value / 1024;
}

static void attachMemoryInfo()
{
    PROCESS_MEMORY_COUNTERS_EX memCounters = {sizeof(memCounters)};
    char buf[1024] = {};
    char* bufptr = buf;

    uint64_t resident = 0;
    uint64_t resident_max = 0;
    uint64_t virt = 0;
    uint64_t virt_max = 0;
    uint64_t total_phys_memory = 0;
    uint64_t total_page_file = 0;

    if (::GetProcessMemoryInfo(::GetCurrentProcess(),
                reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters),
                sizeof(memCounters))) {

        uint64_t pageFileUsageCommit =
            memCounters.PagefileUsage
            ? memCounters.PagefileUsage
            : memCounters.PrivateUsage;

        resident = memCounters.WorkingSetSize;
        resident_max = memCounters.PeakWorkingSetSize;
        virt = pageFileUsageCommit;
        virt_max = memCounters.PeakPagefileUsage;
        
        bufptr += snprintf(bufptr, (int)sizeof(buf) - (bufptr - buf) - 1,
            "PageFaultCount: %" PRIu64 "\n"
            "PeakWorkingSetSize: %" PRIu64 " kB\n"
            "WorkingSetSize: %" PRIu64 " kB\n"
            "QuotaPeakPagedPoolUsage: %" PRIu64 " kB\n"
            "QuotaPagedPoolUsage: %" PRIu64 " kB\n"
            "QuotaPeakNonPagedPoolUsage: %" PRIu64 " kB\n"
            "QuotaNonPagedPoolUsage: %" PRIu64 " kB\n"
            "PagefileUsage (commit): %" PRIu64 " kB\n"
            "PeakPagefileUsage: %" PRIu64 " kB\n",
            uint64_t(memCounters.PageFaultCount),
            toKB(memCounters.PeakWorkingSetSize),
            toKB(memCounters.WorkingSetSize),
            toKB(memCounters.QuotaPeakPagedPoolUsage),
            toKB(memCounters.QuotaPagedPoolUsage),
            toKB(memCounters.QuotaPeakNonPagedPoolUsage),
            toKB(memCounters.QuotaNonPagedPoolUsage),
            toKB(pageFileUsageCommit),
            toKB(memCounters.PeakPagefileUsage));
    } else {
        bufptr += snprintf(bufptr, (int)sizeof(buf) - (bufptr - buf) - 1,
                           "\nGetProcessMemoryInfo() failed with error %u\n",
                           (unsigned)::GetLastError());
    }

    MEMORYSTATUSEX mem = {sizeof(mem)};
    if (::GlobalMemoryStatusEx(&mem)) {
        bufptr += snprintf(bufptr, (int)sizeof(buf) - (bufptr - buf) - 1,
          "\n"
          "TotalPhys: %" PRIu64 " kB\n"
          "AvailPhys: %" PRIu64 " kB\n"
          "TotalPageFile: %" PRIu64 " kB\n"
          "AvailPageFile: %" PRIu64 " kB\n"
          "TotalVirtual: %" PRIu64 " kB\n"
          "AvailVirtual: %" PRIu64 " kB\n",
          toKB(mem.ullTotalPhys),
          toKB(mem.ullAvailPhys),
          toKB(mem.ullTotalPageFile),
          toKB(mem.ullAvailPageFile),
          toKB(mem.ullTotalVirtual),
          toKB(mem.ullAvailVirtual));

        total_phys_memory = mem.ullTotalPhys;
        total_page_file = mem.ullTotalPageFile;
    } else {
        bufptr += snprintf(bufptr, (int)sizeof(buf) - (bufptr - buf) - 1,
                           "\nGlobalMemoryStatusEx() failed with error %u\n",
                           (unsigned)::GetLastError());
    }

    StructuredInfo::get()->setMemUsage(
            resident,
            resident_max,
            virt,
            virt_max,
            total_phys_memory,
            total_page_file);

    if (bufptr > buf) {
        CrashReporter::get()->attachData(
                    CrashReporter::kProcessMemoryInfoFileName, buf);
    }
}

bool HostCrashReporter::onCrashPlatformSpecific() {
    // collect the memory usage at the time of the crash
    attachMemoryInfo();
    return true;
}

}  // namespace anonymous

CrashReporter* CrashReporter::get() {
    return sCrashReporter.ptr();
}

}  // namespace crashreport
}  // namespace android
