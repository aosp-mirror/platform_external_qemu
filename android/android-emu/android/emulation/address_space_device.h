// Copyright 2019 The Android Open Source Project
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
#pragma once

#include <inttypes.h>

extern "C" {

typedef uint32_t (*address_space_device_gen_handle_t)(void);
typedef void (*address_space_device_destroy_handle_t)(uint32_t);
typedef void (*address_space_device_tell_ping_info_t)(uint32_t handle, uint64_t gpa);
typedef void (*address_space_device_ping_t)(uint32_t handle);

struct address_space_device_control_ops {
    address_space_device_gen_handle_t gen_handle;
    address_space_device_destroy_handle_t destroy_handle;
    address_space_device_tell_ping_info_t tell_ping_info;
    address_space_device_ping_t ping;
};

struct address_space_device_control_ops*
create_or_get_address_space_device_control_ops(void);

namespace android {

namespace base {

class Stream;

} // namespace base

namespace emulation {

void host_goldfish_address_space_memory_state_load(android::base::Stream *stream);
void host_goldfish_address_space_memory_state_save(android::base::Stream *stream);

} // namespace emulation
} // namespace android

} // extern "C"
