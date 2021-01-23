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
#pragma once

#include <vulkan/vulkan.h>

#include "VulkanHandles.h"
#include "VulkanDispatch.h"

#include <functional>

namespace goldfish_vk {

class VkDecoderGlobalState;

class VulkanHandleMapping {
public:
    VulkanHandleMapping(VkDecoderGlobalState* state) : m_state (state) { }
    virtual ~VulkanHandleMapping() { }

#define DECLARE_HANDLE_MAP_PURE_VIRTUAL_METHOD(type) \
    virtual void mapHandles_##type(type* handles, size_t count = 1) = 0; \
    virtual void mapHandles_##type##_u64(const type* handles, uint64_t* handle_u64s, size_t count = 1) = 0; \
    virtual void mapHandles_u64_##type(const uint64_t* handle_u64s, type* handles, size_t count = 1) = 0; \

    GOLDFISH_VK_LIST_HANDLE_TYPES(DECLARE_HANDLE_MAP_PURE_VIRTUAL_METHOD)
protected:
    VkDecoderGlobalState* m_state;
};

class DefaultHandleMapping : public VulkanHandleMapping {
public:
    DefaultHandleMapping() : VulkanHandleMapping(nullptr) { }
    virtual ~DefaultHandleMapping() { }

#define DECLARE_HANDLE_MAP_OVERRIDE(type) \
    void mapHandles_##type(type* handles, size_t count) override; \
    void mapHandles_##type##_u64(const type* handles, uint64_t* handle_u64s, size_t count) override; \
    void mapHandles_u64_##type(const uint64_t* handle_u64s, type* handles, size_t count) override; \

    GOLDFISH_VK_LIST_HANDLE_TYPES(DECLARE_HANDLE_MAP_OVERRIDE)
};

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_GLOBAL_API_DECL(type) \
    type unbox_##type(type boxed); \
    type unboxed_to_boxed_##type(type boxed); \
    void delete_##type(type boxed); \
    VulkanDispatch* dispatch_##type(type boxed); \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_GLOBAL_API_DECL)

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_GLOBAL_API_DECL(type) \
    type new_boxed_non_dispatchable_##type(type underlying); \
    void delete_##type(type boxed); \
    void delayed_delete_##type(type boxed, VkDevice device, std::function<void()> callback); \
    type unbox_##type(type boxed); \
    type unboxed_to_boxed_non_dispatchable_##type(type boxed); \

GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_GLOBAL_API_DECL)

} // namespace goldfish_vk
