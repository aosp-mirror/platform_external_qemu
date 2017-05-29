
#include "handle_reassignment.h"

#include <vulkan/vulkan.h>

#include <inttypes.h>
#include <string.h>

void registerReassignmentFuncs(vk_reassign_funcs* funcs);



size_t tmpAllocSz_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in);
VkMappedMemoryRange* initFromBuffer_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, unsigned char* buf);
void deepCopyAssign_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out);
void deepCopyReassign_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out);


size_t tmpAllocSz_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in);
VkSubmitInfo* initFromBuffer_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, unsigned char* buf);
void deepCopyAssign_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out);
void deepCopyReassign_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out);


size_t tmpAllocSz_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in);
VkSparseMemoryBind* initFromBuffer_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, unsigned char* buf);
void deepCopyAssign_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out);
void deepCopyReassign_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out);


size_t tmpAllocSz_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in);
VkSparseBufferMemoryBindInfo* initFromBuffer_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, unsigned char* buf);
void deepCopyAssign_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out);
void deepCopyReassign_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out);


size_t tmpAllocSz_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in);
VkSparseImageOpaqueMemoryBindInfo* initFromBuffer_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, unsigned char* buf);
void deepCopyAssign_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out);
void deepCopyReassign_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out);


size_t tmpAllocSz_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in);
VkSparseImageMemoryBindInfo* initFromBuffer_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, unsigned char* buf);
void deepCopyAssign_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out);
void deepCopyReassign_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out);


size_t tmpAllocSz_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in);
VkBindSparseInfo* initFromBuffer_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, unsigned char* buf);
void deepCopyAssign_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out);
void deepCopyReassign_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out);


size_t tmpAllocSz_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in);
VkBufferViewCreateInfo* initFromBuffer_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out);
void deepCopyReassign_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out);


size_t tmpAllocSz_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in);
VkImageViewCreateInfo* initFromBuffer_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out);
void deepCopyReassign_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out);


size_t tmpAllocSz_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in);
VkPipelineShaderStageCreateInfo* initFromBuffer_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out);
void deepCopyReassign_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out);


size_t tmpAllocSz_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in);
VkGraphicsPipelineCreateInfo* initFromBuffer_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out);
void deepCopyReassign_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out);


size_t tmpAllocSz_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in);
VkComputePipelineCreateInfo* initFromBuffer_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out);
void deepCopyReassign_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out);


size_t tmpAllocSz_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in);
VkPipelineLayoutCreateInfo* initFromBuffer_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out);
void deepCopyReassign_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out);


size_t tmpAllocSz_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in);
VkDescriptorSetLayoutBinding* initFromBuffer_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out);
void deepCopyReassign_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out);


size_t tmpAllocSz_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in);
VkDescriptorSetLayoutCreateInfo* initFromBuffer_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out);
void deepCopyReassign_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out);


size_t tmpAllocSz_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in);
VkDescriptorSetAllocateInfo* initFromBuffer_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out);
void deepCopyReassign_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out);


size_t tmpAllocSz_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in);
VkDescriptorImageInfo* initFromBuffer_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out);
void deepCopyReassign_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out);


size_t tmpAllocSz_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in);
VkDescriptorBufferInfo* initFromBuffer_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, unsigned char* buf);
void deepCopyAssign_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out);
void deepCopyReassign_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out);


size_t tmpAllocSz_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in);
VkWriteDescriptorSet* initFromBuffer_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, unsigned char* buf);
void deepCopyAssign_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out);
void deepCopyReassign_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out);


size_t tmpAllocSz_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in);
VkCopyDescriptorSet* initFromBuffer_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, unsigned char* buf);
void deepCopyAssign_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out);
void deepCopyReassign_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out);


size_t tmpAllocSz_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in);
VkFramebufferCreateInfo* initFromBuffer_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, unsigned char* buf);
void deepCopyAssign_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out);
void deepCopyReassign_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out);


size_t tmpAllocSz_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in);
VkCommandBufferInheritanceInfo* initFromBuffer_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, unsigned char* buf);
void deepCopyAssign_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out);
void deepCopyReassign_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out);


size_t tmpAllocSz_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in);
VkCommandBufferAllocateInfo* initFromBuffer_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, unsigned char* buf);
void deepCopyAssign_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out);
void deepCopyReassign_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out);


size_t tmpAllocSz_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in);
VkCommandBufferBeginInfo* initFromBuffer_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, unsigned char* buf);
void deepCopyAssign_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out);
void deepCopyReassign_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out);


size_t tmpAllocSz_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in);
VkBufferMemoryBarrier* initFromBuffer_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, unsigned char* buf);
void deepCopyAssign_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out);
void deepCopyReassign_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out);


size_t tmpAllocSz_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in);
VkImageMemoryBarrier* initFromBuffer_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, unsigned char* buf);
void deepCopyAssign_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out);
void deepCopyReassign_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out);


size_t tmpAllocSz_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in);
VkRenderPassBeginInfo* initFromBuffer_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, unsigned char* buf);
void deepCopyAssign_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out);
void deepCopyReassign_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out);

