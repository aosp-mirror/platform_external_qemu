
#include "OpenGLESDispatch/VkDispatch.h"
#include "VkHandleDispatch.h"
#include "VkVirtualHandle.h"
#include "VkHostMarshaling.h"
#include "vkUtils.h"
#include "vkUtils_custom.h"

#include <vulkan/vk_platform.h>

#include <vulkan/vulkan.h>
#include <unordered_map>

#include "vk_types.h"
#include "vk_dec.h"


static VkResult s_vkMapMemoryAEMU(void* self, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, uint32_t datalen, char* data) ;
static void s_vkUnmapMemoryAEMU(void* self, VkDevice device, VkDeviceMemory memory, uint32_t datalen, const char* data) ;
static VkResult s_vkFlushMappedMemoryRangeAEMU(void* self, VkDevice device, const VkMappedMemoryRange* memoryRange, uint32_t datalen, const char* data) ;
static VkResult s_vkInvalidateMappedMemoryRangeAEMU(void* self, VkDevice device, const VkMappedMemoryRange* memoryRange, uint32_t datalen, char* data) ;
static void s_vkCmdSetBlendConstantsAEMU(void* self, VkCommandBuffer commandBuffer, f32_4 blendConstants) ;
static VkResult s_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) ;
static void s_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkEnumeratePhysicalDevices(VkInstance instance, u32* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) ;
static void s_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) ;
static void s_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, u32* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) ;
static void s_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) ;
static void s_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) ;
static void s_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) ;
static VkResult s_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) ;
static VkResult s_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) ;
static void s_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkEnumerateInstanceLayerProperties(u32* pPropertyCount, VkLayerProperties* pProperties) ;
static VkResult s_vkEnumerateInstanceExtensionProperties(const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties) ;
static VkResult s_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, u32* pPropertyCount, VkLayerProperties* pProperties) ;
static VkResult s_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties) ;
static void s_vkGetDeviceQueue(VkDevice device, u32 queueFamilyIndex, u32 queueIndex, VkQueue* pQueue) ;
static VkResult s_vkQueueSubmit(VkQueue queue, u32 submitCount, const VkSubmitInfo* pSubmits, VkFence fence) ;
static VkResult s_vkQueueWaitIdle(VkQueue queue) ;
static VkResult s_vkDeviceWaitIdle(VkDevice device) ;
static VkResult s_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) ;
static void s_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) ;
static void s_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) ;
static void s_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) ;
static VkResult s_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) ;
static void s_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) ;
static VkResult s_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) ;
static void s_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, u32* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements) ;
static void s_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, u32* pPropertyCount, VkSparseImageFormatProperties* pProperties) ;
static VkResult s_vkQueueBindSparse(VkQueue queue, u32 bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence) ;
static VkResult s_vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) ;
static void s_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkResetFences(VkDevice device, u32 fenceCount, const VkFence* pFences) ;
static VkResult s_vkGetFenceStatus(VkDevice device, VkFence fence) ;
static VkResult s_vkWaitForFences(VkDevice device, u32 fenceCount, const VkFence* pFences, VkBool32 waitAll, u64 timeout) ;
static VkResult s_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) ;
static void s_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent) ;
static void s_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkGetEventStatus(VkDevice device, VkEvent event) ;
static VkResult s_vkSetEvent(VkDevice device, VkEvent event) ;
static VkResult s_vkResetEvent(VkDevice device, VkEvent event) ;
static VkResult s_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool) ;
static void s_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, u32 firstQuery, u32 queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) ;
static VkResult s_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) ;
static void s_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) ;
static void s_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) ;
static void s_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) ;
static void s_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout) ;
static VkResult s_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) ;
static void s_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) ;
static void s_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache) ;
static void s_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData) ;
static VkResult s_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, u32 srcCacheCount, const VkPipelineCache* pSrcCaches) ;
static VkResult s_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, u32 createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) ;
static VkResult s_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, u32 createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) ;
static void s_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) ;
static void s_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) ;
static void s_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) ;
static void s_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) ;
static void s_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) ;
static VkResult s_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) ;
static VkResult s_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, u32 descriptorSetCount, const VkDescriptorSet* pDescriptorSets) ;
static void s_vkUpdateDescriptorSets(VkDevice device, u32 descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, u32 descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) ;
static VkResult s_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) ;
static void s_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) ;
static void s_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) ;
static void s_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity) ;
static VkResult s_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) ;
static void s_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) ;
static VkResult s_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) ;
static VkResult s_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) ;
static void s_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, u32 commandBufferCount, const VkCommandBuffer* pCommandBuffers) ;
static VkResult s_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) ;
static VkResult s_vkEndCommandBuffer(VkCommandBuffer commandBuffer) ;
static VkResult s_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) ;
static void s_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) ;
static void s_vkCmdSetViewport(VkCommandBuffer commandBuffer, u32 firstViewport, u32 viewportCount, const VkViewport* pViewports) ;
static void s_vkCmdSetScissor(VkCommandBuffer commandBuffer, u32 firstScissor, u32 scissorCount, const VkRect2D* pScissors) ;
static void s_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, f32 lineWidth) ;
static void s_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, f32 depthBiasConstantFactor, f32 depthBiasClamp, f32 depthBiasSlopeFactor) ;
static void s_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, f32 minDepthBounds, f32 maxDepthBounds) ;
static void s_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 compareMask) ;
static void s_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 writeMask) ;
static void s_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 reference) ;
static void s_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount, const VkDescriptorSet* pDescriptorSets, u32 dynamicOffsetCount, const u32* pDynamicOffsets) ;
static void s_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) ;
static void s_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, u32 firstBinding, u32 bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) ;
static void s_vkCmdDraw(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) ;
static void s_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) ;
static void s_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride) ;
static void s_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride) ;
static void s_vkCmdDispatch(VkCommandBuffer commandBuffer, u32 groupCountX, u32 groupCountY, u32 groupCountZ) ;
static void s_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) ;
static void s_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, u32 regionCount, const VkBufferCopy* pRegions) ;
static void s_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageCopy* pRegions) ;
static void s_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageBlit* pRegions, VkFilter filter) ;
static void s_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkBufferImageCopy* pRegions) ;
static void s_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, u32 regionCount, const VkBufferImageCopy* pRegions) ;
static void s_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) ;
static void s_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, u32 data) ;
static void s_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, u32 rangeCount, const VkImageSubresourceRange* pRanges) ;
static void s_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, u32 rangeCount, const VkImageSubresourceRange* pRanges) ;
static void s_vkCmdClearAttachments(VkCommandBuffer commandBuffer, u32 attachmentCount, const VkClearAttachment* pAttachments, u32 rectCount, const VkClearRect* pRects) ;
static void s_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageResolve* pRegions) ;
static void s_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) ;
static void s_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) ;
static void s_vkCmdWaitEvents(VkCommandBuffer commandBuffer, u32 eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, u32 memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, u32 bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) ;
static void s_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, u32 memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, u32 bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) ;
static void s_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 query, VkQueryControlFlags flags) ;
static void s_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 query) ;
static void s_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 firstQuery, u32 queryCount) ;
static void s_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, u32 query) ;
static void s_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 firstQuery, u32 queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) ;
static void s_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, u32 offset, u32 size, const void* pValues) ;
static void s_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) ;
static void s_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) ;
static void s_vkCmdEndRenderPass(VkCommandBuffer commandBuffer) ;
static void s_vkCmdExecuteCommands(VkCommandBuffer commandBuffer, u32 commandBufferCount, const VkCommandBuffer* pCommandBuffers) ;
static VkResult s_vkMapMemoryAEMU(void* self, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, uint32_t datalen, char* data) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDeviceMemory host_memory = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)memory);

    // TODO: Support persistent coherent mappings
    void* driver_ptr = nullptr;
    VkResult res = s_dvk(host_device)->vkMapMemory(host_device, host_memory, offset, size, flags, &driver_ptr);
    if (res == VK_SUCCESS) {
        memcpy(data, driver_ptr, datalen);
        s_dvk(host_device)->vkUnmapMemory(host_device, host_memory);
    }

    return res;
}

