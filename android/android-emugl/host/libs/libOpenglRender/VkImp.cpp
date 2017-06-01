
#include "OpenGLESDispatch/VkDispatch.h"
#include "VkHandleDispatch.h"
#include "VkVirtualHandle.h"

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
    return VK_SUCCESS;
}

static void s_vkUnmapMemoryAEMU(void* self, VkDevice device, VkDeviceMemory memory, uint32_t datalen, const char* data) {
    fprintf(stderr, "%s: call\n", __func__);
    return;
}

static VkResult s_vkFlushMappedMemoryRangeAEMU(void* self, VkDevice device, const VkMappedMemoryRange* memoryRange, uint32_t datalen, const char* data) {
    fprintf(stderr, "%s: call\n", __func__);
    return VK_SUCCESS;
}

static VkResult s_vkInvalidateMappedMemoryRangeAEMU(void* self, VkDevice device, const VkMappedMemoryRange* memoryRange, uint32_t datalen, char* data) {
    fprintf(stderr, "%s: call\n", __func__);
    return VK_SUCCESS;
}

static void s_vkCmdSetBlendConstantsAEMU(void* self, VkCommandBuffer commandBuffer, f32_4 blendConstants) {
    fprintf(stderr, "%s: call\n", __func__);
    return;
}

static VkResult s_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    fprintf(stderr, "%s: call\n", __func__);
    VkResult res = vkGlobalDispatcher()->vkCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (res == VK_SUCCESS) { sMapInstanceDispatch(*pInstance); }
    uint32_t guestHandle = VkVirtualHandleCreate<VkInstance>(*pInstance);
    fprintf(stderr, "%s: guest handle %u\n", __func__, guestHandle);
    *pInstance = (VkInstance)(uintptr_t)guestHandle;
    return res;
}

static void s_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_ivk(instance)->vkDestroyInstance(instance, pAllocator);
}

static VkResult s_vkEnumeratePhysicalDevices(VkInstance instance, u32* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) {
    fprintf(stderr, "%s: call\n", __func__);
    VkInstance host_instance = VkVirtualHandleGetHost<VkInstance>((uint32_t)(uintptr_t)instance);
    fprintf(stderr, "%s: got guest handle %u -> host %lu\n", __func__, instance, host_instance);
    VkResult res = s_ivk(host_instance)->vkEnumeratePhysicalDevices(host_instance, pPhysicalDeviceCount, pPhysicalDevices);
    if (res == VK_SUCCESS) { fprintf(stderr, "%s: success got phys devs\n", __func__); s_ivkPhysdev_new_multi(host_instance, *pPhysicalDeviceCount, pPhysicalDevices); }
    if (pPhysicalDevices && pPhysicalDeviceCount && pPhysicalDeviceCount > 0) {
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
            fprintf(stderr, "%s: physical device: host handle %lu\n", __func__, pPhysicalDevices[i]);
            pPhysicalDevices[i] = (VkPhysicalDevice)(uintptr_t)VkVirtualHandleCreateOrGet(pPhysicalDevices[i]);
            fprintf(stderr, "%s: physical device: guest handle %lu\n", __func__, pPhysicalDevices[i]);
        }
    }
    return res;
}

static void s_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) {
    VkPhysicalDevice host_physicalDevice = VkVirtualHandleGetHost<VkPhysicalDevice>((uint32_t)(uintptr_t)physicalDevice);
    fprintf(stderr, "%s: call. guest %u -> host %lu\n", __func__, (uint32_t)(uintptr_t)physicalDevice, host_physicalDevice);
    VkPhysicalDeviceProperties out;
    fprintf(stderr, "%s: calling host driver\n", __func__);
    s_ivkPhysdev(host_physicalDevice)->vkGetPhysicalDeviceProperties(host_physicalDevice, &out);
    fprintf(stderr, "%s: memcpy\n", __func__);
    memcpy(pProperties, &out, sizeof(VkPhysicalDeviceProperties));
    fprintf(stderr, "%s: exit\n", __func__);
}

static void s_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, u32* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    s_ivkPhysdev(physicalDevice)->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

static void s_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    s_ivkPhysdev(physicalDevice)->vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

static void s_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) {
    fprintf(stderr, "%s: call\n", __func__);
    s_ivkPhysdev(physicalDevice)->vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

static void s_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    s_ivkPhysdev(physicalDevice)->vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

static VkResult s_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_ivkPhysdev(physicalDevice)->vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

static VkResult s_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    fprintf(stderr, "%s: call\n", __func__);
    VkResult res = s_ivkPhysdev(physicalDevice)->vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (res == VK_SUCCESS) { s_dvk_new(physicalDevice, *pDevice); }
    return res;
}

