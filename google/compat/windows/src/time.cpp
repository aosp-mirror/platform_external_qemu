
// Copyright (C) 2023 The Android Open Source Project
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
#include "compat_compiler.h"
#include <Windows.h>
#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sys/time.h>
#include <thread>
#include <time.h>

ANDROID_BEGIN_HEADER

typedef int clockid_t;

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  assert(clk_id == CLOCK_MONOTONIC);
  auto now = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

  tp->tv_sec = static_cast<time_t>(duration.count());
  tp->tv_nsec =
      static_cast<long>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch() - duration)
                            .count());

  return 0; // Success
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
  // Validate input
  if (rqtp == NULL || rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1'000'000'000) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }

  // Create a persistent thread local timer object
  struct ThreadLocalTimerState {
    ThreadLocalTimerState() {
      timerHandle = CreateWaitableTimerEx(
          nullptr /* no security attributes */, nullptr /* no timer name */,
          CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

      if (!timerHandle) {
        // Use an older version of waitable timer as backup.
        timerHandle = CreateWaitableTimer(nullptr, FALSE, nullptr);
      }
    }

    ~ThreadLocalTimerState() {
      if (timerHandle) {
        CloseHandle(timerHandle);
      }
    }

    HANDLE timerHandle = 0;
  };

  static thread_local ThreadLocalTimerState tl_timerInfo;

  // Convert timespec to FILETIME
  ULARGE_INTEGER fileTime;
  fileTime.QuadPart =
      static_cast<ULONGLONG>(rqtp->tv_sec) * 10'000'000 + rqtp->tv_nsec / 100;

  if (!tl_timerInfo.timerHandle) {
    // Oh oh, for some reason we do not have a handle..
    return -1;
  }

  LARGE_INTEGER dueTime;
  // Note: Negative values indicate relative time.
  // (https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setwaitabletimer)
  dueTime.QuadPart = -static_cast<LONGLONG>(fileTime.QuadPart);

  if (!SetWaitableTimer(tl_timerInfo.timerHandle, &dueTime, 0, NULL, NULL,
                        FALSE)) {
    return -1;
  }

  if (WaitForSingleObject(tl_timerInfo.timerHandle, INFINITE) !=
      WAIT_OBJECT_0) {
    return -1; // Sleep interrupted or error
  }

  // Calculate remaining time if needed
  if (rmtp != NULL) {
    // Get current time
    FILETIME currentTime;
    GetSystemTimeAsFileTime(&currentTime);

    // Calculate remaining time
    ULARGE_INTEGER currentFileTime;
    currentFileTime.LowPart = currentTime.dwLowDateTime;
    currentFileTime.HighPart = currentTime.dwHighDateTime;

    ULONGLONG remainingTime = fileTime.QuadPart + currentFileTime.QuadPart;
    rmtp->tv_sec = static_cast<time_t>(remainingTime / 10'000'000);
    rmtp->tv_nsec = static_cast<long>((remainingTime % 10'000'000) * 100);
  }

  return 0;
}

int gettimeofday(struct timeval *tp, void * /* tzp */) {
  if (tp == nullptr) {
    return -1;
  }

  // Get the current time using std::chrono::system_clock
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      now.time_since_epoch());

  // Extract seconds and microseconds
  tp->tv_sec = static_cast<long>(duration.count() / 1'000'000);
  tp->tv_usec = static_cast<long>(duration.count() % 1'000'000);

  // Return success
  return 0;
}

void usleep(int64_t usec) {
  struct timespec req;
  req.tv_sec = static_cast<time_t>(usec / 1'000'000);
  req.tv_nsec = static_cast<long>((usec % 1'000'000) * 1000);

  nanosleep(&req, nullptr);
}

unsigned int sleep(unsigned int seconds) {
  Sleep(seconds * 1000);
  return 0;
}

ANDROID_END_HEADER