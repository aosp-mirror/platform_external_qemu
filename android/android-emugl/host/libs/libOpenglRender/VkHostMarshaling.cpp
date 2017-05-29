
#include "VkHostMarshaling.h"
#include "VkVirtualHandle.h"

#include <string.h>

template <> void VkVirtualHandleToGuestArray<VkInstance>(uint32_t count, const VkInstance* in, VkInstance* out);

template <> VkInstance* AllocForHost<VkInstance>(uint32_t count, const VkInstance* in);

template <> void VkVirtualHandleToGuestArray<VkPhysicalDevice>(uint32_t count, const VkPhysicalDevice* in, VkPhysicalDevice* out);

template <> VkPhysicalDevice* AllocForHost<VkPhysicalDevice>(uint32_t count, const VkPhysicalDevice* in);

template <> void VkVirtualHandleToGuestArray<VkDevice>(uint32_t count, const VkDevice* in, VkDevice* out);

template <> VkDevice* AllocForHost<VkDevice>(uint32_t count, const VkDevice* in);

template <> void VkVirtualHandleToGuestArray<VkQueue>(uint32_t count, const VkQueue* in, VkQueue* out);

template <> VkQueue* AllocForHost<VkQueue>(uint32_t count, const VkQueue* in);

template <> void VkVirtualHandleToGuestArray<VkDeviceMemory>(uint32_t count, const VkDeviceMemory* in, VkDeviceMemory* out);

template <> VkDeviceMemory* AllocForHost<VkDeviceMemory>(uint32_t count, const VkDeviceMemory* in);

template <> void VkVirtualHandleToGuestArray<VkCommandPool>(uint32_t count, const VkCommandPool* in, VkCommandPool* out);

template <> VkCommandPool* AllocForHost<VkCommandPool>(uint32_t count, const VkCommandPool* in);

template <> void VkVirtualHandleToGuestArray<VkCommandBuffer>(uint32_t count, const VkCommandBuffer* in, VkCommandBuffer* out);

template <> VkCommandBuffer* AllocForHost<VkCommandBuffer>(uint32_t count, const VkCommandBuffer* in);

template <> void VkVirtualHandleToGuestArray<VkBuffer>(uint32_t count, const VkBuffer* in, VkBuffer* out);

template <> VkBuffer* AllocForHost<VkBuffer>(uint32_t count, const VkBuffer* in);

template <> void VkVirtualHandleToGuestArray<VkBufferView>(uint32_t count, const VkBufferView* in, VkBufferView* out);

template <> VkBufferView* AllocForHost<VkBufferView>(uint32_t count, const VkBufferView* in);

template <> void VkVirtualHandleToGuestArray<VkImage>(uint32_t count, const VkImage* in, VkImage* out);

template <> VkImage* AllocForHost<VkImage>(uint32_t count, const VkImage* in);

template <> void VkVirtualHandleToGuestArray<VkImageView>(uint32_t count, const VkImageView* in, VkImageView* out);

template <> VkImageView* AllocForHost<VkImageView>(uint32_t count, const VkImageView* in);

template <> void VkVirtualHandleToGuestArray<VkRenderPass>(uint32_t count, const VkRenderPass* in, VkRenderPass* out);

template <> VkRenderPass* AllocForHost<VkRenderPass>(uint32_t count, const VkRenderPass* in);

template <> void VkVirtualHandleToGuestArray<VkFramebuffer>(uint32_t count, const VkFramebuffer* in, VkFramebuffer* out);

template <> VkFramebuffer* AllocForHost<VkFramebuffer>(uint32_t count, const VkFramebuffer* in);

template <> void VkVirtualHandleToGuestArray<VkDescriptorPool>(uint32_t count, const VkDescriptorPool* in, VkDescriptorPool* out);

template <> VkDescriptorPool* AllocForHost<VkDescriptorPool>(uint32_t count, const VkDescriptorPool* in);

template <> void VkVirtualHandleToGuestArray<VkDescriptorSetLayout>(uint32_t count, const VkDescriptorSetLayout* in, VkDescriptorSetLayout* out);

template <> VkDescriptorSetLayout* AllocForHost<VkDescriptorSetLayout>(uint32_t count, const VkDescriptorSetLayout* in);

template <> void VkVirtualHandleToGuestArray<VkDescriptorSet>(uint32_t count, const VkDescriptorSet* in, VkDescriptorSet* out);

template <> VkDescriptorSet* AllocForHost<VkDescriptorSet>(uint32_t count, const VkDescriptorSet* in);

template <> void VkVirtualHandleToGuestArray<VkPipelineLayout>(uint32_t count, const VkPipelineLayout* in, VkPipelineLayout* out);

template <> VkPipelineLayout* AllocForHost<VkPipelineLayout>(uint32_t count, const VkPipelineLayout* in);

template <> void VkVirtualHandleToGuestArray<VkPipelineCache>(uint32_t count, const VkPipelineCache* in, VkPipelineCache* out);

template <> VkPipelineCache* AllocForHost<VkPipelineCache>(uint32_t count, const VkPipelineCache* in);

template <> void VkVirtualHandleToGuestArray<VkPipeline>(uint32_t count, const VkPipeline* in, VkPipeline* out);

template <> VkPipeline* AllocForHost<VkPipeline>(uint32_t count, const VkPipeline* in);

template <> void VkVirtualHandleToGuestArray<VkFence>(uint32_t count, const VkFence* in, VkFence* out);

template <> VkFence* AllocForHost<VkFence>(uint32_t count, const VkFence* in);