static void s_vkUnmapMemoryAEMU(void* self, VkDevice device, VkDeviceMemory memory, uint32_t datalen, const char* data) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDeviceMemory host_memory = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)memory);

    // TODO: Support persistent coherent mappings
    void* driver_ptr = nullptr;
    s_dvk(host_device)->vkMapMemory(host_device, host_memory, 0, VK_WHOLE_SIZE, 0, &driver_ptr);
    memcpy(driver_ptr, data, datalen);
    VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, host_memory, 0, VK_WHOLE_SIZE, };
    s_dvk(host_device)->vkFlushMappedMemoryRanges(host_device, 1, &range);
    s_dvk(host_device)->vkUnmapMemory(host_device, host_memory);

    return;
}

static VkResult s_vkFlushMappedMemoryRangeAEMU(void* self, VkDevice device, const VkMappedMemoryRange* memoryRange, uint32_t datalen, const char* data) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkMappedMemoryRange* host_memoryRange = AllocForHost<VkMappedMemoryRange>(1, memoryRange);

    // TODO: Support persistent coherent mappings
    void* driver_ptr = nullptr;
    s_dvk(host_device)->vkMapMemory(host_device, host_memoryRange->memory, host_memoryRange->offset, host_memoryRange->size, 0, &driver_ptr);
    memcpy(driver_ptr, data, datalen);
    VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, host_memoryRange->memory, 0, VK_WHOLE_SIZE, };
    VkResult res = s_dvk(host_device)->vkFlushMappedMemoryRanges(host_device, 1, &range);
    s_dvk(host_device)->vkUnmapMemory(host_device, host_memoryRange->memory);

    return res;
}

static VkResult s_vkInvalidateMappedMemoryRangeAEMU(void* self, VkDevice device, const VkMappedMemoryRange* memoryRange, uint32_t datalen, char* data) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkMappedMemoryRange* host_memoryRange = AllocForHost<VkMappedMemoryRange>(1, memoryRange);

    // TODO: Support persistent coherent mappings
    void* driver_ptr = nullptr;
    s_dvk(host_device)->vkMapMemory(host_device, host_memoryRange->memory, host_memoryRange->offset, host_memoryRange->size, 0, &driver_ptr);
    VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, host_memoryRange->memory, 0, VK_WHOLE_SIZE, };
    VkResult res = s_dvk(host_device)->vkInvalidateMappedMemoryRanges(host_device, 1, &range);
    memcpy(data, driver_ptr, datalen);
    s_dvk(host_device)->vkUnmapMemory(host_device, host_memoryRange->memory);

    return res;
}

static void s_vkCmdSetBlendConstantsAEMU(void* self, VkCommandBuffer commandBuffer, f32_4 blendConstants) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);

    float actualBlendConstants[4] = {
        blendConstants.r,
        blendConstants.g,
        blendConstants.b,
        blendConstants.a,
    };
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetBlendConstants(host_commandBuffer, actualBlendConstants);

    return;
}

static VkResult s_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkResult res = vkGlobalDispatcher()->vkCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (res == VK_SUCCESS) { sMapInstanceDispatch(*pInstance); }

    // host->guest handle conversions
    if (pInstance) { VkVirtualHandleToGuestArray<VkInstance>(1, pInstance, pInstance); }
    return res;
}

static void s_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkInstance host_instance = VkVirtualHandleGetHost<VkInstance>((uint32_t)(uintptr_t)instance);
    s_ivk(host_instance)->vkDestroyInstance(host_instance, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkEnumeratePhysicalDevices(VkInstance instance, u32* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkInstance host_instance = VkVirtualHandleGetHost<VkInstance>((uint32_t)(uintptr_t)instance);
    VkResult res = s_ivk(host_instance)->vkEnumeratePhysicalDevices(host_instance, pPhysicalDeviceCount, pPhysicalDevices);
    if (res == VK_SUCCESS) { s_ivkPhysdev_new_multi(host_instance, *pPhysicalDeviceCount, pPhysicalDevices); }

    // host->guest handle conversions
    if (pPhysicalDevices) { VkVirtualHandleToGuestArray<VkPhysicalDevice>((*pPhysicalDeviceCount), pPhysicalDevices, pPhysicalDevices); }
    return res;
}

static void s_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceProperties(host_physicalDevice, pProperties);

    // host->guest handle conversions
}

static void s_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, u32* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);
    if (pQueueFamilyPropertyCount) {
        fprintf(stderr, "%s: count b4 %u\n", __func__, *pQueueFamilyPropertyCount);
    }
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceQueueFamilyProperties(host_physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    if (pQueueFamilyPropertyCount) {
        fprintf(stderr, "%s: count after %u\n", __func__, *pQueueFamilyPropertyCount);
    }
    fprintf(stderr, "%s:%d arrive\n", __func__, __LINE__);

    // host->guest handle conversions
}

static void s_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceMemoryProperties(host_physicalDevice, pMemoryProperties);

    // host->guest handle conversions
}

static void s_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceFeatures(host_physicalDevice, pFeatures);

    // host->guest handle conversions
}

static void s_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceFormatProperties(host_physicalDevice, format, pFormatProperties);

    // host->guest handle conversions
}

