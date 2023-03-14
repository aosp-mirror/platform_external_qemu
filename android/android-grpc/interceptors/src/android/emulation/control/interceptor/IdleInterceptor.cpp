// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/interceptor/IdleInterceptor.h"

#include <cstdint>     // for uint64_t
#include <functional>  // for __base
#include <ratio>       // for ratio

#include "aemu/base/Log.h"                         // for LOG, LogMessage
#include "aemu/base/async/ThreadLooper.h"          // for ThreadLooper
#include "android/base/system/System.h"               // for System
#include "host-common/vm_operations.h"  // for QAndroidVmOpera...

namespace android {
namespace control {
namespace interceptor {

using namespace grpc::experimental;

IdleInterceptor::IdleInterceptor(std::chrono::seconds timeout,
                                 std::atomic<uint64_t>* terminationUnixTime,
                                 std::atomic<uint64_t>* activeRequests)
    : mTimeout(timeout),
      mTerminationUnixTime(terminationUnixTime),
      mActiveRequests(activeRequests) {}

IdleInterceptor::~IdleInterceptor() {
    auto idleTime = System::get()->getUnixTime() + mTimeout.count();
    mTerminationUnixTime->store(idleTime);
    mActiveRequests->fetch_sub(1);
}

void IdleInterceptor::Intercept(InterceptorBatchMethods* methods) {
    methods->Proceed();
}

IdleInterceptorFactory::IdleInterceptorFactory(
        std::chrono::seconds timeout,
        const AndroidConsoleAgents* agents)
    : mTimeout(timeout),
      mAgents(agents),
      mTerminationUnixTime(System::get()->getUnixTime() + timeout.count()),
      mTimeoutChecker(
              android::base::ThreadLooper::get(),
              [=]() { return checkIdleTimeout(); },
              std::chrono::milliseconds(mTimeout).count()) {
    mTimeoutChecker.start();
}

Interceptor* IdleInterceptorFactory::CreateServerInterceptor(
        ServerRpcInfo* info) {
    mActiveRequests++;
    return new IdleInterceptor(mTimeout, &mTerminationUnixTime,
                               &mActiveRequests);
}

bool IdleInterceptorFactory::checkIdleTimeout() {
    auto epoch = System::get()->getUnixTime();
    if (mActiveRequests > 0 || epoch < mTerminationUnixTime)
        return true;

    LOG(WARNING) << "Idled to long, shutting down. " << epoch << " > "
                 << mTerminationUnixTime;
    if (mShutdownAttempt == 0) {
        LOG(WARNING) << "Trying nicely..";
        mAgents->vm->vmShutdown();
    } else {
        LOG(INFO) << "Terminating the emulator.";
        auto pid = System::get()->getCurrentProcessId();
        System::get()->killProcess(pid);
    }

    mShutdownAttempt++;
    return true;
}

}  // namespace interceptor
}  // namespace control
}  // namespace android