template <> void VkVirtualHandleToGuestArray<VkSemaphore>(uint32_t count, const VkSemaphore* in, VkSemaphore* out);

template <> VkSemaphore* AllocForHost<VkSemaphore>(uint32_t count, const VkSemaphore* in);

template <> void VkVirtualHandleToGuestArray<VkEvent>(uint32_t count, const VkEvent* in, VkEvent* out);

template <> VkEvent* AllocForHost<VkEvent>(uint32_t count, const VkEvent* in);

template <> void VkVirtualHandleToGuestArray<VkQueryPool>(uint32_t count, const VkQueryPool* in, VkQueryPool* out);

template <> VkQueryPool* AllocForHost<VkQueryPool>(uint32_t count, const VkQueryPool* in);

template <> void VkVirtualHandleToGuestArray<VkShaderModule>(uint32_t count, const VkShaderModule* in, VkShaderModule* out);

template <> VkShaderModule* AllocForHost<VkShaderModule>(uint32_t count, const VkShaderModule* in);

template <> void VkVirtualHandleToGuestArray<VkSampler>(uint32_t count, const VkSampler* in, VkSampler* out);

template <> VkSampler* AllocForHost<VkSampler>(uint32_t count, const VkSampler* in);

template <> void VkVirtualHandleToGuestArray<VkInstance>(uint32_t count, const VkInstance* in, VkInstance* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkInstance)(uintptr_t)VkVirtualHandleCreateOrGet<VkInstance>(in[i]);
    }
}

template <> VkInstance* AllocForHost<VkInstance>(uint32_t count, const VkInstance* in) {
    if (!in) return nullptr;
    VkInstance* res = new VkInstance[count]; memcpy(res, in, count * sizeof(VkInstance));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkInstance>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkPhysicalDevice>(uint32_t count, const VkPhysicalDevice* in, VkPhysicalDevice* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkPhysicalDevice)(uintptr_t)VkVirtualHandleCreateOrGet<VkPhysicalDevice>(in[i]);
    }
}

template <> VkPhysicalDevice* AllocForHost<VkPhysicalDevice>(uint32_t count, const VkPhysicalDevice* in) {
    if (!in) return nullptr;
    VkPhysicalDevice* res = new VkPhysicalDevice[count]; memcpy(res, in, count * sizeof(VkPhysicalDevice));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkDevice>(uint32_t count, const VkDevice* in, VkDevice* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkDevice)(uintptr_t)VkVirtualHandleCreateOrGet<VkDevice>(in[i]);
    }
}

template <> VkDevice* AllocForHost<VkDevice>(uint32_t count, const VkDevice* in) {
    if (!in) return nullptr;
    VkDevice* res = new VkDevice[count]; memcpy(res, in, count * sizeof(VkDevice));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkQueue>(uint32_t count, const VkQueue* in, VkQueue* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkQueue)(uintptr_t)VkVirtualHandleCreateOrGet<VkQueue>(in[i]);
    }
}

template <> VkQueue* AllocForHost<VkQueue>(uint32_t count, const VkQueue* in) {
    if (!in) return nullptr;
    VkQueue* res = new VkQueue[count]; memcpy(res, in, count * sizeof(VkQueue));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkQueue>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkDeviceMemory>(uint32_t count, const VkDeviceMemory* in, VkDeviceMemory* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkDeviceMemory)(uintptr_t)VkVirtualHandleCreateOrGet<VkDeviceMemory>(in[i]);
    }
}

template <> VkDeviceMemory* AllocForHost<VkDeviceMemory>(uint32_t count, const VkDeviceMemory* in) {
    if (!in) return nullptr;
    VkDeviceMemory* res = new VkDeviceMemory[count]; memcpy(res, in, count * sizeof(VkDeviceMemory));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkCommandPool>(uint32_t count, const VkCommandPool* in, VkCommandPool* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkCommandPool)(uintptr_t)VkVirtualHandleCreateOrGet<VkCommandPool>(in[i]);
    }
}

template <> VkCommandPool* AllocForHost<VkCommandPool>(uint32_t count, const VkCommandPool* in) {
    if (!in) return nullptr;
    VkCommandPool* res = new VkCommandPool[count]; memcpy(res, in, count * sizeof(VkCommandPool));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkCommandPool>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkCommandBuffer>(uint32_t count, const VkCommandBuffer* in, VkCommandBuffer* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkCommandBuffer)(uintptr_t)VkVirtualHandleCreateOrGet<VkCommandBuffer>(in[i]);
    }
}

template <> VkCommandBuffer* AllocForHost<VkCommandBuffer>(uint32_t count, const VkCommandBuffer* in) {
    if (!in) return nullptr;
    VkCommandBuffer* res = new VkCommandBuffer[count]; memcpy(res, in, count * sizeof(VkCommandBuffer));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkBuffer>(uint32_t count, const VkBuffer* in, VkBuffer* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkBuffer)(uintptr_t)VkVirtualHandleCreateOrGet<VkBuffer>(in[i]);
    }
}

template <> VkBuffer* AllocForHost<VkBuffer>(uint32_t count, const VkBuffer* in) {
    if (!in) return nullptr;
    VkBuffer* res = new VkBuffer[count]; memcpy(res, in, count * sizeof(VkBuffer));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkBufferView>(uint32_t count, const VkBufferView* in, VkBufferView* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkBufferView)(uintptr_t)VkVirtualHandleCreateOrGet<VkBufferView>(in[i]);
    }
}

