#pragma once

#include <vulkan/vulkan.h>

#define LIST_REASSIGNABLE_TYPES(f) \
    f(VkInstance) \
    f(VkPhysicalDevice) \
    f(VkDevice) \
    f(VkQueue) \
    f(VkDeviceMemory) \
    f(VkCommandPool) \
    f(VkCommandBuffer) \
    f(VkBuffer) \
    f(VkBufferView) \
    f(VkImage) \
    f(VkImageView) \
    f(VkRenderPass) \
    f(VkFramebuffer) \
    f(VkDescriptorPool) \
    f(VkDescriptorSet) \
    f(VkDescriptorSetLayout) \
    f(VkShaderModule) \
    f(VkSampler) \
    f(VkPipelineLayout) \
    f(VkPipelineCache) \
    f(VkPipeline) \
    f(VkFence) \
    f(VkSemaphore) \
    f(VkEvent) \
    f(VkQueryPool) \

#define DEFINE_REASSIGNMENT_TYPEDEF(type) \
    typedef type (*reassign_##type##_t)(type);

#define DEFINE_REASSIGNMENT_CALLBACK_FIELD(type) \
    reassign_##type##_t reassign_##type = 0;

LIST_REASSIGNABLE_TYPES(DEFINE_REASSIGNMENT_TYPEDEF)

struct vk_reassign_funcs {
LIST_REASSIGNABLE_TYPES(DEFINE_REASSIGNMENT_CALLBACK_FIELD)
};

#define DECLARE_REMAP(type) \
void remap_##type(reassign_##type##_t f, uint32_t count, const type* in, type* out); \

LIST_REASSIGNABLE_TYPES(DECLARE_REMAP)

