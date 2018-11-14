// Copyright (C) 2018 The Android Open Source Project
// Copyright (C) 2018 Google Inc.
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
#include <vulkan/vulkan.h>

#include "VulkanHandleMapping.h"

namespace goldfish_vk {

#define DEFAULT_HANDLE_MAP_DEFINE(type) \
void DefaultHandleMapping::mapHandles_##type(type*, size_t) { return; } \
void DefaultHandleMapping::mapHandles_##type##_u64(const type* handles, uint64_t* handle_u64s, size_t count) { \
    for (size_t i = 0; i < count; ++i) { handle_u64s[i] = (uint64_t)(uintptr_t)handles[i]; } \
} \
void DefaultHandleMapping::mapHandles_u64_##type(const uint64_t* handle_u64s, type* handles, size_t count) { \
    for (size_t i = 0; i < count; ++i) { handles[i] = (type)(uintptr_t)handle_u64s[i]; } \
} \

GOLDFISH_VK_LIST_HANDLE_TYPES(DEFAULT_HANDLE_MAP_DEFINE)

} // namespace goldfish_vk