template <> VkBufferView* AllocForHost<VkBufferView>(uint32_t count, const VkBufferView* in) {
    if (!in) return nullptr;
    VkBufferView* res = new VkBufferView[count]; memcpy(res, in, count * sizeof(VkBufferView));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkBufferView>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkImage>(uint32_t count, const VkImage* in, VkImage* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkImage)(uintptr_t)VkVirtualHandleCreateOrGet<VkImage>(in[i]);
    }
}

template <> VkImage* AllocForHost<VkImage>(uint32_t count, const VkImage* in) {
    if (!in) return nullptr;
    VkImage* res = new VkImage[count]; memcpy(res, in, count * sizeof(VkImage));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkImageView>(uint32_t count, const VkImageView* in, VkImageView* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkImageView)(uintptr_t)VkVirtualHandleCreateOrGet<VkImageView>(in[i]);
    }
}

template <> VkImageView* AllocForHost<VkImageView>(uint32_t count, const VkImageView* in) {
    if (!in) return nullptr;
    VkImageView* res = new VkImageView[count]; memcpy(res, in, count * sizeof(VkImageView));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkImageView>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkRenderPass>(uint32_t count, const VkRenderPass* in, VkRenderPass* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkRenderPass)(uintptr_t)VkVirtualHandleCreateOrGet<VkRenderPass>(in[i]);
    }
}

template <> VkRenderPass* AllocForHost<VkRenderPass>(uint32_t count, const VkRenderPass* in) {
    if (!in) return nullptr;
    VkRenderPass* res = new VkRenderPass[count]; memcpy(res, in, count * sizeof(VkRenderPass));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkFramebuffer>(uint32_t count, const VkFramebuffer* in, VkFramebuffer* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkFramebuffer)(uintptr_t)VkVirtualHandleCreateOrGet<VkFramebuffer>(in[i]);
    }
}

template <> VkFramebuffer* AllocForHost<VkFramebuffer>(uint32_t count, const VkFramebuffer* in) {
    if (!in) return nullptr;
    VkFramebuffer* res = new VkFramebuffer[count]; memcpy(res, in, count * sizeof(VkFramebuffer));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkFramebuffer>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkDescriptorPool>(uint32_t count, const VkDescriptorPool* in, VkDescriptorPool* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkDescriptorPool)(uintptr_t)VkVirtualHandleCreateOrGet<VkDescriptorPool>(in[i]);
    }
}

template <> VkDescriptorPool* AllocForHost<VkDescriptorPool>(uint32_t count, const VkDescriptorPool* in) {
    if (!in) return nullptr;
    VkDescriptorPool* res = new VkDescriptorPool[count]; memcpy(res, in, count * sizeof(VkDescriptorPool));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkDescriptorPool>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkDescriptorSetLayout>(uint32_t count, const VkDescriptorSetLayout* in, VkDescriptorSetLayout* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkDescriptorSetLayout)(uintptr_t)VkVirtualHandleCreateOrGet<VkDescriptorSetLayout>(in[i]);
    }
}

template <> VkDescriptorSetLayout* AllocForHost<VkDescriptorSetLayout>(uint32_t count, const VkDescriptorSetLayout* in) {
    if (!in) return nullptr;
    VkDescriptorSetLayout* res = new VkDescriptorSetLayout[count]; memcpy(res, in, count * sizeof(VkDescriptorSetLayout));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkDescriptorSetLayout>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkDescriptorSet>(uint32_t count, const VkDescriptorSet* in, VkDescriptorSet* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkDescriptorSet)(uintptr_t)VkVirtualHandleCreateOrGet<VkDescriptorSet>(in[i]);
    }
}

template <> VkDescriptorSet* AllocForHost<VkDescriptorSet>(uint32_t count, const VkDescriptorSet* in) {
    if (!in) return nullptr;
    VkDescriptorSet* res = new VkDescriptorSet[count]; memcpy(res, in, count * sizeof(VkDescriptorSet));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkDescriptorSet>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkPipelineLayout>(uint32_t count, const VkPipelineLayout* in, VkPipelineLayout* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkPipelineLayout)(uintptr_t)VkVirtualHandleCreateOrGet<VkPipelineLayout>(in[i]);
    }
}

template <> VkPipelineLayout* AllocForHost<VkPipelineLayout>(uint32_t count, const VkPipelineLayout* in) {
    if (!in) return nullptr;
    VkPipelineLayout* res = new VkPipelineLayout[count]; memcpy(res, in, count * sizeof(VkPipelineLayout));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkPipelineLayout>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkPipelineCache>(uint32_t count, const VkPipelineCache* in, VkPipelineCache* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkPipelineCache)(uintptr_t)VkVirtualHandleCreateOrGet<VkPipelineCache>(in[i]);
    }
}

template <> VkPipelineCache* AllocForHost<VkPipelineCache>(uint32_t count, const VkPipelineCache* in) {
    if (!in) return nullptr;
    VkPipelineCache* res = new VkPipelineCache[count]; memcpy(res, in, count * sizeof(VkPipelineCache));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkPipelineCache>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkPipeline>(uint32_t count, const VkPipeline* in, VkPipeline* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkPipeline)(uintptr_t)VkVirtualHandleCreateOrGet<VkPipeline>(in[i]);
    }
}

template <> VkPipeline* AllocForHost<VkPipeline>(uint32_t count, const VkPipeline* in) {
    if (!in) return nullptr;
    VkPipeline* res = new VkPipeline[count]; memcpy(res, in, count * sizeof(VkPipeline));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkPipeline>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkFence>(uint32_t count, const VkFence* in, VkFence* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkFence)(uintptr_t)VkVirtualHandleCreateOrGet<VkFence>(in[i]);
    }
}

