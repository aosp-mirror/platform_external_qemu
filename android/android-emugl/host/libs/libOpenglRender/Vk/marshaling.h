
#include "handle_reassignment.h"

#include <vulkan/vulkan.h>

#include <inttypes.h>
#include <string.h>

void registerReassignmentFuncs(vk_reassign_funcs* funcs);



size_t allocSz_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in);
VkMappedMemoryRange* initFromBuffer_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, unsigned char* buf);
void deepCopyAssign_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out);
void deepCopyReassign_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out);
void deepCopyWithRemap_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out);
VkMappedMemoryRange* remap_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, unsigned char* buf);


size_t allocSz_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in);
VkSubmitInfo* initFromBuffer_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, unsigned char* buf);
void deepCopyAssign_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out);
void deepCopyReassign_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out);
void deepCopyWithRemap_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out);
VkSubmitInfo* remap_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, unsigned char* buf);


size_t allocSz_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in);
VkSparseMemoryBind* initFromBuffer_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, unsigned char* buf);
void deepCopyAssign_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out);
void deepCopyReassign_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out);
void deepCopyWithRemap_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out);
VkSparseMemoryBind* remap_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, unsigned char* buf);


size_t allocSz_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in);
VkSparseBufferMemoryBindInfo* initFromBuffer_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, unsigned char* buf);
void deepCopyAssign_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out);
void deepCopyReassign_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out);
void deepCopyWithRemap_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out);
VkSparseBufferMemoryBindInfo* remap_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, unsigned char* buf);


size_t allocSz_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in);
VkSparseImageOpaqueMemoryBindInfo* initFromBuffer_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, unsigned char* buf);
void deepCopyAssign_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out);
void deepCopyReassign_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out);
void deepCopyWithRemap_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out);
VkSparseImageOpaqueMemoryBindInfo* remap_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, unsigned char* buf);


size_t allocSz_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in);
VkSparseImageMemoryBindInfo* initFromBuffer_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, unsigned char* buf);
void deepCopyAssign_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out);
void deepCopyReassign_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out);
void deepCopyWithRemap_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out);
VkSparseImageMemoryBindInfo* remap_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, unsigned char* buf);


size_t allocSz_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in);
VkBindSparseInfo* initFromBuffer_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, unsigned char* buf);
void deepCopyAssign_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out);
void deepCopyReassign_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out);
void deepCopyWithRemap_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out);
VkBindSparseInfo* remap_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, unsigned char* buf);


size_t allocSz_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in);
VkBufferViewCreateInfo* initFromBuffer_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out);
void deepCopyReassign_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out);
void deepCopyWithRemap_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out);
VkBufferViewCreateInfo* remap_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, unsigned char* buf);


size_t allocSz_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in);
VkImageViewCreateInfo* initFromBuffer_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out);
void deepCopyReassign_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out);
void deepCopyWithRemap_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out);
VkImageViewCreateInfo* remap_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, unsigned char* buf);


size_t allocSz_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in);
VkPipelineShaderStageCreateInfo* initFromBuffer_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out);
void deepCopyReassign_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out);
void deepCopyWithRemap_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out);
VkPipelineShaderStageCreateInfo* remap_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, unsigned char* buf);


size_t allocSz_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in);
VkGraphicsPipelineCreateInfo* initFromBuffer_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out);
void deepCopyReassign_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out);
void deepCopyWithRemap_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out);
VkGraphicsPipelineCreateInfo* remap_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, unsigned char* buf);


size_t allocSz_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in);
VkComputePipelineCreateInfo* initFromBuffer_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out);
void deepCopyReassign_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out);
void deepCopyWithRemap_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out);
VkComputePipelineCreateInfo* remap_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, unsigned char* buf);


size_t allocSz_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in);
VkPipelineLayoutCreateInfo* initFromBuffer_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out);
void deepCopyReassign_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out);
void deepCopyWithRemap_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out);
VkPipelineLayoutCreateInfo* remap_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, unsigned char* buf);


