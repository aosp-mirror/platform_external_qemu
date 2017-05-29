#include "vkUtils_custom.h"

#include <string.h>

using android::base::InplaceStream;

uint32_t string1d_len(const char* string) {
    if (!string) return 4 + 1;
    return 4 + strlen(string) + 1;
}

void string1d_pack(InplaceStream* stream, const char* string) {
    stream->putStringNullTerminated(string);
}

const char* string1d_unpack(InplaceStream* stream) {
    const char* str = stream->getStringNullTerminated();
    uint32_t len = strlen(str) + 1;
    char* res = new char[len];
    memcpy(res, str, len);
    return res;
}

uint32_t strings2d_len(uint32_t count, const char* const* strings) {
    uint32_t res = 0;
    for (uint32_t i = 0; i < count; i++) {
        res += string1d_len(strings[i]);
    }
    return res;
}

void strings2d_pack(InplaceStream* stream, uint32_t count, const char* const* strings) {
    for (uint32_t i = 0; i < count; i++) {
        string1d_pack(stream, strings[i]);
    }
}

const char* const* strings2d_unpack(InplaceStream* stream, uint32_t count) {
    char** res = new char*[count];
    for (uint32_t i = 0; i < count; i++) {
        res[i] = (char*)string1d_unpack(stream);
    }
    return res;
}

// void vkUtilsPack_VkApplicationInfo(InplaceStream* stream, const VkApplicationInfo* data) {
//     if (!data) return;
//     stream->write(&data->sType, 4); // sType
//     stream->write(&data->pNext, 8); // pNext
//     stream->putStringNullTerminated(data->pApplicationName);
//     stream->write(&data->applicationVersion, 4); // applicationVersion
//     stream->putStringNullTerminated(data->pEngineName);
//     stream->write(&data->engineVersion, 4); // engineVersion
//     stream->write(&data->apiVersion, 4); // apiVersion
// }
// 
// void vkUtilsPack_VkInstanceCreateInfo(InplaceStream* stream, const VkInstanceCreateInfo* data) {
//     if (!data) return;
//     stream->write(&data->sType, 4);
//     stream->write(&data->pNext, 8);
//     stream->write(&data->flags, 4);
//     vkUtilsPack_VkApplicationInfo(stream, data->pApplicationInfo);
//     stream->write(&data->enabledLayerCount, 4);
//     strings2d_pack(stream, data->enabledLayerCount, data->ppEnabledLayerNames);
// }
// 
// VkApplicationInfo* vkUtilsUnpack_VkApplicationInfo(InplaceStream* stream) {
//     VkApplicationInfo* res = new VkApplicationInfo;
//     stream->read(&res->sType, 4);
//     stream->read(&res->pNext, 8);
//     res->pApplicationName = string1d_unpack(stream);
//     stream->read(&res->applicationVersion, 4);
//     res->pEngineName = string1d_unpack(stream);
//     stream->read(&res->engineVersion, 4);
//     stream->read(&res->apiVersion, 4);
//     return res;
// }
// 
// VkInstanceCreateInfo* vkUtilsUnpack_VkInstanceCreateInfo(InplaceStream* stream) {
//     VkInstanceCreateInfo* res = new VkInstanceCreateInfo;
//     stream->read(&res->sType, 4);
//     stream->read(&res->pNext, 8);
//     stream->read(&res->flags, 4);
//     VkApplicationInfo* appInfo =
//         vkUtilsUnpack_VkApplicationInfo(stream);
//     res->pApplicationInfo = appInfo;
//     stream->read(&res->enabledLayerCount, 4);
//     res->ppEnabledLayerNames = strings2d_unpack(stream, res->enabledLayerCount);
//     return res;
// }

uint32_t customCount_VkWriteDescriptorSet_pBufferInfo(const VkWriteDescriptorSet* data) {
    if (data->pBufferInfo) return data->descriptorCount;
    return 0;
}
uint32_t customCount_VkWriteDescriptorSet_pImageInfo(const VkWriteDescriptorSet* data) {
    if (data->pImageInfo) return data->descriptorCount;
    return 0;
}
uint32_t customCount_VkWriteDescriptorSet_pTexelBufferView(const VkWriteDescriptorSet* data) {
    if (data->pTexelBufferView) return data->descriptorCount;
    return 0;
}

uint32_t customCount_VkCommandBufferBeginInfo_pInheritanceInfo(const VkCommandBufferBeginInfo* data) {
    return 0;
}