template <> VkFence* AllocForHost<VkFence>(uint32_t count, const VkFence* in) {
    if (!in) return nullptr;
    VkFence* res = new VkFence[count]; memcpy(res, in, count * sizeof(VkFence));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkFence>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkSemaphore>(uint32_t count, const VkSemaphore* in, VkSemaphore* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkSemaphore)(uintptr_t)VkVirtualHandleCreateOrGet<VkSemaphore>(in[i]);
    }
}

template <> VkSemaphore* AllocForHost<VkSemaphore>(uint32_t count, const VkSemaphore* in) {
    if (!in) return nullptr;
    VkSemaphore* res = new VkSemaphore[count]; memcpy(res, in, count * sizeof(VkSemaphore));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkSemaphore>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkEvent>(uint32_t count, const VkEvent* in, VkEvent* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkEvent)(uintptr_t)VkVirtualHandleCreateOrGet<VkEvent>(in[i]);
    }
}

template <> VkEvent* AllocForHost<VkEvent>(uint32_t count, const VkEvent* in) {
    if (!in) return nullptr;
    VkEvent* res = new VkEvent[count]; memcpy(res, in, count * sizeof(VkEvent));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkQueryPool>(uint32_t count, const VkQueryPool* in, VkQueryPool* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkQueryPool)(uintptr_t)VkVirtualHandleCreateOrGet<VkQueryPool>(in[i]);
    }
}

template <> VkQueryPool* AllocForHost<VkQueryPool>(uint32_t count, const VkQueryPool* in) {
    if (!in) return nullptr;
    VkQueryPool* res = new VkQueryPool[count]; memcpy(res, in, count * sizeof(VkQueryPool));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkShaderModule>(uint32_t count, const VkShaderModule* in, VkShaderModule* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkShaderModule)(uintptr_t)VkVirtualHandleCreateOrGet<VkShaderModule>(in[i]);
    }
}

template <> VkShaderModule* AllocForHost<VkShaderModule>(uint32_t count, const VkShaderModule* in) {
    if (!in) return nullptr;
    VkShaderModule* res = new VkShaderModule[count]; memcpy(res, in, count * sizeof(VkShaderModule));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkShaderModule>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}


template <> void VkVirtualHandleToGuestArray<VkSampler>(uint32_t count, const VkSampler* in, VkSampler* out) {
    for (uint32_t i = 0; i < count; i++) {
        out[i] = (VkSampler)(uintptr_t)VkVirtualHandleCreateOrGet<VkSampler>(in[i]);
    }
}

template <> VkSampler* AllocForHost<VkSampler>(uint32_t count, const VkSampler* in) {
    if (!in) return nullptr;
    VkSampler* res = new VkSampler[count]; memcpy(res, in, count * sizeof(VkSampler));

    for (uint32_t i = 0; i < count; i++) {
    res[i] = VkVirtualHandleGetHost<VkSampler>((uint32_t)(uintptr_t)in[i]);
    }
    return res;
}

template <> VkMappedMemoryRange* AllocForHost<VkMappedMemoryRange>(uint32_t count, const VkMappedMemoryRange* in);
template <> VkSubmitInfo* AllocForHost<VkSubmitInfo>(uint32_t count, const VkSubmitInfo* in);
template <> VkBindSparseInfo* AllocForHost<VkBindSparseInfo>(uint32_t count, const VkBindSparseInfo* in);
template <> VkBufferViewCreateInfo* AllocForHost<VkBufferViewCreateInfo>(uint32_t count, const VkBufferViewCreateInfo* in);
template <> VkImageViewCreateInfo* AllocForHost<VkImageViewCreateInfo>(uint32_t count, const VkImageViewCreateInfo* in);
template <> VkGraphicsPipelineCreateInfo* AllocForHost<VkGraphicsPipelineCreateInfo>(uint32_t count, const VkGraphicsPipelineCreateInfo* in);
template <> VkComputePipelineCreateInfo* AllocForHost<VkComputePipelineCreateInfo>(uint32_t count, const VkComputePipelineCreateInfo* in);
template <> VkPipelineLayoutCreateInfo* AllocForHost<VkPipelineLayoutCreateInfo>(uint32_t count, const VkPipelineLayoutCreateInfo* in);
template <> VkDescriptorSetLayoutBinding* AllocForHost<VkDescriptorSetLayoutBinding>(uint32_t count, const VkDescriptorSetLayoutBinding* in);
template <> VkDescriptorSetLayoutCreateInfo* AllocForHost<VkDescriptorSetLayoutCreateInfo>(uint32_t count, const VkDescriptorSetLayoutCreateInfo* in);
template <> VkDescriptorSetAllocateInfo* AllocForHost<VkDescriptorSetAllocateInfo>(uint32_t count, const VkDescriptorSetAllocateInfo* in);
template <> VkDescriptorImageInfo* AllocForHost<VkDescriptorImageInfo>(uint32_t count, const VkDescriptorImageInfo* in);
template <> VkDescriptorBufferInfo* AllocForHost<VkDescriptorBufferInfo>(uint32_t count, const VkDescriptorBufferInfo* in);
template <> VkWriteDescriptorSet* AllocForHost<VkWriteDescriptorSet>(uint32_t count, const VkWriteDescriptorSet* in);
template <> VkCopyDescriptorSet* AllocForHost<VkCopyDescriptorSet>(uint32_t count, const VkCopyDescriptorSet* in);
template <> VkFramebufferCreateInfo* AllocForHost<VkFramebufferCreateInfo>(uint32_t count, const VkFramebufferCreateInfo* in);
template <> VkCommandBufferInheritanceInfo* AllocForHost<VkCommandBufferInheritanceInfo>(uint32_t count, const VkCommandBufferInheritanceInfo* in);
template <> VkCommandBufferAllocateInfo* AllocForHost<VkCommandBufferAllocateInfo>(uint32_t count, const VkCommandBufferAllocateInfo* in);
template <> VkCommandBufferBeginInfo* AllocForHost<VkCommandBufferBeginInfo>(uint32_t count, const VkCommandBufferBeginInfo* in);
template <> VkBufferMemoryBarrier* AllocForHost<VkBufferMemoryBarrier>(uint32_t count, const VkBufferMemoryBarrier* in);
template <> VkImageMemoryBarrier* AllocForHost<VkImageMemoryBarrier>(uint32_t count, const VkImageMemoryBarrier* in);
template <> VkRenderPassBeginInfo* AllocForHost<VkRenderPassBeginInfo>(uint32_t count, const VkRenderPassBeginInfo* in);
template <> VkSparseMemoryBind* AllocForHost<VkSparseMemoryBind>(uint32_t count, const VkSparseMemoryBind* in);
template <> VkSparseBufferMemoryBindInfo* AllocForHost<VkSparseBufferMemoryBindInfo>(uint32_t count, const VkSparseBufferMemoryBindInfo* in);
template <> VkSparseImageOpaqueMemoryBindInfo* AllocForHost<VkSparseImageOpaqueMemoryBindInfo>(uint32_t count, const VkSparseImageOpaqueMemoryBindInfo* in);
template <> VkSparseImageMemoryBindInfo* AllocForHost<VkSparseImageMemoryBindInfo>(uint32_t count, const VkSparseImageMemoryBindInfo* in);

