#pragma once

#include <vulkan/vulkan.h>
#include <inttypes.h>

#include "InplaceStream.h" 

uint32_t string1d_len(const char* string);
void string1d_pack(android::base::InplaceStream* stream, const char* string);
const char* string1d_unpack(android::base::InplaceStream* stream);

uint32_t strings2d_len(uint32_t count, const char* const* strings);
void strings2d_pack(android::base::InplaceStream* stream, uint32_t count, const char* const* strings);
const char* const* strings2d_unpack(android::base::InplaceStream* stream, uint32_t count);

void vkUtilsPack_VkApplicationInfo(android::base::InplaceStream* stream, const VkApplicationInfo* data);
void vkUtilsPack_VkInstanceCreateInfo(android::base::InplaceStream* stream, const VkInstanceCreateInfo* data);
VkApplicationInfo* vkUtilsUnpack_VkApplicationInfo(android::base::InplaceStream* stream);
VkInstanceCreateInfo* vkUtilsUnpack_VkInstanceCreateInfo(android::base::InplaceStream* stream);

uint32_t customCount_VkWriteDescriptorSet_pBufferInfo(const VkWriteDescriptorSet* data);
uint32_t customCount_VkWriteDescriptorSet_pImageInfo(const VkWriteDescriptorSet* data);
uint32_t customCount_VkWriteDescriptorSet_pTexelBufferView(const VkWriteDescriptorSet* data);

uint32_t customCount_VkCommandBufferBeginInfo_pInheritanceInfo(const VkCommandBufferBeginInfo* data);
