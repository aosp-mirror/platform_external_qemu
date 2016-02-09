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
#include "android/utils/debug.h"
#include "client/mac/handler/exception_handler.h"

#include <mach/mach.h>

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
    HostCrashReporter() : CrashReporter(), mHandler() {}

    virtual ~HostCrashReporter() {}

    bool attachCrashHandler(const CrashSystem::CrashPipe& crashpipe) override {
        if (mHandler) {
            return false;
        }

        mHandler.reset(new google_breakpad::ExceptionHandler(
                getDumpDir(), &HostCrashReporter::exceptionFilterCallback,
                nullptr,  // no minidump callback
                nullptr,  // no callback context
                true,     // install signal handlers
                crashpipe.mClient.c_str()));

        return mHandler != nullptr;
    }

    bool waitServicePipeReady(const std::string& pipename,
                              int timeout_ms) override {
        static_assert(kWaitIntervalMS > 0, "kWaitIntervalMS must be greater than 0");
        mach_port_t task_bootstrap_port = 0;
        mach_port_t port;
        task_get_bootstrap_port(mach_task_self(), &task_bootstrap_port);
        for (; timeout_ms > 0; timeout_ms -= kWaitIntervalMS) {
            if (bootstrap_look_up(task_bootstrap_port, pipename.c_str(),
                                  &port) == KERN_SUCCESS) {
                return true;
            }
            ::android::base::System::sleepMs(kWaitIntervalMS);
        }
        return false;
    }

    void setupChildCrashProcess(int pid) override {}

    void writeDump() override { mHandler->WriteMinidump(); }

   static bool exceptionFilterCallback(void* context);

private:
    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler;
};

::android::base::LazyInstance<HostCrashReporter> sCrashReporter =
        LAZY_INSTANCE_INIT;

bool HostCrashReporter::exceptionFilterCallback(void* context) {
    (void)context;

    // collect the memory usage at the time of the crash
    rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        char buf[1024] = {};
        snprintf(buf, sizeof(buf) - 1,
                "==== Process memory usage ====\n"
                "max resident set size = %" PRIu64 " kB\n"
                "integral shared text memory size = %" PRIu64 " kB\n"
                "integral unshared data size = %" PRIu64 " kB\n"
                "integral unshared stack size = %" PRIu64 " kB\n"
                "page reclaims = %" PRIu64 " kB\n"
                "page faults = %" PRIu64 " kB\n"
                "swaps = %" PRIu64 " kB\n"
                "block input operations = %" PRIu64 " kB\n"
                "block output operations = %" PRIu64 " kB\n"
                "messages sent = %" PRIu64 " kB\n"
                "messages received = %" PRIu64 " kB\n"
                "signals received = %" PRIu64 " kB\n"
                "voluntary context switches = %" PRIu64 " kB\n"
                "involuntary context switches = %" PRIu64 " kB\n",
                 uint64_t(usage.ru_maxrss / 1024),
                 uint64_t(usage.ru_ixrss / 1024),
                 uint64_t(usage.ru_idrss / 1024),
                 uint64_t(usage.ru_isrss / 1024),
                 uint64_t(usage.ru_minflt / 1024),
                 uint64_t(usage.ru_majflt / 1024),
                 uint64_t(usage.ru_nswap / 1024),
                 uint64_t(usage.ru_inblock / 1024),
                 uint64_t(usage.ru_oublock / 1024),
                 uint64_t(usage.ru_msgsnd / 1024),
                 uint64_t(usage.ru_msgrcv / 1024),
                 uint64_t(usage.ru_nsignals / 1024),
                 uint64_t(usage.ru_nvcsw / 1024),
                 uint64_t(usage.ru_nivcsw / 1024));

        CrashReporter::get()->attachData(
                    CrashReporter::kProcessMemoryInfoFileName, buf);
    }

    return true;    // proceed with handling the crash
}

}  // namespace anonymous

CrashReporter* CrashReporter::get() {
    return sCrashReporter.ptr();
}

}  // namespace crashreport
}  // namespace android