template <> VkMappedMemoryRange* AllocForHost<VkMappedMemoryRange>(uint32_t count, const VkMappedMemoryRange* in) {
    if (!in) return nullptr;
    VkMappedMemoryRange* res = new VkMappedMemoryRange[count]; memcpy(res, in, count * sizeof(VkMappedMemoryRange));

    for (uint32_t i = 0; i < count; i++) {
    res[i].memory = (VkDeviceMemory)(uintptr_t)VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)(in->memory));
    }
    return res;
}


template <> VkSubmitInfo* AllocForHost<VkSubmitInfo>(uint32_t count, const VkSubmitInfo* in) {
    if (!in) return nullptr;
    VkSubmitInfo* res = new VkSubmitInfo[count]; memcpy(res, in, count * sizeof(VkSubmitInfo));

    for (uint32_t i = 0; i < count; i++) {
    if (in->pWaitSemaphores) { VkSemaphore* tmp = new VkSemaphore[in->waitSemaphoreCount]; for (uint32_t j = 0; j < in->waitSemaphoreCount; i++) { tmp[j] = (VkSemaphore)(uintptr_t)VkVirtualHandleGetHost<VkSemaphore>((uint32_t)(uintptr_t)in->pWaitSemaphores[j]); } res[i].pWaitSemaphores = tmp; } else { res[i].pWaitSemaphores = nullptr; }
    if (in->pCommandBuffers) { VkCommandBuffer* tmp = new VkCommandBuffer[in->commandBufferCount]; for (uint32_t j = 0; j < in->commandBufferCount; i++) { tmp[j] = (VkCommandBuffer)(uintptr_t)VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)in->pCommandBuffers[j]); } res[i].pCommandBuffers = tmp; } else { res[i].pCommandBuffers = nullptr; }
    if (in->pSignalSemaphores) { VkSemaphore* tmp = new VkSemaphore[in->signalSemaphoreCount]; for (uint32_t j = 0; j < in->signalSemaphoreCount; i++) { tmp[j] = (VkSemaphore)(uintptr_t)VkVirtualHandleGetHost<VkSemaphore>((uint32_t)(uintptr_t)in->pSignalSemaphores[j]); } res[i].pSignalSemaphores = tmp; } else { res[i].pSignalSemaphores = nullptr; }
    }
    return res;
}


template <> VkBindSparseInfo* AllocForHost<VkBindSparseInfo>(uint32_t count, const VkBindSparseInfo* in) {
    if (!in) return nullptr;
    VkBindSparseInfo* res = new VkBindSparseInfo[count]; memcpy(res, in, count * sizeof(VkBindSparseInfo));

    for (uint32_t i = 0; i < count; i++) {
    if (in->pWaitSemaphores) { VkSemaphore* tmp = new VkSemaphore[in->waitSemaphoreCount]; for (uint32_t j = 0; j < in->waitSemaphoreCount; i++) { tmp[j] = (VkSemaphore)(uintptr_t)VkVirtualHandleGetHost<VkSemaphore>((uint32_t)(uintptr_t)in->pWaitSemaphores[j]); } res[i].pWaitSemaphores = tmp; } else { res[i].pWaitSemaphores = nullptr; }
    res[i].pBufferBinds = AllocForHost<VkSparseBufferMemoryBindInfo>(in->bufferBindCount, in->pBufferBinds);
    res[i].pImageOpaqueBinds = AllocForHost<VkSparseImageOpaqueMemoryBindInfo>(in->imageOpaqueBindCount, in->pImageOpaqueBinds);
    res[i].pImageBinds = AllocForHost<VkSparseImageMemoryBindInfo>(in->imageBindCount, in->pImageBinds);
    if (in->pSignalSemaphores) { VkSemaphore* tmp = new VkSemaphore[in->signalSemaphoreCount]; for (uint32_t j = 0; j < in->signalSemaphoreCount; i++) { tmp[j] = (VkSemaphore)(uintptr_t)VkVirtualHandleGetHost<VkSemaphore>((uint32_t)(uintptr_t)in->pSignalSemaphores[j]); } res[i].pSignalSemaphores = tmp; } else { res[i].pSignalSemaphores = nullptr; }
    }
    return res;
}