static VkResult s_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    VkResult res = s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceImageFormatProperties(host_physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    VkResult res = s_ivkPhysdev(host_physicalDevice)->vkCreateDevice(host_physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (res == VK_SUCCESS) { s_dvk_new(host_physicalDevice, *pDevice); }

    // host->guest handle conversions
    if (pDevice) { VkVirtualHandleToGuestArray<VkDevice>(1, pDevice, pDevice); }
    return res;
}

static void s_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    s_dvk(host_device)->vkDestroyDevice(host_device, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkEnumerateInstanceLayerProperties(u32* pPropertyCount, VkLayerProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkResult res = vkGlobalDispatcher()->vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkEnumerateInstanceExtensionProperties(const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkResult res = vkGlobalDispatcher()->vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, u32* pPropertyCount, VkLayerProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    VkResult res = s_ivkPhysdev(host_physicalDevice)->vkEnumerateDeviceLayerProperties(host_physicalDevice, pPropertyCount, pProperties);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    VkResult res = s_ivkPhysdev(host_physicalDevice)->vkEnumerateDeviceExtensionProperties(host_physicalDevice, pLayerName, pPropertyCount, pProperties);

    // host->guest handle conversions
    return res;
}

static void s_vkGetDeviceQueue(VkDevice device, u32 queueFamilyIndex, u32 queueIndex, VkQueue* pQueue) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    s_dvk(host_device)->vkGetDeviceQueue(host_device, queueFamilyIndex, queueIndex, pQueue);
    s_dvkQueue_new(host_device, *pQueue);

    // host->guest handle conversions
    if (pQueue) { VkVirtualHandleToGuestArray<VkQueue>(1, pQueue, pQueue); }
}

static VkResult s_vkQueueSubmit(VkQueue queue, u32 submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkQueue host_queue = VkVirtualHandleGetHost<VkQueue>((uint32_t)(uintptr_t)queue);
    VkSubmitInfo* host_pSubmits = AllocForHost<VkSubmitInfo>(submitCount, pSubmits);
    VkFence host_fence = VkVirtualHandleGetHost<VkFence>((uint32_t)(uintptr_t)fence);
    VkResult res = s_dvkQueue(host_queue)->vkQueueSubmit(host_queue, submitCount, host_pSubmits, host_fence);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkQueueWaitIdle(VkQueue queue) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkQueue host_queue = VkVirtualHandleGetHost<VkQueue>((uint32_t)(uintptr_t)queue);
    VkResult res = s_dvkQueue(host_queue)->vkQueueWaitIdle(host_queue);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkDeviceWaitIdle(VkDevice device) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkDeviceWaitIdle(host_device);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkAllocateMemory(host_device, pAllocateInfo, pAllocator, pMemory);

    // host->guest handle conversions
    if (pMemory) { VkVirtualHandleToGuestArray<VkDeviceMemory>(1, pMemory, pMemory); }
    return res;
}

static void s_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDeviceMemory host_memory = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)memory);
    s_dvk(host_device)->vkFreeMemory(host_device, host_memory, pAllocator);

    // host->guest handle conversions
}

static void s_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDeviceMemory host_memory = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)memory);
    s_dvk(host_device)->vkGetDeviceMemoryCommitment(host_device, host_memory, pCommittedMemoryInBytes);

    // host->guest handle conversions
}

static void s_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    s_dvk(host_device)->vkGetBufferMemoryRequirements(host_device, host_buffer, pMemoryRequirements);

    // host->guest handle conversions
}

static VkResult s_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    VkDeviceMemory host_memory = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)memory);
    VkResult res = s_dvk(host_device)->vkBindBufferMemory(host_device, host_buffer, host_memory, memoryOffset);

    // host->guest handle conversions
    return res;
}

static void s_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    s_dvk(host_device)->vkGetImageMemoryRequirements(host_device, host_image, pMemoryRequirements);

    // host->guest handle conversions
}

static VkResult s_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    VkDeviceMemory host_memory = VkVirtualHandleGetHost<VkDeviceMemory>((uint32_t)(uintptr_t)memory);
    VkResult res = s_dvk(host_device)->vkBindImageMemory(host_device, host_image, host_memory, memoryOffset);

    // host->guest handle conversions
    return res;
}

static void s_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, u32* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    s_dvk(host_device)->vkGetImageSparseMemoryRequirements(host_device, host_image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

    // host->guest handle conversions
}

static void s_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, u32* pPropertyCount, VkSparseImageFormatProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceSparseImageFormatProperties(host_physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);

    // host->guest handle conversions
}

static VkResult s_vkQueueBindSparse(VkQueue queue, u32 bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkQueue host_queue = VkVirtualHandleGetHost<VkQueue>((uint32_t)(uintptr_t)queue);
    VkBindSparseInfo* host_pBindInfo = AllocForHost<VkBindSparseInfo>(bindInfoCount, pBindInfo);
    VkFence host_fence = VkVirtualHandleGetHost<VkFence>((uint32_t)(uintptr_t)fence);
    VkResult res = s_dvkQueue(host_queue)->vkQueueBindSparse(host_queue, bindInfoCount, host_pBindInfo, host_fence);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateFence(host_device, pCreateInfo, pAllocator, pFence);

    // host->guest handle conversions
    if (pFence) { VkVirtualHandleToGuestArray<VkFence>(1, pFence, pFence); }
    return res;
}

static void s_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkFence host_fence = VkVirtualHandleGetHost<VkFence>((uint32_t)(uintptr_t)fence);
    s_dvk(host_device)->vkDestroyFence(host_device, host_fence, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkResetFences(VkDevice device, u32 fenceCount, const VkFence* pFences) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkFence* host_pFences = AllocForHost<VkFence>(fenceCount, pFences);
    VkResult res = s_dvk(host_device)->vkResetFences(host_device, fenceCount, host_pFences);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkGetFenceStatus(VkDevice device, VkFence fence) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkFence host_fence = VkVirtualHandleGetHost<VkFence>((uint32_t)(uintptr_t)fence);
    VkResult res = s_dvk(host_device)->vkGetFenceStatus(host_device, host_fence);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkWaitForFences(VkDevice device, u32 fenceCount, const VkFence* pFences, VkBool32 waitAll, u64 timeout) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkFence* host_pFences = AllocForHost<VkFence>(fenceCount, pFences);
    VkResult res = s_dvk(host_device)->vkWaitForFences(host_device, fenceCount, host_pFences, waitAll, timeout);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateSemaphore(host_device, pCreateInfo, pAllocator, pSemaphore);

    // host->guest handle conversions
    if (pSemaphore) { VkVirtualHandleToGuestArray<VkSemaphore>(1, pSemaphore, pSemaphore); }
    return res;
}

static void s_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkSemaphore host_semaphore = VkVirtualHandleGetHost<VkSemaphore>((uint32_t)(uintptr_t)semaphore);
    s_dvk(host_device)->vkDestroySemaphore(host_device, host_semaphore, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateEvent(host_device, pCreateInfo, pAllocator, pEvent);

    // host->guest handle conversions
    if (pEvent) { VkVirtualHandleToGuestArray<VkEvent>(1, pEvent, pEvent); }
    return res;
}

static void s_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkEvent host_event = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)event);
    s_dvk(host_device)->vkDestroyEvent(host_device, host_event, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkGetEventStatus(VkDevice device, VkEvent event) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkEvent host_event = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)event);
    VkResult res = s_dvk(host_device)->vkGetEventStatus(host_device, host_event);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkSetEvent(VkDevice device, VkEvent event) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkEvent host_event = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)event);
    VkResult res = s_dvk(host_device)->vkSetEvent(host_device, host_event);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkResetEvent(VkDevice device, VkEvent event) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkEvent host_event = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)event);
    VkResult res = s_dvk(host_device)->vkResetEvent(host_device, host_event);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateQueryPool(host_device, pCreateInfo, pAllocator, pQueryPool);

    // host->guest handle conversions
    if (pQueryPool) { VkVirtualHandleToGuestArray<VkQueryPool>(1, pQueryPool, pQueryPool); }
    return res;
}

