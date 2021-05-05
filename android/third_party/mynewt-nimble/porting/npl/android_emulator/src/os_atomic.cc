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

#include <stdint.h>             // for uint32_t
#include <mutex>                // for recursive_mutex

#include "nimble/nimble_npl.h"  // for ble_npl_hw_enter_critical, ble_npl_hw...

std::recursive_mutex s_mutex;

extern "C" {

uint32_t ble_npl_hw_enter_critical(void) {
    s_mutex.lock();
    return 0;
}

void ble_npl_hw_exit_critical(uint32_t ctx) {
    s_mutex.unlock();
}
}