template <> VkBufferViewCreateInfo* AllocForHost<VkBufferViewCreateInfo>(uint32_t count, const VkBufferViewCreateInfo* in) {
    if (!in) return nullptr;
    VkBufferViewCreateInfo* res = new VkBufferViewCreateInfo[count]; memcpy(res, in, count * sizeof(VkBufferViewCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].buffer = (VkBuffer)(uintptr_t)VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)(in->buffer));
    }
    return res;
}


template <> VkImageViewCreateInfo* AllocForHost<VkImageViewCreateInfo>(uint32_t count, const VkImageViewCreateInfo* in) {
    if (!in) return nullptr;
    VkImageViewCreateInfo* res = new VkImageViewCreateInfo[count]; memcpy(res, in, count * sizeof(VkImageViewCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].image = (VkImage)(uintptr_t)VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)(in->image));
    }
    return res;
}


template <> VkGraphicsPipelineCreateInfo* AllocForHost<VkGraphicsPipelineCreateInfo>(uint32_t count, const VkGraphicsPipelineCreateInfo* in) {
    if (!in) return nullptr;
    VkGraphicsPipelineCreateInfo* res = new VkGraphicsPipelineCreateInfo[count]; memcpy(res, in, count * sizeof(VkGraphicsPipelineCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].layout = (VkPipelineLayout)(uintptr_t)VkVirtualHandleGetHost<VkPipelineLayout>((uint32_t)(uintptr_t)(in->layout));
    res[i].renderPass = (VkRenderPass)(uintptr_t)VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)(in->renderPass));
    res[i].basePipelineHandle = (VkPipeline)(uintptr_t)VkVirtualHandleGetHost<VkPipeline>((uint32_t)(uintptr_t)(in->basePipelineHandle));
    }
    return res;
}


template <> VkComputePipelineCreateInfo* AllocForHost<VkComputePipelineCreateInfo>(uint32_t count, const VkComputePipelineCreateInfo* in) {
    if (!in) return nullptr;
    VkComputePipelineCreateInfo* res = new VkComputePipelineCreateInfo[count]; memcpy(res, in, count * sizeof(VkComputePipelineCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].layout = (VkPipelineLayout)(uintptr_t)VkVirtualHandleGetHost<VkPipelineLayout>((uint32_t)(uintptr_t)(in->layout));
    res[i].basePipelineHandle = (VkPipeline)(uintptr_t)VkVirtualHandleGetHost<VkPipeline>((uint32_t)(uintptr_t)(in->basePipelineHandle));
    }
    return res;
}


template <> VkPipelineLayoutCreateInfo* AllocForHost<VkPipelineLayoutCreateInfo>(uint32_t count, const VkPipelineLayoutCreateInfo* in) {
    if (!in) return nullptr;
    VkPipelineLayoutCreateInfo* res = new VkPipelineLayoutCreateInfo[count]; memcpy(res, in, count * sizeof(VkPipelineLayoutCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    if (in->pSetLayouts) { VkDescriptorSetLayout* tmp = new VkDescriptorSetLayout[in->setLayoutCount]; for (uint32_t j = 0; j < in->setLayoutCount; i++) { tmp[j] = (VkDescriptorSetLayout)(uintptr_t)VkVirtualHandleGetHost<VkDescriptorSetLayout>((uint32_t)(uintptr_t)in->pSetLayouts[j]); } res[i].pSetLayouts = tmp; } else { res[i].pSetLayouts = nullptr; }
    }
    return res;
}


template <> VkDescriptorSetLayoutBinding* AllocForHost<VkDescriptorSetLayoutBinding>(uint32_t count, const VkDescriptorSetLayoutBinding* in) {
    if (!in) return nullptr;
    VkDescriptorSetLayoutBinding* res = new VkDescriptorSetLayoutBinding[count]; memcpy(res, in, count * sizeof(VkDescriptorSetLayoutBinding));

    for (uint32_t i = 0; i < count; i++) {
    if (in->pImmutableSamplers) { VkSampler* tmp = new VkSampler[in->descriptorCount]; for (uint32_t j = 0; j < in->descriptorCount; i++) { tmp[j] = (VkSampler)(uintptr_t)VkVirtualHandleGetHost<VkSampler>((uint32_t)(uintptr_t)in->pImmutableSamplers[j]); } res[i].pImmutableSamplers = tmp; } else { res[i].pImmutableSamplers = nullptr; }
    }
    return res;
}


template <> VkDescriptorSetLayoutCreateInfo* AllocForHost<VkDescriptorSetLayoutCreateInfo>(uint32_t count, const VkDescriptorSetLayoutCreateInfo* in) {
    if (!in) return nullptr;
    VkDescriptorSetLayoutCreateInfo* res = new VkDescriptorSetLayoutCreateInfo[count]; memcpy(res, in, count * sizeof(VkDescriptorSetLayoutCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].pBindings = AllocForHost<VkDescriptorSetLayoutBinding>(in->bindingCount, in->pBindings);
    }
    return res;
}


template <> VkDescriptorSetAllocateInfo* AllocForHost<VkDescriptorSetAllocateInfo>(uint32_t count, const VkDescriptorSetAllocateInfo* in) {
    if (!in) return nullptr;
    VkDescriptorSetAllocateInfo* res = new VkDescriptorSetAllocateInfo[count]; memcpy(res, in, count * sizeof(VkDescriptorSetAllocateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].descriptorPool = (VkDescriptorPool)(uintptr_t)VkVirtualHandleGetHost<VkDescriptorPool>((uint32_t)(uintptr_t)(in->descriptorPool));
    if (in->pSetLayouts) { VkDescriptorSetLayout* tmp = new VkDescriptorSetLayout[in->descriptorSetCount]; for (uint32_t j = 0; j < in->descriptorSetCount; i++) { tmp[j] = (VkDescriptorSetLayout)(uintptr_t)VkVirtualHandleGetHost<VkDescriptorSetLayout>((uint32_t)(uintptr_t)in->pSetLayouts[j]); } res[i].pSetLayouts = tmp; } else { res[i].pSetLayouts = nullptr; }
    }
    return res;
}


template <> VkDescriptorImageInfo* AllocForHost<VkDescriptorImageInfo>(uint32_t count, const VkDescriptorImageInfo* in) {
    if (!in) return nullptr;
    VkDescriptorImageInfo* res = new VkDescriptorImageInfo[count]; memcpy(res, in, count * sizeof(VkDescriptorImageInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].sampler = (VkSampler)(uintptr_t)VkVirtualHandleGetHost<VkSampler>((uint32_t)(uintptr_t)(in->sampler));
    res[i].imageView = (VkImageView)(uintptr_t)VkVirtualHandleGetHost<VkImageView>((uint32_t)(uintptr_t)(in->imageView));
    }
    return res;
}


template <> VkDescriptorBufferInfo* AllocForHost<VkDescriptorBufferInfo>(uint32_t count, const VkDescriptorBufferInfo* in) {
    if (!in) return nullptr;
    VkDescriptorBufferInfo* res = new VkDescriptorBufferInfo[count]; memcpy(res, in, count * sizeof(VkDescriptorBufferInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].buffer = (VkBuffer)(uintptr_t)VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)(in->buffer));
    }
    return res;
}