static void s_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    s_dvk(host_device)->vkDestroyQueryPool(host_device, host_queryPool, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, u32 firstQuery, u32 queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    VkResult res = s_dvk(host_device)->vkGetQueryPoolResults(host_device, host_queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateBuffer(host_device, pCreateInfo, pAllocator, pBuffer);

    // host->guest handle conversions
    if (pBuffer) { VkVirtualHandleToGuestArray<VkBuffer>(1, pBuffer, pBuffer); }
    return res;
}

static void s_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    s_dvk(host_device)->vkDestroyBuffer(host_device, host_buffer, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkBufferViewCreateInfo* host_pCreateInfo = AllocForHost<VkBufferViewCreateInfo>(1, pCreateInfo);
    VkResult res = s_dvk(host_device)->vkCreateBufferView(host_device, host_pCreateInfo, pAllocator, pView);

    // host->guest handle conversions
    if (pView) { VkVirtualHandleToGuestArray<VkBufferView>(1, pView, pView); }
    return res;
}

static void s_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkBufferView host_bufferView = VkVirtualHandleGetHost<VkBufferView>((uint32_t)(uintptr_t)bufferView);
    s_dvk(host_device)->vkDestroyBufferView(host_device, host_bufferView, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateImage(host_device, pCreateInfo, pAllocator, pImage);

    // host->guest handle conversions
    if (pImage) { VkVirtualHandleToGuestArray<VkImage>(1, pImage, pImage); }
    return res;
}

static void s_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    s_dvk(host_device)->vkDestroyImage(host_device, host_image, pAllocator);

    // host->guest handle conversions
}

static void s_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    s_dvk(host_device)->vkGetImageSubresourceLayout(host_device, host_image, pSubresource, pLayout);

    // host->guest handle conversions
}

static VkResult s_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImageViewCreateInfo* host_pCreateInfo = AllocForHost<VkImageViewCreateInfo>(1, pCreateInfo);
    VkResult res = s_dvk(host_device)->vkCreateImageView(host_device, host_pCreateInfo, pAllocator, pView);

    // host->guest handle conversions
    if (pView) { VkVirtualHandleToGuestArray<VkImageView>(1, pView, pView); }
    return res;
}

static void s_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkImageView host_imageView = VkVirtualHandleGetHost<VkImageView>((uint32_t)(uintptr_t)imageView);
    s_dvk(host_device)->vkDestroyImageView(host_device, host_imageView, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateShaderModule(host_device, pCreateInfo, pAllocator, pShaderModule);

    // host->guest handle conversions
    if (pShaderModule) { VkVirtualHandleToGuestArray<VkShaderModule>(1, pShaderModule, pShaderModule); }
    return res;
}

static void s_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkShaderModule host_shaderModule = VkVirtualHandleGetHost<VkShaderModule>((uint32_t)(uintptr_t)shaderModule);
    s_dvk(host_device)->vkDestroyShaderModule(host_device, host_shaderModule, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreatePipelineCache(host_device, pCreateInfo, pAllocator, pPipelineCache);

    // host->guest handle conversions
    if (pPipelineCache) { VkVirtualHandleToGuestArray<VkPipelineCache>(1, pPipelineCache, pPipelineCache); }
    return res;
}

static void s_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineCache host_pipelineCache = VkVirtualHandleGetHost<VkPipelineCache>((uint32_t)(uintptr_t)pipelineCache);
    s_dvk(host_device)->vkDestroyPipelineCache(host_device, host_pipelineCache, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineCache host_pipelineCache = VkVirtualHandleGetHost<VkPipelineCache>((uint32_t)(uintptr_t)pipelineCache);
    VkResult res = s_dvk(host_device)->vkGetPipelineCacheData(host_device, host_pipelineCache, pDataSize, pData);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, u32 srcCacheCount, const VkPipelineCache* pSrcCaches) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineCache host_dstCache = VkVirtualHandleGetHost<VkPipelineCache>((uint32_t)(uintptr_t)dstCache);
    VkPipelineCache* host_pSrcCaches = AllocForHost<VkPipelineCache>(srcCacheCount, pSrcCaches);
    VkResult res = s_dvk(host_device)->vkMergePipelineCaches(host_device, host_dstCache, srcCacheCount, host_pSrcCaches);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, u32 createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineCache host_pipelineCache = VkVirtualHandleGetHost<VkPipelineCache>((uint32_t)(uintptr_t)pipelineCache);
    VkGraphicsPipelineCreateInfo* host_pCreateInfos = AllocForHost<VkGraphicsPipelineCreateInfo>(createInfoCount, pCreateInfos);
    VkResult res = s_dvk(host_device)->vkCreateGraphicsPipelines(host_device, host_pipelineCache, createInfoCount, host_pCreateInfos, pAllocator, pPipelines);

    // host->guest handle conversions
    if (pPipelines) { VkVirtualHandleToGuestArray<VkPipeline>(createInfoCount, pPipelines, pPipelines); }
    return res;
}

static VkResult s_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, u32 createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineCache host_pipelineCache = VkVirtualHandleGetHost<VkPipelineCache>((uint32_t)(uintptr_t)pipelineCache);
    VkComputePipelineCreateInfo* host_pCreateInfos = AllocForHost<VkComputePipelineCreateInfo>(createInfoCount, pCreateInfos);
    VkResult res = s_dvk(host_device)->vkCreateComputePipelines(host_device, host_pipelineCache, createInfoCount, host_pCreateInfos, pAllocator, pPipelines);

    // host->guest handle conversions
    if (pPipelines) { VkVirtualHandleToGuestArray<VkPipeline>(createInfoCount, pPipelines, pPipelines); }
    return res;
}

static void s_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipeline host_pipeline = VkVirtualHandleGetHost<VkPipeline>((uint32_t)(uintptr_t)pipeline);
    s_dvk(host_device)->vkDestroyPipeline(host_device, host_pipeline, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineLayoutCreateInfo* host_pCreateInfo = AllocForHost<VkPipelineLayoutCreateInfo>(1, pCreateInfo);
    VkResult res = s_dvk(host_device)->vkCreatePipelineLayout(host_device, host_pCreateInfo, pAllocator, pPipelineLayout);

    // host->guest handle conversions
    if (pPipelineLayout) { VkVirtualHandleToGuestArray<VkPipelineLayout>(1, pPipelineLayout, pPipelineLayout); }
    return res;
}

static void s_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkPipelineLayout host_pipelineLayout = VkVirtualHandleGetHost<VkPipelineLayout>((uint32_t)(uintptr_t)pipelineLayout);
    s_dvk(host_device)->vkDestroyPipelineLayout(host_device, host_pipelineLayout, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateSampler(host_device, pCreateInfo, pAllocator, pSampler);

    // host->guest handle conversions
    if (pSampler) { VkVirtualHandleToGuestArray<VkSampler>(1, pSampler, pSampler); }
    return res;
}

static void s_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkSampler host_sampler = VkVirtualHandleGetHost<VkSampler>((uint32_t)(uintptr_t)sampler);
    s_dvk(host_device)->vkDestroySampler(host_device, host_sampler, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDescriptorSetLayoutCreateInfo* host_pCreateInfo = AllocForHost<VkDescriptorSetLayoutCreateInfo>(1, pCreateInfo);
    VkResult res = s_dvk(host_device)->vkCreateDescriptorSetLayout(host_device, host_pCreateInfo, pAllocator, pSetLayout);

    // host->guest handle conversions
    if (pSetLayout) { VkVirtualHandleToGuestArray<VkDescriptorSetLayout>(1, pSetLayout, pSetLayout); }
    return res;
}

