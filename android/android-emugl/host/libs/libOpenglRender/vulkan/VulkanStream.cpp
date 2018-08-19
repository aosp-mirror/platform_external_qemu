// Copyright (C) 2018 The Android Open Source Project
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

#include "VulkanStream.h"

namespace goldfish_vk {

VulkanStream::VulkanStream() {
}

void VulkanStream::putStringArray(const char* const* strings, uint32_t count) {
    putBe32(count);
    for (uint32_t i = 0; i < count; ++i) {
        putString(strings[i]);
    }
}

std::vector<std::string> VulkanStream::getStringArray() {
    uint32_t count = getBe32();
    std::vector<std::string> res;
    for (uint32_t i = 0; i < count; ++i) {
        res.push_back(getString());
    }
    return res;
}

} // namespace goldfish_vk