template <> VkWriteDescriptorSet* AllocForHost<VkWriteDescriptorSet>(uint32_t count, const VkWriteDescriptorSet* in) {
    if (!in) return nullptr;
    VkWriteDescriptorSet* res = new VkWriteDescriptorSet[count]; memcpy(res, in, count * sizeof(VkWriteDescriptorSet));

    for (uint32_t i = 0; i < count; i++) {
    res[i].dstSet = (VkDescriptorSet)(uintptr_t)VkVirtualHandleGetHost<VkDescriptorSet>((uint32_t)(uintptr_t)(in->dstSet));
    res[i].pImageInfo = AllocForHost<VkDescriptorImageInfo>(customCount_VkWriteDescriptorSet_pImageInfo(in), in->pImageInfo);
    res[i].pBufferInfo = AllocForHost<VkDescriptorBufferInfo>(customCount_VkWriteDescriptorSet_pBufferInfo(in), in->pBufferInfo);
    if (in->pTexelBufferView) { VkBufferView* tmp = new VkBufferView[customCount_VkWriteDescriptorSet_pTexelBufferView(in)]; for (uint32_t j = 0; j < customCount_VkWriteDescriptorSet_pTexelBufferView(in); i++) { tmp[j] = (VkBufferView)(uintptr_t)VkVirtualHandleGetHost<VkBufferView>((uint32_t)(uintptr_t)in->pTexelBufferView[j]); } res[i].pTexelBufferView = tmp; } else { res[i].pTexelBufferView = nullptr; }
    }
    return res;
}


template <> VkCopyDescriptorSet* AllocForHost<VkCopyDescriptorSet>(uint32_t count, const VkCopyDescriptorSet* in) {
    if (!in) return nullptr;
    VkCopyDescriptorSet* res = new VkCopyDescriptorSet[count]; memcpy(res, in, count * sizeof(VkCopyDescriptorSet));

    for (uint32_t i = 0; i < count; i++) {
    res[i].srcSet = (VkDescriptorSet)(uintptr_t)VkVirtualHandleGetHost<VkDescriptorSet>((uint32_t)(uintptr_t)(in->srcSet));
    res[i].dstSet = (VkDescriptorSet)(uintptr_t)VkVirtualHandleGetHost<VkDescriptorSet>((uint32_t)(uintptr_t)(in->dstSet));
    }
    return res;
}


template <> VkFramebufferCreateInfo* AllocForHost<VkFramebufferCreateInfo>(uint32_t count, const VkFramebufferCreateInfo* in) {
    if (!in) return nullptr;
    VkFramebufferCreateInfo* res = new VkFramebufferCreateInfo[count]; memcpy(res, in, count * sizeof(VkFramebufferCreateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].renderPass = (VkRenderPass)(uintptr_t)VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)(in->renderPass));
    if (in->pAttachments) { VkImageView* tmp = new VkImageView[in->attachmentCount]; for (uint32_t j = 0; j < in->attachmentCount; i++) { tmp[j] = (VkImageView)(uintptr_t)VkVirtualHandleGetHost<VkImageView>((uint32_t)(uintptr_t)in->pAttachments[j]); } res[i].pAttachments = tmp; } else { res[i].pAttachments = nullptr; }
    }
    return res;
}