static void s_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDescriptorSetLayout host_descriptorSetLayout = VkVirtualHandleGetHost<VkDescriptorSetLayout>((uint32_t)(uintptr_t)descriptorSetLayout);
    s_dvk(host_device)->vkDestroyDescriptorSetLayout(host_device, host_descriptorSetLayout, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateDescriptorPool(host_device, pCreateInfo, pAllocator, pDescriptorPool);

    // host->guest handle conversions
    if (pDescriptorPool) { VkVirtualHandleToGuestArray<VkDescriptorPool>(1, pDescriptorPool, pDescriptorPool); }
    return res;
}

static void s_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDescriptorPool host_descriptorPool = VkVirtualHandleGetHost<VkDescriptorPool>((uint32_t)(uintptr_t)descriptorPool);
    s_dvk(host_device)->vkDestroyDescriptorPool(host_device, host_descriptorPool, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDescriptorPool host_descriptorPool = VkVirtualHandleGetHost<VkDescriptorPool>((uint32_t)(uintptr_t)descriptorPool);
    VkResult res = s_dvk(host_device)->vkResetDescriptorPool(host_device, host_descriptorPool, flags);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDescriptorSetAllocateInfo* host_pAllocateInfo = AllocForHost<VkDescriptorSetAllocateInfo>(1, pAllocateInfo);
    VkResult res = s_dvk(host_device)->vkAllocateDescriptorSets(host_device, host_pAllocateInfo, pDescriptorSets);

    // host->guest handle conversions
    if (pDescriptorSets) { VkVirtualHandleToGuestArray<VkDescriptorSet>((pAllocateInfo->descriptorSetCount), pDescriptorSets, pDescriptorSets); }
    return res;
}

static VkResult s_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, u32 descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkDescriptorPool host_descriptorPool = VkVirtualHandleGetHost<VkDescriptorPool>((uint32_t)(uintptr_t)descriptorPool);
    VkDescriptorSet* host_pDescriptorSets = AllocForHost<VkDescriptorSet>(descriptorSetCount, pDescriptorSets);
    VkResult res = s_dvk(host_device)->vkFreeDescriptorSets(host_device, host_descriptorPool, descriptorSetCount, host_pDescriptorSets);

    // host->guest handle conversions
    return res;
}

static void s_vkUpdateDescriptorSets(VkDevice device, u32 descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, u32 descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkWriteDescriptorSet* host_pDescriptorWrites = AllocForHost<VkWriteDescriptorSet>(descriptorWriteCount, pDescriptorWrites);
    VkCopyDescriptorSet* host_pDescriptorCopies = AllocForHost<VkCopyDescriptorSet>(descriptorCopyCount, pDescriptorCopies);
    s_dvk(host_device)->vkUpdateDescriptorSets(host_device, descriptorWriteCount, host_pDescriptorWrites, descriptorCopyCount, host_pDescriptorCopies);

    // host->guest handle conversions
}

static VkResult s_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkFramebufferCreateInfo* host_pCreateInfo = AllocForHost<VkFramebufferCreateInfo>(1, pCreateInfo);
    VkResult res = s_dvk(host_device)->vkCreateFramebuffer(host_device, host_pCreateInfo, pAllocator, pFramebuffer);

    // host->guest handle conversions
    if (pFramebuffer) { VkVirtualHandleToGuestArray<VkFramebuffer>(1, pFramebuffer, pFramebuffer); }
    return res;
}

static void s_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkFramebuffer host_framebuffer = VkVirtualHandleGetHost<VkFramebuffer>((uint32_t)(uintptr_t)framebuffer);
    s_dvk(host_device)->vkDestroyFramebuffer(host_device, host_framebuffer, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateRenderPass(host_device, pCreateInfo, pAllocator, pRenderPass);

    // host->guest handle conversions
    if (pRenderPass) { VkVirtualHandleToGuestArray<VkRenderPass>(1, pRenderPass, pRenderPass); }
    return res;
}

static void s_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkRenderPass host_renderPass = VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)renderPass);
    s_dvk(host_device)->vkDestroyRenderPass(host_device, host_renderPass, pAllocator);

    // host->guest handle conversions
}

static void s_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkRenderPass host_renderPass = VkVirtualHandleGetHost<VkRenderPass>((uint32_t)(uintptr_t)renderPass);
    s_dvk(host_device)->vkGetRenderAreaGranularity(host_device, host_renderPass, pGranularity);

    // host->guest handle conversions
}

static VkResult s_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkResult res = s_dvk(host_device)->vkCreateCommandPool(host_device, pCreateInfo, pAllocator, pCommandPool);

    // host->guest handle conversions
    if (pCommandPool) { VkVirtualHandleToGuestArray<VkCommandPool>(1, pCommandPool, pCommandPool); }
    return res;
}

static void s_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkCommandPool host_commandPool = VkVirtualHandleGetHost<VkCommandPool>((uint32_t)(uintptr_t)commandPool);
    s_dvk(host_device)->vkDestroyCommandPool(host_device, host_commandPool, pAllocator);

    // host->guest handle conversions
}

static VkResult s_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkCommandPool host_commandPool = VkVirtualHandleGetHost<VkCommandPool>((uint32_t)(uintptr_t)commandPool);
    VkResult res = s_dvk(host_device)->vkResetCommandPool(host_device, host_commandPool, flags);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkCommandBufferAllocateInfo* host_pAllocateInfo = AllocForHost<VkCommandBufferAllocateInfo>(1, pAllocateInfo);
    VkResult res = s_dvk(host_device)->vkAllocateCommandBuffers(host_device, host_pAllocateInfo, pCommandBuffers);
    s_dvkCmdBuf_new_multi(host_device, pAllocateInfo, pCommandBuffers);

    // host->guest handle conversions
    if (pCommandBuffers) { VkVirtualHandleToGuestArray<VkCommandBuffer>((pAllocateInfo->commandBufferCount), pCommandBuffers, pCommandBuffers); }
    return res;
}

static void s_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, u32 commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkDevice host_device = VkVirtualHandleGetHost<VkDevice>((uint32_t)(uintptr_t)device);
    VkCommandPool host_commandPool = VkVirtualHandleGetHost<VkCommandPool>((uint32_t)(uintptr_t)commandPool);
    VkCommandBuffer* host_pCommandBuffers = AllocForHost<VkCommandBuffer>(commandBufferCount, pCommandBuffers);
    s_dvk(host_device)->vkFreeCommandBuffers(host_device, host_commandPool, commandBufferCount, host_pCommandBuffers);

    // host->guest handle conversions
}

static VkResult s_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkCommandBufferBeginInfo* host_pBeginInfo = AllocForHost<VkCommandBufferBeginInfo>(1, pBeginInfo);
    VkResult res = s_dvkCmdBuf(host_commandBuffer)->vkBeginCommandBuffer(host_commandBuffer, host_pBeginInfo);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkEndCommandBuffer(VkCommandBuffer commandBuffer) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkResult res = s_dvkCmdBuf(host_commandBuffer)->vkEndCommandBuffer(host_commandBuffer);

    // host->guest handle conversions
    return res;
}

static VkResult s_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkResult res = s_dvkCmdBuf(host_commandBuffer)->vkResetCommandBuffer(host_commandBuffer, flags);

    // host->guest handle conversions
    return res;
}

static void s_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkPipeline host_pipeline = VkVirtualHandleGetHost<VkPipeline>((uint32_t)(uintptr_t)pipeline);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBindPipeline(host_commandBuffer, pipelineBindPoint, host_pipeline);

    // host->guest handle conversions
}

