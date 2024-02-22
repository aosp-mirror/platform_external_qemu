// Copyright 2024 The Android Open Source Project
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
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include "host-common/crash-handler.h"
#include "android/utils/debug.h"

namespace android {
namespace crashreport {

/**
 * @brief Monitors a task's execution and triggers a custom action upon timeout.
 *
 * The TimeoutMonitor class provides a mechanism to enforce a time limit on
 * a task's execution. If the task does not complete within the specified
 * timeout, a provided callback function is called. This can be used to ensure
 * timely execution of critical sections or to recover from hanging operations.
 *
 * Note: This will spin up a thread.
 */
class TimeoutMonitor {
public:
    /**
     * @brief Constructs a TimeoutMonitor object.
     *
     * @param timeout The maximum allowed execution time.
     * @param callback The function to be called upon timeout.
     */
    TimeoutMonitor(std::chrono::milliseconds timeout,
                   std::function<void()> callback);
    ~TimeoutMonitor();

private:
    std::chrono::milliseconds mTimeoutMs;
    bool mThreadRunning;
    std::thread mWatcherThread;
    std::condition_variable mCv;
    std::mutex mMutex;
    std::function<void()> mTimeoutFn;
};


/**
 * @brief Monitors a task's execution and triggers a crash upon timeout.
 */
class CrashOnTimeout : public TimeoutMonitor {
public:
    /**
     * @brief Constructs a CrashOnTimeout object.
     *
     * This will invoke the crash handler with a message if a task failed to complete
     * in the alotted time. Example usage:
     *
     *  TimeoutMonitor monitor(50ms, "Too slow!");
     *  std::this_thread::sleep_for(10ms);
     *
     * @param timeout The maximum allowed execution time.
     * @param Message to be included when the crash method is called.
     */
    CrashOnTimeout(std::chrono::milliseconds timeout, std::string message)
        : TimeoutMonitor(timeout, [msg = message, t = timeout]() {
              auto timeout_msg = "Task timeout after " +
                                 std::to_string(t.count()) + " ms. :" + msg;
              crashhandler_die(timeout_msg.c_str());
          }) {}
};
}  // namespace crashreport
}  // namespace android