template <> VkCommandBufferInheritanceInfo* AllocForHost<VkCommandBufferInheritanceInfo>(uint32_t count, const VkCommandBufferInheritanceInfo* in) {
    if (!in) return nullptr;
    VkCommandBufferInheritanceInfo* res = new VkCommandBufferInheritanceInfo[count]; memcpy(res, in, count * sizeof(VkCommandBufferInheritanceInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].renderPass = (VkRenderPass)(uintptr_t)VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)(in->renderPass));
    res[i].framebuffer = (VkFramebuffer)(uintptr_t)VkVirtualHandleGetHost<VkFramebuffer>((uint32_t)(uintptr_t)(in->framebuffer));
    }
    return res;
}


template <> VkCommandBufferAllocateInfo* AllocForHost<VkCommandBufferAllocateInfo>(uint32_t count, const VkCommandBufferAllocateInfo* in) {
    if (!in) return nullptr;
    VkCommandBufferAllocateInfo* res = new VkCommandBufferAllocateInfo[count]; memcpy(res, in, count * sizeof(VkCommandBufferAllocateInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].commandPool = (VkCommandPool)(uintptr_t)VkVirtualHandleGetHost<VkCommandPool>((uint32_t)(uintptr_t)(in->commandPool));
    }
    return res;
}


template <> VkCommandBufferBeginInfo* AllocForHost<VkCommandBufferBeginInfo>(uint32_t count, const VkCommandBufferBeginInfo* in) {
    if (!in) return nullptr;
    VkCommandBufferBeginInfo* res = new VkCommandBufferBeginInfo[count]; memcpy(res, in, count * sizeof(VkCommandBufferBeginInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].pInheritanceInfo = AllocForHost<VkCommandBufferInheritanceInfo>(1, in->pInheritanceInfo);
    }
    return res;
}


template <> VkBufferMemoryBarrier* AllocForHost<VkBufferMemoryBarrier>(uint32_t count, const VkBufferMemoryBarrier* in) {
    if (!in) return nullptr;
    VkBufferMemoryBarrier* res = new VkBufferMemoryBarrier[count]; memcpy(res, in, count * sizeof(VkBufferMemoryBarrier));

    for (uint32_t i = 0; i < count; i++) {
    res[i].buffer = (VkBuffer)(uintptr_t)VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)(in->buffer));
    }
    return res;
}


template <> VkImageMemoryBarrier* AllocForHost<VkImageMemoryBarrier>(uint32_t count, const VkImageMemoryBarrier* in) {
    if (!in) return nullptr;
    VkImageMemoryBarrier* res = new VkImageMemoryBarrier[count]; memcpy(res, in, count * sizeof(VkImageMemoryBarrier));

    for (uint32_t i = 0; i < count; i++) {
    res[i].image = (VkImage)(uintptr_t)VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)(in->image));
    }
    return res;
}


template <> VkRenderPassBeginInfo* AllocForHost<VkRenderPassBeginInfo>(uint32_t count, const VkRenderPassBeginInfo* in) {
    if (!in) return nullptr;
    VkRenderPassBeginInfo* res = new VkRenderPassBeginInfo[count]; memcpy(res, in, count * sizeof(VkRenderPassBeginInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].renderPass = (VkRenderPass)(uintptr_t)VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)(in->renderPass));
    res[i].framebuffer = (VkFramebuffer)(uintptr_t)VkVirtualHandleGetHost<VkFramebuffer>((uint32_t)(uintptr_t)(in->framebuffer));
    }
    return res;
}


template <> VkSparseMemoryBind* AllocForHost<VkSparseMemoryBind>(uint32_t count, const VkSparseMemoryBind* in) {
    if (!in) return nullptr;
    VkSparseMemoryBind* res = new VkSparseMemoryBind[count]; memcpy(res, in, count * sizeof(VkSparseMemoryBind));

    for (uint32_t i = 0; i < count; i++) {
    res[i].memory = (VkDeviceMemory)(uintptr_t)VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)(in->memory));
    }
    return res;
}


template <> VkSparseBufferMemoryBindInfo* AllocForHost<VkSparseBufferMemoryBindInfo>(uint32_t count, const VkSparseBufferMemoryBindInfo* in) {
    if (!in) return nullptr;
    VkSparseBufferMemoryBindInfo* res = new VkSparseBufferMemoryBindInfo[count]; memcpy(res, in, count * sizeof(VkSparseBufferMemoryBindInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].buffer = (VkBuffer)(uintptr_t)VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)(in->buffer));
    res[i].pBinds = AllocForHost<VkSparseMemoryBind>(in->bindCount, in->pBinds);
    }
    return res;
}


template <> VkSparseImageOpaqueMemoryBindInfo* AllocForHost<VkSparseImageOpaqueMemoryBindInfo>(uint32_t count, const VkSparseImageOpaqueMemoryBindInfo* in) {
    if (!in) return nullptr;
    VkSparseImageOpaqueMemoryBindInfo* res = new VkSparseImageOpaqueMemoryBindInfo[count]; memcpy(res, in, count * sizeof(VkSparseImageOpaqueMemoryBindInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].image = (VkImage)(uintptr_t)VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)(in->image));
    res[i].pBinds = AllocForHost<VkSparseMemoryBind>(in->bindCount, in->pBinds);
    }
    return res;
}


template <> VkSparseImageMemoryBindInfo* AllocForHost<VkSparseImageMemoryBindInfo>(uint32_t count, const VkSparseImageMemoryBindInfo* in) {
    if (!in) return nullptr;
    VkSparseImageMemoryBindInfo* res = new VkSparseImageMemoryBindInfo[count]; memcpy(res, in, count * sizeof(VkSparseImageMemoryBindInfo));

    for (uint32_t i = 0; i < count; i++) {
    res[i].image = (VkImage)(uintptr_t)VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)(in->image));
    }
    return res;
}

