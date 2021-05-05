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

#include <stdint.h>                // for uint32_t
#include <chrono>                  // for milliseconds
#include <mutex>                   // for timed_mutex

#include "nimble/nimble_npl.h"     // for BLE_NPL_INVALID_PARAM, BLE_NPL_OK
#include "nimble/nimble_npl_os.h"  // for BLE_NPL_TIME_FOREVER
#include "nimble/os_types.h"       // for ble_npl_mutex

ble_npl_error_t ble_npl_mutex_init(struct ble_npl_mutex* mu) {
    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    mu->lock = new std::timed_mutex();
    return BLE_NPL_OK;
}

ble_npl_error_t ble_npl_mutex_release(struct ble_npl_mutex* mu) {
    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    std::timed_mutex* mtx = reinterpret_cast<std::timed_mutex*>(mu->lock);
    mtx->unlock();
    mu->mu_owner = NULL;
    return BLE_NPL_OK;
}

ble_npl_error_t ble_npl_mutex_pend(struct ble_npl_mutex* mu, uint32_t timeout) {
    int err;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    std::timed_mutex* mtx = reinterpret_cast<std::timed_mutex*>(mu->lock);
    if (timeout == BLE_NPL_TIME_FOREVER) {
        mtx->lock();
    } else {
        if (!mtx->try_lock_for(std::chrono::milliseconds(timeout))) {
            return BLE_NPL_TIMEOUT;
        }
    }
    mu->mu_owner = ble_npl_get_current_task_id();
    return BLE_NPL_OK;
}