size_t allocSz_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in);
VkDescriptorSetLayoutBinding* initFromBuffer_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out);
void deepCopyReassign_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out);
void deepCopyWithRemap_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out);
VkDescriptorSetLayoutBinding* remap_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, unsigned char* buf);


size_t allocSz_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in);
VkDescriptorSetLayoutCreateInfo* initFromBuffer_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out);
void deepCopyReassign_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out);
void deepCopyWithRemap_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out);
VkDescriptorSetLayoutCreateInfo* remap_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, unsigned char* buf);


size_t allocSz_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in);
VkDescriptorSetAllocateInfo* initFromBuffer_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out);
void deepCopyReassign_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out);
void deepCopyWithRemap_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out);
VkDescriptorSetAllocateInfo* remap_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, unsigned char* buf);


size_t allocSz_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in);
VkDescriptorImageInfo* initFromBuffer_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out);
void deepCopyReassign_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out);
void deepCopyWithRemap_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out);
VkDescriptorImageInfo* remap_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, unsigned char* buf);


size_t allocSz_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in);
VkDescriptorBufferInfo* initFromBuffer_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out);
void deepCopyReassign_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out);
void deepCopyWithRemap_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out);
VkDescriptorBufferInfo* remap_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, unsigned char* buf);


size_t allocSz_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in);
VkWriteDescriptorSet* initFromBuffer_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, unsigned char* buf);
void deepCopyAssign_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out);
void deepCopyReassign_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out);
void deepCopyWithRemap_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out);
VkWriteDescriptorSet* remap_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, unsigned char* buf);


size_t allocSz_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in);
VkCopyDescriptorSet* initFromBuffer_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, unsigned char* buf);
void deepCopyAssign_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out);
void deepCopyReassign_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out);
void deepCopyWithRemap_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out);
VkCopyDescriptorSet* remap_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, unsigned char* buf);


size_t allocSz_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in);
VkFramebufferCreateInfo* initFromBuffer_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out);
void deepCopyReassign_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out);
void deepCopyWithRemap_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out);
VkFramebufferCreateInfo* remap_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, unsigned char* buf);


size_t allocSz_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in);
VkCommandBufferInheritanceInfo* initFromBuffer_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, unsigned char* buf);
void deepCopyAssign_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out);
void deepCopyReassign_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out);
void deepCopyWithRemap_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out);
VkCommandBufferInheritanceInfo* remap_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, unsigned char* buf);


size_t allocSz_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in);
VkCommandBufferAllocateInfo* initFromBuffer_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, unsigned char* buf);
void deepCopyAssign_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out);
void deepCopyReassign_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out);
void deepCopyWithRemap_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out);
VkCommandBufferAllocateInfo* remap_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, unsigned char* buf);


size_t allocSz_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in);
VkCommandBufferBeginInfo* initFromBuffer_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, unsigned char* buf);
void deepCopyAssign_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out);
void deepCopyReassign_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out);
void deepCopyWithRemap_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out);
VkCommandBufferBeginInfo* remap_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, unsigned char* buf);


size_t allocSz_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in);
VkBufferMemoryBarrier* initFromBuffer_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, unsigned char* buf);
void deepCopyAssign_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out);
void deepCopyReassign_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out);
void deepCopyWithRemap_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out);
VkBufferMemoryBarrier* remap_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, unsigned char* buf);


size_t allocSz_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in);
VkImageMemoryBarrier* initFromBuffer_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, unsigned char* buf);
void deepCopyAssign_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out);
void deepCopyReassign_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out);
void deepCopyWithRemap_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out);
VkImageMemoryBarrier* remap_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, unsigned char* buf);


size_t allocSz_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in);
VkRenderPassBeginInfo* initFromBuffer_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, unsigned char* buf);
void deepCopyAssign_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out);
void deepCopyReassign_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out);
void deepCopyWithRemap_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out);
VkRenderPassBeginInfo* remap_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, unsigned char* buf);

