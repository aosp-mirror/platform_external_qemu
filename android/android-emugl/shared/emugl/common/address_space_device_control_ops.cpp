// Copyright 2020 The Android Open Source Project
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

#include "emugl/common/address_space_device_control_ops.h"

namespace {

struct address_space_device_control_ops g_address_space_device_control_ops;

}  // namespace

void set_emugl_address_space_device_control_ops(struct address_space_device_control_ops* ops) {
    g_address_space_device_control_ops = *ops;
}

const struct address_space_device_control_ops &get_emugl_address_space_device_control_ops() {
    return g_address_space_device_control_ops;
}
