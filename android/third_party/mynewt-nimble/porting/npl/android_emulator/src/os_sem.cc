// Copyright (C) 2021 The Android Open Source Project
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

// This contains abstractions to run NimBLE against rootcanal (emulated
// bluetooth chip)

#include <assert.h>                // for assert
#include <stdint.h>                // for uint16_t, uint32_t
#include <chrono>                  // for milliseconds, hours
#include <mutex>                   // for condition_variable, mutex, lock_guard
#include <condition_variable>
#include <ratio>                   // for ratio

#include "nimble/nimble_npl.h"     // for BLE_NPL_INVALID_PARAM, BLE_NPL_OK
#include "nimble/nimble_npl_os.h"  // for BLE_NPL_TIME_FOREVER
#include "nimble/os_types.h"       // for ble_npl_sem

// Ye olde semaphore
class Semaphore {
public:
    Semaphore(int c) : mCount(c) {}

    int count() { return mCount; }

    bool acquire_for(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCv.wait_for(lock, timeout, [&]() { return mCount > 0; })) {
            return false;
        }

        mCount--;
        return true;
    }

    void release() {
        const std::lock_guard<std::mutex> lock(mMutex);
        mCount++;
        mCv.notify_one();
    }

    std::condition_variable mCv;
    std::mutex mMutex;
    int mCount;
};

ble_npl_error_t ble_npl_sem_init(struct ble_npl_sem* sem, uint16_t tokens) {
    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    sem->lock = new Semaphore(tokens);
    return BLE_NPL_OK;
}

ble_npl_error_t ble_npl_sem_release(struct ble_npl_sem* sem) {
    int err;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    Semaphore* s = reinterpret_cast<Semaphore*>(sem->lock);
    s->release();

    return BLE_NPL_OK;
}

ble_npl_error_t ble_npl_sem_pend(struct ble_npl_sem* sem, uint32_t timeout) {
    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    Semaphore* s = reinterpret_cast<Semaphore*>(sem->lock);
    if (timeout == BLE_NPL_TIME_FOREVER) {
        s->acquire_for(std::chrono::hours(24 * 365));
    } else {
        if (!s->acquire_for(std::chrono::milliseconds(timeout))) {
            return BLE_NPL_TIMEOUT;
        }
    }

    return BLE_NPL_OK;
}

uint16_t ble_npl_sem_get_count(struct ble_npl_sem* sem) {
    int count;

    assert(sem);
    assert(&sem->lock);
    Semaphore* s = reinterpret_cast<Semaphore*>(sem->lock);

    return s->count();
}