static void s_vkCmdSetViewport(VkCommandBuffer commandBuffer, u32 firstViewport, u32 viewportCount, const VkViewport* pViewports) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetViewport(host_commandBuffer, firstViewport, viewportCount, pViewports);

    // host->guest handle conversions
}

static void s_vkCmdSetScissor(VkCommandBuffer commandBuffer, u32 firstScissor, u32 scissorCount, const VkRect2D* pScissors) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetScissor(host_commandBuffer, firstScissor, scissorCount, pScissors);

    // host->guest handle conversions
}

static void s_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, f32 lineWidth) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetLineWidth(host_commandBuffer, lineWidth);

    // host->guest handle conversions
}

static void s_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, f32 depthBiasConstantFactor, f32 depthBiasClamp, f32 depthBiasSlopeFactor) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetDepthBias(host_commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);

    // host->guest handle conversions
}

static void s_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, f32 minDepthBounds, f32 maxDepthBounds) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetDepthBounds(host_commandBuffer, minDepthBounds, maxDepthBounds);

    // host->guest handle conversions
}

static void s_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 compareMask) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetStencilCompareMask(host_commandBuffer, faceMask, compareMask);

    // host->guest handle conversions
}

static void s_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 writeMask) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetStencilWriteMask(host_commandBuffer, faceMask, writeMask);

    // host->guest handle conversions
}

static void s_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 reference) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetStencilReference(host_commandBuffer, faceMask, reference);

    // host->guest handle conversions
}

static void s_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount, const VkDescriptorSet* pDescriptorSets, u32 dynamicOffsetCount, const u32* pDynamicOffsets) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkPipelineLayout host_layout = VkVirtualHandleGetHost<VkPipelineLayout>((uint32_t)(uintptr_t)layout);
    VkDescriptorSet* host_pDescriptorSets = AllocForHost<VkDescriptorSet>(descriptorSetCount, pDescriptorSets);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBindDescriptorSets(host_commandBuffer, pipelineBindPoint, host_layout, firstSet, descriptorSetCount, host_pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);

    // host->guest handle conversions
}

static void s_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBindIndexBuffer(host_commandBuffer, host_buffer, offset, indexType);

    // host->guest handle conversions
}

static void s_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, u32 firstBinding, u32 bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer* host_pBuffers = AllocForHost<VkBuffer>(bindingCount, pBuffers);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBindVertexBuffers(host_commandBuffer, firstBinding, bindingCount, host_pBuffers, pOffsets);

    // host->guest handle conversions
}

static void s_vkCmdDraw(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdDraw(host_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

    // host->guest handle conversions
}

static void s_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdDrawIndexed(host_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

    // host->guest handle conversions
}

static void s_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdDrawIndirect(host_commandBuffer, host_buffer, offset, drawCount, stride);

    // host->guest handle conversions
}

static void s_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdDrawIndexedIndirect(host_commandBuffer, host_buffer, offset, drawCount, stride);

    // host->guest handle conversions
}

static void s_vkCmdDispatch(VkCommandBuffer commandBuffer, u32 groupCountX, u32 groupCountY, u32 groupCountZ) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdDispatch(host_commandBuffer, groupCountX, groupCountY, groupCountZ);

    // host->guest handle conversions
}

static void s_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_buffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)buffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdDispatchIndirect(host_commandBuffer, host_buffer, offset);

    // host->guest handle conversions
}

static void s_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, u32 regionCount, const VkBufferCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_srcBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)srcBuffer);
    VkBuffer host_dstBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)dstBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdCopyBuffer(host_commandBuffer, host_srcBuffer, host_dstBuffer, regionCount, pRegions);

    // host->guest handle conversions
}

static void s_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkImage host_srcImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)srcImage);
    VkImage host_dstImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)dstImage);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdCopyImage(host_commandBuffer, host_srcImage, srcImageLayout, host_dstImage, dstImageLayout, regionCount, pRegions);

    // host->guest handle conversions
}

static void s_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageBlit* pRegions, VkFilter filter) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkImage host_srcImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)srcImage);
    VkImage host_dstImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)dstImage);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBlitImage(host_commandBuffer, host_srcImage, srcImageLayout, host_dstImage, dstImageLayout, regionCount, pRegions, filter);

    // host->guest handle conversions
}

static void s_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkBufferImageCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_srcBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)srcBuffer);
    VkImage host_dstImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)dstImage);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdCopyBufferToImage(host_commandBuffer, host_srcBuffer, host_dstImage, dstImageLayout, regionCount, pRegions);

    // host->guest handle conversions
}

static void s_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, u32 regionCount, const VkBufferImageCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkImage host_srcImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)srcImage);
    VkBuffer host_dstBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)dstBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdCopyImageToBuffer(host_commandBuffer, host_srcImage, srcImageLayout, host_dstBuffer, regionCount, pRegions);

    // host->guest handle conversions
}

static void s_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_dstBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)dstBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdUpdateBuffer(host_commandBuffer, host_dstBuffer, dstOffset, dataSize, pData);

    // host->guest handle conversions
}

static void s_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, u32 data) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBuffer host_dstBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)dstBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdFillBuffer(host_commandBuffer, host_dstBuffer, dstOffset, size, data);

    // host->guest handle conversions
}

static void s_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, u32 rangeCount, const VkImageSubresourceRange* pRanges) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdClearColorImage(host_commandBuffer, host_image, imageLayout, pColor, rangeCount, pRanges);

    // host->guest handle conversions
}

static void s_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, u32 rangeCount, const VkImageSubresourceRange* pRanges) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkImage host_image = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)image);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdClearDepthStencilImage(host_commandBuffer, host_image, imageLayout, pDepthStencil, rangeCount, pRanges);

    // host->guest handle conversions
}

static void s_vkCmdClearAttachments(VkCommandBuffer commandBuffer, u32 attachmentCount, const VkClearAttachment* pAttachments, u32 rectCount, const VkClearRect* pRects) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdClearAttachments(host_commandBuffer, attachmentCount, pAttachments, rectCount, pRects);

    // host->guest handle conversions
}

static void s_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageResolve* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkImage host_srcImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)srcImage);
    VkImage host_dstImage = VkVirtualHandleGetHost<VkImage>((uint32_t)(uintptr_t)dstImage);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdResolveImage(host_commandBuffer, host_srcImage, srcImageLayout, host_dstImage, dstImageLayout, regionCount, pRegions);

    // host->guest handle conversions
}

static void s_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkEvent host_event = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)event);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdSetEvent(host_commandBuffer, host_event, stageMask);

    // host->guest handle conversions
}

static void s_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkEvent host_event = VkVirtualHandleGetHost<VkEvent>((uint32_t)(uintptr_t)event);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdResetEvent(host_commandBuffer, host_event, stageMask);

    // host->guest handle conversions
}

static void s_vkCmdWaitEvents(VkCommandBuffer commandBuffer, u32 eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, u32 memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, u32 bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkEvent* host_pEvents = AllocForHost<VkEvent>(eventCount, pEvents);
    VkBufferMemoryBarrier* host_pBufferMemoryBarriers = AllocForHost<VkBufferMemoryBarrier>(bufferMemoryBarrierCount, pBufferMemoryBarriers);
    VkImageMemoryBarrier* host_pImageMemoryBarriers = AllocForHost<VkImageMemoryBarrier>(imageMemoryBarrierCount, pImageMemoryBarriers);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdWaitEvents(host_commandBuffer, eventCount, host_pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, host_pBufferMemoryBarriers, imageMemoryBarrierCount, host_pImageMemoryBarriers);

    // host->guest handle conversions
}

