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
#pragma once

#include <vulkan.h>

namespace goldfish_vk {

class VulkanHandleMapping {
public:
    VulkanHandleMapping() = default;

    virtual void mapHandles_VkInstance(VkInstance* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkDevice(VkDevice* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkQueue(VkQueue* handles, size_t count = 1) = 0;

    virtual void mapHandles_VkSemaphore(VkSemaphore* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkCommandBuffer(VkCommandBuffer* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkDeviceMemory(VkDeviceMemory* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkBuffer(VkBuffer* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkImage(VkImage* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkShaderModule(VkShaderModule* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkPipelineLayout(VkPipelineLayout* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkRenderPass(VkRenderPass* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkPipeline(VkPipeline* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkDescriptorSetLayout(VkDescriptorSetLayout* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkSampler(VkSampler* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkDescriptorPool(VkDescriptorPool* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkImageView(VkImageView* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkDescriptorSet(VkDescriptorSet* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkBufferView(VkBufferView* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkCommandPool(VkCommandPool* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkFramebuffer(VkFramebuffer* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkPhysicalDevice(VkPhysicalDevice* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkSamplerYcbcrConversion(VkSamplerYcbcrConversion* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkFence(VkFence* handles, size_t count = 1) = 0;

#ifdef VK_KHR_swapchain
    virtual void mapHandles_VkSurfaceKHR(VkSurfaceKHR* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkSwapchainKHR(VkSwapchainKHR* handles, size_t count = 1) = 0;
#endif

#ifdef VK_KHR_display
    virtual void mapHandles_VkDisplayKHR(VkDisplayKHR* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkDisplayModeKHR(VkDisplayModeKHR* handles, size_t count = 1) = 0;
#endif

#ifdef VK_NVX_device_generated_commands
    virtual void mapHandles_VkObjectTableNVX(VkObjectTableNVX* handles, size_t count = 1) = 0;
    virtual void mapHandles_VkIndirectCommandsLayoutNVX(VkIndirectCommandsLayoutNVX* handles, size_t count = 1) = 0;
#endif

#ifdef VK_EXT_validation_cache
    virtual void mapHandles_VkValidationCacheEXT(VkValidationCacheEXT* handles, size_t count = 1) = 0;
#endif

};

} // namespace goldfish_vk