static void s_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyDevice(device, pAllocator);
}

static VkResult s_vkEnumerateInstanceLayerProperties(u32* pPropertyCount, VkLayerProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    return vkGlobalDispatcher()->vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

static VkResult s_vkEnumerateInstanceExtensionProperties(const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    return vkGlobalDispatcher()->vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

static VkResult s_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, u32* pPropertyCount, VkLayerProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_ivkPhysdev(physicalDevice)->vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}

static VkResult s_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, u32* pPropertyCount, VkExtensionProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_ivkPhysdev(physicalDevice)->vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

static void s_vkGetDeviceQueue(VkDevice device, u32 queueFamilyIndex, u32 queueIndex, VkQueue* pQueue) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    s_dvkQueue_new(device, *pQueue);
}

static VkResult s_vkQueueSubmit(VkQueue queue, u32 submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvkQueue(queue)->vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

static VkResult s_vkQueueWaitIdle(VkQueue queue) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvkQueue(queue)->vkQueueWaitIdle(queue);
}

static VkResult s_vkDeviceWaitIdle(VkDevice device) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkDeviceWaitIdle(device);
}

static VkResult s_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

static void s_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkFreeMemory(device, memory, pAllocator);
}

static void s_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

static void s_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

static VkResult s_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

static void s_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

static VkResult s_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkBindImageMemory(device, image, memory, memoryOffset);
}

static void s_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, u32* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

static void s_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, u32* pPropertyCount, VkSparseImageFormatProperties* pProperties) {
    fprintf(stderr, "%s: call\n", __func__);
    s_ivkPhysdev(physicalDevice)->vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

static VkResult s_vkQueueBindSparse(VkQueue queue, u32 bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvkQueue(queue)->vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}

static VkResult s_vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

static void s_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyFence(device, fence, pAllocator);
}

static VkResult s_vkResetFences(VkDevice device, u32 fenceCount, const VkFence* pFences) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkResetFences(device, fenceCount, pFences);
}

static VkResult s_vkGetFenceStatus(VkDevice device, VkFence fence) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkGetFenceStatus(device, fence);
}

static VkResult s_vkWaitForFences(VkDevice device, u32 fenceCount, const VkFence* pFences, VkBool32 waitAll, u64 timeout) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

static VkResult s_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

static void s_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroySemaphore(device, semaphore, pAllocator);
}

static VkResult s_vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateEvent(device, pCreateInfo, pAllocator, pEvent);
}

static void s_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyEvent(device, event, pAllocator);
}

static VkResult s_vkGetEventStatus(VkDevice device, VkEvent event) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkGetEventStatus(device, event);
}

static VkResult s_vkSetEvent(VkDevice device, VkEvent event) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkSetEvent(device, event);
}

static VkResult s_vkResetEvent(VkDevice device, VkEvent event) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkResetEvent(device, event);
}

static VkResult s_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
}

static void s_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyQueryPool(device, queryPool, pAllocator);
}

static VkResult s_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, u32 firstQuery, u32 queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkGetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

static VkResult s_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

static void s_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyBuffer(device, buffer, pAllocator);
}

static VkResult s_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
}

static void s_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyBufferView(device, bufferView, pAllocator);
}

static VkResult s_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

static void s_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyImage(device, image, pAllocator);
}

static void s_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

static VkResult s_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

static void s_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyImageView(device, imageView, pAllocator);
}

static VkResult s_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

static void s_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyShaderModule(device, shaderModule, pAllocator);
}

static VkResult s_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
}

static void s_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyPipelineCache(device, pipelineCache, pAllocator);
}

static VkResult s_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkGetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}

static VkResult s_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, u32 srcCacheCount, const VkPipelineCache* pSrcCaches) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkMergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
}

static VkResult s_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, u32 createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

static VkResult s_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, u32 createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

static void s_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyPipeline(device, pipeline, pAllocator);
}

static VkResult s_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

static void s_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

static VkResult s_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

static void s_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroySampler(device, sampler, pAllocator);
}

static VkResult s_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

static void s_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

static VkResult s_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

static void s_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

static VkResult s_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkResetDescriptorPool(device, descriptorPool, flags);
}

static VkResult s_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

static VkResult s_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, u32 descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