static void s_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, u32 memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, u32 bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkBufferMemoryBarrier* host_pBufferMemoryBarriers = AllocForHost<VkBufferMemoryBarrier>(bufferMemoryBarrierCount, pBufferMemoryBarriers);
    VkImageMemoryBarrier* host_pImageMemoryBarriers = AllocForHost<VkImageMemoryBarrier>(imageMemoryBarrierCount, pImageMemoryBarriers);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdPipelineBarrier(host_commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, host_pBufferMemoryBarriers, imageMemoryBarrierCount, host_pImageMemoryBarriers);

    // host->guest handle conversions
}

static void s_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 query, VkQueryControlFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBeginQuery(host_commandBuffer, host_queryPool, query, flags);

    // host->guest handle conversions
}

static void s_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 query) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdEndQuery(host_commandBuffer, host_queryPool, query);

    // host->guest handle conversions
}

static void s_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 firstQuery, u32 queryCount) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdResetQueryPool(host_commandBuffer, host_queryPool, firstQuery, queryCount);

    // host->guest handle conversions
}

static void s_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, u32 query) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdWriteTimestamp(host_commandBuffer, pipelineStage, host_queryPool, query);

    // host->guest handle conversions
}

static void s_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 firstQuery, u32 queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkQueryPool host_queryPool = VkVirtualHandleGetHost<VkQueryPool>((uint32_t)(uintptr_t)queryPool);
    VkBuffer host_dstBuffer = VkVirtualHandleGetHost<VkBuffer>((uint32_t)(uintptr_t)dstBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdCopyQueryPoolResults(host_commandBuffer, host_queryPool, firstQuery, queryCount, host_dstBuffer, dstOffset, stride, flags);

    // host->guest handle conversions
}

static void s_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, u32 offset, u32 size, const void* pValues) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkPipelineLayout host_layout = VkVirtualHandleGetHost<VkPipelineLayout>((uint32_t)(uintptr_t)layout);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdPushConstants(host_commandBuffer, host_layout, stageFlags, offset, size, pValues);

    // host->guest handle conversions
}

static void s_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkRenderPassBeginInfo* host_pRenderPassBegin = AllocForHost<VkRenderPassBeginInfo>(1, pRenderPassBegin);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdBeginRenderPass(host_commandBuffer, host_pRenderPassBegin, contents);

    // host->guest handle conversions
}

static void s_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdNextSubpass(host_commandBuffer, contents);

    // host->guest handle conversions
}

static void s_vkCmdEndRenderPass(VkCommandBuffer commandBuffer) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdEndRenderPass(host_commandBuffer);

    // host->guest handle conversions
}

static void s_vkCmdExecuteCommands(VkCommandBuffer commandBuffer, u32 commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    fprintf(stderr, "%s: call\n", __func__);

    // guest->host handle conversions
    VkCommandBuffer host_commandBuffer = VkVirtualHandleGetHost<VkCommandBuffer>((uint32_t)(uintptr_t)commandBuffer);
    VkCommandBuffer* host_pCommandBuffers = AllocForHost<VkCommandBuffer>(commandBufferCount, pCommandBuffers);
    s_dvkCmdBuf(host_commandBuffer)->vkCmdExecuteCommands(host_commandBuffer, commandBufferCount, host_pCommandBuffers);

    // host->guest handle conversions
}

