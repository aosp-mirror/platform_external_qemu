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

#include <stdint.h>     // for uint32_t
#include <chrono>       // for milliseconds, duration_cast, system_c...
#include <thread>       // for sleep_for
#include <type_traits>  // for enable_if<>::type

#include "nimble/nimble_npl.h"  // for BLE_NPL_OK, ble_npl_error_t, ble_npl_...
#include "nimble/os_types.h"    // for ble_npl_time_t

extern "C" {

/**
 * Return ticks [ms] since system start as uint32_t.
 */
ble_npl_time_t ble_npl_time_get(void) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis =
            std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                    .count();
    return millis;
}

ble_npl_error_t ble_npl_time_ms_to_ticks(uint32_t ms,
                                         ble_npl_time_t* out_ticks) {
    *out_ticks = ms;
    return BLE_NPL_OK;
}

ble_npl_error_t ble_npl_time_ticks_to_ms(ble_npl_time_t ticks,
                                         uint32_t* out_ms) {
    *out_ms = ticks;
    return BLE_NPL_OK;
}

ble_npl_time_t ble_npl_time_ms_to_ticks32(uint32_t ms) {
    return ms;
}

uint32_t ble_npl_time_ticks_to_ms32(ble_npl_time_t ticks) {
    return ticks;
}

void ble_npl_time_delay(ble_npl_time_t ticks) {
    long ms = ble_npl_time_ticks_to_ms32(ticks);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
}
