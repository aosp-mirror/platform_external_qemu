// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/base/containers/Lookup.h"

#include <unordered_map>

namespace vk_emu {

// Class that defines per-Vulkan-object auxiliary data that is useful
// for Vulkan emulation.
// This is to organize our custom logic for Vulkan emulation in a tractable
// manner, using Entity-Component-System style, where entities are Vulkan
// handles, components are auxiliary data, and the system is
// VkDecoderGlobalState.
// Subclasses of VkDecoderComponent can then communicate with each other.
template <class VulkanObject, class AuxiliaryData>
class VkDecoderComponent {
public:
    VkDecoderComponent() = default;

    void set(VulkanObject o, AuxiliaryData d) {
        mData[o] = d;
    }

    void erase(VulkanObject o) {
        mData.erase(o);
    }

    AuxiliaryData* get(VulkanObject o) {
        return android::base::find(mData, o);
    }

    bool exists(VulkanObject o) const {
        return mData.find(o) != mData.end();
    }

private:
    std::unordered_map<VulkanObject, AuxiliaryData> mData;
};

} // namespace vk_emu