static void s_vkUpdateDescriptorSets(VkDevice device, u32 descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, u32 descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

static VkResult s_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

static void s_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

static VkResult s_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

static void s_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyRenderPass(device, renderPass, pAllocator);
}

static void s_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkGetRenderAreaGranularity(device, renderPass, pGranularity);
}

static VkResult s_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

static void s_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkDestroyCommandPool(device, commandPool, pAllocator);
}

static VkResult s_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvk(device)->vkResetCommandPool(device, commandPool, flags);
}

static VkResult s_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) {
    fprintf(stderr, "%s: call\n", __func__);
    VkResult res = s_dvk(device)->vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    s_dvkCmdBuf_new_multi(device, pAllocateInfo, pCommandBuffers);
    return res;
}

static void s_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, u32 commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvk(device)->vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

static VkResult s_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvkCmdBuf(commandBuffer)->vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

static VkResult s_vkEndCommandBuffer(VkCommandBuffer commandBuffer) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvkCmdBuf(commandBuffer)->vkEndCommandBuffer(commandBuffer);
}

static VkResult s_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);
    return s_dvkCmdBuf(commandBuffer)->vkResetCommandBuffer(commandBuffer, flags);
}

static void s_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

static void s_vkCmdSetViewport(VkCommandBuffer commandBuffer, u32 firstViewport, u32 viewportCount, const VkViewport* pViewports) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

static void s_vkCmdSetScissor(VkCommandBuffer commandBuffer, u32 firstScissor, u32 scissorCount, const VkRect2D* pScissors) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

static void s_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, f32 lineWidth) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetLineWidth(commandBuffer, lineWidth);
}

static void s_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, f32 depthBiasConstantFactor, f32 depthBiasClamp, f32 depthBiasSlopeFactor) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

static void s_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, f32 minDepthBounds, f32 maxDepthBounds) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

static void s_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 compareMask) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

static void s_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 writeMask) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

static void s_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, u32 reference) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetStencilReference(commandBuffer, faceMask, reference);
}

static void s_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, u32 firstSet, u32 descriptorSetCount, const VkDescriptorSet* pDescriptorSets, u32 dynamicOffsetCount, const u32* pDynamicOffsets) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

static void s_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

static void s_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, u32 firstBinding, u32 bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

static void s_vkCmdDraw(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

static void s_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

static void s_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

static void s_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

static void s_vkCmdDispatch(VkCommandBuffer commandBuffer, u32 groupCountX, u32 groupCountY, u32 groupCountZ) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

static void s_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdDispatchIndirect(commandBuffer, buffer, offset);
}

static void s_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, u32 regionCount, const VkBufferCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

static void s_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

static void s_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageBlit* pRegions, VkFilter filter) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

static void s_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkBufferImageCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

static void s_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, u32 regionCount, const VkBufferImageCopy* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

static void s_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

static void s_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, u32 data) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

static void s_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, u32 rangeCount, const VkImageSubresourceRange* pRanges) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

static void s_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, u32 rangeCount, const VkImageSubresourceRange* pRanges) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

static void s_vkCmdClearAttachments(VkCommandBuffer commandBuffer, u32 attachmentCount, const VkClearAttachment* pAttachments, u32 rectCount, const VkClearRect* pRects) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

static void s_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, u32 regionCount, const VkImageResolve* pRegions) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

static void s_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdSetEvent(commandBuffer, event, stageMask);
}

static void s_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdResetEvent(commandBuffer, event, stageMask);
}

static void s_vkCmdWaitEvents(VkCommandBuffer commandBuffer, u32 eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, u32 memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, u32 bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdWaitEvents(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

static void s_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, u32 memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, u32 bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, u32 imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

static void s_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 query, VkQueryControlFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBeginQuery(commandBuffer, queryPool, query, flags);
}

static void s_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 query) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdEndQuery(commandBuffer, queryPool, query);
}

static void s_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 firstQuery, u32 queryCount) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

static void s_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, u32 query) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
}

static void s_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, u32 firstQuery, u32 queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

static void s_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, u32 offset, u32 size, const void* pValues) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

static void s_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

static void s_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdNextSubpass(commandBuffer, contents);
}

static void s_vkCmdEndRenderPass(VkCommandBuffer commandBuffer) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdEndRenderPass(commandBuffer);
}

static void s_vkCmdExecuteCommands(VkCommandBuffer commandBuffer, u32 commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    fprintf(stderr, "%s: call\n", __func__);
    s_dvkCmdBuf(commandBuffer)->vkCmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
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
