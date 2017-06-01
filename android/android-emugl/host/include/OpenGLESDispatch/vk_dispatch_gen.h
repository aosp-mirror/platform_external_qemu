/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// WARNING: This file is generated. See ../README.md for instructions.

#pragma once

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>

typedef VkFlags VkSwapchainImageUsageFlagsANDROID;

typedef void (*vkVoidFunction_func_t)(void);
typedef void* (*vkAllocationFunction_func_t)(
    void* pUserData, size_t size, size_t alignment,
    VkSystemAllocationScope allocationScope);
typedef void* (*vkReallocationFunction_func_t)(
    void* pUserData, void* pOriginal, size_t size, size_t alignment,
    VkSystemAllocationScope allocationScope);
typedef void (*vkFreeFunction_func_t)(void* pUserData, void* pMemory);
typedef void (*vkInternalAllocationNotification_func_t)(
    void* pUserData, size_t size, VkInternalAllocationType allocationType,
    VkSystemAllocationScope allocationScope);
typedef void (*vkInternalFreeNotification_func_t)(
    void* pUserData, size_t size, VkInternalAllocationType allocationType,
    VkSystemAllocationScope allocationScope);
typedef VkResult (*vkCreateInstance_func_t)(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
typedef void (*vkDestroyInstance_func_t)(
    VkInstance instance, const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkEnumeratePhysicalDevices_func_t)(
    VkInstance instance, uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices);
typedef PFN_vkVoidFunction (*vkGetDeviceProcAddr_func_t)(VkDevice device,
                                                         const char* pName);
typedef PFN_vkVoidFunction (*vkGetInstanceProcAddr_func_t)(VkInstance instance,
                                                           const char* pName);
typedef void (*vkGetPhysicalDeviceProperties_func_t)(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties);
typedef void (*vkGetPhysicalDeviceQueueFamilyProperties_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties);
typedef void (*vkGetPhysicalDeviceMemoryProperties_func_t)(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties);
typedef void (*vkGetPhysicalDeviceFeatures_func_t)(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures);
typedef void (*vkGetPhysicalDeviceFormatProperties_func_t)(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties* pFormatProperties);
typedef VkResult (*vkGetPhysicalDeviceImageFormatProperties_func_t)(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties);
typedef VkResult (*vkCreateDevice_func_t)(
    VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
typedef void (*vkDestroyDevice_func_t)(VkDevice device,
                                       const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkEnumerateInstanceLayerProperties_func_t)(
    uint32_t* pPropertyCount, VkLayerProperties* pProperties);
typedef VkResult (*vkEnumerateInstanceExtensionProperties_func_t)(
    const char* pLayerName, uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties);
typedef VkResult (*vkEnumerateDeviceLayerProperties_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount,
    VkLayerProperties* pProperties);
typedef VkResult (*vkEnumerateDeviceExtensionProperties_func_t)(
    VkPhysicalDevice physicalDevice, const char* pLayerName,
    uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
typedef void (*vkGetDeviceQueue_func_t)(VkDevice device,
                                        uint32_t queueFamilyIndex,
                                        uint32_t queueIndex, VkQueue* pQueue);
typedef VkResult (*vkQueueSubmit_func_t)(VkQueue queue, uint32_t submitCount,
                                         const VkSubmitInfo* pSubmits,
                                         VkFence fence);
typedef VkResult (*vkQueueWaitIdle_func_t)(VkQueue queue);
typedef VkResult (*vkDeviceWaitIdle_func_t)(VkDevice device);
typedef VkResult (*vkAllocateMemory_func_t)(
    VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory);
typedef void (*vkFreeMemory_func_t)(VkDevice device, VkDeviceMemory memory,
                                    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkMapMemory_func_t)(VkDevice device, VkDeviceMemory memory,
                                       VkDeviceSize offset, VkDeviceSize size,
                                       VkMemoryMapFlags flags, void** ppData);
typedef void (*vkUnmapMemory_func_t)(VkDevice device, VkDeviceMemory memory);
typedef VkResult (*vkFlushMappedMemoryRanges_func_t)(
    VkDevice device, uint32_t memoryRangeCount,
    const VkMappedMemoryRange* pMemoryRanges);
typedef VkResult (*vkInvalidateMappedMemoryRanges_func_t)(
    VkDevice device, uint32_t memoryRangeCount,
    const VkMappedMemoryRange* pMemoryRanges);
typedef void (*vkGetDeviceMemoryCommitment_func_t)(
    VkDevice device, VkDeviceMemory memory,
    VkDeviceSize* pCommittedMemoryInBytes);
typedef void (*vkGetBufferMemoryRequirements_func_t)(
    VkDevice device, VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements);
typedef VkResult (*vkBindBufferMemory_func_t)(VkDevice device, VkBuffer buffer,
                                              VkDeviceMemory memory,
                                              VkDeviceSize memoryOffset);
typedef void (*vkGetImageMemoryRequirements_func_t)(
    VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements);
typedef VkResult (*vkBindImageMemory_func_t)(VkDevice device, VkImage image,
                                             VkDeviceMemory memory,
                                             VkDeviceSize memoryOffset);
typedef void (*vkGetImageSparseMemoryRequirements_func_t)(
    VkDevice device, VkImage image, uint32_t* pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements);
typedef void (*vkGetPhysicalDeviceSparseImageFormatProperties_func_t)(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkSampleCountFlagBits samples, VkImageUsageFlags usage,
    VkImageTiling tiling, uint32_t* pPropertyCount,
    VkSparseImageFormatProperties* pProperties);
typedef VkResult (*vkQueueBindSparse_func_t)(VkQueue queue,
                                             uint32_t bindInfoCount,
                                             const VkBindSparseInfo* pBindInfo,
                                             VkFence fence);
typedef VkResult (*vkCreateFence_func_t)(
    VkDevice device, const VkFenceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkFence* pFence);
typedef void (*vkDestroyFence_func_t)(VkDevice device, VkFence fence,
                                      const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkResetFences_func_t)(VkDevice device, uint32_t fenceCount,
                                         const VkFence* pFences);
typedef VkResult (*vkGetFenceStatus_func_t)(VkDevice device, VkFence fence);
typedef VkResult (*vkWaitForFences_func_t)(VkDevice device, uint32_t fenceCount,
                                           const VkFence* pFences,
                                           VkBool32 waitAll, uint64_t timeout);
typedef VkResult (*vkCreateSemaphore_func_t)(
    VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore);
typedef void (*vkDestroySemaphore_func_t)(
    VkDevice device, VkSemaphore semaphore,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateEvent_func_t)(
    VkDevice device, const VkEventCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkEvent* pEvent);
typedef void (*vkDestroyEvent_func_t)(VkDevice device, VkEvent event,
                                      const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkGetEventStatus_func_t)(VkDevice device, VkEvent event);
typedef VkResult (*vkSetEvent_func_t)(VkDevice device, VkEvent event);
typedef VkResult (*vkResetEvent_func_t)(VkDevice device, VkEvent event);
typedef VkResult (*vkCreateQueryPool_func_t)(
    VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);
typedef void (*vkDestroyQueryPool_func_t)(
    VkDevice device, VkQueryPool queryPool,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkGetQueryPoolResults_func_t)(
    VkDevice device, VkQueryPool queryPool, uint32_t firstQuery,
    uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride,
    VkQueryResultFlags flags);
typedef VkResult (*vkCreateBuffer_func_t)(
    VkDevice device, const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);
typedef void (*vkDestroyBuffer_func_t)(VkDevice device, VkBuffer buffer,
                                       const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateBufferView_func_t)(
    VkDevice device, const VkBufferViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkBufferView* pView);
typedef void (*vkDestroyBufferView_func_t)(
    VkDevice device, VkBufferView bufferView,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateImage_func_t)(
    VkDevice device, const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkImage* pImage);
typedef void (*vkDestroyImage_func_t)(VkDevice device, VkImage image,
                                      const VkAllocationCallbacks* pAllocator);
typedef void (*vkGetImageSubresourceLayout_func_t)(
    VkDevice device, VkImage image, const VkImageSubresource* pSubresource,
    VkSubresourceLayout* pLayout);
typedef VkResult (*vkCreateImageView_func_t)(
    VkDevice device, const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkImageView* pView);
typedef void (*vkDestroyImageView_func_t)(
    VkDevice device, VkImageView imageView,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateShaderModule_func_t)(
    VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
typedef void (*vkDestroyShaderModule_func_t)(
    VkDevice device, VkShaderModule shaderModule,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreatePipelineCache_func_t)(
    VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache);
typedef void (*vkDestroyPipelineCache_func_t)(
    VkDevice device, VkPipelineCache pipelineCache,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkGetPipelineCacheData_func_t)(VkDevice device,
                                                  VkPipelineCache pipelineCache,
                                                  size_t* pDataSize,
                                                  void* pData);
typedef VkResult (*vkMergePipelineCaches_func_t)(
    VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount,
    const VkPipelineCache* pSrcCaches);
typedef VkResult (*vkCreateGraphicsPipelines_func_t)(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
typedef VkResult (*vkCreateComputePipelines_func_t)(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkComputePipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
typedef void (*vkDestroyPipeline_func_t)(
    VkDevice device, VkPipeline pipeline,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreatePipelineLayout_func_t)(
    VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout);
typedef void (*vkDestroyPipelineLayout_func_t)(
    VkDevice device, VkPipelineLayout pipelineLayout,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateSampler_func_t)(
    VkDevice device, const VkSamplerCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);
typedef void (*vkDestroySampler_func_t)(
    VkDevice device, VkSampler sampler,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateDescriptorSetLayout_func_t)(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);
typedef void (*vkDestroyDescriptorSetLayout_func_t)(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateDescriptorPool_func_t)(
    VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);
typedef void (*vkDestroyDescriptorPool_func_t)(
    VkDevice device, VkDescriptorPool descriptorPool,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkResetDescriptorPool_func_t)(
    VkDevice device, VkDescriptorPool descriptorPool,
    VkDescriptorPoolResetFlags flags);
typedef VkResult (*vkAllocateDescriptorSets_func_t)(
    VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet* pDescriptorSets);
typedef VkResult (*vkFreeDescriptorSets_func_t)(
    VkDevice device, VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
typedef void (*vkUpdateDescriptorSets_func_t)(
    VkDevice device, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet* pDescriptorCopies);
typedef VkResult (*vkCreateFramebuffer_func_t)(
    VkDevice device, const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer);
typedef void (*vkDestroyFramebuffer_func_t)(
    VkDevice device, VkFramebuffer framebuffer,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateRenderPass_func_t)(
    VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
typedef void (*vkDestroyRenderPass_func_t)(
    VkDevice device, VkRenderPass renderPass,
    const VkAllocationCallbacks* pAllocator);
typedef void (*vkGetRenderAreaGranularity_func_t)(VkDevice device,
                                                  VkRenderPass renderPass,
                                                  VkExtent2D* pGranularity);
typedef VkResult (*vkCreateCommandPool_func_t)(
    VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool);
typedef void (*vkDestroyCommandPool_func_t)(
    VkDevice device, VkCommandPool commandPool,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkResetCommandPool_func_t)(VkDevice device,
                                              VkCommandPool commandPool,
                                              VkCommandPoolResetFlags flags);
typedef VkResult (*vkAllocateCommandBuffers_func_t)(
    VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers);
typedef void (*vkFreeCommandBuffers_func_t)(
    VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers);
typedef VkResult (*vkBeginCommandBuffer_func_t)(
    VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo);
typedef VkResult (*vkEndCommandBuffer_func_t)(VkCommandBuffer commandBuffer);
typedef VkResult (*vkResetCommandBuffer_func_t)(
    VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags);
typedef void (*vkCmdBindPipeline_func_t)(VkCommandBuffer commandBuffer,
                                         VkPipelineBindPoint pipelineBindPoint,
                                         VkPipeline pipeline);
typedef void (*vkCmdSetViewport_func_t)(VkCommandBuffer commandBuffer,
                                        uint32_t firstViewport,
                                        uint32_t viewportCount,
                                        const VkViewport* pViewports);
typedef void (*vkCmdSetScissor_func_t)(VkCommandBuffer commandBuffer,
                                       uint32_t firstScissor,
                                       uint32_t scissorCount,
                                       const VkRect2D* pScissors);
typedef void (*vkCmdSetLineWidth_func_t)(VkCommandBuffer commandBuffer,
                                         float lineWidth);
typedef void (*vkCmdSetDepthBias_func_t)(VkCommandBuffer commandBuffer,
                                         float depthBiasConstantFactor,
                                         float depthBiasClamp,
                                         float depthBiasSlopeFactor);
typedef void (*vkCmdSetBlendConstants_func_t)(VkCommandBuffer commandBuffer,
                                              const float blendConstants[4]);
typedef void (*vkCmdSetDepthBounds_func_t)(VkCommandBuffer commandBuffer,
                                           float minDepthBounds,
                                           float maxDepthBounds);
typedef void (*vkCmdSetStencilCompareMask_func_t)(VkCommandBuffer commandBuffer,
                                                  VkStencilFaceFlags faceMask,
                                                  uint32_t compareMask);
typedef void (*vkCmdSetStencilWriteMask_func_t)(VkCommandBuffer commandBuffer,
                                                VkStencilFaceFlags faceMask,
                                                uint32_t writeMask);
typedef void (*vkCmdSetStencilReference_func_t)(VkCommandBuffer commandBuffer,
                                                VkStencilFaceFlags faceMask,
                                                uint32_t reference);
typedef void (*vkCmdBindDescriptorSets_func_t)(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets);
typedef void (*vkCmdBindIndexBuffer_func_t)(VkCommandBuffer commandBuffer,
                                            VkBuffer buffer,
                                            VkDeviceSize offset,
                                            VkIndexType indexType);
typedef void (*vkCmdBindVertexBuffers_func_t)(VkCommandBuffer commandBuffer,
                                              uint32_t firstBinding,
                                              uint32_t bindingCount,
                                              const VkBuffer* pBuffers,
                                              const VkDeviceSize* pOffsets);
typedef void (*vkCmdDraw_func_t)(VkCommandBuffer commandBuffer,
                                 uint32_t vertexCount, uint32_t instanceCount,
                                 uint32_t firstVertex, uint32_t firstInstance);
typedef void (*vkCmdDrawIndexed_func_t)(
    VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
typedef void (*vkCmdDrawIndirect_func_t)(VkCommandBuffer commandBuffer,
                                         VkBuffer buffer, VkDeviceSize offset,
                                         uint32_t drawCount, uint32_t stride);
typedef void (*vkCmdDrawIndexedIndirect_func_t)(VkCommandBuffer commandBuffer,
                                                VkBuffer buffer,
                                                VkDeviceSize offset,
                                                uint32_t drawCount,
                                                uint32_t stride);
typedef void (*vkCmdDispatch_func_t)(VkCommandBuffer commandBuffer,
                                     uint32_t groupCountX, uint32_t groupCountY,
                                     uint32_t groupCountZ);
typedef void (*vkCmdDispatchIndirect_func_t)(VkCommandBuffer commandBuffer,
                                             VkBuffer buffer,
                                             VkDeviceSize offset);
typedef void (*vkCmdCopyBuffer_func_t)(VkCommandBuffer commandBuffer,
                                       VkBuffer srcBuffer, VkBuffer dstBuffer,
                                       uint32_t regionCount,
                                       const VkBufferCopy* pRegions);
typedef void (*vkCmdCopyImage_func_t)(VkCommandBuffer commandBuffer,
                                      VkImage srcImage,
                                      VkImageLayout srcImageLayout,
                                      VkImage dstImage,
                                      VkImageLayout dstImageLayout,
                                      uint32_t regionCount,
                                      const VkImageCopy* pRegions);
typedef void (*vkCmdBlitImage_func_t)(
    VkCommandBuffer commandBuffer, VkImage srcImage,
    VkImageLayout srcImageLayout, VkImage dstImage,
    VkImageLayout dstImageLayout, uint32_t regionCount,
    const VkImageBlit* pRegions, VkFilter filter);
typedef void (*vkCmdCopyBufferToImage_func_t)(
    VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
    VkImageLayout dstImageLayout, uint32_t regionCount,
    const VkBufferImageCopy* pRegions);
typedef void (*vkCmdCopyImageToBuffer_func_t)(
    VkCommandBuffer commandBuffer, VkImage srcImage,
    VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount,
    const VkBufferImageCopy* pRegions);
typedef void (*vkCmdUpdateBuffer_func_t)(VkCommandBuffer commandBuffer,
                                         VkBuffer dstBuffer,
                                         VkDeviceSize dstOffset,
                                         VkDeviceSize dataSize,
                                         const void* pData);
typedef void (*vkCmdFillBuffer_func_t)(VkCommandBuffer commandBuffer,
                                       VkBuffer dstBuffer,
                                       VkDeviceSize dstOffset,
                                       VkDeviceSize size, uint32_t data);
typedef void (*vkCmdClearColorImage_func_t)(
    VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
    const VkClearColorValue* pColor, uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges);
typedef void (*vkCmdClearDepthStencilImage_func_t)(
    VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges);
typedef void (*vkCmdClearAttachments_func_t)(
    VkCommandBuffer commandBuffer, uint32_t attachmentCount,
    const VkClearAttachment* pAttachments, uint32_t rectCount,
    const VkClearRect* pRects);
typedef void (*vkCmdResolveImage_func_t)(VkCommandBuffer commandBuffer,
                                         VkImage srcImage,
                                         VkImageLayout srcImageLayout,
                                         VkImage dstImage,
                                         VkImageLayout dstImageLayout,
                                         uint32_t regionCount,
                                         const VkImageResolve* pRegions);
typedef void (*vkCmdSetEvent_func_t)(VkCommandBuffer commandBuffer,
                                     VkEvent event,
                                     VkPipelineStageFlags stageMask);
typedef void (*vkCmdResetEvent_func_t)(VkCommandBuffer commandBuffer,
                                       VkEvent event,
                                       VkPipelineStageFlags stageMask);
typedef void (*vkCmdWaitEvents_func_t)(
    VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers);
typedef void (*vkCmdPipelineBarrier_func_t)(
    VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers);
typedef void (*vkCmdBeginQuery_func_t)(VkCommandBuffer commandBuffer,
                                       VkQueryPool queryPool, uint32_t query,
                                       VkQueryControlFlags flags);
typedef void (*vkCmdEndQuery_func_t)(VkCommandBuffer commandBuffer,
                                     VkQueryPool queryPool, uint32_t query);
typedef void (*vkCmdResetQueryPool_func_t)(VkCommandBuffer commandBuffer,
                                           VkQueryPool queryPool,
                                           uint32_t firstQuery,
                                           uint32_t queryCount);
typedef void (*vkCmdWriteTimestamp_func_t)(
    VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
    VkQueryPool queryPool, uint32_t query);
typedef void (*vkCmdCopyQueryPoolResults_func_t)(
    VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
    uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
    VkDeviceSize stride, VkQueryResultFlags flags);
typedef void (*vkCmdPushConstants_func_t)(VkCommandBuffer commandBuffer,
                                          VkPipelineLayout layout,
                                          VkShaderStageFlags stageFlags,
                                          uint32_t offset, uint32_t size,
                                          const void* pValues);
typedef void (*vkCmdBeginRenderPass_func_t)(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
typedef void (*vkCmdNextSubpass_func_t)(VkCommandBuffer commandBuffer,
                                        VkSubpassContents contents);
typedef void (*vkCmdEndRenderPass_func_t)(VkCommandBuffer commandBuffer);
typedef void (*vkCmdExecuteCommands_func_t)(
    VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers);
typedef void (*vkDestroySurfaceKHR_func_t)(
    VkInstance instance, VkSurfaceKHR surface,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkGetPhysicalDeviceSurfaceSupportKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
    VkSurfaceKHR surface, VkBool32* pSupported);
typedef VkResult (*vkGetPhysicalDeviceSurfaceCapabilitiesKHR_func_t)(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities);
typedef VkResult (*vkGetPhysicalDeviceSurfaceFormatsKHR_func_t)(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats);
typedef VkResult (*vkGetPhysicalDeviceSurfacePresentModesKHR_func_t)(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes);
typedef VkResult (*vkCreateSwapchainKHR_func_t)(
    VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
typedef void (*vkDestroySwapchainKHR_func_t)(
    VkDevice device, VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkGetSwapchainImagesKHR_func_t)(
    VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages);
typedef VkResult (*vkAcquireNextImageKHR_func_t)(
    VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
    VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
typedef VkResult (*vkQueuePresentKHR_func_t)(
    VkQueue queue, const VkPresentInfoKHR* pPresentInfo);
typedef VkResult (*vkGetPhysicalDeviceDisplayPropertiesKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount,
    VkDisplayPropertiesKHR* pProperties);
typedef VkResult (*vkGetPhysicalDeviceDisplayPlanePropertiesKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount,
    VkDisplayPlanePropertiesKHR* pProperties);
typedef VkResult (*vkGetDisplayPlaneSupportedDisplaysKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t planeIndex,
    uint32_t* pDisplayCount, VkDisplayKHR* pDisplays);
typedef VkResult (*vkGetDisplayModePropertiesKHR_func_t)(
    VkPhysicalDevice physicalDevice, VkDisplayKHR display,
    uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties);
typedef VkResult (*vkCreateDisplayModeKHR_func_t)(
    VkPhysicalDevice physicalDevice, VkDisplayKHR display,
    const VkDisplayModeCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode);
typedef VkResult (*vkGetDisplayPlaneCapabilitiesKHR_func_t)(
    VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, uint32_t planeIndex,
    VkDisplayPlaneCapabilitiesKHR* pCapabilities);
typedef VkResult (*vkCreateDisplayPlaneSurfaceKHR_func_t)(
    VkInstance instance, const VkDisplaySurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkResult (*vkCreateSharedSwapchainsKHR_func_t)(
    VkDevice device, uint32_t swapchainCount,
    const VkSwapchainCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains);
#ifdef VK_USE_PLATFORM_XLIB_KHR
typedef VkResult (*vkCreateXlibSurfaceKHR_func_t)(
    VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
typedef VkBool32 (*vkGetPhysicalDeviceXlibPresentationSupportKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
    struct Display* dpy, VisualID visualID);
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
typedef VkResult (*vkCreateXcbSurfaceKHR_func_t)(
    VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
typedef VkBool32 (*vkGetPhysicalDeviceXcbPresentationSupportKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
    struct xcb_connection_t* connection, xcb_visualid_t visual_id);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
typedef VkResult (*vkCreateWaylandSurfaceKHR_func_t)(
    VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
typedef VkBool32 (*vkGetPhysicalDeviceWaylandPresentationSupportKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
    struct wl_display* display);
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
typedef VkResult (*vkCreateMirSurfaceKHR_func_t)(
    VkInstance instance, const VkMirSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
typedef VkBool32 (*vkGetPhysicalDeviceMirPresentationSupportKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
    struct MirConnection* connection);
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
typedef VkResult (*vkCreateAndroidSurfaceKHR_func_t)(
    VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
typedef VkResult (*vkCreateWin32SurfaceKHR_func_t)(
    VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
typedef VkResult (*vkGetPhysicalDeviceWin32PresentationSupportKHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);
#endif
typedef VkResult (*vkGetSwapchainGrallocUsageANDROID_func_t)(
    VkDevice device, VkFormat format, VkImageUsageFlags imageUsage,
    int* grallocUsage);
typedef VkResult (*vkAcquireImageANDROID_func_t)(VkDevice device, VkImage image,
                                                 int nativeFenceFd,
                                                 VkSemaphore semaphore,
                                                 VkFence fence);
typedef VkResult (*vkQueueSignalReleaseImageANDROID_func_t)(
    VkQueue queue, uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores, VkImage image, int* pNativeFenceFd);
typedef VkBool32 (*vkDebugReportCallbackEXT_func_t)(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
    uint64_t object, size_t location, int32_t messageCode,
    const char* pLayerPrefix, const char* pMessage, void* pUserData);
typedef VkResult (*vkCreateDebugReportCallbackEXT_func_t)(
    VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugReportCallbackEXT* pCallback);
typedef void (*vkDestroyDebugReportCallbackEXT_func_t)(
    VkInstance instance, VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks* pAllocator);
typedef void (*vkDebugReportMessageEXT_func_t)(
    VkInstance instance, VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location,
    int32_t messageCode, const char* pLayerPrefix, const char* pMessage);
typedef VkResult (*vkDebugMarkerSetObjectTagEXT_func_t)(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT* pTagInfo);
typedef VkResult (*vkDebugMarkerSetObjectNameEXT_func_t)(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT* pNameInfo);
typedef void (*vkCmdDebugMarkerBeginEXT_func_t)(
    VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
typedef void (*vkCmdDebugMarkerEndEXT_func_t)(VkCommandBuffer commandBuffer);
typedef void (*vkCmdDebugMarkerInsertEXT_func_t)(
    VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
typedef void (*vkCmdDrawIndirectCountAMD_func_t)(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride);
typedef void (*vkCmdDrawIndexedIndirectCountAMD_func_t)(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride);
typedef VkResult (*vkGetPhysicalDeviceExternalImageFormatPropertiesNV_func_t)(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties);
#ifdef VK_USE_PLATFORM_WIN32_KHR
typedef VkResult (*vkGetMemoryWin32HandleNV_func_t)(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle);
#endif
typedef void (*vkGetPhysicalDeviceFeatures2KHR_func_t)(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR* pFeatures);
typedef void (*vkGetPhysicalDeviceProperties2KHR_func_t)(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2KHR* pProperties);
typedef void (*vkGetPhysicalDeviceFormatProperties2KHR_func_t)(
    VkPhysicalDevice physicalDevice, VkFormat format,
    VkFormatProperties2KHR* pFormatProperties);
typedef VkResult (*vkGetPhysicalDeviceImageFormatProperties2KHR_func_t)(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2KHR* pImageFormatInfo,
    VkImageFormatProperties2KHR* pImageFormatProperties);
typedef void (*vkGetPhysicalDeviceQueueFamilyProperties2KHR_func_t)(
    VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties2KHR* pQueueFamilyProperties);
typedef void (*vkGetPhysicalDeviceMemoryProperties2KHR_func_t)(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2KHR* pMemoryProperties);
typedef void (*vkGetPhysicalDeviceSparseImageFormatProperties2KHR_func_t)(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSparseImageFormatInfo2KHR* pFormatInfo,
    uint32_t* pPropertyCount, VkSparseImageFormatProperties2KHR* pProperties);
typedef void (*vkGetDeviceGroupPeerMemoryFeaturesKHX_func_t)(
    VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex,
    uint32_t remoteDeviceIndex,
    VkPeerMemoryFeatureFlagsKHX* pPeerMemoryFeatures);
typedef VkResult (*vkBindBufferMemory2KHX_func_t)(
    VkDevice device, uint32_t bindInfoCount,
    const VkBindBufferMemoryInfoKHX* pBindInfos);
typedef VkResult (*vkBindImageMemory2KHX_func_t)(
    VkDevice device, uint32_t bindInfoCount,
    const VkBindImageMemoryInfoKHX* pBindInfos);
typedef void (*vkCmdSetDeviceMaskKHX_func_t)(VkCommandBuffer commandBuffer,
                                             uint32_t deviceMask);
typedef VkResult (*vkGetDeviceGroupPresentCapabilitiesKHX_func_t)(
    VkDevice device,
    VkDeviceGroupPresentCapabilitiesKHX* pDeviceGroupPresentCapabilities);
typedef VkResult (*vkGetDeviceGroupSurfacePresentModesKHX_func_t)(
    VkDevice device, VkSurfaceKHR surface,
    VkDeviceGroupPresentModeFlagsKHX* pModes);
typedef VkResult (*vkAcquireNextImage2KHX_func_t)(
    VkDevice device, const VkAcquireNextImageInfoKHX* pAcquireInfo,
    uint32_t* pImageIndex);
typedef void (*vkCmdDispatchBaseKHX_func_t)(
    VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY,
    uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY,
    uint32_t groupCountZ);
typedef VkResult (*vkGetPhysicalDevicePresentRectanglesKHX_func_t)(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pRectCount,
    VkRect2D* pRects);
#ifdef VK_USE_PLATFORM_VI_NN
typedef VkResult (*vkCreateViSurfaceNN_func_t)(
    VkInstance instance, const VkViSurfaceCreateInfoNN* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
typedef void (*vkTrimCommandPoolKHR_func_t)(VkDevice device,
                                            VkCommandPool commandPool,
                                            VkCommandPoolTrimFlagsKHR flags);
typedef VkResult (*vkEnumeratePhysicalDeviceGroupsKHX_func_t)(
    VkInstance instance, uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupPropertiesKHX* pPhysicalDeviceGroupProperties);
typedef void (*vkGetPhysicalDeviceExternalBufferPropertiesKHX_func_t)(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalBufferInfoKHX* pExternalBufferInfo,
    VkExternalBufferPropertiesKHX* pExternalBufferProperties);
#ifdef VK_USE_PLATFORM_WIN32_KHX
typedef VkResult (*vkGetMemoryWin32HandleKHX_func_t)(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagBitsKHX handleType, HANDLE* pHandle);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHX
typedef VkResult (*vkGetMemoryWin32HandlePropertiesKHX_func_t)(
    VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHX handleType,
    HANDLE handle,
    VkMemoryWin32HandlePropertiesKHX* pMemoryWin32HandleProperties);
#endif
typedef VkResult (*vkGetMemoryFdKHX_func_t)(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagBitsKHX handleType, int32_t* pFd);
typedef VkResult (*vkGetMemoryFdPropertiesKHX_func_t)(
    VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHX handleType,
    int32_t fd, VkMemoryFdPropertiesKHX* pMemoryFdProperties);
typedef void (*vkGetPhysicalDeviceExternalSemaphorePropertiesKHX_func_t)(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfoKHX* pExternalSemaphoreInfo,
    VkExternalSemaphorePropertiesKHX* pExternalSemaphoreProperties);
#ifdef VK_USE_PLATFORM_WIN32_KHX
typedef VkResult (*vkImportSemaphoreWin32HandleKHX_func_t)(
    VkDevice device,
    const VkImportSemaphoreWin32HandleInfoKHX* pImportSemaphoreWin32HandleInfo);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHX
typedef VkResult (*vkGetSemaphoreWin32HandleKHX_func_t)(
    VkDevice device, VkSemaphore semaphore,
    VkExternalSemaphoreHandleTypeFlagBitsKHX handleType, HANDLE* pHandle);
#endif
typedef VkResult (*vkImportSemaphoreFdKHX_func_t)(
    VkDevice device, const VkImportSemaphoreFdInfoKHX* pImportSemaphoreFdInfo);
typedef VkResult (*vkGetSemaphoreFdKHX_func_t)(
    VkDevice device, VkSemaphore semaphore,
    VkExternalSemaphoreHandleTypeFlagBitsKHX handleType, int32_t* pFd);
typedef void (*vkCmdPushDescriptorSetKHR_func_t)(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites);
typedef VkResult (*vkCreateDescriptorUpdateTemplateKHR_func_t)(
    VkDevice device, const VkDescriptorUpdateTemplateCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplateKHR* pDescriptorUpdateTemplate);
typedef void (*vkDestroyDescriptorUpdateTemplateKHR_func_t)(
    VkDevice device, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator);
typedef void (*vkUpdateDescriptorSetWithTemplateKHR_func_t)(
    VkDevice device, VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, const void* pData);
typedef void (*vkCmdPushDescriptorSetWithTemplateKHR_func_t)(
    VkCommandBuffer commandBuffer,
    VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate,
    VkPipelineLayout layout, uint32_t set, const void* pData);
typedef void (*vkCmdProcessCommandsNVX_func_t)(
    VkCommandBuffer commandBuffer,
    const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo);
typedef void (*vkCmdReserveSpaceForCommandsNVX_func_t)(
    VkCommandBuffer commandBuffer,
    const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo);
typedef VkResult (*vkCreateIndirectCommandsLayoutNVX_func_t)(
    VkDevice device, const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout);
typedef void (*vkDestroyIndirectCommandsLayoutNVX_func_t)(
    VkDevice device, VkIndirectCommandsLayoutNVX indirectCommandsLayout,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkCreateObjectTableNVX_func_t)(
    VkDevice device, const VkObjectTableCreateInfoNVX* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable);
typedef void (*vkDestroyObjectTableNVX_func_t)(
    VkDevice device, VkObjectTableNVX objectTable,
    const VkAllocationCallbacks* pAllocator);
typedef VkResult (*vkRegisterObjectsNVX_func_t)(
    VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount,
    const VkObjectTableEntryNVX* const* ppObjectTableEntries,
    const uint32_t* pObjectIndices);
typedef VkResult (*vkUnregisterObjectsNVX_func_t)(
    VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount,
    const VkObjectEntryTypeNVX* pObjectEntryTypes,
    const uint32_t* pObjectIndices);
typedef void (*vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX_func_t)(
    VkPhysicalDevice physicalDevice,
    VkDeviceGeneratedCommandsFeaturesNVX* pFeatures,
    VkDeviceGeneratedCommandsLimitsNVX* pLimits);
typedef void (*vkCmdSetViewportWScalingNV_func_t)(
    VkCommandBuffer commandBuffer, uint32_t firstViewport,
    uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings);
typedef VkResult (*vkReleaseDisplayEXT_func_t)(VkPhysicalDevice physicalDevice,
                                               VkDisplayKHR display);
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
typedef VkResult (*vkAcquireXlibDisplayEXT_func_t)(
    VkPhysicalDevice physicalDevice, struct Display* dpy, VkDisplayKHR display);
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
typedef VkResult (*vkGetRandROutputDisplayEXT_func_t)(
    VkPhysicalDevice physicalDevice, struct Display* dpy, RROutput rrOutput,
    VkDisplayKHR* pDisplay);
#endif
typedef VkResult (*vkGetPhysicalDeviceSurfaceCapabilities2EXT_func_t)(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VkSurfaceCapabilities2EXT* pSurfaceCapabilities);
typedef VkResult (*vkDisplayPowerControlEXT_func_t)(
    VkDevice device, VkDisplayKHR display,
    const VkDisplayPowerInfoEXT* pDisplayPowerInfo);
typedef VkResult (*vkRegisterDeviceEventEXT_func_t)(
    VkDevice device, const VkDeviceEventInfoEXT* pDeviceEventInfo,
    const VkAllocationCallbacks* pAllocator, VkFence* pFence);
typedef VkResult (*vkRegisterDisplayEventEXT_func_t)(
    VkDevice device, VkDisplayKHR display,
    const VkDisplayEventInfoEXT* pDisplayEventInfo,
    const VkAllocationCallbacks* pAllocator, VkFence* pFence);
typedef VkResult (*vkGetSwapchainCounterEXT_func_t)(
    VkDevice device, VkSwapchainKHR swapchain,
    VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue);
typedef VkResult (*vkGetRefreshCycleDurationGOOGLE_func_t)(
    VkDevice device, VkSwapchainKHR swapchain,
    VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties);
typedef VkResult (*vkGetPastPresentationTimingGOOGLE_func_t)(
    VkDevice device, VkSwapchainKHR swapchain,
    uint32_t* pPresentationTimingCount,
    VkPastPresentationTimingGOOGLE* pPresentationTimings);
typedef void (*vkCmdSetDiscardRectangleEXT_func_t)(
    VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle,
    uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles);
typedef void (*vkSetHdrMetadataEXT_func_t)(VkDevice device,
                                           uint32_t swapchainCount,
                                           const VkSwapchainKHR* pSwapchains,
                                           const VkHdrMetadataEXT* pMetadata);
#ifdef VK_USE_PLATFORM_IOS_MVK
typedef VkResult (*vkCreateIOSSurfaceMVK_func_t)(
    VkInstance instance, const VkIOSSurfaceCreateInfoMVK* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
typedef VkResult (*vkCreateMacOSSurfaceMVK_func_t)(
    VkInstance instance, const VkMacOSSurfaceCreateInfoMVK* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
#endif

#define LIST_VK_DLL_FUNCS(f) f(vkGetInstanceProcAddr)

#define LIST_VK_GLOBAL_FUNCS(f)                             \
  f(vkCreateInstance) f(vkEnumerateInstanceLayerProperties) \
      f(vkEnumerateInstanceExtensionProperties)

#define LIST_VK_INSTANCE_FUNCS(f)                                                                                   \
  f(vkDestroyInstance) f(vkEnumeratePhysicalDevices) f(vkGetDeviceProcAddr) f(                                      \
      vkGetPhysicalDeviceProperties) f(vkGetPhysicalDeviceQueueFamilyProperties)                                    \
      f(vkGetPhysicalDeviceMemoryProperties) f(vkGetPhysicalDeviceFeatures) f(                                      \
          vkGetPhysicalDeviceFormatProperties) f(vkGetPhysicalDeviceImageFormatProperties)                          \
          f(vkCreateDevice) f(vkEnumerateDeviceLayerProperties) f(                                                  \
              vkEnumerateDeviceExtensionProperties) f(vkGetPhysicalDeviceSparseImageFormatProperties)               \
              f(vkDestroySurfaceKHR) f(vkGetPhysicalDeviceSurfaceSupportKHR) f(                                     \
                  vkGetPhysicalDeviceSurfaceCapabilitiesKHR) f(vkGetPhysicalDeviceSurfaceFormatsKHR)                \
                  f(vkGetPhysicalDeviceSurfacePresentModesKHR) f(                                                   \
                      vkGetPhysicalDeviceDisplayPropertiesKHR) f(vkGetPhysicalDeviceDisplayPlanePropertiesKHR)      \
                      f(vkGetDisplayPlaneSupportedDisplaysKHR) f(                                                   \
                          vkGetDisplayModePropertiesKHR) f(vkCreateDisplayModeKHR)                                  \
                          f(vkGetDisplayPlaneCapabilitiesKHR) f(                                                    \
                              vkCreateDisplayPlaneSurfaceKHR) f(vkCreateDebugReportCallbackEXT)                     \
                              f(vkDestroyDebugReportCallbackEXT) f(                                                 \
                                  vkDebugReportMessageEXT) f(vkGetPhysicalDeviceExternalImageFormatPropertiesNV)    \
                                  f(vkGetPhysicalDeviceFeatures2KHR) f(                                             \
                                      vkGetPhysicalDeviceProperties2KHR) f(vkGetPhysicalDeviceFormatProperties2KHR) \
                                      f(vkGetPhysicalDeviceImageFormatProperties2KHR) f(                            \
                                          vkGetPhysicalDeviceQueueFamilyProperties2KHR)                             \
                                          f(vkGetPhysicalDeviceMemoryProperties2KHR) f(                             \
                                              vkGetPhysicalDeviceSparseImageFormatProperties2KHR)                   \
                                              f(vkGetPhysicalDevicePresentRectanglesKHX) f(                         \
                                                  vkEnumeratePhysicalDeviceGroupsKHX)                               \
                                                  f(vkGetPhysicalDeviceExternalBufferPropertiesKHX)                 \
                                                      f(vkGetPhysicalDeviceExternalSemaphorePropertiesKHX)          \
                                                          f(vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX)      \
                                                              f(vkReleaseDisplayEXT)                                \
                                                                  f(vkGetPhysicalDeviceSurfaceCapabilities2EXT)

#define LIST_VK_DEVICE_FUNCS(f)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
  f(vkDestroyDevice) f(vkGetDeviceQueue) f(vkQueueSubmit) f(vkQueueWaitIdle) f(vkDeviceWaitIdle) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
      vkAllocateMemory) f(vkFreeMemory) f(vkMapMemory) f(vkUnmapMemory) f(vkFlushMappedMemoryRanges)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
      f(vkInvalidateMappedMemoryRanges) f(vkGetDeviceMemoryCommitment) f(vkGetBufferMemoryRequirements) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
          vkBindBufferMemory) f(vkGetImageMemoryRequirements) f(vkBindImageMemory) f(vkGetImageSparseMemoryRequirements)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
          f(vkQueueBindSparse) f(vkCreateFence) f(vkDestroyFence) f(vkResetFences) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
              vkGetFenceStatus) f(vkWaitForFences) f(vkCreateSemaphore) f(vkDestroySemaphore)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
              f(vkCreateEvent) f(vkDestroyEvent) f(vkGetEventStatus) f(vkSetEvent) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
                  vkResetEvent) f(vkCreateQueryPool) f(vkDestroyQueryPool) f(vkGetQueryPoolResults)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                  f(vkCreateBuffer) f(vkDestroyBuffer) f(vkCreateBufferView) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                      vkDestroyBufferView) f(vkCreateImage) f(vkDestroyImage) f(vkGetImageSubresourceLayout)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
                      f(vkCreateImageView) f(vkDestroyImageView) f(vkCreateShaderModule) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                          vkDestroyShaderModule) f(vkCreatePipelineCache) f(vkDestroyPipelineCache)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                          f(vkGetPipelineCacheData) f(vkMergePipelineCaches) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                              vkCreateGraphicsPipelines) f(vkCreateComputePipelines) f(vkDestroyPipeline) f(vkCreatePipelineLayout) f(vkDestroyPipelineLayout) f(vkCreateSampler) f(vkDestroySampler) f(vkCreateDescriptorSetLayout) f(vkDestroyDescriptorSetLayout) f(vkCreateDescriptorPool) f(vkDestroyDescriptorPool) f(vkResetDescriptorPool) f(vkAllocateDescriptorSets) f(vkFreeDescriptorSets) f(vkUpdateDescriptorSets) f(vkCreateFramebuffer) f(vkDestroyFramebuffer) f(vkCreateRenderPass) f(vkDestroyRenderPass) f(vkGetRenderAreaGranularity) f(vkCreateCommandPool) f(vkDestroyCommandPool) f(vkResetCommandPool) f(vkAllocateCommandBuffers) f(vkFreeCommandBuffers) f(vkBeginCommandBuffer)                                                                                                \
                              f(vkEndCommandBuffer) f(vkResetCommandBuffer) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                  vkCmdBindPipeline) f(vkCmdSetViewport) f(vkCmdSetScissor)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                                  f(vkCmdSetLineWidth) f(vkCmdSetDepthBias) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                      vkCmdSetBlendConstants) f(vkCmdSetDepthBounds)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
                                      f(vkCmdSetStencilCompareMask) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                          vkCmdSetStencilWriteMask) f(vkCmdSetStencilReference)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                                          f(vkCmdBindDescriptorSets) f(vkCmdBindIndexBuffer) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                                              vkCmdBindVertexBuffers) f(vkCmdDraw) f(vkCmdDrawIndexed)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                              f(vkCmdDrawIndirect) f(vkCmdDrawIndexedIndirect) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
                                                  vkCmdDispatch) f(vkCmdDispatchIndirect)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                                  f(vkCmdCopyBuffer) f(vkCmdCopyImage) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                                      vkCmdBlitImage) f(vkCmdCopyBufferToImage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                                                      f(vkCmdCopyImageToBuffer) f(vkCmdUpdateBuffer) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
                                                          vkCmdFillBuffer) f(vkCmdClearColorImage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
                                                          f(vkCmdClearDepthStencilImage) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                                                              vkCmdClearAttachments)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
                                                              f(vkCmdResolveImage) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
                                                                  vkCmdSetEvent) f(vkCmdResetEvent)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                                                                  f(vkCmdWaitEvents) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
                                                                      vkCmdPipelineBarrier)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                                                                      f(vkCmdBeginQuery) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                                                                          vkCmdEndQuery)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
                                                                          f(vkCmdResetQueryPool)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
                                                                              f(vkCmdWriteTimestamp)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
                                                                                  f(vkCmdCopyQueryPoolResults)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                                                                      f(vkCmdPushConstants) f(vkCmdBeginRenderPass) f(vkCmdNextSubpass) f(vkCmdEndRenderPass) f(vkCmdExecuteCommands) f(vkCreateSwapchainKHR) f(vkDestroySwapchainKHR) f(vkGetSwapchainImagesKHR) f(vkAcquireNextImageKHR) f(vkQueuePresentKHR) f(vkCreateSharedSwapchainsKHR) f(vkGetSwapchainGrallocUsageANDROID) f(vkAcquireImageANDROID) f(vkQueueSignalReleaseImageANDROID) f(vkDebugMarkerSetObjectTagEXT) f(vkDebugMarkerSetObjectNameEXT) f(vkCmdDebugMarkerBeginEXT) f(vkCmdDebugMarkerEndEXT) f(vkCmdDebugMarkerInsertEXT) f(vkCmdDrawIndirectCountAMD) f(vkCmdDrawIndexedIndirectCountAMD) f(vkGetDeviceGroupPeerMemoryFeaturesKHX) f(vkBindBufferMemory2KHX) f(vkBindImageMemory2KHX) f(vkCmdSetDeviceMaskKHX) \
                                                                                          f(vkGetDeviceGroupPresentCapabilitiesKHX) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                                                                              vkGetDeviceGroupSurfacePresentModesKHX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
                                                                                              f(vkAcquireNextImage2KHX) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
                                                                                                  vkCmdDispatchBaseKHX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
                                                                                                  f(vkTrimCommandPoolKHR) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
                                                                                                      vkGetMemoryFdKHX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
                                                                                                      f(vkGetMemoryFdPropertiesKHX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                                                                                                          f(vkImportSemaphoreFdKHX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                                                                                                              f(vkGetSemaphoreFdKHX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
                                                                                                                  f(vkCmdPushDescriptorSetKHR)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                                                                                                      f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
                                                                                                                          vkCreateDescriptorUpdateTemplateKHR) f(vkDestroyDescriptorUpdateTemplateKHR) f(vkUpdateDescriptorSetWithTemplateKHR) f(vkCmdPushDescriptorSetWithTemplateKHR) f(vkCmdProcessCommandsNVX) f(vkCmdReserveSpaceForCommandsNVX) f(vkCreateIndirectCommandsLayoutNVX) f(vkDestroyIndirectCommandsLayoutNVX) f(vkCreateObjectTableNVX) f(vkDestroyObjectTableNVX) f(vkRegisterObjectsNVX) f(vkUnregisterObjectsNVX) f(vkCmdSetViewportWScalingNV) f(vkDisplayPowerControlEXT) f(vkRegisterDeviceEventEXT) f(vkRegisterDisplayEventEXT)                                                                                                                                                 \
                                                                                                                          f(vkGetSwapchainCounterEXT) f(vkGetRefreshCycleDurationGOOGLE) f(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                                                                                                                              vkGetPastPresentationTimingGOOGLE)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
                                                                                                                              f(vkCmdSetDiscardRectangleEXT)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
                                                                                                                                  f(vkSetHdrMetadataEXT)

struct VkDllDispatch {
  vkGetInstanceProcAddr_func_t vkGetInstanceProcAddr = nullptr;

};  // struct VkDllDispatch

struct VkGlobalDispatch {
  vkCreateInstance_func_t vkCreateInstance = nullptr;
  vkEnumerateInstanceLayerProperties_func_t vkEnumerateInstanceLayerProperties =
      nullptr;
  vkEnumerateInstanceExtensionProperties_func_t
      vkEnumerateInstanceExtensionProperties = nullptr;

};  // struct VkGlobalDispatch

struct VkInstanceDispatch {
  VkInstance instance;

  vkDestroyInstance_func_t vkDestroyInstance = nullptr;
  vkEnumeratePhysicalDevices_func_t vkEnumeratePhysicalDevices = nullptr;
  vkGetDeviceProcAddr_func_t vkGetDeviceProcAddr = nullptr;
  vkGetPhysicalDeviceProperties_func_t vkGetPhysicalDeviceProperties = nullptr;
  vkGetPhysicalDeviceQueueFamilyProperties_func_t
      vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
  vkGetPhysicalDeviceMemoryProperties_func_t
      vkGetPhysicalDeviceMemoryProperties = nullptr;
  vkGetPhysicalDeviceFeatures_func_t vkGetPhysicalDeviceFeatures = nullptr;
  vkGetPhysicalDeviceFormatProperties_func_t
      vkGetPhysicalDeviceFormatProperties = nullptr;
  vkGetPhysicalDeviceImageFormatProperties_func_t
      vkGetPhysicalDeviceImageFormatProperties = nullptr;
  vkCreateDevice_func_t vkCreateDevice = nullptr;
  vkEnumerateDeviceLayerProperties_func_t vkEnumerateDeviceLayerProperties =
      nullptr;
  vkEnumerateDeviceExtensionProperties_func_t
      vkEnumerateDeviceExtensionProperties = nullptr;
  vkGetPhysicalDeviceSparseImageFormatProperties_func_t
      vkGetPhysicalDeviceSparseImageFormatProperties = nullptr;
  vkDestroySurfaceKHR_func_t vkDestroySurfaceKHR = nullptr;
  vkGetPhysicalDeviceSurfaceSupportKHR_func_t
      vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR_func_t
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
  vkGetPhysicalDeviceSurfaceFormatsKHR_func_t
      vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
  vkGetPhysicalDeviceSurfacePresentModesKHR_func_t
      vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
  vkGetPhysicalDeviceDisplayPropertiesKHR_func_t
      vkGetPhysicalDeviceDisplayPropertiesKHR = nullptr;
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR_func_t
      vkGetPhysicalDeviceDisplayPlanePropertiesKHR = nullptr;
  vkGetDisplayPlaneSupportedDisplaysKHR_func_t
      vkGetDisplayPlaneSupportedDisplaysKHR = nullptr;
  vkGetDisplayModePropertiesKHR_func_t vkGetDisplayModePropertiesKHR = nullptr;
  vkCreateDisplayModeKHR_func_t vkCreateDisplayModeKHR = nullptr;
  vkGetDisplayPlaneCapabilitiesKHR_func_t vkGetDisplayPlaneCapabilitiesKHR =
      nullptr;
  vkCreateDisplayPlaneSurfaceKHR_func_t vkCreateDisplayPlaneSurfaceKHR =
      nullptr;
#ifdef VK_USE_PLATFORM_XLIB_KHR
  vkCreateXlibSurfaceKHR_func_t vkCreateXlibSurfaceKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  vkGetPhysicalDeviceXlibPresentationSupportKHR_func_t
      vkGetPhysicalDeviceXlibPresentationSupportKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  vkCreateXcbSurfaceKHR_func_t vkCreateXcbSurfaceKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  vkGetPhysicalDeviceXcbPresentationSupportKHR_func_t
      vkGetPhysicalDeviceXcbPresentationSupportKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  vkCreateWaylandSurfaceKHR_func_t vkCreateWaylandSurfaceKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  vkGetPhysicalDeviceWaylandPresentationSupportKHR_func_t
      vkGetPhysicalDeviceWaylandPresentationSupportKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
  vkCreateMirSurfaceKHR_func_t vkCreateMirSurfaceKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
  vkGetPhysicalDeviceMirPresentationSupportKHR_func_t
      vkGetPhysicalDeviceMirPresentationSupportKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
  vkCreateAndroidSurfaceKHR_func_t vkCreateAndroidSurfaceKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  vkCreateWin32SurfaceKHR_func_t vkCreateWin32SurfaceKHR = nullptr;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  vkGetPhysicalDeviceWin32PresentationSupportKHR_func_t
      vkGetPhysicalDeviceWin32PresentationSupportKHR = nullptr;
#endif
  vkCreateDebugReportCallbackEXT_func_t vkCreateDebugReportCallbackEXT =
      nullptr;
  vkDestroyDebugReportCallbackEXT_func_t vkDestroyDebugReportCallbackEXT =
      nullptr;
  vkDebugReportMessageEXT_func_t vkDebugReportMessageEXT = nullptr;
  vkGetPhysicalDeviceExternalImageFormatPropertiesNV_func_t
      vkGetPhysicalDeviceExternalImageFormatPropertiesNV = nullptr;
  vkGetPhysicalDeviceFeatures2KHR_func_t vkGetPhysicalDeviceFeatures2KHR =
      nullptr;
  vkGetPhysicalDeviceProperties2KHR_func_t vkGetPhysicalDeviceProperties2KHR =
      nullptr;
  vkGetPhysicalDeviceFormatProperties2KHR_func_t
      vkGetPhysicalDeviceFormatProperties2KHR = nullptr;
  vkGetPhysicalDeviceImageFormatProperties2KHR_func_t
      vkGetPhysicalDeviceImageFormatProperties2KHR = nullptr;
  vkGetPhysicalDeviceQueueFamilyProperties2KHR_func_t
      vkGetPhysicalDeviceQueueFamilyProperties2KHR = nullptr;
  vkGetPhysicalDeviceMemoryProperties2KHR_func_t
      vkGetPhysicalDeviceMemoryProperties2KHR = nullptr;
  vkGetPhysicalDeviceSparseImageFormatProperties2KHR_func_t
      vkGetPhysicalDeviceSparseImageFormatProperties2KHR = nullptr;
  vkGetPhysicalDevicePresentRectanglesKHX_func_t
      vkGetPhysicalDevicePresentRectanglesKHX = nullptr;
#ifdef VK_USE_PLATFORM_VI_NN
  vkCreateViSurfaceNN_func_t vkCreateViSurfaceNN = nullptr;
#endif
  vkEnumeratePhysicalDeviceGroupsKHX_func_t vkEnumeratePhysicalDeviceGroupsKHX =
      nullptr;
  vkGetPhysicalDeviceExternalBufferPropertiesKHX_func_t
      vkGetPhysicalDeviceExternalBufferPropertiesKHX = nullptr;
  vkGetPhysicalDeviceExternalSemaphorePropertiesKHX_func_t
      vkGetPhysicalDeviceExternalSemaphorePropertiesKHX = nullptr;
  vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX_func_t
      vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = nullptr;
  vkReleaseDisplayEXT_func_t vkReleaseDisplayEXT = nullptr;
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
  vkAcquireXlibDisplayEXT_func_t vkAcquireXlibDisplayEXT = nullptr;
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
  vkGetRandROutputDisplayEXT_func_t vkGetRandROutputDisplayEXT = nullptr;
#endif
  vkGetPhysicalDeviceSurfaceCapabilities2EXT_func_t
      vkGetPhysicalDeviceSurfaceCapabilities2EXT = nullptr;
#ifdef VK_USE_PLATFORM_IOS_MVK
  vkCreateIOSSurfaceMVK_func_t vkCreateIOSSurfaceMVK = nullptr;
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
  vkCreateMacOSSurfaceMVK_func_t vkCreateMacOSSurfaceMVK = nullptr;
#endif

};  // struct VkInstanceDispatch

struct VkDeviceDispatch {
  VkDevice device;

  vkDestroyDevice_func_t vkDestroyDevice = nullptr;
  vkGetDeviceQueue_func_t vkGetDeviceQueue = nullptr;
  vkQueueSubmit_func_t vkQueueSubmit = nullptr;
  vkQueueWaitIdle_func_t vkQueueWaitIdle = nullptr;
  vkDeviceWaitIdle_func_t vkDeviceWaitIdle = nullptr;
  vkAllocateMemory_func_t vkAllocateMemory = nullptr;
  vkFreeMemory_func_t vkFreeMemory = nullptr;
  vkMapMemory_func_t vkMapMemory = nullptr;
  vkUnmapMemory_func_t vkUnmapMemory = nullptr;
  vkFlushMappedMemoryRanges_func_t vkFlushMappedMemoryRanges = nullptr;
  vkInvalidateMappedMemoryRanges_func_t vkInvalidateMappedMemoryRanges =
      nullptr;
  vkGetDeviceMemoryCommitment_func_t vkGetDeviceMemoryCommitment = nullptr;
  vkGetBufferMemoryRequirements_func_t vkGetBufferMemoryRequirements = nullptr;
  vkBindBufferMemory_func_t vkBindBufferMemory = nullptr;
  vkGetImageMemoryRequirements_func_t vkGetImageMemoryRequirements = nullptr;
  vkBindImageMemory_func_t vkBindImageMemory = nullptr;
  vkGetImageSparseMemoryRequirements_func_t vkGetImageSparseMemoryRequirements =
      nullptr;
  vkQueueBindSparse_func_t vkQueueBindSparse = nullptr;
  vkCreateFence_func_t vkCreateFence = nullptr;
  vkDestroyFence_func_t vkDestroyFence = nullptr;
  vkResetFences_func_t vkResetFences = nullptr;
  vkGetFenceStatus_func_t vkGetFenceStatus = nullptr;
  vkWaitForFences_func_t vkWaitForFences = nullptr;
  vkCreateSemaphore_func_t vkCreateSemaphore = nullptr;
  vkDestroySemaphore_func_t vkDestroySemaphore = nullptr;
  vkCreateEvent_func_t vkCreateEvent = nullptr;
  vkDestroyEvent_func_t vkDestroyEvent = nullptr;
  vkGetEventStatus_func_t vkGetEventStatus = nullptr;
  vkSetEvent_func_t vkSetEvent = nullptr;
  vkResetEvent_func_t vkResetEvent = nullptr;
  vkCreateQueryPool_func_t vkCreateQueryPool = nullptr;
  vkDestroyQueryPool_func_t vkDestroyQueryPool = nullptr;
  vkGetQueryPoolResults_func_t vkGetQueryPoolResults = nullptr;
  vkCreateBuffer_func_t vkCreateBuffer = nullptr;
  vkDestroyBuffer_func_t vkDestroyBuffer = nullptr;
  vkCreateBufferView_func_t vkCreateBufferView = nullptr;
  vkDestroyBufferView_func_t vkDestroyBufferView = nullptr;
  vkCreateImage_func_t vkCreateImage = nullptr;
  vkDestroyImage_func_t vkDestroyImage = nullptr;
  vkGetImageSubresourceLayout_func_t vkGetImageSubresourceLayout = nullptr;
  vkCreateImageView_func_t vkCreateImageView = nullptr;
  vkDestroyImageView_func_t vkDestroyImageView = nullptr;
  vkCreateShaderModule_func_t vkCreateShaderModule = nullptr;
  vkDestroyShaderModule_func_t vkDestroyShaderModule = nullptr;
  vkCreatePipelineCache_func_t vkCreatePipelineCache = nullptr;
  vkDestroyPipelineCache_func_t vkDestroyPipelineCache = nullptr;
  vkGetPipelineCacheData_func_t vkGetPipelineCacheData = nullptr;
  vkMergePipelineCaches_func_t vkMergePipelineCaches = nullptr;
  vkCreateGraphicsPipelines_func_t vkCreateGraphicsPipelines = nullptr;
  vkCreateComputePipelines_func_t vkCreateComputePipelines = nullptr;
  vkDestroyPipeline_func_t vkDestroyPipeline = nullptr;
  vkCreatePipelineLayout_func_t vkCreatePipelineLayout = nullptr;
  vkDestroyPipelineLayout_func_t vkDestroyPipelineLayout = nullptr;
  vkCreateSampler_func_t vkCreateSampler = nullptr;
  vkDestroySampler_func_t vkDestroySampler = nullptr;
  vkCreateDescriptorSetLayout_func_t vkCreateDescriptorSetLayout = nullptr;
  vkDestroyDescriptorSetLayout_func_t vkDestroyDescriptorSetLayout = nullptr;
  vkCreateDescriptorPool_func_t vkCreateDescriptorPool = nullptr;
  vkDestroyDescriptorPool_func_t vkDestroyDescriptorPool = nullptr;
  vkResetDescriptorPool_func_t vkResetDescriptorPool = nullptr;
  vkAllocateDescriptorSets_func_t vkAllocateDescriptorSets = nullptr;
  vkFreeDescriptorSets_func_t vkFreeDescriptorSets = nullptr;
  vkUpdateDescriptorSets_func_t vkUpdateDescriptorSets = nullptr;
  vkCreateFramebuffer_func_t vkCreateFramebuffer = nullptr;
  vkDestroyFramebuffer_func_t vkDestroyFramebuffer = nullptr;
  vkCreateRenderPass_func_t vkCreateRenderPass = nullptr;
  vkDestroyRenderPass_func_t vkDestroyRenderPass = nullptr;
  vkGetRenderAreaGranularity_func_t vkGetRenderAreaGranularity = nullptr;
  vkCreateCommandPool_func_t vkCreateCommandPool = nullptr;
  vkDestroyCommandPool_func_t vkDestroyCommandPool = nullptr;
  vkResetCommandPool_func_t vkResetCommandPool = nullptr;
  vkAllocateCommandBuffers_func_t vkAllocateCommandBuffers = nullptr;
  vkFreeCommandBuffers_func_t vkFreeCommandBuffers = nullptr;
  vkBeginCommandBuffer_func_t vkBeginCommandBuffer = nullptr;
  vkEndCommandBuffer_func_t vkEndCommandBuffer = nullptr;
  vkResetCommandBuffer_func_t vkResetCommandBuffer = nullptr;
  vkCmdBindPipeline_func_t vkCmdBindPipeline = nullptr;
  vkCmdSetViewport_func_t vkCmdSetViewport = nullptr;
  vkCmdSetScissor_func_t vkCmdSetScissor = nullptr;
  vkCmdSetLineWidth_func_t vkCmdSetLineWidth = nullptr;
  vkCmdSetDepthBias_func_t vkCmdSetDepthBias = nullptr;
  vkCmdSetBlendConstants_func_t vkCmdSetBlendConstants = nullptr;
  vkCmdSetDepthBounds_func_t vkCmdSetDepthBounds = nullptr;
  vkCmdSetStencilCompareMask_func_t vkCmdSetStencilCompareMask = nullptr;
  vkCmdSetStencilWriteMask_func_t vkCmdSetStencilWriteMask = nullptr;
  vkCmdSetStencilReference_func_t vkCmdSetStencilReference = nullptr;
  vkCmdBindDescriptorSets_func_t vkCmdBindDescriptorSets = nullptr;
  vkCmdBindIndexBuffer_func_t vkCmdBindIndexBuffer = nullptr;
  vkCmdBindVertexBuffers_func_t vkCmdBindVertexBuffers = nullptr;
  vkCmdDraw_func_t vkCmdDraw = nullptr;
  vkCmdDrawIndexed_func_t vkCmdDrawIndexed = nullptr;
  vkCmdDrawIndirect_func_t vkCmdDrawIndirect = nullptr;
  vkCmdDrawIndexedIndirect_func_t vkCmdDrawIndexedIndirect = nullptr;
  vkCmdDispatch_func_t vkCmdDispatch = nullptr;
  vkCmdDispatchIndirect_func_t vkCmdDispatchIndirect = nullptr;
  vkCmdCopyBuffer_func_t vkCmdCopyBuffer = nullptr;
  vkCmdCopyImage_func_t vkCmdCopyImage = nullptr;
  vkCmdBlitImage_func_t vkCmdBlitImage = nullptr;
  vkCmdCopyBufferToImage_func_t vkCmdCopyBufferToImage = nullptr;
  vkCmdCopyImageToBuffer_func_t vkCmdCopyImageToBuffer = nullptr;
  vkCmdUpdateBuffer_func_t vkCmdUpdateBuffer = nullptr;
  vkCmdFillBuffer_func_t vkCmdFillBuffer = nullptr;
  vkCmdClearColorImage_func_t vkCmdClearColorImage = nullptr;
  vkCmdClearDepthStencilImage_func_t vkCmdClearDepthStencilImage = nullptr;
  vkCmdClearAttachments_func_t vkCmdClearAttachments = nullptr;
  vkCmdResolveImage_func_t vkCmdResolveImage = nullptr;
  vkCmdSetEvent_func_t vkCmdSetEvent = nullptr;
  vkCmdResetEvent_func_t vkCmdResetEvent = nullptr;
  vkCmdWaitEvents_func_t vkCmdWaitEvents = nullptr;
  vkCmdPipelineBarrier_func_t vkCmdPipelineBarrier = nullptr;
  vkCmdBeginQuery_func_t vkCmdBeginQuery = nullptr;
  vkCmdEndQuery_func_t vkCmdEndQuery = nullptr;
  vkCmdResetQueryPool_func_t vkCmdResetQueryPool = nullptr;
  vkCmdWriteTimestamp_func_t vkCmdWriteTimestamp = nullptr;
  vkCmdCopyQueryPoolResults_func_t vkCmdCopyQueryPoolResults = nullptr;
  vkCmdPushConstants_func_t vkCmdPushConstants = nullptr;
  vkCmdBeginRenderPass_func_t vkCmdBeginRenderPass = nullptr;
  vkCmdNextSubpass_func_t vkCmdNextSubpass = nullptr;
  vkCmdEndRenderPass_func_t vkCmdEndRenderPass = nullptr;
  vkCmdExecuteCommands_func_t vkCmdExecuteCommands = nullptr;
  vkCreateSwapchainKHR_func_t vkCreateSwapchainKHR = nullptr;
  vkDestroySwapchainKHR_func_t vkDestroySwapchainKHR = nullptr;
  vkGetSwapchainImagesKHR_func_t vkGetSwapchainImagesKHR = nullptr;
  vkAcquireNextImageKHR_func_t vkAcquireNextImageKHR = nullptr;
  vkQueuePresentKHR_func_t vkQueuePresentKHR = nullptr;
  vkCreateSharedSwapchainsKHR_func_t vkCreateSharedSwapchainsKHR = nullptr;
  vkGetSwapchainGrallocUsageANDROID_func_t vkGetSwapchainGrallocUsageANDROID =
      nullptr;
  vkAcquireImageANDROID_func_t vkAcquireImageANDROID = nullptr;
  vkQueueSignalReleaseImageANDROID_func_t vkQueueSignalReleaseImageANDROID =
      nullptr;
  vkDebugMarkerSetObjectTagEXT_func_t vkDebugMarkerSetObjectTagEXT = nullptr;
  vkDebugMarkerSetObjectNameEXT_func_t vkDebugMarkerSetObjectNameEXT = nullptr;
  vkCmdDebugMarkerBeginEXT_func_t vkCmdDebugMarkerBeginEXT = nullptr;
  vkCmdDebugMarkerEndEXT_func_t vkCmdDebugMarkerEndEXT = nullptr;
  vkCmdDebugMarkerInsertEXT_func_t vkCmdDebugMarkerInsertEXT = nullptr;
  vkCmdDrawIndirectCountAMD_func_t vkCmdDrawIndirectCountAMD = nullptr;
  vkCmdDrawIndexedIndirectCountAMD_func_t vkCmdDrawIndexedIndirectCountAMD =
      nullptr;
#ifdef VK_USE_PLATFORM_WIN32_KHR
  vkGetMemoryWin32HandleNV_func_t vkGetMemoryWin32HandleNV = nullptr;
#endif
  vkGetDeviceGroupPeerMemoryFeaturesKHX_func_t
      vkGetDeviceGroupPeerMemoryFeaturesKHX = nullptr;
  vkBindBufferMemory2KHX_func_t vkBindBufferMemory2KHX = nullptr;
  vkBindImageMemory2KHX_func_t vkBindImageMemory2KHX = nullptr;
  vkCmdSetDeviceMaskKHX_func_t vkCmdSetDeviceMaskKHX = nullptr;
  vkGetDeviceGroupPresentCapabilitiesKHX_func_t
      vkGetDeviceGroupPresentCapabilitiesKHX = nullptr;
  vkGetDeviceGroupSurfacePresentModesKHX_func_t
      vkGetDeviceGroupSurfacePresentModesKHX = nullptr;
  vkAcquireNextImage2KHX_func_t vkAcquireNextImage2KHX = nullptr;
  vkCmdDispatchBaseKHX_func_t vkCmdDispatchBaseKHX = nullptr;
  vkTrimCommandPoolKHR_func_t vkTrimCommandPoolKHR = nullptr;
#ifdef VK_USE_PLATFORM_WIN32_KHX
  vkGetMemoryWin32HandleKHX_func_t vkGetMemoryWin32HandleKHX = nullptr;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHX
  vkGetMemoryWin32HandlePropertiesKHX_func_t
      vkGetMemoryWin32HandlePropertiesKHX = nullptr;
#endif
  vkGetMemoryFdKHX_func_t vkGetMemoryFdKHX = nullptr;
  vkGetMemoryFdPropertiesKHX_func_t vkGetMemoryFdPropertiesKHX = nullptr;
#ifdef VK_USE_PLATFORM_WIN32_KHX
  vkImportSemaphoreWin32HandleKHX_func_t vkImportSemaphoreWin32HandleKHX =
      nullptr;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHX
  vkGetSemaphoreWin32HandleKHX_func_t vkGetSemaphoreWin32HandleKHX = nullptr;
#endif
  vkImportSemaphoreFdKHX_func_t vkImportSemaphoreFdKHX = nullptr;
  vkGetSemaphoreFdKHX_func_t vkGetSemaphoreFdKHX = nullptr;
  vkCmdPushDescriptorSetKHR_func_t vkCmdPushDescriptorSetKHR = nullptr;
  vkCreateDescriptorUpdateTemplateKHR_func_t
      vkCreateDescriptorUpdateTemplateKHR = nullptr;
  vkDestroyDescriptorUpdateTemplateKHR_func_t
      vkDestroyDescriptorUpdateTemplateKHR = nullptr;
  vkUpdateDescriptorSetWithTemplateKHR_func_t
      vkUpdateDescriptorSetWithTemplateKHR = nullptr;
  vkCmdPushDescriptorSetWithTemplateKHR_func_t
      vkCmdPushDescriptorSetWithTemplateKHR = nullptr;
  vkCmdProcessCommandsNVX_func_t vkCmdProcessCommandsNVX = nullptr;
  vkCmdReserveSpaceForCommandsNVX_func_t vkCmdReserveSpaceForCommandsNVX =
      nullptr;
  vkCreateIndirectCommandsLayoutNVX_func_t vkCreateIndirectCommandsLayoutNVX =
      nullptr;
  vkDestroyIndirectCommandsLayoutNVX_func_t vkDestroyIndirectCommandsLayoutNVX =
      nullptr;
  vkCreateObjectTableNVX_func_t vkCreateObjectTableNVX = nullptr;
  vkDestroyObjectTableNVX_func_t vkDestroyObjectTableNVX = nullptr;
  vkRegisterObjectsNVX_func_t vkRegisterObjectsNVX = nullptr;
  vkUnregisterObjectsNVX_func_t vkUnregisterObjectsNVX = nullptr;
  vkCmdSetViewportWScalingNV_func_t vkCmdSetViewportWScalingNV = nullptr;
  vkDisplayPowerControlEXT_func_t vkDisplayPowerControlEXT = nullptr;
  vkRegisterDeviceEventEXT_func_t vkRegisterDeviceEventEXT = nullptr;
  vkRegisterDisplayEventEXT_func_t vkRegisterDisplayEventEXT = nullptr;
  vkGetSwapchainCounterEXT_func_t vkGetSwapchainCounterEXT = nullptr;
  vkGetRefreshCycleDurationGOOGLE_func_t vkGetRefreshCycleDurationGOOGLE =
      nullptr;
  vkGetPastPresentationTimingGOOGLE_func_t vkGetPastPresentationTimingGOOGLE =
      nullptr;
  vkCmdSetDiscardRectangleEXT_func_t vkCmdSetDiscardRectangleEXT = nullptr;
  vkSetHdrMetadataEXT_func_t vkSetHdrMetadataEXT = nullptr;

};  // struct VkDeviceDispatch
