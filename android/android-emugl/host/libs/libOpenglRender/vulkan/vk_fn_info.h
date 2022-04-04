// Copyright 2022 The Android Open Source Project
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

#ifndef VK_FN_INFO_H
#define VK_FN_INFO_H

#include <vulkan/vulkan.h>

#include <initializer_list>
#include <tuple>

namespace vk_util {
namespace vk_fn_info {
template <class T>
struct GetVkFnInfo;

#define REGISTER_VK_FN_INFO(coreName, allNames)                 \
    struct coreName;                                            \
    template <>                                                 \
    struct GetVkFnInfo<coreName> {                              \
        static constexpr auto names = std::make_tuple allNames; \
        using type = PFN_vk##coreName;                          \
    };

REGISTER_VK_FN_INFO(GetPhysicalDeviceProperties2,
                    ("vkGetPhysicalDeviceProperties2KHR", "vkGetPhysicalDeviceProperties2"))
REGISTER_VK_FN_INFO(GetPhysicalDeviceImageFormatProperties2,
                    ("vkGetPhysicalDeviceImageFormatProperties2KHR",
                     "vkGetPhysicalDeviceImageFormatProperties2"))
}  // namespace vk_fn_info
}  // namespace vk_util

#endif /* VK_FN_INFO_H */