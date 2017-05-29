#pragma once
#include "InplaceStream.h"
#include <vulkan/vulkan.h>
#include <inttypes.h>
uint32_t vkUtilsEncodingSize_VkOffset2D(const VkOffset2D* data);
void vkUtilsPack_VkOffset2D(android::base::InplaceStream* stream, const VkOffset2D* data);
VkOffset2D* vkUtilsUnpack_VkOffset2D(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkOffset3D(const VkOffset3D* data);
void vkUtilsPack_VkOffset3D(android::base::InplaceStream* stream, const VkOffset3D* data);
VkOffset3D* vkUtilsUnpack_VkOffset3D(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExtent2D(const VkExtent2D* data);
void vkUtilsPack_VkExtent2D(android::base::InplaceStream* stream, const VkExtent2D* data);
VkExtent2D* vkUtilsUnpack_VkExtent2D(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExtent3D(const VkExtent3D* data);
void vkUtilsPack_VkExtent3D(android::base::InplaceStream* stream, const VkExtent3D* data);
VkExtent3D* vkUtilsUnpack_VkExtent3D(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkViewport(const VkViewport* data);
void vkUtilsPack_VkViewport(android::base::InplaceStream* stream, const VkViewport* data);
VkViewport* vkUtilsUnpack_VkViewport(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkRect2D(const VkRect2D* data);
void vkUtilsPack_VkRect2D(android::base::InplaceStream* stream, const VkRect2D* data);
VkRect2D* vkUtilsUnpack_VkRect2D(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkClearRect(const VkClearRect* data);
void vkUtilsPack_VkClearRect(android::base::InplaceStream* stream, const VkClearRect* data);
VkClearRect* vkUtilsUnpack_VkClearRect(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkComponentMapping(const VkComponentMapping* data);
void vkUtilsPack_VkComponentMapping(android::base::InplaceStream* stream, const VkComponentMapping* data);
VkComponentMapping* vkUtilsUnpack_VkComponentMapping(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceProperties(const VkPhysicalDeviceProperties* data);
void vkUtilsPack_VkPhysicalDeviceProperties(android::base::InplaceStream* stream, const VkPhysicalDeviceProperties* data);
VkPhysicalDeviceProperties* vkUtilsUnpack_VkPhysicalDeviceProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExtensionProperties(const VkExtensionProperties* data);
void vkUtilsPack_VkExtensionProperties(android::base::InplaceStream* stream, const VkExtensionProperties* data);
VkExtensionProperties* vkUtilsUnpack_VkExtensionProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkLayerProperties(const VkLayerProperties* data);
void vkUtilsPack_VkLayerProperties(android::base::InplaceStream* stream, const VkLayerProperties* data);
VkLayerProperties* vkUtilsUnpack_VkLayerProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSubmitInfo(const VkSubmitInfo* data);
void vkUtilsPack_VkSubmitInfo(android::base::InplaceStream* stream, const VkSubmitInfo* data);
VkSubmitInfo* vkUtilsUnpack_VkSubmitInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkApplicationInfo(const VkApplicationInfo* data);
void vkUtilsPack_VkApplicationInfo(android::base::InplaceStream* stream, const VkApplicationInfo* data);
VkApplicationInfo* vkUtilsUnpack_VkApplicationInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkAllocationCallbacks(const VkAllocationCallbacks* data);
void vkUtilsPack_VkAllocationCallbacks(android::base::InplaceStream* stream, const VkAllocationCallbacks* data);
VkAllocationCallbacks* vkUtilsUnpack_VkAllocationCallbacks(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceQueueCreateInfo(const VkDeviceQueueCreateInfo* data);
void vkUtilsPack_VkDeviceQueueCreateInfo(android::base::InplaceStream* stream, const VkDeviceQueueCreateInfo* data);
VkDeviceQueueCreateInfo* vkUtilsUnpack_VkDeviceQueueCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceCreateInfo(const VkDeviceCreateInfo* data);
void vkUtilsPack_VkDeviceCreateInfo(android::base::InplaceStream* stream, const VkDeviceCreateInfo* data);
VkDeviceCreateInfo* vkUtilsUnpack_VkDeviceCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkInstanceCreateInfo(const VkInstanceCreateInfo* data);
void vkUtilsPack_VkInstanceCreateInfo(android::base::InplaceStream* stream, const VkInstanceCreateInfo* data);
VkInstanceCreateInfo* vkUtilsUnpack_VkInstanceCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkQueueFamilyProperties(const VkQueueFamilyProperties* data);
void vkUtilsPack_VkQueueFamilyProperties(android::base::InplaceStream* stream, const VkQueueFamilyProperties* data);
VkQueueFamilyProperties* vkUtilsUnpack_VkQueueFamilyProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties* data);
void vkUtilsPack_VkPhysicalDeviceMemoryProperties(android::base::InplaceStream* stream, const VkPhysicalDeviceMemoryProperties* data);
VkPhysicalDeviceMemoryProperties* vkUtilsUnpack_VkPhysicalDeviceMemoryProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryAllocateInfo(const VkMemoryAllocateInfo* data);
void vkUtilsPack_VkMemoryAllocateInfo(android::base::InplaceStream* stream, const VkMemoryAllocateInfo* data);
VkMemoryAllocateInfo* vkUtilsUnpack_VkMemoryAllocateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryRequirements(const VkMemoryRequirements* data);
void vkUtilsPack_VkMemoryRequirements(android::base::InplaceStream* stream, const VkMemoryRequirements* data);
VkMemoryRequirements* vkUtilsUnpack_VkMemoryRequirements(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseImageFormatProperties(const VkSparseImageFormatProperties* data);
void vkUtilsPack_VkSparseImageFormatProperties(android::base::InplaceStream* stream, const VkSparseImageFormatProperties* data);
VkSparseImageFormatProperties* vkUtilsUnpack_VkSparseImageFormatProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseImageMemoryRequirements(const VkSparseImageMemoryRequirements* data);
void vkUtilsPack_VkSparseImageMemoryRequirements(android::base::InplaceStream* stream, const VkSparseImageMemoryRequirements* data);
VkSparseImageMemoryRequirements* vkUtilsUnpack_VkSparseImageMemoryRequirements(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryType(const VkMemoryType* data);
void vkUtilsPack_VkMemoryType(android::base::InplaceStream* stream, const VkMemoryType* data);
VkMemoryType* vkUtilsUnpack_VkMemoryType(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryHeap(const VkMemoryHeap* data);
void vkUtilsPack_VkMemoryHeap(android::base::InplaceStream* stream, const VkMemoryHeap* data);
VkMemoryHeap* vkUtilsUnpack_VkMemoryHeap(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMappedMemoryRange(const VkMappedMemoryRange* data);
void vkUtilsPack_VkMappedMemoryRange(android::base::InplaceStream* stream, const VkMappedMemoryRange* data);
VkMappedMemoryRange* vkUtilsUnpack_VkMappedMemoryRange(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkFormatProperties(const VkFormatProperties* data);
void vkUtilsPack_VkFormatProperties(android::base::InplaceStream* stream, const VkFormatProperties* data);
VkFormatProperties* vkUtilsUnpack_VkFormatProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageFormatProperties(const VkImageFormatProperties* data);
void vkUtilsPack_VkImageFormatProperties(android::base::InplaceStream* stream, const VkImageFormatProperties* data);
VkImageFormatProperties* vkUtilsUnpack_VkImageFormatProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorImageInfo(const VkDescriptorImageInfo* data);
void vkUtilsPack_VkDescriptorImageInfo(android::base::InplaceStream* stream, const VkDescriptorImageInfo* data);
VkDescriptorImageInfo* vkUtilsUnpack_VkDescriptorImageInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorBufferInfo(const VkDescriptorBufferInfo* data);
void vkUtilsPack_VkDescriptorBufferInfo(android::base::InplaceStream* stream, const VkDescriptorBufferInfo* data);
VkDescriptorBufferInfo* vkUtilsUnpack_VkDescriptorBufferInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkWriteDescriptorSet(const VkWriteDescriptorSet* data);
void vkUtilsPack_VkWriteDescriptorSet(android::base::InplaceStream* stream, const VkWriteDescriptorSet* data);
VkWriteDescriptorSet* vkUtilsUnpack_VkWriteDescriptorSet(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkCopyDescriptorSet(const VkCopyDescriptorSet* data);
void vkUtilsPack_VkCopyDescriptorSet(android::base::InplaceStream* stream, const VkCopyDescriptorSet* data);
VkCopyDescriptorSet* vkUtilsUnpack_VkCopyDescriptorSet(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBufferCreateInfo(const VkBufferCreateInfo* data);
void vkUtilsPack_VkBufferCreateInfo(android::base::InplaceStream* stream, const VkBufferCreateInfo* data);
VkBufferCreateInfo* vkUtilsUnpack_VkBufferCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBufferViewCreateInfo(const VkBufferViewCreateInfo* data);
void vkUtilsPack_VkBufferViewCreateInfo(android::base::InplaceStream* stream, const VkBufferViewCreateInfo* data);
VkBufferViewCreateInfo* vkUtilsUnpack_VkBufferViewCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageSubresource(const VkImageSubresource* data);
void vkUtilsPack_VkImageSubresource(android::base::InplaceStream* stream, const VkImageSubresource* data);
VkImageSubresource* vkUtilsUnpack_VkImageSubresource(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageSubresourceRange(const VkImageSubresourceRange* data);
void vkUtilsPack_VkImageSubresourceRange(android::base::InplaceStream* stream, const VkImageSubresourceRange* data);
VkImageSubresourceRange* vkUtilsUnpack_VkImageSubresourceRange(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryBarrier(const VkMemoryBarrier* data);
void vkUtilsPack_VkMemoryBarrier(android::base::InplaceStream* stream, const VkMemoryBarrier* data);
VkMemoryBarrier* vkUtilsUnpack_VkMemoryBarrier(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBufferMemoryBarrier(const VkBufferMemoryBarrier* data);
void vkUtilsPack_VkBufferMemoryBarrier(android::base::InplaceStream* stream, const VkBufferMemoryBarrier* data);
VkBufferMemoryBarrier* vkUtilsUnpack_VkBufferMemoryBarrier(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageMemoryBarrier(const VkImageMemoryBarrier* data);
void vkUtilsPack_VkImageMemoryBarrier(android::base::InplaceStream* stream, const VkImageMemoryBarrier* data);
VkImageMemoryBarrier* vkUtilsUnpack_VkImageMemoryBarrier(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageCreateInfo(const VkImageCreateInfo* data);
void vkUtilsPack_VkImageCreateInfo(android::base::InplaceStream* stream, const VkImageCreateInfo* data);
VkImageCreateInfo* vkUtilsUnpack_VkImageCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSubresourceLayout(const VkSubresourceLayout* data);
void vkUtilsPack_VkSubresourceLayout(android::base::InplaceStream* stream, const VkSubresourceLayout* data);
VkSubresourceLayout* vkUtilsUnpack_VkSubresourceLayout(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageViewCreateInfo(const VkImageViewCreateInfo* data);
void vkUtilsPack_VkImageViewCreateInfo(android::base::InplaceStream* stream, const VkImageViewCreateInfo* data);
VkImageViewCreateInfo* vkUtilsUnpack_VkImageViewCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBufferCopy(const VkBufferCopy* data);
void vkUtilsPack_VkBufferCopy(android::base::InplaceStream* stream, const VkBufferCopy* data);
VkBufferCopy* vkUtilsUnpack_VkBufferCopy(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseMemoryBind(const VkSparseMemoryBind* data);
void vkUtilsPack_VkSparseMemoryBind(android::base::InplaceStream* stream, const VkSparseMemoryBind* data);
VkSparseMemoryBind* vkUtilsUnpack_VkSparseMemoryBind(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseImageMemoryBind(const VkSparseImageMemoryBind* data);
void vkUtilsPack_VkSparseImageMemoryBind(android::base::InplaceStream* stream, const VkSparseImageMemoryBind* data);
VkSparseImageMemoryBind* vkUtilsUnpack_VkSparseImageMemoryBind(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseBufferMemoryBindInfo(const VkSparseBufferMemoryBindInfo* data);
void vkUtilsPack_VkSparseBufferMemoryBindInfo(android::base::InplaceStream* stream, const VkSparseBufferMemoryBindInfo* data);
VkSparseBufferMemoryBindInfo* vkUtilsUnpack_VkSparseBufferMemoryBindInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseImageOpaqueMemoryBindInfo(const VkSparseImageOpaqueMemoryBindInfo* data);
void vkUtilsPack_VkSparseImageOpaqueMemoryBindInfo(android::base::InplaceStream* stream, const VkSparseImageOpaqueMemoryBindInfo* data);
VkSparseImageOpaqueMemoryBindInfo* vkUtilsUnpack_VkSparseImageOpaqueMemoryBindInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseImageMemoryBindInfo(const VkSparseImageMemoryBindInfo* data);
void vkUtilsPack_VkSparseImageMemoryBindInfo(android::base::InplaceStream* stream, const VkSparseImageMemoryBindInfo* data);
VkSparseImageMemoryBindInfo* vkUtilsUnpack_VkSparseImageMemoryBindInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBindSparseInfo(const VkBindSparseInfo* data);
void vkUtilsPack_VkBindSparseInfo(android::base::InplaceStream* stream, const VkBindSparseInfo* data);
VkBindSparseInfo* vkUtilsUnpack_VkBindSparseInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageSubresourceLayers(const VkImageSubresourceLayers* data);
void vkUtilsPack_VkImageSubresourceLayers(android::base::InplaceStream* stream, const VkImageSubresourceLayers* data);
VkImageSubresourceLayers* vkUtilsUnpack_VkImageSubresourceLayers(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageCopy(const VkImageCopy* data);
void vkUtilsPack_VkImageCopy(android::base::InplaceStream* stream, const VkImageCopy* data);
VkImageCopy* vkUtilsUnpack_VkImageCopy(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageBlit(const VkImageBlit* data);
void vkUtilsPack_VkImageBlit(android::base::InplaceStream* stream, const VkImageBlit* data);
VkImageBlit* vkUtilsUnpack_VkImageBlit(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBufferImageCopy(const VkBufferImageCopy* data);
void vkUtilsPack_VkBufferImageCopy(android::base::InplaceStream* stream, const VkBufferImageCopy* data);
VkBufferImageCopy* vkUtilsUnpack_VkBufferImageCopy(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageResolve(const VkImageResolve* data);
void vkUtilsPack_VkImageResolve(android::base::InplaceStream* stream, const VkImageResolve* data);
VkImageResolve* vkUtilsUnpack_VkImageResolve(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkShaderModuleCreateInfo(const VkShaderModuleCreateInfo* data);
void vkUtilsPack_VkShaderModuleCreateInfo(android::base::InplaceStream* stream, const VkShaderModuleCreateInfo* data);
VkShaderModuleCreateInfo* vkUtilsUnpack_VkShaderModuleCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorSetLayoutBinding(const VkDescriptorSetLayoutBinding* data);
void vkUtilsPack_VkDescriptorSetLayoutBinding(android::base::InplaceStream* stream, const VkDescriptorSetLayoutBinding* data);
VkDescriptorSetLayoutBinding* vkUtilsUnpack_VkDescriptorSetLayoutBinding(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo* data);
void vkUtilsPack_VkDescriptorSetLayoutCreateInfo(android::base::InplaceStream* stream, const VkDescriptorSetLayoutCreateInfo* data);
VkDescriptorSetLayoutCreateInfo* vkUtilsUnpack_VkDescriptorSetLayoutCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorPoolSize(const VkDescriptorPoolSize* data);
void vkUtilsPack_VkDescriptorPoolSize(android::base::InplaceStream* stream, const VkDescriptorPoolSize* data);
VkDescriptorPoolSize* vkUtilsUnpack_VkDescriptorPoolSize(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorPoolCreateInfo(const VkDescriptorPoolCreateInfo* data);
void vkUtilsPack_VkDescriptorPoolCreateInfo(android::base::InplaceStream* stream, const VkDescriptorPoolCreateInfo* data);
VkDescriptorPoolCreateInfo* vkUtilsUnpack_VkDescriptorPoolCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorSetAllocateInfo(const VkDescriptorSetAllocateInfo* data);
void vkUtilsPack_VkDescriptorSetAllocateInfo(android::base::InplaceStream* stream, const VkDescriptorSetAllocateInfo* data);
VkDescriptorSetAllocateInfo* vkUtilsUnpack_VkDescriptorSetAllocateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSpecializationMapEntry(const VkSpecializationMapEntry* data);
void vkUtilsPack_VkSpecializationMapEntry(android::base::InplaceStream* stream, const VkSpecializationMapEntry* data);
VkSpecializationMapEntry* vkUtilsUnpack_VkSpecializationMapEntry(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSpecializationInfo(const VkSpecializationInfo* data);
void vkUtilsPack_VkSpecializationInfo(android::base::InplaceStream* stream, const VkSpecializationInfo* data);
VkSpecializationInfo* vkUtilsUnpack_VkSpecializationInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineShaderStageCreateInfo(const VkPipelineShaderStageCreateInfo* data);
void vkUtilsPack_VkPipelineShaderStageCreateInfo(android::base::InplaceStream* stream, const VkPipelineShaderStageCreateInfo* data);
VkPipelineShaderStageCreateInfo* vkUtilsUnpack_VkPipelineShaderStageCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkComputePipelineCreateInfo(const VkComputePipelineCreateInfo* data);
void vkUtilsPack_VkComputePipelineCreateInfo(android::base::InplaceStream* stream, const VkComputePipelineCreateInfo* data);
VkComputePipelineCreateInfo* vkUtilsUnpack_VkComputePipelineCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkVertexInputBindingDescription(const VkVertexInputBindingDescription* data);
void vkUtilsPack_VkVertexInputBindingDescription(android::base::InplaceStream* stream, const VkVertexInputBindingDescription* data);
VkVertexInputBindingDescription* vkUtilsUnpack_VkVertexInputBindingDescription(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkVertexInputAttributeDescription(const VkVertexInputAttributeDescription* data);
void vkUtilsPack_VkVertexInputAttributeDescription(android::base::InplaceStream* stream, const VkVertexInputAttributeDescription* data);
VkVertexInputAttributeDescription* vkUtilsUnpack_VkVertexInputAttributeDescription(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineVertexInputStateCreateInfo(const VkPipelineVertexInputStateCreateInfo* data);
void vkUtilsPack_VkPipelineVertexInputStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineVertexInputStateCreateInfo* data);
VkPipelineVertexInputStateCreateInfo* vkUtilsUnpack_VkPipelineVertexInputStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineInputAssemblyStateCreateInfo(const VkPipelineInputAssemblyStateCreateInfo* data);
void vkUtilsPack_VkPipelineInputAssemblyStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineInputAssemblyStateCreateInfo* data);
VkPipelineInputAssemblyStateCreateInfo* vkUtilsUnpack_VkPipelineInputAssemblyStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineTessellationStateCreateInfo(const VkPipelineTessellationStateCreateInfo* data);
void vkUtilsPack_VkPipelineTessellationStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineTessellationStateCreateInfo* data);
VkPipelineTessellationStateCreateInfo* vkUtilsUnpack_VkPipelineTessellationStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineViewportStateCreateInfo(const VkPipelineViewportStateCreateInfo* data);
void vkUtilsPack_VkPipelineViewportStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineViewportStateCreateInfo* data);
VkPipelineViewportStateCreateInfo* vkUtilsUnpack_VkPipelineViewportStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineRasterizationStateCreateInfo(const VkPipelineRasterizationStateCreateInfo* data);
void vkUtilsPack_VkPipelineRasterizationStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineRasterizationStateCreateInfo* data);
VkPipelineRasterizationStateCreateInfo* vkUtilsUnpack_VkPipelineRasterizationStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineMultisampleStateCreateInfo(const VkPipelineMultisampleStateCreateInfo* data);
void vkUtilsPack_VkPipelineMultisampleStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineMultisampleStateCreateInfo* data);
VkPipelineMultisampleStateCreateInfo* vkUtilsUnpack_VkPipelineMultisampleStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineColorBlendAttachmentState(const VkPipelineColorBlendAttachmentState* data);
void vkUtilsPack_VkPipelineColorBlendAttachmentState(android::base::InplaceStream* stream, const VkPipelineColorBlendAttachmentState* data);
VkPipelineColorBlendAttachmentState* vkUtilsUnpack_VkPipelineColorBlendAttachmentState(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendStateCreateInfo* data);
void vkUtilsPack_VkPipelineColorBlendStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineColorBlendStateCreateInfo* data);
VkPipelineColorBlendStateCreateInfo* vkUtilsUnpack_VkPipelineColorBlendStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkStencilOpState(const VkStencilOpState* data);
void vkUtilsPack_VkStencilOpState(android::base::InplaceStream* stream, const VkStencilOpState* data);
VkStencilOpState* vkUtilsUnpack_VkStencilOpState(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineDepthStencilStateCreateInfo(const VkPipelineDepthStencilStateCreateInfo* data);
void vkUtilsPack_VkPipelineDepthStencilStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineDepthStencilStateCreateInfo* data);
VkPipelineDepthStencilStateCreateInfo* vkUtilsUnpack_VkPipelineDepthStencilStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineDynamicStateCreateInfo(const VkPipelineDynamicStateCreateInfo* data);
void vkUtilsPack_VkPipelineDynamicStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineDynamicStateCreateInfo* data);
VkPipelineDynamicStateCreateInfo* vkUtilsUnpack_VkPipelineDynamicStateCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkGraphicsPipelineCreateInfo(const VkGraphicsPipelineCreateInfo* data);
void vkUtilsPack_VkGraphicsPipelineCreateInfo(android::base::InplaceStream* stream, const VkGraphicsPipelineCreateInfo* data);
VkGraphicsPipelineCreateInfo* vkUtilsUnpack_VkGraphicsPipelineCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineCacheCreateInfo(const VkPipelineCacheCreateInfo* data);
void vkUtilsPack_VkPipelineCacheCreateInfo(android::base::InplaceStream* stream, const VkPipelineCacheCreateInfo* data);
VkPipelineCacheCreateInfo* vkUtilsUnpack_VkPipelineCacheCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPushConstantRange(const VkPushConstantRange* data);
void vkUtilsPack_VkPushConstantRange(android::base::InplaceStream* stream, const VkPushConstantRange* data);
VkPushConstantRange* vkUtilsUnpack_VkPushConstantRange(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo* data);
void vkUtilsPack_VkPipelineLayoutCreateInfo(android::base::InplaceStream* stream, const VkPipelineLayoutCreateInfo* data);
VkPipelineLayoutCreateInfo* vkUtilsUnpack_VkPipelineLayoutCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSamplerCreateInfo(const VkSamplerCreateInfo* data);
void vkUtilsPack_VkSamplerCreateInfo(android::base::InplaceStream* stream, const VkSamplerCreateInfo* data);
VkSamplerCreateInfo* vkUtilsUnpack_VkSamplerCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkCommandPoolCreateInfo(const VkCommandPoolCreateInfo* data);
void vkUtilsPack_VkCommandPoolCreateInfo(android::base::InplaceStream* stream, const VkCommandPoolCreateInfo* data);
VkCommandPoolCreateInfo* vkUtilsUnpack_VkCommandPoolCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkCommandBufferAllocateInfo(const VkCommandBufferAllocateInfo* data);
void vkUtilsPack_VkCommandBufferAllocateInfo(android::base::InplaceStream* stream, const VkCommandBufferAllocateInfo* data);
VkCommandBufferAllocateInfo* vkUtilsUnpack_VkCommandBufferAllocateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkCommandBufferInheritanceInfo(const VkCommandBufferInheritanceInfo* data);
void vkUtilsPack_VkCommandBufferInheritanceInfo(android::base::InplaceStream* stream, const VkCommandBufferInheritanceInfo* data);
VkCommandBufferInheritanceInfo* vkUtilsUnpack_VkCommandBufferInheritanceInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkCommandBufferBeginInfo(const VkCommandBufferBeginInfo* data);
void vkUtilsPack_VkCommandBufferBeginInfo(android::base::InplaceStream* stream, const VkCommandBufferBeginInfo* data);
VkCommandBufferBeginInfo* vkUtilsUnpack_VkCommandBufferBeginInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkRenderPassBeginInfo(const VkRenderPassBeginInfo* data);
void vkUtilsPack_VkRenderPassBeginInfo(android::base::InplaceStream* stream, const VkRenderPassBeginInfo* data);
VkRenderPassBeginInfo* vkUtilsUnpack_VkRenderPassBeginInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkClearColorValue(const VkClearColorValue* data);
void vkUtilsPack_VkClearColorValue(android::base::InplaceStream* stream, const VkClearColorValue* data);
VkClearColorValue* vkUtilsUnpack_VkClearColorValue(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkClearDepthStencilValue(const VkClearDepthStencilValue* data);
void vkUtilsPack_VkClearDepthStencilValue(android::base::InplaceStream* stream, const VkClearDepthStencilValue* data);
VkClearDepthStencilValue* vkUtilsUnpack_VkClearDepthStencilValue(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkClearValue(const VkClearValue* data);
void vkUtilsPack_VkClearValue(android::base::InplaceStream* stream, const VkClearValue* data);
VkClearValue* vkUtilsUnpack_VkClearValue(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkClearAttachment(const VkClearAttachment* data);
void vkUtilsPack_VkClearAttachment(android::base::InplaceStream* stream, const VkClearAttachment* data);
VkClearAttachment* vkUtilsUnpack_VkClearAttachment(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkAttachmentDescription(const VkAttachmentDescription* data);
void vkUtilsPack_VkAttachmentDescription(android::base::InplaceStream* stream, const VkAttachmentDescription* data);
VkAttachmentDescription* vkUtilsUnpack_VkAttachmentDescription(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkAttachmentReference(const VkAttachmentReference* data);
void vkUtilsPack_VkAttachmentReference(android::base::InplaceStream* stream, const VkAttachmentReference* data);
VkAttachmentReference* vkUtilsUnpack_VkAttachmentReference(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSubpassDescription(const VkSubpassDescription* data);
void vkUtilsPack_VkSubpassDescription(android::base::InplaceStream* stream, const VkSubpassDescription* data);
VkSubpassDescription* vkUtilsUnpack_VkSubpassDescription(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSubpassDependency(const VkSubpassDependency* data);
void vkUtilsPack_VkSubpassDependency(android::base::InplaceStream* stream, const VkSubpassDependency* data);
VkSubpassDependency* vkUtilsUnpack_VkSubpassDependency(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkRenderPassCreateInfo(const VkRenderPassCreateInfo* data);
void vkUtilsPack_VkRenderPassCreateInfo(android::base::InplaceStream* stream, const VkRenderPassCreateInfo* data);
VkRenderPassCreateInfo* vkUtilsUnpack_VkRenderPassCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkEventCreateInfo(const VkEventCreateInfo* data);
void vkUtilsPack_VkEventCreateInfo(android::base::InplaceStream* stream, const VkEventCreateInfo* data);
VkEventCreateInfo* vkUtilsUnpack_VkEventCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkFenceCreateInfo(const VkFenceCreateInfo* data);
void vkUtilsPack_VkFenceCreateInfo(android::base::InplaceStream* stream, const VkFenceCreateInfo* data);
VkFenceCreateInfo* vkUtilsUnpack_VkFenceCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures* data);
void vkUtilsPack_VkPhysicalDeviceFeatures(android::base::InplaceStream* stream, const VkPhysicalDeviceFeatures* data);
VkPhysicalDeviceFeatures* vkUtilsUnpack_VkPhysicalDeviceFeatures(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceLimits(const VkPhysicalDeviceLimits* data);
void vkUtilsPack_VkPhysicalDeviceLimits(android::base::InplaceStream* stream, const VkPhysicalDeviceLimits* data);
VkPhysicalDeviceLimits* vkUtilsUnpack_VkPhysicalDeviceLimits(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceSparseProperties(const VkPhysicalDeviceSparseProperties* data);
void vkUtilsPack_VkPhysicalDeviceSparseProperties(android::base::InplaceStream* stream, const VkPhysicalDeviceSparseProperties* data);
VkPhysicalDeviceSparseProperties* vkUtilsUnpack_VkPhysicalDeviceSparseProperties(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSemaphoreCreateInfo(const VkSemaphoreCreateInfo* data);
void vkUtilsPack_VkSemaphoreCreateInfo(android::base::InplaceStream* stream, const VkSemaphoreCreateInfo* data);
VkSemaphoreCreateInfo* vkUtilsUnpack_VkSemaphoreCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkQueryPoolCreateInfo(const VkQueryPoolCreateInfo* data);
void vkUtilsPack_VkQueryPoolCreateInfo(android::base::InplaceStream* stream, const VkQueryPoolCreateInfo* data);
VkQueryPoolCreateInfo* vkUtilsUnpack_VkQueryPoolCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkFramebufferCreateInfo(const VkFramebufferCreateInfo* data);
void vkUtilsPack_VkFramebufferCreateInfo(android::base::InplaceStream* stream, const VkFramebufferCreateInfo* data);
VkFramebufferCreateInfo* vkUtilsUnpack_VkFramebufferCreateInfo(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDrawIndirectCommand(const VkDrawIndirectCommand* data);
void vkUtilsPack_VkDrawIndirectCommand(android::base::InplaceStream* stream, const VkDrawIndirectCommand* data);
VkDrawIndirectCommand* vkUtilsUnpack_VkDrawIndirectCommand(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDrawIndexedIndirectCommand(const VkDrawIndexedIndirectCommand* data);
void vkUtilsPack_VkDrawIndexedIndirectCommand(android::base::InplaceStream* stream, const VkDrawIndexedIndirectCommand* data);
VkDrawIndexedIndirectCommand* vkUtilsUnpack_VkDrawIndexedIndirectCommand(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDispatchIndirectCommand(const VkDispatchIndirectCommand* data);
void vkUtilsPack_VkDispatchIndirectCommand(android::base::InplaceStream* stream, const VkDispatchIndirectCommand* data);
VkDispatchIndirectCommand* vkUtilsUnpack_VkDispatchIndirectCommand(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSurfaceCapabilitiesKHR(const VkSurfaceCapabilitiesKHR* data);
void vkUtilsPack_VkSurfaceCapabilitiesKHR(android::base::InplaceStream* stream, const VkSurfaceCapabilitiesKHR* data);
VkSurfaceCapabilitiesKHR* vkUtilsUnpack_VkSurfaceCapabilitiesKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSurfaceFormatKHR(const VkSurfaceFormatKHR* data);
void vkUtilsPack_VkSurfaceFormatKHR(android::base::InplaceStream* stream, const VkSurfaceFormatKHR* data);
VkSurfaceFormatKHR* vkUtilsUnpack_VkSurfaceFormatKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSwapchainCreateInfoKHR(const VkSwapchainCreateInfoKHR* data);
void vkUtilsPack_VkSwapchainCreateInfoKHR(android::base::InplaceStream* stream, const VkSwapchainCreateInfoKHR* data);
VkSwapchainCreateInfoKHR* vkUtilsUnpack_VkSwapchainCreateInfoKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPresentInfoKHR(const VkPresentInfoKHR* data);
void vkUtilsPack_VkPresentInfoKHR(android::base::InplaceStream* stream, const VkPresentInfoKHR* data);
VkPresentInfoKHR* vkUtilsUnpack_VkPresentInfoKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayPropertiesKHR(const VkDisplayPropertiesKHR* data);
void vkUtilsPack_VkDisplayPropertiesKHR(android::base::InplaceStream* stream, const VkDisplayPropertiesKHR* data);
VkDisplayPropertiesKHR* vkUtilsUnpack_VkDisplayPropertiesKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayModeParametersKHR(const VkDisplayModeParametersKHR* data);
void vkUtilsPack_VkDisplayModeParametersKHR(android::base::InplaceStream* stream, const VkDisplayModeParametersKHR* data);
VkDisplayModeParametersKHR* vkUtilsUnpack_VkDisplayModeParametersKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayModePropertiesKHR(const VkDisplayModePropertiesKHR* data);
void vkUtilsPack_VkDisplayModePropertiesKHR(android::base::InplaceStream* stream, const VkDisplayModePropertiesKHR* data);
VkDisplayModePropertiesKHR* vkUtilsUnpack_VkDisplayModePropertiesKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayModeCreateInfoKHR(const VkDisplayModeCreateInfoKHR* data);
void vkUtilsPack_VkDisplayModeCreateInfoKHR(android::base::InplaceStream* stream, const VkDisplayModeCreateInfoKHR* data);
VkDisplayModeCreateInfoKHR* vkUtilsUnpack_VkDisplayModeCreateInfoKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayPlanePropertiesKHR(const VkDisplayPlanePropertiesKHR* data);
void vkUtilsPack_VkDisplayPlanePropertiesKHR(android::base::InplaceStream* stream, const VkDisplayPlanePropertiesKHR* data);
VkDisplayPlanePropertiesKHR* vkUtilsUnpack_VkDisplayPlanePropertiesKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayPlaneCapabilitiesKHR(const VkDisplayPlaneCapabilitiesKHR* data);
void vkUtilsPack_VkDisplayPlaneCapabilitiesKHR(android::base::InplaceStream* stream, const VkDisplayPlaneCapabilitiesKHR* data);
VkDisplayPlaneCapabilitiesKHR* vkUtilsUnpack_VkDisplayPlaneCapabilitiesKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplaySurfaceCreateInfoKHR(const VkDisplaySurfaceCreateInfoKHR* data);
void vkUtilsPack_VkDisplaySurfaceCreateInfoKHR(android::base::InplaceStream* stream, const VkDisplaySurfaceCreateInfoKHR* data);
VkDisplaySurfaceCreateInfoKHR* vkUtilsUnpack_VkDisplaySurfaceCreateInfoKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayPresentInfoKHR(const VkDisplayPresentInfoKHR* data);
void vkUtilsPack_VkDisplayPresentInfoKHR(android::base::InplaceStream* stream, const VkDisplayPresentInfoKHR* data);
VkDisplayPresentInfoKHR* vkUtilsUnpack_VkDisplayPresentInfoKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDebugReportCallbackCreateInfoEXT(const VkDebugReportCallbackCreateInfoEXT* data);
void vkUtilsPack_VkDebugReportCallbackCreateInfoEXT(android::base::InplaceStream* stream, const VkDebugReportCallbackCreateInfoEXT* data);
VkDebugReportCallbackCreateInfoEXT* vkUtilsUnpack_VkDebugReportCallbackCreateInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineRasterizationStateRasterizationOrderAMD(const VkPipelineRasterizationStateRasterizationOrderAMD* data);
void vkUtilsPack_VkPipelineRasterizationStateRasterizationOrderAMD(android::base::InplaceStream* stream, const VkPipelineRasterizationStateRasterizationOrderAMD* data);
VkPipelineRasterizationStateRasterizationOrderAMD* vkUtilsUnpack_VkPipelineRasterizationStateRasterizationOrderAMD(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDebugMarkerObjectNameInfoEXT(const VkDebugMarkerObjectNameInfoEXT* data);
void vkUtilsPack_VkDebugMarkerObjectNameInfoEXT(android::base::InplaceStream* stream, const VkDebugMarkerObjectNameInfoEXT* data);
VkDebugMarkerObjectNameInfoEXT* vkUtilsUnpack_VkDebugMarkerObjectNameInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDebugMarkerObjectTagInfoEXT(const VkDebugMarkerObjectTagInfoEXT* data);
void vkUtilsPack_VkDebugMarkerObjectTagInfoEXT(android::base::InplaceStream* stream, const VkDebugMarkerObjectTagInfoEXT* data);
VkDebugMarkerObjectTagInfoEXT* vkUtilsUnpack_VkDebugMarkerObjectTagInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDebugMarkerMarkerInfoEXT(const VkDebugMarkerMarkerInfoEXT* data);
void vkUtilsPack_VkDebugMarkerMarkerInfoEXT(android::base::InplaceStream* stream, const VkDebugMarkerMarkerInfoEXT* data);
VkDebugMarkerMarkerInfoEXT* vkUtilsUnpack_VkDebugMarkerMarkerInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDedicatedAllocationImageCreateInfoNV(const VkDedicatedAllocationImageCreateInfoNV* data);
void vkUtilsPack_VkDedicatedAllocationImageCreateInfoNV(android::base::InplaceStream* stream, const VkDedicatedAllocationImageCreateInfoNV* data);
VkDedicatedAllocationImageCreateInfoNV* vkUtilsUnpack_VkDedicatedAllocationImageCreateInfoNV(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDedicatedAllocationBufferCreateInfoNV(const VkDedicatedAllocationBufferCreateInfoNV* data);
void vkUtilsPack_VkDedicatedAllocationBufferCreateInfoNV(android::base::InplaceStream* stream, const VkDedicatedAllocationBufferCreateInfoNV* data);
VkDedicatedAllocationBufferCreateInfoNV* vkUtilsUnpack_VkDedicatedAllocationBufferCreateInfoNV(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDedicatedAllocationMemoryAllocateInfoNV(const VkDedicatedAllocationMemoryAllocateInfoNV* data);
void vkUtilsPack_VkDedicatedAllocationMemoryAllocateInfoNV(android::base::InplaceStream* stream, const VkDedicatedAllocationMemoryAllocateInfoNV* data);
VkDedicatedAllocationMemoryAllocateInfoNV* vkUtilsUnpack_VkDedicatedAllocationMemoryAllocateInfoNV(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkRenderPassMultiviewCreateInfoKHX(const VkRenderPassMultiviewCreateInfoKHX* data);
void vkUtilsPack_VkRenderPassMultiviewCreateInfoKHX(android::base::InplaceStream* stream, const VkRenderPassMultiviewCreateInfoKHX* data);
VkRenderPassMultiviewCreateInfoKHX* vkUtilsUnpack_VkRenderPassMultiviewCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMultiviewFeaturesKHX(const VkPhysicalDeviceMultiviewFeaturesKHX* data);
void vkUtilsPack_VkPhysicalDeviceMultiviewFeaturesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceMultiviewFeaturesKHX* data);
VkPhysicalDeviceMultiviewFeaturesKHX* vkUtilsUnpack_VkPhysicalDeviceMultiviewFeaturesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMultiviewPropertiesKHX(const VkPhysicalDeviceMultiviewPropertiesKHX* data);
void vkUtilsPack_VkPhysicalDeviceMultiviewPropertiesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceMultiviewPropertiesKHX* data);
VkPhysicalDeviceMultiviewPropertiesKHX* vkUtilsUnpack_VkPhysicalDeviceMultiviewPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalImageFormatPropertiesNV(const VkExternalImageFormatPropertiesNV* data);
void vkUtilsPack_VkExternalImageFormatPropertiesNV(android::base::InplaceStream* stream, const VkExternalImageFormatPropertiesNV* data);
VkExternalImageFormatPropertiesNV* vkUtilsUnpack_VkExternalImageFormatPropertiesNV(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalMemoryImageCreateInfoNV(const VkExternalMemoryImageCreateInfoNV* data);
void vkUtilsPack_VkExternalMemoryImageCreateInfoNV(android::base::InplaceStream* stream, const VkExternalMemoryImageCreateInfoNV* data);
VkExternalMemoryImageCreateInfoNV* vkUtilsUnpack_VkExternalMemoryImageCreateInfoNV(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExportMemoryAllocateInfoNV(const VkExportMemoryAllocateInfoNV* data);
void vkUtilsPack_VkExportMemoryAllocateInfoNV(android::base::InplaceStream* stream, const VkExportMemoryAllocateInfoNV* data);
VkExportMemoryAllocateInfoNV* vkUtilsUnpack_VkExportMemoryAllocateInfoNV(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceFeatures2KHR(const VkPhysicalDeviceFeatures2KHR* data);
void vkUtilsPack_VkPhysicalDeviceFeatures2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceFeatures2KHR* data);
VkPhysicalDeviceFeatures2KHR* vkUtilsUnpack_VkPhysicalDeviceFeatures2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceProperties2KHR(const VkPhysicalDeviceProperties2KHR* data);
void vkUtilsPack_VkPhysicalDeviceProperties2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceProperties2KHR* data);
VkPhysicalDeviceProperties2KHR* vkUtilsUnpack_VkPhysicalDeviceProperties2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkFormatProperties2KHR(const VkFormatProperties2KHR* data);
void vkUtilsPack_VkFormatProperties2KHR(android::base::InplaceStream* stream, const VkFormatProperties2KHR* data);
VkFormatProperties2KHR* vkUtilsUnpack_VkFormatProperties2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageFormatProperties2KHR(const VkImageFormatProperties2KHR* data);
void vkUtilsPack_VkImageFormatProperties2KHR(android::base::InplaceStream* stream, const VkImageFormatProperties2KHR* data);
VkImageFormatProperties2KHR* vkUtilsUnpack_VkImageFormatProperties2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceImageFormatInfo2KHR(const VkPhysicalDeviceImageFormatInfo2KHR* data);
void vkUtilsPack_VkPhysicalDeviceImageFormatInfo2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceImageFormatInfo2KHR* data);
VkPhysicalDeviceImageFormatInfo2KHR* vkUtilsUnpack_VkPhysicalDeviceImageFormatInfo2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkQueueFamilyProperties2KHR(const VkQueueFamilyProperties2KHR* data);
void vkUtilsPack_VkQueueFamilyProperties2KHR(android::base::InplaceStream* stream, const VkQueueFamilyProperties2KHR* data);
VkQueueFamilyProperties2KHR* vkUtilsUnpack_VkQueueFamilyProperties2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMemoryProperties2KHR(const VkPhysicalDeviceMemoryProperties2KHR* data);
void vkUtilsPack_VkPhysicalDeviceMemoryProperties2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceMemoryProperties2KHR* data);
VkPhysicalDeviceMemoryProperties2KHR* vkUtilsUnpack_VkPhysicalDeviceMemoryProperties2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSparseImageFormatProperties2KHR(const VkSparseImageFormatProperties2KHR* data);
void vkUtilsPack_VkSparseImageFormatProperties2KHR(android::base::InplaceStream* stream, const VkSparseImageFormatProperties2KHR* data);
VkSparseImageFormatProperties2KHR* vkUtilsUnpack_VkSparseImageFormatProperties2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceSparseImageFormatInfo2KHR(const VkPhysicalDeviceSparseImageFormatInfo2KHR* data);
void vkUtilsPack_VkPhysicalDeviceSparseImageFormatInfo2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceSparseImageFormatInfo2KHR* data);
VkPhysicalDeviceSparseImageFormatInfo2KHR* vkUtilsUnpack_VkPhysicalDeviceSparseImageFormatInfo2KHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryAllocateFlagsInfoKHX(const VkMemoryAllocateFlagsInfoKHX* data);
void vkUtilsPack_VkMemoryAllocateFlagsInfoKHX(android::base::InplaceStream* stream, const VkMemoryAllocateFlagsInfoKHX* data);
VkMemoryAllocateFlagsInfoKHX* vkUtilsUnpack_VkMemoryAllocateFlagsInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBindBufferMemoryInfoKHX(const VkBindBufferMemoryInfoKHX* data);
void vkUtilsPack_VkBindBufferMemoryInfoKHX(android::base::InplaceStream* stream, const VkBindBufferMemoryInfoKHX* data);
VkBindBufferMemoryInfoKHX* vkUtilsUnpack_VkBindBufferMemoryInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBindImageMemoryInfoKHX(const VkBindImageMemoryInfoKHX* data);
void vkUtilsPack_VkBindImageMemoryInfoKHX(android::base::InplaceStream* stream, const VkBindImageMemoryInfoKHX* data);
VkBindImageMemoryInfoKHX* vkUtilsUnpack_VkBindImageMemoryInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupRenderPassBeginInfoKHX(const VkDeviceGroupRenderPassBeginInfoKHX* data);
void vkUtilsPack_VkDeviceGroupRenderPassBeginInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupRenderPassBeginInfoKHX* data);
VkDeviceGroupRenderPassBeginInfoKHX* vkUtilsUnpack_VkDeviceGroupRenderPassBeginInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupCommandBufferBeginInfoKHX(const VkDeviceGroupCommandBufferBeginInfoKHX* data);
void vkUtilsPack_VkDeviceGroupCommandBufferBeginInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupCommandBufferBeginInfoKHX* data);
VkDeviceGroupCommandBufferBeginInfoKHX* vkUtilsUnpack_VkDeviceGroupCommandBufferBeginInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupSubmitInfoKHX(const VkDeviceGroupSubmitInfoKHX* data);
void vkUtilsPack_VkDeviceGroupSubmitInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupSubmitInfoKHX* data);
VkDeviceGroupSubmitInfoKHX* vkUtilsUnpack_VkDeviceGroupSubmitInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupBindSparseInfoKHX(const VkDeviceGroupBindSparseInfoKHX* data);
void vkUtilsPack_VkDeviceGroupBindSparseInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupBindSparseInfoKHX* data);
VkDeviceGroupBindSparseInfoKHX* vkUtilsUnpack_VkDeviceGroupBindSparseInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupPresentCapabilitiesKHX(const VkDeviceGroupPresentCapabilitiesKHX* data);
void vkUtilsPack_VkDeviceGroupPresentCapabilitiesKHX(android::base::InplaceStream* stream, const VkDeviceGroupPresentCapabilitiesKHX* data);
VkDeviceGroupPresentCapabilitiesKHX* vkUtilsUnpack_VkDeviceGroupPresentCapabilitiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImageSwapchainCreateInfoKHX(const VkImageSwapchainCreateInfoKHX* data);
void vkUtilsPack_VkImageSwapchainCreateInfoKHX(android::base::InplaceStream* stream, const VkImageSwapchainCreateInfoKHX* data);
VkImageSwapchainCreateInfoKHX* vkUtilsUnpack_VkImageSwapchainCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkBindImageMemorySwapchainInfoKHX(const VkBindImageMemorySwapchainInfoKHX* data);
void vkUtilsPack_VkBindImageMemorySwapchainInfoKHX(android::base::InplaceStream* stream, const VkBindImageMemorySwapchainInfoKHX* data);
VkBindImageMemorySwapchainInfoKHX* vkUtilsUnpack_VkBindImageMemorySwapchainInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkAcquireNextImageInfoKHX(const VkAcquireNextImageInfoKHX* data);
void vkUtilsPack_VkAcquireNextImageInfoKHX(android::base::InplaceStream* stream, const VkAcquireNextImageInfoKHX* data);
VkAcquireNextImageInfoKHX* vkUtilsUnpack_VkAcquireNextImageInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupPresentInfoKHX(const VkDeviceGroupPresentInfoKHX* data);
void vkUtilsPack_VkDeviceGroupPresentInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupPresentInfoKHX* data);
VkDeviceGroupPresentInfoKHX* vkUtilsUnpack_VkDeviceGroupPresentInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupSwapchainCreateInfoKHX(const VkDeviceGroupSwapchainCreateInfoKHX* data);
void vkUtilsPack_VkDeviceGroupSwapchainCreateInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupSwapchainCreateInfoKHX* data);
VkDeviceGroupSwapchainCreateInfoKHX* vkUtilsUnpack_VkDeviceGroupSwapchainCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkValidationFlagsEXT(const VkValidationFlagsEXT* data);
void vkUtilsPack_VkValidationFlagsEXT(android::base::InplaceStream* stream, const VkValidationFlagsEXT* data);
VkValidationFlagsEXT* vkUtilsUnpack_VkValidationFlagsEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceGroupPropertiesKHX(const VkPhysicalDeviceGroupPropertiesKHX* data);
void vkUtilsPack_VkPhysicalDeviceGroupPropertiesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceGroupPropertiesKHX* data);
VkPhysicalDeviceGroupPropertiesKHX* vkUtilsUnpack_VkPhysicalDeviceGroupPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceGroupDeviceCreateInfoKHX(const VkDeviceGroupDeviceCreateInfoKHX* data);
void vkUtilsPack_VkDeviceGroupDeviceCreateInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupDeviceCreateInfoKHX* data);
VkDeviceGroupDeviceCreateInfoKHX* vkUtilsUnpack_VkDeviceGroupDeviceCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalMemoryPropertiesKHX(const VkExternalMemoryPropertiesKHX* data);
void vkUtilsPack_VkExternalMemoryPropertiesKHX(android::base::InplaceStream* stream, const VkExternalMemoryPropertiesKHX* data);
VkExternalMemoryPropertiesKHX* vkUtilsUnpack_VkExternalMemoryPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceExternalImageFormatInfoKHX(const VkPhysicalDeviceExternalImageFormatInfoKHX* data);
void vkUtilsPack_VkPhysicalDeviceExternalImageFormatInfoKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceExternalImageFormatInfoKHX* data);
VkPhysicalDeviceExternalImageFormatInfoKHX* vkUtilsUnpack_VkPhysicalDeviceExternalImageFormatInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalImageFormatPropertiesKHX(const VkExternalImageFormatPropertiesKHX* data);
void vkUtilsPack_VkExternalImageFormatPropertiesKHX(android::base::InplaceStream* stream, const VkExternalImageFormatPropertiesKHX* data);
VkExternalImageFormatPropertiesKHX* vkUtilsUnpack_VkExternalImageFormatPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceExternalBufferInfoKHX(const VkPhysicalDeviceExternalBufferInfoKHX* data);
void vkUtilsPack_VkPhysicalDeviceExternalBufferInfoKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceExternalBufferInfoKHX* data);
VkPhysicalDeviceExternalBufferInfoKHX* vkUtilsUnpack_VkPhysicalDeviceExternalBufferInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalBufferPropertiesKHX(const VkExternalBufferPropertiesKHX* data);
void vkUtilsPack_VkExternalBufferPropertiesKHX(android::base::InplaceStream* stream, const VkExternalBufferPropertiesKHX* data);
VkExternalBufferPropertiesKHX* vkUtilsUnpack_VkExternalBufferPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceIDPropertiesKHX(const VkPhysicalDeviceIDPropertiesKHX* data);
void vkUtilsPack_VkPhysicalDeviceIDPropertiesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceIDPropertiesKHX* data);
VkPhysicalDeviceIDPropertiesKHX* vkUtilsUnpack_VkPhysicalDeviceIDPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalMemoryImageCreateInfoKHX(const VkExternalMemoryImageCreateInfoKHX* data);
void vkUtilsPack_VkExternalMemoryImageCreateInfoKHX(android::base::InplaceStream* stream, const VkExternalMemoryImageCreateInfoKHX* data);
VkExternalMemoryImageCreateInfoKHX* vkUtilsUnpack_VkExternalMemoryImageCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalMemoryBufferCreateInfoKHX(const VkExternalMemoryBufferCreateInfoKHX* data);
void vkUtilsPack_VkExternalMemoryBufferCreateInfoKHX(android::base::InplaceStream* stream, const VkExternalMemoryBufferCreateInfoKHX* data);
VkExternalMemoryBufferCreateInfoKHX* vkUtilsUnpack_VkExternalMemoryBufferCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExportMemoryAllocateInfoKHX(const VkExportMemoryAllocateInfoKHX* data);
void vkUtilsPack_VkExportMemoryAllocateInfoKHX(android::base::InplaceStream* stream, const VkExportMemoryAllocateInfoKHX* data);
VkExportMemoryAllocateInfoKHX* vkUtilsUnpack_VkExportMemoryAllocateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImportMemoryFdInfoKHX(const VkImportMemoryFdInfoKHX* data);
void vkUtilsPack_VkImportMemoryFdInfoKHX(android::base::InplaceStream* stream, const VkImportMemoryFdInfoKHX* data);
VkImportMemoryFdInfoKHX* vkUtilsUnpack_VkImportMemoryFdInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkMemoryFdPropertiesKHX(const VkMemoryFdPropertiesKHX* data);
void vkUtilsPack_VkMemoryFdPropertiesKHX(android::base::InplaceStream* stream, const VkMemoryFdPropertiesKHX* data);
VkMemoryFdPropertiesKHX* vkUtilsUnpack_VkMemoryFdPropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceExternalSemaphoreInfoKHX(const VkPhysicalDeviceExternalSemaphoreInfoKHX* data);
void vkUtilsPack_VkPhysicalDeviceExternalSemaphoreInfoKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceExternalSemaphoreInfoKHX* data);
VkPhysicalDeviceExternalSemaphoreInfoKHX* vkUtilsUnpack_VkPhysicalDeviceExternalSemaphoreInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExternalSemaphorePropertiesKHX(const VkExternalSemaphorePropertiesKHX* data);
void vkUtilsPack_VkExternalSemaphorePropertiesKHX(android::base::InplaceStream* stream, const VkExternalSemaphorePropertiesKHX* data);
VkExternalSemaphorePropertiesKHX* vkUtilsUnpack_VkExternalSemaphorePropertiesKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkExportSemaphoreCreateInfoKHX(const VkExportSemaphoreCreateInfoKHX* data);
void vkUtilsPack_VkExportSemaphoreCreateInfoKHX(android::base::InplaceStream* stream, const VkExportSemaphoreCreateInfoKHX* data);
VkExportSemaphoreCreateInfoKHX* vkUtilsUnpack_VkExportSemaphoreCreateInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkImportSemaphoreFdInfoKHX(const VkImportSemaphoreFdInfoKHX* data);
void vkUtilsPack_VkImportSemaphoreFdInfoKHX(android::base::InplaceStream* stream, const VkImportSemaphoreFdInfoKHX* data);
VkImportSemaphoreFdInfoKHX* vkUtilsUnpack_VkImportSemaphoreFdInfoKHX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDevicePushDescriptorPropertiesKHR(const VkPhysicalDevicePushDescriptorPropertiesKHR* data);
void vkUtilsPack_VkPhysicalDevicePushDescriptorPropertiesKHR(android::base::InplaceStream* stream, const VkPhysicalDevicePushDescriptorPropertiesKHR* data);
VkPhysicalDevicePushDescriptorPropertiesKHR* vkUtilsUnpack_VkPhysicalDevicePushDescriptorPropertiesKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkRectLayerKHR(const VkRectLayerKHR* data);
void vkUtilsPack_VkRectLayerKHR(android::base::InplaceStream* stream, const VkRectLayerKHR* data);
VkRectLayerKHR* vkUtilsUnpack_VkRectLayerKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPresentRegionKHR(const VkPresentRegionKHR* data);
void vkUtilsPack_VkPresentRegionKHR(android::base::InplaceStream* stream, const VkPresentRegionKHR* data);
VkPresentRegionKHR* vkUtilsUnpack_VkPresentRegionKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPresentRegionsKHR(const VkPresentRegionsKHR* data);
void vkUtilsPack_VkPresentRegionsKHR(android::base::InplaceStream* stream, const VkPresentRegionsKHR* data);
VkPresentRegionsKHR* vkUtilsUnpack_VkPresentRegionsKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorUpdateTemplateEntryKHR(const VkDescriptorUpdateTemplateEntryKHR* data);
void vkUtilsPack_VkDescriptorUpdateTemplateEntryKHR(android::base::InplaceStream* stream, const VkDescriptorUpdateTemplateEntryKHR* data);
VkDescriptorUpdateTemplateEntryKHR* vkUtilsUnpack_VkDescriptorUpdateTemplateEntryKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDescriptorUpdateTemplateCreateInfoKHR(const VkDescriptorUpdateTemplateCreateInfoKHR* data);
void vkUtilsPack_VkDescriptorUpdateTemplateCreateInfoKHR(android::base::InplaceStream* stream, const VkDescriptorUpdateTemplateCreateInfoKHR* data);
VkDescriptorUpdateTemplateCreateInfoKHR* vkUtilsUnpack_VkDescriptorUpdateTemplateCreateInfoKHR(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSurfaceCapabilities2EXT(const VkSurfaceCapabilities2EXT* data);
void vkUtilsPack_VkSurfaceCapabilities2EXT(android::base::InplaceStream* stream, const VkSurfaceCapabilities2EXT* data);
VkSurfaceCapabilities2EXT* vkUtilsUnpack_VkSurfaceCapabilities2EXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayPowerInfoEXT(const VkDisplayPowerInfoEXT* data);
void vkUtilsPack_VkDisplayPowerInfoEXT(android::base::InplaceStream* stream, const VkDisplayPowerInfoEXT* data);
VkDisplayPowerInfoEXT* vkUtilsUnpack_VkDisplayPowerInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDeviceEventInfoEXT(const VkDeviceEventInfoEXT* data);
void vkUtilsPack_VkDeviceEventInfoEXT(android::base::InplaceStream* stream, const VkDeviceEventInfoEXT* data);
VkDeviceEventInfoEXT* vkUtilsUnpack_VkDeviceEventInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkDisplayEventInfoEXT(const VkDisplayEventInfoEXT* data);
void vkUtilsPack_VkDisplayEventInfoEXT(android::base::InplaceStream* stream, const VkDisplayEventInfoEXT* data);
VkDisplayEventInfoEXT* vkUtilsUnpack_VkDisplayEventInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkSwapchainCounterCreateInfoEXT(const VkSwapchainCounterCreateInfoEXT* data);
void vkUtilsPack_VkSwapchainCounterCreateInfoEXT(android::base::InplaceStream* stream, const VkSwapchainCounterCreateInfoEXT* data);
VkSwapchainCounterCreateInfoEXT* vkUtilsUnpack_VkSwapchainCounterCreateInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkRefreshCycleDurationGOOGLE(const VkRefreshCycleDurationGOOGLE* data);
void vkUtilsPack_VkRefreshCycleDurationGOOGLE(android::base::InplaceStream* stream, const VkRefreshCycleDurationGOOGLE* data);
VkRefreshCycleDurationGOOGLE* vkUtilsUnpack_VkRefreshCycleDurationGOOGLE(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPastPresentationTimingGOOGLE(const VkPastPresentationTimingGOOGLE* data);
void vkUtilsPack_VkPastPresentationTimingGOOGLE(android::base::InplaceStream* stream, const VkPastPresentationTimingGOOGLE* data);
VkPastPresentationTimingGOOGLE* vkUtilsUnpack_VkPastPresentationTimingGOOGLE(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPresentTimeGOOGLE(const VkPresentTimeGOOGLE* data);
void vkUtilsPack_VkPresentTimeGOOGLE(android::base::InplaceStream* stream, const VkPresentTimeGOOGLE* data);
VkPresentTimeGOOGLE* vkUtilsUnpack_VkPresentTimeGOOGLE(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPresentTimesInfoGOOGLE(const VkPresentTimesInfoGOOGLE* data);
void vkUtilsPack_VkPresentTimesInfoGOOGLE(android::base::InplaceStream* stream, const VkPresentTimesInfoGOOGLE* data);
VkPresentTimesInfoGOOGLE* vkUtilsUnpack_VkPresentTimesInfoGOOGLE(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX(const VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* data);
void vkUtilsPack_VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX(android::base::InplaceStream* stream, const VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* data);
VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* vkUtilsUnpack_VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceDiscardRectanglePropertiesEXT(const VkPhysicalDeviceDiscardRectanglePropertiesEXT* data);
void vkUtilsPack_VkPhysicalDeviceDiscardRectanglePropertiesEXT(android::base::InplaceStream* stream, const VkPhysicalDeviceDiscardRectanglePropertiesEXT* data);
VkPhysicalDeviceDiscardRectanglePropertiesEXT* vkUtilsUnpack_VkPhysicalDeviceDiscardRectanglePropertiesEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkPipelineDiscardRectangleStateCreateInfoEXT(const VkPipelineDiscardRectangleStateCreateInfoEXT* data);
void vkUtilsPack_VkPipelineDiscardRectangleStateCreateInfoEXT(android::base::InplaceStream* stream, const VkPipelineDiscardRectangleStateCreateInfoEXT* data);
VkPipelineDiscardRectangleStateCreateInfoEXT* vkUtilsUnpack_VkPipelineDiscardRectangleStateCreateInfoEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkXYColorEXT(const VkXYColorEXT* data);
void vkUtilsPack_VkXYColorEXT(android::base::InplaceStream* stream, const VkXYColorEXT* data);
VkXYColorEXT* vkUtilsUnpack_VkXYColorEXT(android::base::InplaceStream* stream);

uint32_t vkUtilsEncodingSize_VkHdrMetadataEXT(const VkHdrMetadataEXT* data);
void vkUtilsPack_VkHdrMetadataEXT(android::base::InplaceStream* stream, const VkHdrMetadataEXT* data);
VkHdrMetadataEXT* vkUtilsUnpack_VkHdrMetadataEXT(android::base::InplaceStream* stream);