void initVulkanHostFuncs(vk_decoder_context_t* dec) {
    dec->vkMapMemoryAEMU = s_vkMapMemoryAEMU;
    dec->vkUnmapMemoryAEMU = s_vkUnmapMemoryAEMU;
    dec->vkFlushMappedMemoryRangeAEMU = s_vkFlushMappedMemoryRangeAEMU;
    dec->vkInvalidateMappedMemoryRangeAEMU = s_vkInvalidateMappedMemoryRangeAEMU;
    dec->vkCmdSetBlendConstantsAEMU = s_vkCmdSetBlendConstantsAEMU;
    dec->vkCreateInstance = s_vkCreateInstance;
    dec->vkDestroyInstance = s_vkDestroyInstance;
    dec->vkEnumeratePhysicalDevices = s_vkEnumeratePhysicalDevices;
    dec->vkGetPhysicalDeviceProperties = s_vkGetPhysicalDeviceProperties;
    dec->vkGetPhysicalDeviceQueueFamilyProperties = s_vkGetPhysicalDeviceQueueFamilyProperties;
    dec->vkGetPhysicalDeviceMemoryProperties = s_vkGetPhysicalDeviceMemoryProperties;
    dec->vkGetPhysicalDeviceFeatures = s_vkGetPhysicalDeviceFeatures;
    dec->vkGetPhysicalDeviceFormatProperties = s_vkGetPhysicalDeviceFormatProperties;
    dec->vkGetPhysicalDeviceImageFormatProperties = s_vkGetPhysicalDeviceImageFormatProperties;
    dec->vkCreateDevice = s_vkCreateDevice;
    dec->vkDestroyDevice = s_vkDestroyDevice;
    dec->vkEnumerateInstanceLayerProperties = s_vkEnumerateInstanceLayerProperties;
    dec->vkEnumerateInstanceExtensionProperties = s_vkEnumerateInstanceExtensionProperties;
    dec->vkEnumerateDeviceLayerProperties = s_vkEnumerateDeviceLayerProperties;
    dec->vkEnumerateDeviceExtensionProperties = s_vkEnumerateDeviceExtensionProperties;
    dec->vkGetDeviceQueue = s_vkGetDeviceQueue;
    dec->vkQueueSubmit = s_vkQueueSubmit;
    dec->vkQueueWaitIdle = s_vkQueueWaitIdle;
    dec->vkDeviceWaitIdle = s_vkDeviceWaitIdle;
    dec->vkAllocateMemory = s_vkAllocateMemory;
    dec->vkFreeMemory = s_vkFreeMemory;
    dec->vkGetDeviceMemoryCommitment = s_vkGetDeviceMemoryCommitment;
    dec->vkGetBufferMemoryRequirements = s_vkGetBufferMemoryRequirements;
    dec->vkBindBufferMemory = s_vkBindBufferMemory;
    dec->vkGetImageMemoryRequirements = s_vkGetImageMemoryRequirements;
    dec->vkBindImageMemory = s_vkBindImageMemory;
    dec->vkGetImageSparseMemoryRequirements = s_vkGetImageSparseMemoryRequirements;
    dec->vkGetPhysicalDeviceSparseImageFormatProperties = s_vkGetPhysicalDeviceSparseImageFormatProperties;
    dec->vkQueueBindSparse = s_vkQueueBindSparse;
    dec->vkCreateFence = s_vkCreateFence;
    dec->vkDestroyFence = s_vkDestroyFence;
    dec->vkResetFences = s_vkResetFences;
    dec->vkGetFenceStatus = s_vkGetFenceStatus;
    dec->vkWaitForFences = s_vkWaitForFences;
    dec->vkCreateSemaphore = s_vkCreateSemaphore;
    dec->vkDestroySemaphore = s_vkDestroySemaphore;
    dec->vkCreateEvent = s_vkCreateEvent;
    dec->vkDestroyEvent = s_vkDestroyEvent;
    dec->vkGetEventStatus = s_vkGetEventStatus;
    dec->vkSetEvent = s_vkSetEvent;
    dec->vkResetEvent = s_vkResetEvent;
    dec->vkCreateQueryPool = s_vkCreateQueryPool;
    dec->vkDestroyQueryPool = s_vkDestroyQueryPool;
    dec->vkGetQueryPoolResults = s_vkGetQueryPoolResults;
    dec->vkCreateBuffer = s_vkCreateBuffer;
    dec->vkDestroyBuffer = s_vkDestroyBuffer;
    dec->vkCreateBufferView = s_vkCreateBufferView;
    dec->vkDestroyBufferView = s_vkDestroyBufferView;
    dec->vkCreateImage = s_vkCreateImage;
    dec->vkDestroyImage = s_vkDestroyImage;
    dec->vkGetImageSubresourceLayout = s_vkGetImageSubresourceLayout;
    dec->vkCreateImageView = s_vkCreateImageView;
    dec->vkDestroyImageView = s_vkDestroyImageView;
    dec->vkCreateShaderModule = s_vkCreateShaderModule;
    dec->vkDestroyShaderModule = s_vkDestroyShaderModule;
    dec->vkCreatePipelineCache = s_vkCreatePipelineCache;
    dec->vkDestroyPipelineCache = s_vkDestroyPipelineCache;
    dec->vkGetPipelineCacheData = s_vkGetPipelineCacheData;
    dec->vkMergePipelineCaches = s_vkMergePipelineCaches;
    dec->vkCreateGraphicsPipelines = s_vkCreateGraphicsPipelines;
    dec->vkCreateComputePipelines = s_vkCreateComputePipelines;
    dec->vkDestroyPipeline = s_vkDestroyPipeline;
    dec->vkCreatePipelineLayout = s_vkCreatePipelineLayout;
    dec->vkDestroyPipelineLayout = s_vkDestroyPipelineLayout;
    dec->vkCreateSampler = s_vkCreateSampler;
    dec->vkDestroySampler = s_vkDestroySampler;
    dec->vkCreateDescriptorSetLayout = s_vkCreateDescriptorSetLayout;
    dec->vkDestroyDescriptorSetLayout = s_vkDestroyDescriptorSetLayout;
    dec->vkCreateDescriptorPool = s_vkCreateDescriptorPool;
    dec->vkDestroyDescriptorPool = s_vkDestroyDescriptorPool;
    dec->vkResetDescriptorPool = s_vkResetDescriptorPool;
    dec->vkAllocateDescriptorSets = s_vkAllocateDescriptorSets;
    dec->vkFreeDescriptorSets = s_vkFreeDescriptorSets;
    dec->vkUpdateDescriptorSets = s_vkUpdateDescriptorSets;
    dec->vkCreateFramebuffer = s_vkCreateFramebuffer;
    dec->vkDestroyFramebuffer = s_vkDestroyFramebuffer;
    dec->vkCreateRenderPass = s_vkCreateRenderPass;
    dec->vkDestroyRenderPass = s_vkDestroyRenderPass;
    dec->vkGetRenderAreaGranularity = s_vkGetRenderAreaGranularity;
    dec->vkCreateCommandPool = s_vkCreateCommandPool;
    dec->vkDestroyCommandPool = s_vkDestroyCommandPool;
    dec->vkResetCommandPool = s_vkResetCommandPool;
    dec->vkAllocateCommandBuffers = s_vkAllocateCommandBuffers;
    dec->vkFreeCommandBuffers = s_vkFreeCommandBuffers;
    dec->vkBeginCommandBuffer = s_vkBeginCommandBuffer;
    dec->vkEndCommandBuffer = s_vkEndCommandBuffer;
    dec->vkResetCommandBuffer = s_vkResetCommandBuffer;
    dec->vkCmdBindPipeline = s_vkCmdBindPipeline;
    dec->vkCmdSetViewport = s_vkCmdSetViewport;
    dec->vkCmdSetScissor = s_vkCmdSetScissor;
    dec->vkCmdSetLineWidth = s_vkCmdSetLineWidth;
    dec->vkCmdSetDepthBias = s_vkCmdSetDepthBias;
    dec->vkCmdSetDepthBounds = s_vkCmdSetDepthBounds;
    dec->vkCmdSetStencilCompareMask = s_vkCmdSetStencilCompareMask;
    dec->vkCmdSetStencilWriteMask = s_vkCmdSetStencilWriteMask;
    dec->vkCmdSetStencilReference = s_vkCmdSetStencilReference;
    dec->vkCmdBindDescriptorSets = s_vkCmdBindDescriptorSets;
    dec->vkCmdBindIndexBuffer = s_vkCmdBindIndexBuffer;
    dec->vkCmdBindVertexBuffers = s_vkCmdBindVertexBuffers;
    dec->vkCmdDraw = s_vkCmdDraw;
    dec->vkCmdDrawIndexed = s_vkCmdDrawIndexed;
    dec->vkCmdDrawIndirect = s_vkCmdDrawIndirect;
    dec->vkCmdDrawIndexedIndirect = s_vkCmdDrawIndexedIndirect;
    dec->vkCmdDispatch = s_vkCmdDispatch;
    dec->vkCmdDispatchIndirect = s_vkCmdDispatchIndirect;
    dec->vkCmdCopyBuffer = s_vkCmdCopyBuffer;
    dec->vkCmdCopyImage = s_vkCmdCopyImage;
    dec->vkCmdBlitImage = s_vkCmdBlitImage;
    dec->vkCmdCopyBufferToImage = s_vkCmdCopyBufferToImage;
    dec->vkCmdCopyImageToBuffer = s_vkCmdCopyImageToBuffer;
    dec->vkCmdUpdateBuffer = s_vkCmdUpdateBuffer;
    dec->vkCmdFillBuffer = s_vkCmdFillBuffer;
    dec->vkCmdClearColorImage = s_vkCmdClearColorImage;
    dec->vkCmdClearDepthStencilImage = s_vkCmdClearDepthStencilImage;
    dec->vkCmdClearAttachments = s_vkCmdClearAttachments;
    dec->vkCmdResolveImage = s_vkCmdResolveImage;
    dec->vkCmdSetEvent = s_vkCmdSetEvent;
    dec->vkCmdResetEvent = s_vkCmdResetEvent;
    dec->vkCmdWaitEvents = s_vkCmdWaitEvents;
    dec->vkCmdPipelineBarrier = s_vkCmdPipelineBarrier;
    dec->vkCmdBeginQuery = s_vkCmdBeginQuery;
    dec->vkCmdEndQuery = s_vkCmdEndQuery;
    dec->vkCmdResetQueryPool = s_vkCmdResetQueryPool;
    dec->vkCmdWriteTimestamp = s_vkCmdWriteTimestamp;
    dec->vkCmdCopyQueryPoolResults = s_vkCmdCopyQueryPoolResults;
    dec->vkCmdPushConstants = s_vkCmdPushConstants;
    dec->vkCmdBeginRenderPass = s_vkCmdBeginRenderPass;
    dec->vkCmdNextSubpass = s_vkCmdNextSubpass;
    dec->vkCmdEndRenderPass = s_vkCmdEndRenderPass;
    dec->vkCmdExecuteCommands = s_vkCmdExecuteCommands;
}
