#include "vkUtils.h"
#include "vkUtils_custom.h"
#include "vk_types.h"
#include <string.h>

uint32_t vkUtilsEncodingSize_VkOffset2D(const VkOffset2D* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // s32 x
    res += 1 * 4; // s32 y
    return res;
}

void vkUtilsPack_VkOffset2D(android::base::InplaceStream* stream, const VkOffset2D* data) {
    if (!data) return;
    stream->write(&data->x, 1 * 4); // s32 x
    stream->write(&data->y, 1 * 4); // s32 y
}

VkOffset2D* vkUtilsUnpack_VkOffset2D(android::base::InplaceStream* stream) {
    VkOffset2D* data = new VkOffset2D; memset(data, 0, sizeof(VkOffset2D)); 
    stream->read(&data->x, 1 * 4); // s32 x
    stream->read(&data->y, 1 * 4); // s32 y
    return data;
}

uint32_t vkUtilsEncodingSize_VkOffset3D(const VkOffset3D* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // s32 x
    res += 1 * 4; // s32 y
    res += 1 * 4; // s32 z
    return res;
}

void vkUtilsPack_VkOffset3D(android::base::InplaceStream* stream, const VkOffset3D* data) {
    if (!data) return;
    stream->write(&data->x, 1 * 4); // s32 x
    stream->write(&data->y, 1 * 4); // s32 y
    stream->write(&data->z, 1 * 4); // s32 z
}

VkOffset3D* vkUtilsUnpack_VkOffset3D(android::base::InplaceStream* stream) {
    VkOffset3D* data = new VkOffset3D; memset(data, 0, sizeof(VkOffset3D)); 
    stream->read(&data->x, 1 * 4); // s32 x
    stream->read(&data->y, 1 * 4); // s32 y
    stream->read(&data->z, 1 * 4); // s32 z
    return data;
}

uint32_t vkUtilsEncodingSize_VkExtent2D(const VkExtent2D* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 width
    res += 1 * 4; // u32 height
    return res;
}

void vkUtilsPack_VkExtent2D(android::base::InplaceStream* stream, const VkExtent2D* data) {
    if (!data) return;
    stream->write(&data->width, 1 * 4); // u32 width
    stream->write(&data->height, 1 * 4); // u32 height
}

VkExtent2D* vkUtilsUnpack_VkExtent2D(android::base::InplaceStream* stream) {
    VkExtent2D* data = new VkExtent2D; memset(data, 0, sizeof(VkExtent2D)); 
    stream->read(&data->width, 1 * 4); // u32 width
    stream->read(&data->height, 1 * 4); // u32 height
    return data;
}

uint32_t vkUtilsEncodingSize_VkExtent3D(const VkExtent3D* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 width
    res += 1 * 4; // u32 height
    res += 1 * 4; // u32 depth
    return res;
}

void vkUtilsPack_VkExtent3D(android::base::InplaceStream* stream, const VkExtent3D* data) {
    if (!data) return;
    stream->write(&data->width, 1 * 4); // u32 width
    stream->write(&data->height, 1 * 4); // u32 height
    stream->write(&data->depth, 1 * 4); // u32 depth
}

VkExtent3D* vkUtilsUnpack_VkExtent3D(android::base::InplaceStream* stream) {
    VkExtent3D* data = new VkExtent3D; memset(data, 0, sizeof(VkExtent3D)); 
    stream->read(&data->width, 1 * 4); // u32 width
    stream->read(&data->height, 1 * 4); // u32 height
    stream->read(&data->depth, 1 * 4); // u32 depth
    return data;
}

uint32_t vkUtilsEncodingSize_VkViewport(const VkViewport* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // f32 x
    res += 1 * 4; // f32 y
    res += 1 * 4; // f32 width
    res += 1 * 4; // f32 height
    res += 1 * 4; // f32 minDepth
    res += 1 * 4; // f32 maxDepth
    return res;
}

void vkUtilsPack_VkViewport(android::base::InplaceStream* stream, const VkViewport* data) {
    if (!data) return;
    stream->write(&data->x, 1 * 4); // f32 x
    stream->write(&data->y, 1 * 4); // f32 y
    stream->write(&data->width, 1 * 4); // f32 width
    stream->write(&data->height, 1 * 4); // f32 height
    stream->write(&data->minDepth, 1 * 4); // f32 minDepth
    stream->write(&data->maxDepth, 1 * 4); // f32 maxDepth
}

VkViewport* vkUtilsUnpack_VkViewport(android::base::InplaceStream* stream) {
    VkViewport* data = new VkViewport; memset(data, 0, sizeof(VkViewport)); 
    stream->read(&data->x, 1 * 4); // f32 x
    stream->read(&data->y, 1 * 4); // f32 y
    stream->read(&data->width, 1 * 4); // f32 width
    stream->read(&data->height, 1 * 4); // f32 height
    stream->read(&data->minDepth, 1 * 4); // f32 minDepth
    stream->read(&data->maxDepth, 1 * 4); // f32 maxDepth
    return data;
}

uint32_t vkUtilsEncodingSize_VkRect2D(const VkRect2D* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkOffset2D(&data->offset); // VkOffset2D offset
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->extent); // VkExtent2D extent
    return res;
}

void vkUtilsPack_VkRect2D(android::base::InplaceStream* stream, const VkRect2D* data) {
    if (!data) return;
    vkUtilsPack_VkOffset2D(stream, &data->offset); // VkOffset2D offset
    vkUtilsPack_VkExtent2D(stream, &data->extent); // VkExtent2D extent
}

VkRect2D* vkUtilsUnpack_VkRect2D(android::base::InplaceStream* stream) {
    VkRect2D* data = new VkRect2D; memset(data, 0, sizeof(VkRect2D)); 
    { VkOffset2D* tmpUnpacked = vkUtilsUnpack_VkOffset2D(stream); memcpy(&data->offset, tmpUnpacked, sizeof(VkOffset2D)); delete tmpUnpacked; } // VkOffset2D offset
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->extent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D extent
    return data;
}

uint32_t vkUtilsEncodingSize_VkClearRect(const VkClearRect* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkRect2D(&data->rect); // VkRect2D rect
    res += 1 * 4; // u32 baseArrayLayer
    res += 1 * 4; // u32 layerCount
    return res;
}

void vkUtilsPack_VkClearRect(android::base::InplaceStream* stream, const VkClearRect* data) {
    if (!data) return;
    vkUtilsPack_VkRect2D(stream, &data->rect); // VkRect2D rect
    stream->write(&data->baseArrayLayer, 1 * 4); // u32 baseArrayLayer
    stream->write(&data->layerCount, 1 * 4); // u32 layerCount
}

VkClearRect* vkUtilsUnpack_VkClearRect(android::base::InplaceStream* stream) {
    VkClearRect* data = new VkClearRect; memset(data, 0, sizeof(VkClearRect)); 
    { VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&data->rect, tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } // VkRect2D rect
    stream->read(&data->baseArrayLayer, 1 * 4); // u32 baseArrayLayer
    stream->read(&data->layerCount, 1 * 4); // u32 layerCount
    return data;
}

uint32_t vkUtilsEncodingSize_VkComponentMapping(const VkComponentMapping* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkComponentSwizzle r
    res += 1 * 4; // VkComponentSwizzle g
    res += 1 * 4; // VkComponentSwizzle b
    res += 1 * 4; // VkComponentSwizzle a
    return res;
}

void vkUtilsPack_VkComponentMapping(android::base::InplaceStream* stream, const VkComponentMapping* data) {
    if (!data) return;
    stream->write(&data->r, 1 * 4); // VkComponentSwizzle r
    stream->write(&data->g, 1 * 4); // VkComponentSwizzle g
    stream->write(&data->b, 1 * 4); // VkComponentSwizzle b
    stream->write(&data->a, 1 * 4); // VkComponentSwizzle a
}

VkComponentMapping* vkUtilsUnpack_VkComponentMapping(android::base::InplaceStream* stream) {
    VkComponentMapping* data = new VkComponentMapping; memset(data, 0, sizeof(VkComponentMapping)); 
    stream->read(&data->r, 1 * 4); // VkComponentSwizzle r
    stream->read(&data->g, 1 * 4); // VkComponentSwizzle g
    stream->read(&data->b, 1 * 4); // VkComponentSwizzle b
    stream->read(&data->a, 1 * 4); // VkComponentSwizzle a
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceProperties(const VkPhysicalDeviceProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 apiVersion
    res += 1 * 4; // u32 driverVersion
    res += 1 * 4; // u32 vendorID
    res += 1 * 4; // u32 deviceID
    res += 1 * 4; // VkPhysicalDeviceType deviceType
    res += VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1; // char deviceName
    res += VK_UUID_SIZE * 1; // u8 pipelineCacheUUID
    res += 1 * vkUtilsEncodingSize_VkPhysicalDeviceLimits(&data->limits); // VkPhysicalDeviceLimits limits
    res += 1 * vkUtilsEncodingSize_VkPhysicalDeviceSparseProperties(&data->sparseProperties); // VkPhysicalDeviceSparseProperties sparseProperties
    return res;
}

void vkUtilsPack_VkPhysicalDeviceProperties(android::base::InplaceStream* stream, const VkPhysicalDeviceProperties* data) {
    if (!data) return;
    stream->write(&data->apiVersion, 1 * 4); // u32 apiVersion
    stream->write(&data->driverVersion, 1 * 4); // u32 driverVersion
    stream->write(&data->vendorID, 1 * 4); // u32 vendorID
    stream->write(&data->deviceID, 1 * 4); // u32 deviceID
    stream->write(&data->deviceType, 1 * 4); // VkPhysicalDeviceType deviceType
    stream->write(&data->deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char deviceName
    stream->write(&data->pipelineCacheUUID, VK_UUID_SIZE * 1); // u8 pipelineCacheUUID
    vkUtilsPack_VkPhysicalDeviceLimits(stream, &data->limits); // VkPhysicalDeviceLimits limits
    vkUtilsPack_VkPhysicalDeviceSparseProperties(stream, &data->sparseProperties); // VkPhysicalDeviceSparseProperties sparseProperties
}

VkPhysicalDeviceProperties* vkUtilsUnpack_VkPhysicalDeviceProperties(android::base::InplaceStream* stream) {
    VkPhysicalDeviceProperties* data = new VkPhysicalDeviceProperties; memset(data, 0, sizeof(VkPhysicalDeviceProperties)); 
    stream->read(&data->apiVersion, 1 * 4); // u32 apiVersion
    stream->read(&data->driverVersion, 1 * 4); // u32 driverVersion
    stream->read(&data->vendorID, 1 * 4); // u32 vendorID
    stream->read(&data->deviceID, 1 * 4); // u32 deviceID
    stream->read(&data->deviceType, 1 * 4); // VkPhysicalDeviceType deviceType
    stream->read(&data->deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char deviceName
    stream->read(&data->pipelineCacheUUID, VK_UUID_SIZE * 1); // u8 pipelineCacheUUID
    { VkPhysicalDeviceLimits* tmpUnpacked = vkUtilsUnpack_VkPhysicalDeviceLimits(stream); memcpy(&data->limits, tmpUnpacked, sizeof(VkPhysicalDeviceLimits)); delete tmpUnpacked; } // VkPhysicalDeviceLimits limits
    { VkPhysicalDeviceSparseProperties* tmpUnpacked = vkUtilsUnpack_VkPhysicalDeviceSparseProperties(stream); memcpy(&data->sparseProperties, tmpUnpacked, sizeof(VkPhysicalDeviceSparseProperties)); delete tmpUnpacked; } // VkPhysicalDeviceSparseProperties sparseProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkExtensionProperties(const VkExtensionProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1; // char extensionName
    res += 1 * 4; // u32 specVersion
    return res;
}

void vkUtilsPack_VkExtensionProperties(android::base::InplaceStream* stream, const VkExtensionProperties* data) {
    if (!data) return;
    stream->write(&data->extensionName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char extensionName
    stream->write(&data->specVersion, 1 * 4); // u32 specVersion
}

VkExtensionProperties* vkUtilsUnpack_VkExtensionProperties(android::base::InplaceStream* stream) {
    VkExtensionProperties* data = new VkExtensionProperties; memset(data, 0, sizeof(VkExtensionProperties)); 
    stream->read(&data->extensionName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char extensionName
    stream->read(&data->specVersion, 1 * 4); // u32 specVersion
    return data;
}

uint32_t vkUtilsEncodingSize_VkLayerProperties(const VkLayerProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1; // char layerName
    res += 1 * 4; // u32 specVersion
    res += 1 * 4; // u32 implementationVersion
    res += VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1; // char description
    return res;
}

void vkUtilsPack_VkLayerProperties(android::base::InplaceStream* stream, const VkLayerProperties* data) {
    if (!data) return;
    stream->write(&data->layerName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char layerName
    stream->write(&data->specVersion, 1 * 4); // u32 specVersion
    stream->write(&data->implementationVersion, 1 * 4); // u32 implementationVersion
    stream->write(&data->description, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char description
}

VkLayerProperties* vkUtilsUnpack_VkLayerProperties(android::base::InplaceStream* stream) {
    VkLayerProperties* data = new VkLayerProperties; memset(data, 0, sizeof(VkLayerProperties)); 
    stream->read(&data->layerName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char layerName
    stream->read(&data->specVersion, 1 * 4); // u32 specVersion
    stream->read(&data->implementationVersion, 1 * 4); // u32 implementationVersion
    stream->read(&data->description, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * 1); // char description
    return data;
}

uint32_t vkUtilsEncodingSize_VkSubmitInfo(const VkSubmitInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 waitSemaphoreCount
    res += data->waitSemaphoreCount * 8; // VkSemaphore pWaitSemaphores
    res += data->waitSemaphoreCount * 4; // VkPipelineStageFlags pWaitDstStageMask
    res += 1 * 4; // u32 commandBufferCount
    res += data->commandBufferCount * 8; // VkCommandBuffer pCommandBuffers
    res += 1 * 4; // u32 signalSemaphoreCount
    res += data->signalSemaphoreCount * 8; // VkSemaphore pSignalSemaphores
    return res;
}

void vkUtilsPack_VkSubmitInfo(android::base::InplaceStream* stream, const VkSubmitInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    stream->write(data->pWaitSemaphores, data->waitSemaphoreCount * 8); // VkSemaphore pWaitSemaphores
    stream->write(data->pWaitDstStageMask, data->waitSemaphoreCount * 4); // VkPipelineStageFlags pWaitDstStageMask
    stream->write(&data->commandBufferCount, 1 * 4); // u32 commandBufferCount
    stream->write(data->pCommandBuffers, data->commandBufferCount * 8); // VkCommandBuffer pCommandBuffers
    stream->write(&data->signalSemaphoreCount, 1 * 4); // u32 signalSemaphoreCount
    stream->write(data->pSignalSemaphores, data->signalSemaphoreCount * 8); // VkSemaphore pSignalSemaphores
}

VkSubmitInfo* vkUtilsUnpack_VkSubmitInfo(android::base::InplaceStream* stream) {
    VkSubmitInfo* data = new VkSubmitInfo; memset(data, 0, sizeof(VkSubmitInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    data->pWaitSemaphores = data->waitSemaphoreCount ? new VkSemaphore[data->waitSemaphoreCount] : nullptr; // VkSemaphore pWaitSemaphores
    stream->read((char*)data->pWaitSemaphores, data->waitSemaphoreCount * 8); // VkSemaphore pWaitSemaphores
    data->pWaitDstStageMask = data->waitSemaphoreCount ? new VkPipelineStageFlags[data->waitSemaphoreCount] : nullptr; // VkPipelineStageFlags pWaitDstStageMask
    stream->read((char*)data->pWaitDstStageMask, data->waitSemaphoreCount * 4); // VkPipelineStageFlags pWaitDstStageMask
    stream->read(&data->commandBufferCount, 1 * 4); // u32 commandBufferCount
    data->pCommandBuffers = data->commandBufferCount ? new VkCommandBuffer[data->commandBufferCount] : nullptr; // VkCommandBuffer pCommandBuffers
    stream->read((char*)data->pCommandBuffers, data->commandBufferCount * 8); // VkCommandBuffer pCommandBuffers
    stream->read(&data->signalSemaphoreCount, 1 * 4); // u32 signalSemaphoreCount
    data->pSignalSemaphores = data->signalSemaphoreCount ? new VkSemaphore[data->signalSemaphoreCount] : nullptr; // VkSemaphore pSignalSemaphores
    stream->read((char*)data->pSignalSemaphores, data->signalSemaphoreCount * 8); // VkSemaphore pSignalSemaphores
    return data;
}

uint32_t vkUtilsEncodingSize_VkApplicationInfo(const VkApplicationInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += string1d_len(data->pApplicationName) * 1; // char pApplicationName
    res += 1 * 4; // u32 applicationVersion
    res += string1d_len(data->pEngineName) * 1; // char pEngineName
    res += 1 * 4; // u32 engineVersion
    res += 1 * 4; // u32 apiVersion
    return res;
}

void vkUtilsPack_VkApplicationInfo(android::base::InplaceStream* stream, const VkApplicationInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    string1d_pack(stream, data->pApplicationName); // char pApplicationName
    stream->write(&data->applicationVersion, 1 * 4); // u32 applicationVersion
    string1d_pack(stream, data->pEngineName); // char pEngineName
    stream->write(&data->engineVersion, 1 * 4); // u32 engineVersion
    stream->write(&data->apiVersion, 1 * 4); // u32 apiVersion
}

VkApplicationInfo* vkUtilsUnpack_VkApplicationInfo(android::base::InplaceStream* stream) {
    VkApplicationInfo* data = new VkApplicationInfo; memset(data, 0, sizeof(VkApplicationInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    data->pApplicationName = string1d_unpack(stream); // char pApplicationName
    stream->read(&data->applicationVersion, 1 * 4); // u32 applicationVersion
    data->pEngineName = string1d_unpack(stream); // char pEngineName
    stream->read(&data->engineVersion, 1 * 4); // u32 engineVersion
    stream->read(&data->apiVersion, 1 * 4); // u32 apiVersion
    return data;
}

uint32_t vkUtilsEncodingSize_VkAllocationCallbacks(const VkAllocationCallbacks* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // void pUserData
    res += 1 * 8; // PFN_vkAllocationFunction pfnAllocation
    res += 1 * 8; // PFN_vkReallocationFunction pfnReallocation
    res += 1 * 8; // PFN_vkFreeFunction pfnFree
    res += 1 * 8; // PFN_vkInternalAllocationNotification pfnInternalAllocation
    res += 1 * 8; // PFN_vkInternalFreeNotification pfnInternalFree
    return res;
}

void vkUtilsPack_VkAllocationCallbacks(android::base::InplaceStream* stream, const VkAllocationCallbacks* data) {
    if (!data) return;
    stream->write(&data->pUserData, 8); // void pUserData
    stream->write(&data->pfnAllocation, 1 * 8); // PFN_vkAllocationFunction pfnAllocation
    stream->write(&data->pfnReallocation, 1 * 8); // PFN_vkReallocationFunction pfnReallocation
    stream->write(&data->pfnFree, 1 * 8); // PFN_vkFreeFunction pfnFree
    stream->write(&data->pfnInternalAllocation, 1 * 8); // PFN_vkInternalAllocationNotification pfnInternalAllocation
    stream->write(&data->pfnInternalFree, 1 * 8); // PFN_vkInternalFreeNotification pfnInternalFree
}

VkAllocationCallbacks* vkUtilsUnpack_VkAllocationCallbacks(android::base::InplaceStream* stream) {
    VkAllocationCallbacks* data = new VkAllocationCallbacks; memset(data, 0, sizeof(VkAllocationCallbacks)); 
    stream->read(&data->pUserData, 8); // void pUserData
    stream->read(&data->pfnAllocation, 1 * 8); // PFN_vkAllocationFunction pfnAllocation
    stream->read(&data->pfnReallocation, 1 * 8); // PFN_vkReallocationFunction pfnReallocation
    stream->read(&data->pfnFree, 1 * 8); // PFN_vkFreeFunction pfnFree
    stream->read(&data->pfnInternalAllocation, 1 * 8); // PFN_vkInternalAllocationNotification pfnInternalAllocation
    stream->read(&data->pfnInternalFree, 1 * 8); // PFN_vkInternalFreeNotification pfnInternalFree
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceQueueCreateInfo(const VkDeviceQueueCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDeviceQueueCreateFlags flags
    res += 1 * 4; // u32 queueFamilyIndex
    res += 1 * 4; // u32 queueCount
    res += data->queueCount * 4; // f32 pQueuePriorities
    return res;
}

void vkUtilsPack_VkDeviceQueueCreateInfo(android::base::InplaceStream* stream, const VkDeviceQueueCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDeviceQueueCreateFlags flags
    stream->write(&data->queueFamilyIndex, 1 * 4); // u32 queueFamilyIndex
    stream->write(&data->queueCount, 1 * 4); // u32 queueCount
    stream->write(data->pQueuePriorities, data->queueCount * 4); // f32 pQueuePriorities
}

VkDeviceQueueCreateInfo* vkUtilsUnpack_VkDeviceQueueCreateInfo(android::base::InplaceStream* stream) {
    VkDeviceQueueCreateInfo* data = new VkDeviceQueueCreateInfo; memset(data, 0, sizeof(VkDeviceQueueCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDeviceQueueCreateFlags flags
    stream->read(&data->queueFamilyIndex, 1 * 4); // u32 queueFamilyIndex
    stream->read(&data->queueCount, 1 * 4); // u32 queueCount
    data->pQueuePriorities = data->queueCount ? new f32[data->queueCount] : nullptr; // f32 pQueuePriorities
    stream->read((char*)data->pQueuePriorities, data->queueCount * 4); // f32 pQueuePriorities
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceCreateInfo(const VkDeviceCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDeviceCreateFlags flags
    res += 1 * 4; // u32 queueCreateInfoCount
    res += 8; if (data->pQueueCreateInfos) { res += data->queueCreateInfoCount * vkUtilsEncodingSize_VkDeviceQueueCreateInfo(data->pQueueCreateInfos); } // VkDeviceQueueCreateInfo pQueueCreateInfos
    res += 1 * 4; // u32 enabledLayerCount
    res += strings2d_len(data->enabledLayerCount, data->ppEnabledLayerNames) * 1; // char ppEnabledLayerNames
    res += 1 * 4; // u32 enabledExtensionCount
    res += strings2d_len(data->enabledExtensionCount, data->ppEnabledExtensionNames) * 1; // char ppEnabledExtensionNames
    res += 8; if (data->pEnabledFeatures) { res += 1 * vkUtilsEncodingSize_VkPhysicalDeviceFeatures(data->pEnabledFeatures); } // VkPhysicalDeviceFeatures pEnabledFeatures
    return res;
}

void vkUtilsPack_VkDeviceCreateInfo(android::base::InplaceStream* stream, const VkDeviceCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDeviceCreateFlags flags
    stream->write(&data->queueCreateInfoCount, 1 * 4); // u32 queueCreateInfoCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pQueueCreateInfos); stream->write(&ptrval, 8);
    if (data->pQueueCreateInfos) { for (uint32_t i = 0; i < data->queueCreateInfoCount; i++) { vkUtilsPack_VkDeviceQueueCreateInfo(stream, data->pQueueCreateInfos + i); } } } // VkDeviceQueueCreateInfo pQueueCreateInfos
    stream->write(&data->enabledLayerCount, 1 * 4); // u32 enabledLayerCount
    strings2d_pack(stream, data->enabledLayerCount, data->ppEnabledLayerNames); // char ppEnabledLayerNames
    stream->write(&data->enabledExtensionCount, 1 * 4); // u32 enabledExtensionCount
    strings2d_pack(stream, data->enabledExtensionCount, data->ppEnabledExtensionNames); // char ppEnabledExtensionNames
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pEnabledFeatures); stream->write(&ptrval, 8);
    if (data->pEnabledFeatures) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPhysicalDeviceFeatures(stream, data->pEnabledFeatures + i); } } } // VkPhysicalDeviceFeatures pEnabledFeatures
}

VkDeviceCreateInfo* vkUtilsUnpack_VkDeviceCreateInfo(android::base::InplaceStream* stream) {
    VkDeviceCreateInfo* data = new VkDeviceCreateInfo; memset(data, 0, sizeof(VkDeviceCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDeviceCreateFlags flags
    stream->read(&data->queueCreateInfoCount, 1 * 4); // u32 queueCreateInfoCount
    { // VkDeviceQueueCreateInfo pQueueCreateInfos
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkDeviceQueueCreateInfo* tmpArr = ptrval ? new VkDeviceQueueCreateInfo[data->queueCreateInfoCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->queueCreateInfoCount; i++) {
        VkDeviceQueueCreateInfo* tmpUnpacked = vkUtilsUnpack_VkDeviceQueueCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkDeviceQueueCreateInfo)); delete tmpUnpacked; } }
        data->pQueueCreateInfos = tmpArr;
    }
    stream->read(&data->enabledLayerCount, 1 * 4); // u32 enabledLayerCount
    data->ppEnabledLayerNames = strings2d_unpack(stream, data->enabledLayerCount); // char ppEnabledLayerNames
    stream->read(&data->enabledExtensionCount, 1 * 4); // u32 enabledExtensionCount
    data->ppEnabledExtensionNames = strings2d_unpack(stream, data->enabledExtensionCount); // char ppEnabledExtensionNames
    { // VkPhysicalDeviceFeatures pEnabledFeatures
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPhysicalDeviceFeatures* tmpArr = ptrval ? new VkPhysicalDeviceFeatures[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPhysicalDeviceFeatures* tmpUnpacked = vkUtilsUnpack_VkPhysicalDeviceFeatures(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPhysicalDeviceFeatures)); delete tmpUnpacked; } }
        data->pEnabledFeatures = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkInstanceCreateInfo(const VkInstanceCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkInstanceCreateFlags flags
    res += 8; if (data->pApplicationInfo) { res += 1 * vkUtilsEncodingSize_VkApplicationInfo(data->pApplicationInfo); } // VkApplicationInfo pApplicationInfo
    res += 1 * 4; // u32 enabledLayerCount
    res += strings2d_len(data->enabledLayerCount, data->ppEnabledLayerNames) * 1; // char ppEnabledLayerNames
    res += 1 * 4; // u32 enabledExtensionCount
    res += strings2d_len(data->enabledExtensionCount, data->ppEnabledExtensionNames) * 1; // char ppEnabledExtensionNames
    return res;
}

void vkUtilsPack_VkInstanceCreateInfo(android::base::InplaceStream* stream, const VkInstanceCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkInstanceCreateFlags flags
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pApplicationInfo); stream->write(&ptrval, 8);
    if (data->pApplicationInfo) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkApplicationInfo(stream, data->pApplicationInfo + i); } } } // VkApplicationInfo pApplicationInfo
    stream->write(&data->enabledLayerCount, 1 * 4); // u32 enabledLayerCount
    strings2d_pack(stream, data->enabledLayerCount, data->ppEnabledLayerNames); // char ppEnabledLayerNames
    stream->write(&data->enabledExtensionCount, 1 * 4); // u32 enabledExtensionCount
    strings2d_pack(stream, data->enabledExtensionCount, data->ppEnabledExtensionNames); // char ppEnabledExtensionNames
}

VkInstanceCreateInfo* vkUtilsUnpack_VkInstanceCreateInfo(android::base::InplaceStream* stream) {
    VkInstanceCreateInfo* data = new VkInstanceCreateInfo; memset(data, 0, sizeof(VkInstanceCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkInstanceCreateFlags flags
    { // VkApplicationInfo pApplicationInfo
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkApplicationInfo* tmpArr = ptrval ? new VkApplicationInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkApplicationInfo* tmpUnpacked = vkUtilsUnpack_VkApplicationInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkApplicationInfo)); delete tmpUnpacked; } }
        data->pApplicationInfo = tmpArr;
    }
    stream->read(&data->enabledLayerCount, 1 * 4); // u32 enabledLayerCount
    data->ppEnabledLayerNames = strings2d_unpack(stream, data->enabledLayerCount); // char ppEnabledLayerNames
    stream->read(&data->enabledExtensionCount, 1 * 4); // u32 enabledExtensionCount
    data->ppEnabledExtensionNames = strings2d_unpack(stream, data->enabledExtensionCount); // char ppEnabledExtensionNames
    return data;
}

uint32_t vkUtilsEncodingSize_VkQueueFamilyProperties(const VkQueueFamilyProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkQueueFlags queueFlags
    res += 1 * 4; // u32 queueCount
    res += 1 * 4; // u32 timestampValidBits
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->minImageTransferGranularity); // VkExtent3D minImageTransferGranularity
    return res;
}

void vkUtilsPack_VkQueueFamilyProperties(android::base::InplaceStream* stream, const VkQueueFamilyProperties* data) {
    if (!data) return;
    stream->write(&data->queueFlags, 1 * 4); // VkQueueFlags queueFlags
    stream->write(&data->queueCount, 1 * 4); // u32 queueCount
    stream->write(&data->timestampValidBits, 1 * 4); // u32 timestampValidBits
    vkUtilsPack_VkExtent3D(stream, &data->minImageTransferGranularity); // VkExtent3D minImageTransferGranularity
}

VkQueueFamilyProperties* vkUtilsUnpack_VkQueueFamilyProperties(android::base::InplaceStream* stream) {
    VkQueueFamilyProperties* data = new VkQueueFamilyProperties; memset(data, 0, sizeof(VkQueueFamilyProperties)); 
    stream->read(&data->queueFlags, 1 * 4); // VkQueueFlags queueFlags
    stream->read(&data->queueCount, 1 * 4); // u32 queueCount
    stream->read(&data->timestampValidBits, 1 * 4); // u32 timestampValidBits
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->minImageTransferGranularity, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D minImageTransferGranularity
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 memoryTypeCount
    res += VK_MAX_MEMORY_TYPES * vkUtilsEncodingSize_VkMemoryType(data->memoryTypes); // VkMemoryType memoryTypes
    res += 1 * 4; // u32 memoryHeapCount
    res += VK_MAX_MEMORY_HEAPS * vkUtilsEncodingSize_VkMemoryHeap(data->memoryHeaps); // VkMemoryHeap memoryHeaps
    return res;
}

void vkUtilsPack_VkPhysicalDeviceMemoryProperties(android::base::InplaceStream* stream, const VkPhysicalDeviceMemoryProperties* data) {
    if (!data) return;
    stream->write(&data->memoryTypeCount, 1 * 4); // u32 memoryTypeCount
    {
    { for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) { vkUtilsPack_VkMemoryType(stream, data->memoryTypes + i); } } } // VkMemoryType memoryTypes
    stream->write(&data->memoryHeapCount, 1 * 4); // u32 memoryHeapCount
    {
    { for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; i++) { vkUtilsPack_VkMemoryHeap(stream, data->memoryHeaps + i); } } } // VkMemoryHeap memoryHeaps
}

VkPhysicalDeviceMemoryProperties* vkUtilsUnpack_VkPhysicalDeviceMemoryProperties(android::base::InplaceStream* stream) {
    VkPhysicalDeviceMemoryProperties* data = new VkPhysicalDeviceMemoryProperties; memset(data, 0, sizeof(VkPhysicalDeviceMemoryProperties)); 
    stream->read(&data->memoryTypeCount, 1 * 4); // u32 memoryTypeCount
    { // VkMemoryType memoryTypes
        { for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        VkMemoryType* tmpUnpacked = vkUtilsUnpack_VkMemoryType(stream); data->memoryTypes[i] = *tmpUnpacked; } }
    }
    stream->read(&data->memoryHeapCount, 1 * 4); // u32 memoryHeapCount
    { // VkMemoryHeap memoryHeaps
        { for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; i++) {
        VkMemoryHeap* tmpUnpacked = vkUtilsUnpack_VkMemoryHeap(stream); data->memoryHeaps[i] = *tmpUnpacked; } }
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryAllocateInfo(const VkMemoryAllocateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkDeviceSize allocationSize
    res += 1 * 4; // u32 memoryTypeIndex
    return res;
}

void vkUtilsPack_VkMemoryAllocateInfo(android::base::InplaceStream* stream, const VkMemoryAllocateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->allocationSize, 1 * 8); // VkDeviceSize allocationSize
    stream->write(&data->memoryTypeIndex, 1 * 4); // u32 memoryTypeIndex
}

VkMemoryAllocateInfo* vkUtilsUnpack_VkMemoryAllocateInfo(android::base::InplaceStream* stream) {
    VkMemoryAllocateInfo* data = new VkMemoryAllocateInfo; memset(data, 0, sizeof(VkMemoryAllocateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->allocationSize, 1 * 8); // VkDeviceSize allocationSize
    stream->read(&data->memoryTypeIndex, 1 * 4); // u32 memoryTypeIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryRequirements(const VkMemoryRequirements* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDeviceSize size
    res += 1 * 8; // VkDeviceSize alignment
    res += 1 * 4; // u32 memoryTypeBits
    return res;
}

void vkUtilsPack_VkMemoryRequirements(android::base::InplaceStream* stream, const VkMemoryRequirements* data) {
    if (!data) return;
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
    stream->write(&data->alignment, 1 * 8); // VkDeviceSize alignment
    stream->write(&data->memoryTypeBits, 1 * 4); // u32 memoryTypeBits
}

VkMemoryRequirements* vkUtilsUnpack_VkMemoryRequirements(android::base::InplaceStream* stream) {
    VkMemoryRequirements* data = new VkMemoryRequirements; memset(data, 0, sizeof(VkMemoryRequirements)); 
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    stream->read(&data->alignment, 1 * 8); // VkDeviceSize alignment
    stream->read(&data->memoryTypeBits, 1 * 4); // u32 memoryTypeBits
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseImageFormatProperties(const VkSparseImageFormatProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkImageAspectFlagBits aspectMask
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->imageGranularity); // VkExtent3D imageGranularity
    res += 1 * 4; // VkSparseImageFormatFlags flags
    return res;
}

void vkUtilsPack_VkSparseImageFormatProperties(android::base::InplaceStream* stream, const VkSparseImageFormatProperties* data) {
    if (!data) return;
    stream->write(&data->aspectMask, 1 * 4); // VkImageAspectFlagBits aspectMask
    vkUtilsPack_VkExtent3D(stream, &data->imageGranularity); // VkExtent3D imageGranularity
    stream->write(&data->flags, 1 * 4); // VkSparseImageFormatFlags flags
}

VkSparseImageFormatProperties* vkUtilsUnpack_VkSparseImageFormatProperties(android::base::InplaceStream* stream) {
    VkSparseImageFormatProperties* data = new VkSparseImageFormatProperties; memset(data, 0, sizeof(VkSparseImageFormatProperties)); 
    stream->read(&data->aspectMask, 1 * 4); // VkImageAspectFlagBits aspectMask
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->imageGranularity, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D imageGranularity
    stream->read(&data->flags, 1 * 4); // VkSparseImageFormatFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseImageMemoryRequirements(const VkSparseImageMemoryRequirements* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkSparseImageFormatProperties(&data->formatProperties); // VkSparseImageFormatProperties formatProperties
    res += 1 * 4; // u32 imageMipTailFirstLod
    res += 1 * 8; // VkDeviceSize imageMipTailSize
    res += 1 * 8; // VkDeviceSize imageMipTailOffset
    res += 1 * 8; // VkDeviceSize imageMipTailStride
    return res;
}

void vkUtilsPack_VkSparseImageMemoryRequirements(android::base::InplaceStream* stream, const VkSparseImageMemoryRequirements* data) {
    if (!data) return;
    vkUtilsPack_VkSparseImageFormatProperties(stream, &data->formatProperties); // VkSparseImageFormatProperties formatProperties
    stream->write(&data->imageMipTailFirstLod, 1 * 4); // u32 imageMipTailFirstLod
    stream->write(&data->imageMipTailSize, 1 * 8); // VkDeviceSize imageMipTailSize
    stream->write(&data->imageMipTailOffset, 1 * 8); // VkDeviceSize imageMipTailOffset
    stream->write(&data->imageMipTailStride, 1 * 8); // VkDeviceSize imageMipTailStride
}

VkSparseImageMemoryRequirements* vkUtilsUnpack_VkSparseImageMemoryRequirements(android::base::InplaceStream* stream) {
    VkSparseImageMemoryRequirements* data = new VkSparseImageMemoryRequirements; memset(data, 0, sizeof(VkSparseImageMemoryRequirements)); 
    { VkSparseImageFormatProperties* tmpUnpacked = vkUtilsUnpack_VkSparseImageFormatProperties(stream); memcpy(&data->formatProperties, tmpUnpacked, sizeof(VkSparseImageFormatProperties)); delete tmpUnpacked; } // VkSparseImageFormatProperties formatProperties
    stream->read(&data->imageMipTailFirstLod, 1 * 4); // u32 imageMipTailFirstLod
    stream->read(&data->imageMipTailSize, 1 * 8); // VkDeviceSize imageMipTailSize
    stream->read(&data->imageMipTailOffset, 1 * 8); // VkDeviceSize imageMipTailOffset
    stream->read(&data->imageMipTailStride, 1 * 8); // VkDeviceSize imageMipTailStride
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryType(const VkMemoryType* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkMemoryPropertyFlags propertyFlags
    res += 1 * 4; // u32 heapIndex
    return res;
}

void vkUtilsPack_VkMemoryType(android::base::InplaceStream* stream, const VkMemoryType* data) {
    if (!data) return;
    stream->write(&data->propertyFlags, 1 * 4); // VkMemoryPropertyFlags propertyFlags
    stream->write(&data->heapIndex, 1 * 4); // u32 heapIndex
}

VkMemoryType* vkUtilsUnpack_VkMemoryType(android::base::InplaceStream* stream) {
    VkMemoryType* data = new VkMemoryType; memset(data, 0, sizeof(VkMemoryType)); 
    stream->read(&data->propertyFlags, 1 * 4); // VkMemoryPropertyFlags propertyFlags
    stream->read(&data->heapIndex, 1 * 4); // u32 heapIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryHeap(const VkMemoryHeap* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDeviceSize size
    res += 1 * 4; // VkMemoryHeapFlags flags
    return res;
}

void vkUtilsPack_VkMemoryHeap(android::base::InplaceStream* stream, const VkMemoryHeap* data) {
    if (!data) return;
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
    stream->write(&data->flags, 1 * 4); // VkMemoryHeapFlags flags
}

VkMemoryHeap* vkUtilsUnpack_VkMemoryHeap(android::base::InplaceStream* stream) {
    VkMemoryHeap* data = new VkMemoryHeap; memset(data, 0, sizeof(VkMemoryHeap)); 
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    stream->read(&data->flags, 1 * 4); // VkMemoryHeapFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkMappedMemoryRange(const VkMappedMemoryRange* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkDeviceMemory memory
    res += 1 * 8; // VkDeviceSize offset
    res += 1 * 8; // VkDeviceSize size
    return res;
}

void vkUtilsPack_VkMappedMemoryRange(android::base::InplaceStream* stream, const VkMappedMemoryRange* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->write(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
}

VkMappedMemoryRange* vkUtilsUnpack_VkMappedMemoryRange(android::base::InplaceStream* stream) {
    VkMappedMemoryRange* data = new VkMappedMemoryRange; memset(data, 0, sizeof(VkMappedMemoryRange)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->read(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    return data;
}

uint32_t vkUtilsEncodingSize_VkFormatProperties(const VkFormatProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkFormatFeatureFlags linearTilingFeatures
    res += 1 * 4; // VkFormatFeatureFlags optimalTilingFeatures
    res += 1 * 4; // VkFormatFeatureFlags bufferFeatures
    return res;
}

void vkUtilsPack_VkFormatProperties(android::base::InplaceStream* stream, const VkFormatProperties* data) {
    if (!data) return;
    stream->write(&data->linearTilingFeatures, 1 * 4); // VkFormatFeatureFlags linearTilingFeatures
    stream->write(&data->optimalTilingFeatures, 1 * 4); // VkFormatFeatureFlags optimalTilingFeatures
    stream->write(&data->bufferFeatures, 1 * 4); // VkFormatFeatureFlags bufferFeatures
}

VkFormatProperties* vkUtilsUnpack_VkFormatProperties(android::base::InplaceStream* stream) {
    VkFormatProperties* data = new VkFormatProperties; memset(data, 0, sizeof(VkFormatProperties)); 
    stream->read(&data->linearTilingFeatures, 1 * 4); // VkFormatFeatureFlags linearTilingFeatures
    stream->read(&data->optimalTilingFeatures, 1 * 4); // VkFormatFeatureFlags optimalTilingFeatures
    stream->read(&data->bufferFeatures, 1 * 4); // VkFormatFeatureFlags bufferFeatures
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageFormatProperties(const VkImageFormatProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->maxExtent); // VkExtent3D maxExtent
    res += 1 * 4; // u32 maxMipLevels
    res += 1 * 4; // u32 maxArrayLayers
    res += 1 * 4; // VkSampleCountFlags sampleCounts
    res += 1 * 8; // VkDeviceSize maxResourceSize
    return res;
}

void vkUtilsPack_VkImageFormatProperties(android::base::InplaceStream* stream, const VkImageFormatProperties* data) {
    if (!data) return;
    vkUtilsPack_VkExtent3D(stream, &data->maxExtent); // VkExtent3D maxExtent
    stream->write(&data->maxMipLevels, 1 * 4); // u32 maxMipLevels
    stream->write(&data->maxArrayLayers, 1 * 4); // u32 maxArrayLayers
    stream->write(&data->sampleCounts, 1 * 4); // VkSampleCountFlags sampleCounts
    stream->write(&data->maxResourceSize, 1 * 8); // VkDeviceSize maxResourceSize
}

VkImageFormatProperties* vkUtilsUnpack_VkImageFormatProperties(android::base::InplaceStream* stream) {
    VkImageFormatProperties* data = new VkImageFormatProperties; memset(data, 0, sizeof(VkImageFormatProperties)); 
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->maxExtent, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D maxExtent
    stream->read(&data->maxMipLevels, 1 * 4); // u32 maxMipLevels
    stream->read(&data->maxArrayLayers, 1 * 4); // u32 maxArrayLayers
    stream->read(&data->sampleCounts, 1 * 4); // VkSampleCountFlags sampleCounts
    stream->read(&data->maxResourceSize, 1 * 8); // VkDeviceSize maxResourceSize
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorImageInfo(const VkDescriptorImageInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkSampler sampler
    res += 1 * 8; // VkImageView imageView
    res += 1 * 4; // VkImageLayout imageLayout
    return res;
}

void vkUtilsPack_VkDescriptorImageInfo(android::base::InplaceStream* stream, const VkDescriptorImageInfo* data) {
    if (!data) return;
    stream->write(&data->sampler, 1 * 8); // VkSampler sampler
    stream->write(&data->imageView, 1 * 8); // VkImageView imageView
    stream->write(&data->imageLayout, 1 * 4); // VkImageLayout imageLayout
}

VkDescriptorImageInfo* vkUtilsUnpack_VkDescriptorImageInfo(android::base::InplaceStream* stream) {
    VkDescriptorImageInfo* data = new VkDescriptorImageInfo; memset(data, 0, sizeof(VkDescriptorImageInfo)); 
    stream->read(&data->sampler, 1 * 8); // VkSampler sampler
    stream->read(&data->imageView, 1 * 8); // VkImageView imageView
    stream->read(&data->imageLayout, 1 * 4); // VkImageLayout imageLayout
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorBufferInfo(const VkDescriptorBufferInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkBuffer buffer
    res += 1 * 8; // VkDeviceSize offset
    res += 1 * 8; // VkDeviceSize range
    return res;
}

void vkUtilsPack_VkDescriptorBufferInfo(android::base::InplaceStream* stream, const VkDescriptorBufferInfo* data) {
    if (!data) return;
    stream->write(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->write(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->write(&data->range, 1 * 8); // VkDeviceSize range
}

VkDescriptorBufferInfo* vkUtilsUnpack_VkDescriptorBufferInfo(android::base::InplaceStream* stream) {
    VkDescriptorBufferInfo* data = new VkDescriptorBufferInfo; memset(data, 0, sizeof(VkDescriptorBufferInfo)); 
    stream->read(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->read(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->read(&data->range, 1 * 8); // VkDeviceSize range
    return data;
}

uint32_t vkUtilsEncodingSize_VkWriteDescriptorSet(const VkWriteDescriptorSet* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkDescriptorSet dstSet
    res += 1 * 4; // u32 dstBinding
    res += 1 * 4; // u32 dstArrayElement
    res += 1 * 4; // u32 descriptorCount
    res += 1 * 4; // VkDescriptorType descriptorType
    res += 8; if (data->pImageInfo) { res += customCount_VkWriteDescriptorSet_pImageInfo(data) * vkUtilsEncodingSize_VkDescriptorImageInfo(data->pImageInfo); } // VkDescriptorImageInfo pImageInfo
    res += 8; if (data->pBufferInfo) { res += customCount_VkWriteDescriptorSet_pBufferInfo(data) * vkUtilsEncodingSize_VkDescriptorBufferInfo(data->pBufferInfo); } // VkDescriptorBufferInfo pBufferInfo
    res += customCount_VkWriteDescriptorSet_pTexelBufferView(data) * 8; // VkBufferView pTexelBufferView
    return res;
}

void vkUtilsPack_VkWriteDescriptorSet(android::base::InplaceStream* stream, const VkWriteDescriptorSet* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->dstSet, 1 * 8); // VkDescriptorSet dstSet
    stream->write(&data->dstBinding, 1 * 4); // u32 dstBinding
    stream->write(&data->dstArrayElement, 1 * 4); // u32 dstArrayElement
    stream->write(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    stream->write(&data->descriptorType, 1 * 4); // VkDescriptorType descriptorType
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pImageInfo); stream->write(&ptrval, 8);
    if (data->pImageInfo) { for (uint32_t i = 0; i < customCount_VkWriteDescriptorSet_pImageInfo(data); i++) { vkUtilsPack_VkDescriptorImageInfo(stream, data->pImageInfo + i); } } } // VkDescriptorImageInfo pImageInfo
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pBufferInfo); stream->write(&ptrval, 8);
    if (data->pBufferInfo) { for (uint32_t i = 0; i < customCount_VkWriteDescriptorSet_pBufferInfo(data); i++) { vkUtilsPack_VkDescriptorBufferInfo(stream, data->pBufferInfo + i); } } } // VkDescriptorBufferInfo pBufferInfo
    stream->write(data->pTexelBufferView, customCount_VkWriteDescriptorSet_pTexelBufferView(data) * 8); // VkBufferView pTexelBufferView
}

VkWriteDescriptorSet* vkUtilsUnpack_VkWriteDescriptorSet(android::base::InplaceStream* stream) {
    VkWriteDescriptorSet* data = new VkWriteDescriptorSet; memset(data, 0, sizeof(VkWriteDescriptorSet)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->dstSet, 1 * 8); // VkDescriptorSet dstSet
    stream->read(&data->dstBinding, 1 * 4); // u32 dstBinding
    stream->read(&data->dstArrayElement, 1 * 4); // u32 dstArrayElement
    stream->read(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    stream->read(&data->descriptorType, 1 * 4); // VkDescriptorType descriptorType
    { // VkDescriptorImageInfo pImageInfo
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkDescriptorImageInfo* tmpArr = ptrval ? new VkDescriptorImageInfo[customCount_VkWriteDescriptorSet_pImageInfo(data)] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < customCount_VkWriteDescriptorSet_pImageInfo(data); i++) {
        VkDescriptorImageInfo* tmpUnpacked = vkUtilsUnpack_VkDescriptorImageInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkDescriptorImageInfo)); delete tmpUnpacked; } }
        data->pImageInfo = tmpArr;
    }
    { // VkDescriptorBufferInfo pBufferInfo
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkDescriptorBufferInfo* tmpArr = ptrval ? new VkDescriptorBufferInfo[customCount_VkWriteDescriptorSet_pBufferInfo(data)] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < customCount_VkWriteDescriptorSet_pBufferInfo(data); i++) {
        VkDescriptorBufferInfo* tmpUnpacked = vkUtilsUnpack_VkDescriptorBufferInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkDescriptorBufferInfo)); delete tmpUnpacked; } }
        data->pBufferInfo = tmpArr;
    }
    data->pTexelBufferView = customCount_VkWriteDescriptorSet_pTexelBufferView(data) ? new VkBufferView[customCount_VkWriteDescriptorSet_pTexelBufferView(data)] : nullptr; // VkBufferView pTexelBufferView
    stream->read((char*)data->pTexelBufferView, customCount_VkWriteDescriptorSet_pTexelBufferView(data) * 8); // VkBufferView pTexelBufferView
    return data;
}

uint32_t vkUtilsEncodingSize_VkCopyDescriptorSet(const VkCopyDescriptorSet* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkDescriptorSet srcSet
    res += 1 * 4; // u32 srcBinding
    res += 1 * 4; // u32 srcArrayElement
    res += 1 * 8; // VkDescriptorSet dstSet
    res += 1 * 4; // u32 dstBinding
    res += 1 * 4; // u32 dstArrayElement
    res += 1 * 4; // u32 descriptorCount
    return res;
}

void vkUtilsPack_VkCopyDescriptorSet(android::base::InplaceStream* stream, const VkCopyDescriptorSet* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->srcSet, 1 * 8); // VkDescriptorSet srcSet
    stream->write(&data->srcBinding, 1 * 4); // u32 srcBinding
    stream->write(&data->srcArrayElement, 1 * 4); // u32 srcArrayElement
    stream->write(&data->dstSet, 1 * 8); // VkDescriptorSet dstSet
    stream->write(&data->dstBinding, 1 * 4); // u32 dstBinding
    stream->write(&data->dstArrayElement, 1 * 4); // u32 dstArrayElement
    stream->write(&data->descriptorCount, 1 * 4); // u32 descriptorCount
}

VkCopyDescriptorSet* vkUtilsUnpack_VkCopyDescriptorSet(android::base::InplaceStream* stream) {
    VkCopyDescriptorSet* data = new VkCopyDescriptorSet; memset(data, 0, sizeof(VkCopyDescriptorSet)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->srcSet, 1 * 8); // VkDescriptorSet srcSet
    stream->read(&data->srcBinding, 1 * 4); // u32 srcBinding
    stream->read(&data->srcArrayElement, 1 * 4); // u32 srcArrayElement
    stream->read(&data->dstSet, 1 * 8); // VkDescriptorSet dstSet
    stream->read(&data->dstBinding, 1 * 4); // u32 dstBinding
    stream->read(&data->dstArrayElement, 1 * 4); // u32 dstArrayElement
    stream->read(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    return data;
}

uint32_t vkUtilsEncodingSize_VkBufferCreateInfo(const VkBufferCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBufferCreateFlags flags
    res += 1 * 8; // VkDeviceSize size
    res += 1 * 4; // VkBufferUsageFlags usage
    res += 1 * 4; // VkSharingMode sharingMode
    res += 1 * 4; // u32 queueFamilyIndexCount
    res += data->queueFamilyIndexCount * 4; // u32 pQueueFamilyIndices
    return res;
}

void vkUtilsPack_VkBufferCreateInfo(android::base::InplaceStream* stream, const VkBufferCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkBufferCreateFlags flags
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
    stream->write(&data->usage, 1 * 4); // VkBufferUsageFlags usage
    stream->write(&data->sharingMode, 1 * 4); // VkSharingMode sharingMode
    stream->write(&data->queueFamilyIndexCount, 1 * 4); // u32 queueFamilyIndexCount
    stream->write(data->pQueueFamilyIndices, data->queueFamilyIndexCount * 4); // u32 pQueueFamilyIndices
}

VkBufferCreateInfo* vkUtilsUnpack_VkBufferCreateInfo(android::base::InplaceStream* stream) {
    VkBufferCreateInfo* data = new VkBufferCreateInfo; memset(data, 0, sizeof(VkBufferCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkBufferCreateFlags flags
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    stream->read(&data->usage, 1 * 4); // VkBufferUsageFlags usage
    stream->read(&data->sharingMode, 1 * 4); // VkSharingMode sharingMode
    stream->read(&data->queueFamilyIndexCount, 1 * 4); // u32 queueFamilyIndexCount
    data->pQueueFamilyIndices = data->queueFamilyIndexCount ? new u32[data->queueFamilyIndexCount] : nullptr; // u32 pQueueFamilyIndices
    stream->read((char*)data->pQueueFamilyIndices, data->queueFamilyIndexCount * 4); // u32 pQueueFamilyIndices
    return data;
}

uint32_t vkUtilsEncodingSize_VkBufferViewCreateInfo(const VkBufferViewCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBufferViewCreateFlags flags
    res += 1 * 8; // VkBuffer buffer
    res += 1 * 4; // VkFormat format
    res += 1 * 8; // VkDeviceSize offset
    res += 1 * 8; // VkDeviceSize range
    return res;
}

void vkUtilsPack_VkBufferViewCreateInfo(android::base::InplaceStream* stream, const VkBufferViewCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkBufferViewCreateFlags flags
    stream->write(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->write(&data->format, 1 * 4); // VkFormat format
    stream->write(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->write(&data->range, 1 * 8); // VkDeviceSize range
}

VkBufferViewCreateInfo* vkUtilsUnpack_VkBufferViewCreateInfo(android::base::InplaceStream* stream) {
    VkBufferViewCreateInfo* data = new VkBufferViewCreateInfo; memset(data, 0, sizeof(VkBufferViewCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkBufferViewCreateFlags flags
    stream->read(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->read(&data->format, 1 * 4); // VkFormat format
    stream->read(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->read(&data->range, 1 * 8); // VkDeviceSize range
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageSubresource(const VkImageSubresource* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkImageAspectFlagBits aspectMask
    res += 1 * 4; // u32 mipLevel
    res += 1 * 4; // u32 arrayLayer
    return res;
}

void vkUtilsPack_VkImageSubresource(android::base::InplaceStream* stream, const VkImageSubresource* data) {
    if (!data) return;
    stream->write(&data->aspectMask, 1 * 4); // VkImageAspectFlagBits aspectMask
    stream->write(&data->mipLevel, 1 * 4); // u32 mipLevel
    stream->write(&data->arrayLayer, 1 * 4); // u32 arrayLayer
}

VkImageSubresource* vkUtilsUnpack_VkImageSubresource(android::base::InplaceStream* stream) {
    VkImageSubresource* data = new VkImageSubresource; memset(data, 0, sizeof(VkImageSubresource)); 
    stream->read(&data->aspectMask, 1 * 4); // VkImageAspectFlagBits aspectMask
    stream->read(&data->mipLevel, 1 * 4); // u32 mipLevel
    stream->read(&data->arrayLayer, 1 * 4); // u32 arrayLayer
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageSubresourceRange(const VkImageSubresourceRange* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkImageAspectFlags aspectMask
    res += 1 * 4; // u32 baseMipLevel
    res += 1 * 4; // u32 levelCount
    res += 1 * 4; // u32 baseArrayLayer
    res += 1 * 4; // u32 layerCount
    return res;
}

void vkUtilsPack_VkImageSubresourceRange(android::base::InplaceStream* stream, const VkImageSubresourceRange* data) {
    if (!data) return;
    stream->write(&data->aspectMask, 1 * 4); // VkImageAspectFlags aspectMask
    stream->write(&data->baseMipLevel, 1 * 4); // u32 baseMipLevel
    stream->write(&data->levelCount, 1 * 4); // u32 levelCount
    stream->write(&data->baseArrayLayer, 1 * 4); // u32 baseArrayLayer
    stream->write(&data->layerCount, 1 * 4); // u32 layerCount
}

VkImageSubresourceRange* vkUtilsUnpack_VkImageSubresourceRange(android::base::InplaceStream* stream) {
    VkImageSubresourceRange* data = new VkImageSubresourceRange; memset(data, 0, sizeof(VkImageSubresourceRange)); 
    stream->read(&data->aspectMask, 1 * 4); // VkImageAspectFlags aspectMask
    stream->read(&data->baseMipLevel, 1 * 4); // u32 baseMipLevel
    stream->read(&data->levelCount, 1 * 4); // u32 levelCount
    stream->read(&data->baseArrayLayer, 1 * 4); // u32 baseArrayLayer
    stream->read(&data->layerCount, 1 * 4); // u32 layerCount
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryBarrier(const VkMemoryBarrier* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkAccessFlags srcAccessMask
    res += 1 * 4; // VkAccessFlags dstAccessMask
    return res;
}

void vkUtilsPack_VkMemoryBarrier(android::base::InplaceStream* stream, const VkMemoryBarrier* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->write(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
}

VkMemoryBarrier* vkUtilsUnpack_VkMemoryBarrier(android::base::InplaceStream* stream) {
    VkMemoryBarrier* data = new VkMemoryBarrier; memset(data, 0, sizeof(VkMemoryBarrier)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->read(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    return data;
}

uint32_t vkUtilsEncodingSize_VkBufferMemoryBarrier(const VkBufferMemoryBarrier* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkAccessFlags srcAccessMask
    res += 1 * 4; // VkAccessFlags dstAccessMask
    res += 1 * 4; // u32 srcQueueFamilyIndex
    res += 1 * 4; // u32 dstQueueFamilyIndex
    res += 1 * 8; // VkBuffer buffer
    res += 1 * 8; // VkDeviceSize offset
    res += 1 * 8; // VkDeviceSize size
    return res;
}

void vkUtilsPack_VkBufferMemoryBarrier(android::base::InplaceStream* stream, const VkBufferMemoryBarrier* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->write(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    stream->write(&data->srcQueueFamilyIndex, 1 * 4); // u32 srcQueueFamilyIndex
    stream->write(&data->dstQueueFamilyIndex, 1 * 4); // u32 dstQueueFamilyIndex
    stream->write(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->write(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
}

VkBufferMemoryBarrier* vkUtilsUnpack_VkBufferMemoryBarrier(android::base::InplaceStream* stream) {
    VkBufferMemoryBarrier* data = new VkBufferMemoryBarrier; memset(data, 0, sizeof(VkBufferMemoryBarrier)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->read(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    stream->read(&data->srcQueueFamilyIndex, 1 * 4); // u32 srcQueueFamilyIndex
    stream->read(&data->dstQueueFamilyIndex, 1 * 4); // u32 dstQueueFamilyIndex
    stream->read(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->read(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageMemoryBarrier(const VkImageMemoryBarrier* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkAccessFlags srcAccessMask
    res += 1 * 4; // VkAccessFlags dstAccessMask
    res += 1 * 4; // VkImageLayout oldLayout
    res += 1 * 4; // VkImageLayout newLayout
    res += 1 * 4; // u32 srcQueueFamilyIndex
    res += 1 * 4; // u32 dstQueueFamilyIndex
    res += 1 * 8; // VkImage image
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceRange(&data->subresourceRange); // VkImageSubresourceRange subresourceRange
    return res;
}

void vkUtilsPack_VkImageMemoryBarrier(android::base::InplaceStream* stream, const VkImageMemoryBarrier* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->write(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    stream->write(&data->oldLayout, 1 * 4); // VkImageLayout oldLayout
    stream->write(&data->newLayout, 1 * 4); // VkImageLayout newLayout
    stream->write(&data->srcQueueFamilyIndex, 1 * 4); // u32 srcQueueFamilyIndex
    stream->write(&data->dstQueueFamilyIndex, 1 * 4); // u32 dstQueueFamilyIndex
    stream->write(&data->image, 1 * 8); // VkImage image
    vkUtilsPack_VkImageSubresourceRange(stream, &data->subresourceRange); // VkImageSubresourceRange subresourceRange
}

VkImageMemoryBarrier* vkUtilsUnpack_VkImageMemoryBarrier(android::base::InplaceStream* stream) {
    VkImageMemoryBarrier* data = new VkImageMemoryBarrier; memset(data, 0, sizeof(VkImageMemoryBarrier)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->read(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    stream->read(&data->oldLayout, 1 * 4); // VkImageLayout oldLayout
    stream->read(&data->newLayout, 1 * 4); // VkImageLayout newLayout
    stream->read(&data->srcQueueFamilyIndex, 1 * 4); // u32 srcQueueFamilyIndex
    stream->read(&data->dstQueueFamilyIndex, 1 * 4); // u32 dstQueueFamilyIndex
    stream->read(&data->image, 1 * 8); // VkImage image
    { VkImageSubresourceRange* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceRange(stream); memcpy(&data->subresourceRange, tmpUnpacked, sizeof(VkImageSubresourceRange)); delete tmpUnpacked; } // VkImageSubresourceRange subresourceRange
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageCreateInfo(const VkImageCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkImageCreateFlags flags
    res += 1 * 4; // VkImageType imageType
    res += 1 * 4; // VkFormat format
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->extent); // VkExtent3D extent
    res += 1 * 4; // u32 mipLevels
    res += 1 * 4; // u32 arrayLayers
    res += 1 * 4; // VkSampleCountFlagBits samples
    res += 1 * 4; // VkImageTiling tiling
    res += 1 * 4; // VkImageUsageFlags usage
    res += 1 * 4; // VkSharingMode sharingMode
    res += 1 * 4; // u32 queueFamilyIndexCount
    res += data->queueFamilyIndexCount * 4; // u32 pQueueFamilyIndices
    res += 1 * 4; // VkImageLayout initialLayout
    return res;
}

void vkUtilsPack_VkImageCreateInfo(android::base::InplaceStream* stream, const VkImageCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkImageCreateFlags flags
    stream->write(&data->imageType, 1 * 4); // VkImageType imageType
    stream->write(&data->format, 1 * 4); // VkFormat format
    vkUtilsPack_VkExtent3D(stream, &data->extent); // VkExtent3D extent
    stream->write(&data->mipLevels, 1 * 4); // u32 mipLevels
    stream->write(&data->arrayLayers, 1 * 4); // u32 arrayLayers
    stream->write(&data->samples, 1 * 4); // VkSampleCountFlagBits samples
    stream->write(&data->tiling, 1 * 4); // VkImageTiling tiling
    stream->write(&data->usage, 1 * 4); // VkImageUsageFlags usage
    stream->write(&data->sharingMode, 1 * 4); // VkSharingMode sharingMode
    stream->write(&data->queueFamilyIndexCount, 1 * 4); // u32 queueFamilyIndexCount
    stream->write(data->pQueueFamilyIndices, data->queueFamilyIndexCount * 4); // u32 pQueueFamilyIndices
    stream->write(&data->initialLayout, 1 * 4); // VkImageLayout initialLayout
}

VkImageCreateInfo* vkUtilsUnpack_VkImageCreateInfo(android::base::InplaceStream* stream) {
    VkImageCreateInfo* data = new VkImageCreateInfo; memset(data, 0, sizeof(VkImageCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkImageCreateFlags flags
    stream->read(&data->imageType, 1 * 4); // VkImageType imageType
    stream->read(&data->format, 1 * 4); // VkFormat format
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->extent, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D extent
    stream->read(&data->mipLevels, 1 * 4); // u32 mipLevels
    stream->read(&data->arrayLayers, 1 * 4); // u32 arrayLayers
    stream->read(&data->samples, 1 * 4); // VkSampleCountFlagBits samples
    stream->read(&data->tiling, 1 * 4); // VkImageTiling tiling
    stream->read(&data->usage, 1 * 4); // VkImageUsageFlags usage
    stream->read(&data->sharingMode, 1 * 4); // VkSharingMode sharingMode
    stream->read(&data->queueFamilyIndexCount, 1 * 4); // u32 queueFamilyIndexCount
    data->pQueueFamilyIndices = data->queueFamilyIndexCount ? new u32[data->queueFamilyIndexCount] : nullptr; // u32 pQueueFamilyIndices
    stream->read((char*)data->pQueueFamilyIndices, data->queueFamilyIndexCount * 4); // u32 pQueueFamilyIndices
    stream->read(&data->initialLayout, 1 * 4); // VkImageLayout initialLayout
    return data;
}

uint32_t vkUtilsEncodingSize_VkSubresourceLayout(const VkSubresourceLayout* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDeviceSize offset
    res += 1 * 8; // VkDeviceSize size
    res += 1 * 8; // VkDeviceSize rowPitch
    res += 1 * 8; // VkDeviceSize arrayPitch
    res += 1 * 8; // VkDeviceSize depthPitch
    return res;
}

void vkUtilsPack_VkSubresourceLayout(android::base::InplaceStream* stream, const VkSubresourceLayout* data) {
    if (!data) return;
    stream->write(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
    stream->write(&data->rowPitch, 1 * 8); // VkDeviceSize rowPitch
    stream->write(&data->arrayPitch, 1 * 8); // VkDeviceSize arrayPitch
    stream->write(&data->depthPitch, 1 * 8); // VkDeviceSize depthPitch
}

VkSubresourceLayout* vkUtilsUnpack_VkSubresourceLayout(android::base::InplaceStream* stream) {
    VkSubresourceLayout* data = new VkSubresourceLayout; memset(data, 0, sizeof(VkSubresourceLayout)); 
    stream->read(&data->offset, 1 * 8); // VkDeviceSize offset
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    stream->read(&data->rowPitch, 1 * 8); // VkDeviceSize rowPitch
    stream->read(&data->arrayPitch, 1 * 8); // VkDeviceSize arrayPitch
    stream->read(&data->depthPitch, 1 * 8); // VkDeviceSize depthPitch
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageViewCreateInfo(const VkImageViewCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkImageViewCreateFlags flags
    res += 1 * 8; // VkImage image
    res += 1 * 4; // VkImageViewType viewType
    res += 1 * 4; // VkFormat format
    res += 1 * vkUtilsEncodingSize_VkComponentMapping(&data->components); // VkComponentMapping components
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceRange(&data->subresourceRange); // VkImageSubresourceRange subresourceRange
    return res;
}

void vkUtilsPack_VkImageViewCreateInfo(android::base::InplaceStream* stream, const VkImageViewCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkImageViewCreateFlags flags
    stream->write(&data->image, 1 * 8); // VkImage image
    stream->write(&data->viewType, 1 * 4); // VkImageViewType viewType
    stream->write(&data->format, 1 * 4); // VkFormat format
    vkUtilsPack_VkComponentMapping(stream, &data->components); // VkComponentMapping components
    vkUtilsPack_VkImageSubresourceRange(stream, &data->subresourceRange); // VkImageSubresourceRange subresourceRange
}

VkImageViewCreateInfo* vkUtilsUnpack_VkImageViewCreateInfo(android::base::InplaceStream* stream) {
    VkImageViewCreateInfo* data = new VkImageViewCreateInfo; memset(data, 0, sizeof(VkImageViewCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkImageViewCreateFlags flags
    stream->read(&data->image, 1 * 8); // VkImage image
    stream->read(&data->viewType, 1 * 4); // VkImageViewType viewType
    stream->read(&data->format, 1 * 4); // VkFormat format
    { VkComponentMapping* tmpUnpacked = vkUtilsUnpack_VkComponentMapping(stream); memcpy(&data->components, tmpUnpacked, sizeof(VkComponentMapping)); delete tmpUnpacked; } // VkComponentMapping components
    { VkImageSubresourceRange* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceRange(stream); memcpy(&data->subresourceRange, tmpUnpacked, sizeof(VkImageSubresourceRange)); delete tmpUnpacked; } // VkImageSubresourceRange subresourceRange
    return data;
}

uint32_t vkUtilsEncodingSize_VkBufferCopy(const VkBufferCopy* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDeviceSize srcOffset
    res += 1 * 8; // VkDeviceSize dstOffset
    res += 1 * 8; // VkDeviceSize size
    return res;
}

void vkUtilsPack_VkBufferCopy(android::base::InplaceStream* stream, const VkBufferCopy* data) {
    if (!data) return;
    stream->write(&data->srcOffset, 1 * 8); // VkDeviceSize srcOffset
    stream->write(&data->dstOffset, 1 * 8); // VkDeviceSize dstOffset
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
}

VkBufferCopy* vkUtilsUnpack_VkBufferCopy(android::base::InplaceStream* stream) {
    VkBufferCopy* data = new VkBufferCopy; memset(data, 0, sizeof(VkBufferCopy)); 
    stream->read(&data->srcOffset, 1 * 8); // VkDeviceSize srcOffset
    stream->read(&data->dstOffset, 1 * 8); // VkDeviceSize dstOffset
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseMemoryBind(const VkSparseMemoryBind* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDeviceSize resourceOffset
    res += 1 * 8; // VkDeviceSize size
    res += 1 * 8; // VkDeviceMemory memory
    res += 1 * 8; // VkDeviceSize memoryOffset
    res += 1 * 4; // VkSparseMemoryBindFlags flags
    return res;
}

void vkUtilsPack_VkSparseMemoryBind(android::base::InplaceStream* stream, const VkSparseMemoryBind* data) {
    if (!data) return;
    stream->write(&data->resourceOffset, 1 * 8); // VkDeviceSize resourceOffset
    stream->write(&data->size, 1 * 8); // VkDeviceSize size
    stream->write(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->write(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->write(&data->flags, 1 * 4); // VkSparseMemoryBindFlags flags
}

VkSparseMemoryBind* vkUtilsUnpack_VkSparseMemoryBind(android::base::InplaceStream* stream) {
    VkSparseMemoryBind* data = new VkSparseMemoryBind; memset(data, 0, sizeof(VkSparseMemoryBind)); 
    stream->read(&data->resourceOffset, 1 * 8); // VkDeviceSize resourceOffset
    stream->read(&data->size, 1 * 8); // VkDeviceSize size
    stream->read(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->read(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->read(&data->flags, 1 * 4); // VkSparseMemoryBindFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseImageMemoryBind(const VkSparseImageMemoryBind* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkImageSubresource(&data->subresource); // VkImageSubresource subresource
    res += 1 * vkUtilsEncodingSize_VkOffset3D(&data->offset); // VkOffset3D offset
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->extent); // VkExtent3D extent
    res += 1 * 8; // VkDeviceMemory memory
    res += 1 * 8; // VkDeviceSize memoryOffset
    res += 1 * 4; // VkSparseMemoryBindFlags flags
    return res;
}

void vkUtilsPack_VkSparseImageMemoryBind(android::base::InplaceStream* stream, const VkSparseImageMemoryBind* data) {
    if (!data) return;
    vkUtilsPack_VkImageSubresource(stream, &data->subresource); // VkImageSubresource subresource
    vkUtilsPack_VkOffset3D(stream, &data->offset); // VkOffset3D offset
    vkUtilsPack_VkExtent3D(stream, &data->extent); // VkExtent3D extent
    stream->write(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->write(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->write(&data->flags, 1 * 4); // VkSparseMemoryBindFlags flags
}

VkSparseImageMemoryBind* vkUtilsUnpack_VkSparseImageMemoryBind(android::base::InplaceStream* stream) {
    VkSparseImageMemoryBind* data = new VkSparseImageMemoryBind; memset(data, 0, sizeof(VkSparseImageMemoryBind)); 
    { VkImageSubresource* tmpUnpacked = vkUtilsUnpack_VkImageSubresource(stream); memcpy(&data->subresource, tmpUnpacked, sizeof(VkImageSubresource)); delete tmpUnpacked; } // VkImageSubresource subresource
    { VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); memcpy(&data->offset, tmpUnpacked, sizeof(VkOffset3D)); delete tmpUnpacked; } // VkOffset3D offset
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->extent, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D extent
    stream->read(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->read(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->read(&data->flags, 1 * 4); // VkSparseMemoryBindFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseBufferMemoryBindInfo(const VkSparseBufferMemoryBindInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkBuffer buffer
    res += 1 * 4; // u32 bindCount
    res += 8; if (data->pBinds) { res += data->bindCount * vkUtilsEncodingSize_VkSparseMemoryBind(data->pBinds); } // VkSparseMemoryBind pBinds
    return res;
}

void vkUtilsPack_VkSparseBufferMemoryBindInfo(android::base::InplaceStream* stream, const VkSparseBufferMemoryBindInfo* data) {
    if (!data) return;
    stream->write(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->write(&data->bindCount, 1 * 4); // u32 bindCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pBinds); stream->write(&ptrval, 8);
    if (data->pBinds) { for (uint32_t i = 0; i < data->bindCount; i++) { vkUtilsPack_VkSparseMemoryBind(stream, data->pBinds + i); } } } // VkSparseMemoryBind pBinds
}

VkSparseBufferMemoryBindInfo* vkUtilsUnpack_VkSparseBufferMemoryBindInfo(android::base::InplaceStream* stream) {
    VkSparseBufferMemoryBindInfo* data = new VkSparseBufferMemoryBindInfo; memset(data, 0, sizeof(VkSparseBufferMemoryBindInfo)); 
    stream->read(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->read(&data->bindCount, 1 * 4); // u32 bindCount
    { // VkSparseMemoryBind pBinds
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSparseMemoryBind* tmpArr = ptrval ? new VkSparseMemoryBind[data->bindCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->bindCount; i++) {
        VkSparseMemoryBind* tmpUnpacked = vkUtilsUnpack_VkSparseMemoryBind(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSparseMemoryBind)); delete tmpUnpacked; } }
        data->pBinds = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseImageOpaqueMemoryBindInfo(const VkSparseImageOpaqueMemoryBindInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkImage image
    res += 1 * 4; // u32 bindCount
    res += 8; if (data->pBinds) { res += data->bindCount * vkUtilsEncodingSize_VkSparseMemoryBind(data->pBinds); } // VkSparseMemoryBind pBinds
    return res;
}

void vkUtilsPack_VkSparseImageOpaqueMemoryBindInfo(android::base::InplaceStream* stream, const VkSparseImageOpaqueMemoryBindInfo* data) {
    if (!data) return;
    stream->write(&data->image, 1 * 8); // VkImage image
    stream->write(&data->bindCount, 1 * 4); // u32 bindCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pBinds); stream->write(&ptrval, 8);
    if (data->pBinds) { for (uint32_t i = 0; i < data->bindCount; i++) { vkUtilsPack_VkSparseMemoryBind(stream, data->pBinds + i); } } } // VkSparseMemoryBind pBinds
}

VkSparseImageOpaqueMemoryBindInfo* vkUtilsUnpack_VkSparseImageOpaqueMemoryBindInfo(android::base::InplaceStream* stream) {
    VkSparseImageOpaqueMemoryBindInfo* data = new VkSparseImageOpaqueMemoryBindInfo; memset(data, 0, sizeof(VkSparseImageOpaqueMemoryBindInfo)); 
    stream->read(&data->image, 1 * 8); // VkImage image
    stream->read(&data->bindCount, 1 * 4); // u32 bindCount
    { // VkSparseMemoryBind pBinds
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSparseMemoryBind* tmpArr = ptrval ? new VkSparseMemoryBind[data->bindCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->bindCount; i++) {
        VkSparseMemoryBind* tmpUnpacked = vkUtilsUnpack_VkSparseMemoryBind(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSparseMemoryBind)); delete tmpUnpacked; } }
        data->pBinds = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseImageMemoryBindInfo(const VkSparseImageMemoryBindInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkImage image
    res += 1 * 4; // u32 bindCount
    res += 8; if (data->pBinds) { res += data->bindCount * vkUtilsEncodingSize_VkSparseImageMemoryBind(data->pBinds); } // VkSparseImageMemoryBind pBinds
    return res;
}

void vkUtilsPack_VkSparseImageMemoryBindInfo(android::base::InplaceStream* stream, const VkSparseImageMemoryBindInfo* data) {
    if (!data) return;
    stream->write(&data->image, 1 * 8); // VkImage image
    stream->write(&data->bindCount, 1 * 4); // u32 bindCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pBinds); stream->write(&ptrval, 8);
    if (data->pBinds) { for (uint32_t i = 0; i < data->bindCount; i++) { vkUtilsPack_VkSparseImageMemoryBind(stream, data->pBinds + i); } } } // VkSparseImageMemoryBind pBinds
}

VkSparseImageMemoryBindInfo* vkUtilsUnpack_VkSparseImageMemoryBindInfo(android::base::InplaceStream* stream) {
    VkSparseImageMemoryBindInfo* data = new VkSparseImageMemoryBindInfo; memset(data, 0, sizeof(VkSparseImageMemoryBindInfo)); 
    stream->read(&data->image, 1 * 8); // VkImage image
    stream->read(&data->bindCount, 1 * 4); // u32 bindCount
    { // VkSparseImageMemoryBind pBinds
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSparseImageMemoryBind* tmpArr = ptrval ? new VkSparseImageMemoryBind[data->bindCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->bindCount; i++) {
        VkSparseImageMemoryBind* tmpUnpacked = vkUtilsUnpack_VkSparseImageMemoryBind(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSparseImageMemoryBind)); delete tmpUnpacked; } }
        data->pBinds = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkBindSparseInfo(const VkBindSparseInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 waitSemaphoreCount
    res += data->waitSemaphoreCount * 8; // VkSemaphore pWaitSemaphores
    res += 1 * 4; // u32 bufferBindCount
    res += 8; if (data->pBufferBinds) { res += data->bufferBindCount * vkUtilsEncodingSize_VkSparseBufferMemoryBindInfo(data->pBufferBinds); } // VkSparseBufferMemoryBindInfo pBufferBinds
    res += 1 * 4; // u32 imageOpaqueBindCount
    res += 8; if (data->pImageOpaqueBinds) { res += data->imageOpaqueBindCount * vkUtilsEncodingSize_VkSparseImageOpaqueMemoryBindInfo(data->pImageOpaqueBinds); } // VkSparseImageOpaqueMemoryBindInfo pImageOpaqueBinds
    res += 1 * 4; // u32 imageBindCount
    res += 8; if (data->pImageBinds) { res += data->imageBindCount * vkUtilsEncodingSize_VkSparseImageMemoryBindInfo(data->pImageBinds); } // VkSparseImageMemoryBindInfo pImageBinds
    res += 1 * 4; // u32 signalSemaphoreCount
    res += data->signalSemaphoreCount * 8; // VkSemaphore pSignalSemaphores
    return res;
}

void vkUtilsPack_VkBindSparseInfo(android::base::InplaceStream* stream, const VkBindSparseInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    stream->write(data->pWaitSemaphores, data->waitSemaphoreCount * 8); // VkSemaphore pWaitSemaphores
    stream->write(&data->bufferBindCount, 1 * 4); // u32 bufferBindCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pBufferBinds); stream->write(&ptrval, 8);
    if (data->pBufferBinds) { for (uint32_t i = 0; i < data->bufferBindCount; i++) { vkUtilsPack_VkSparseBufferMemoryBindInfo(stream, data->pBufferBinds + i); } } } // VkSparseBufferMemoryBindInfo pBufferBinds
    stream->write(&data->imageOpaqueBindCount, 1 * 4); // u32 imageOpaqueBindCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pImageOpaqueBinds); stream->write(&ptrval, 8);
    if (data->pImageOpaqueBinds) { for (uint32_t i = 0; i < data->imageOpaqueBindCount; i++) { vkUtilsPack_VkSparseImageOpaqueMemoryBindInfo(stream, data->pImageOpaqueBinds + i); } } } // VkSparseImageOpaqueMemoryBindInfo pImageOpaqueBinds
    stream->write(&data->imageBindCount, 1 * 4); // u32 imageBindCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pImageBinds); stream->write(&ptrval, 8);
    if (data->pImageBinds) { for (uint32_t i = 0; i < data->imageBindCount; i++) { vkUtilsPack_VkSparseImageMemoryBindInfo(stream, data->pImageBinds + i); } } } // VkSparseImageMemoryBindInfo pImageBinds
    stream->write(&data->signalSemaphoreCount, 1 * 4); // u32 signalSemaphoreCount
    stream->write(data->pSignalSemaphores, data->signalSemaphoreCount * 8); // VkSemaphore pSignalSemaphores
}

VkBindSparseInfo* vkUtilsUnpack_VkBindSparseInfo(android::base::InplaceStream* stream) {
    VkBindSparseInfo* data = new VkBindSparseInfo; memset(data, 0, sizeof(VkBindSparseInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    data->pWaitSemaphores = data->waitSemaphoreCount ? new VkSemaphore[data->waitSemaphoreCount] : nullptr; // VkSemaphore pWaitSemaphores
    stream->read((char*)data->pWaitSemaphores, data->waitSemaphoreCount * 8); // VkSemaphore pWaitSemaphores
    stream->read(&data->bufferBindCount, 1 * 4); // u32 bufferBindCount
    { // VkSparseBufferMemoryBindInfo pBufferBinds
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSparseBufferMemoryBindInfo* tmpArr = ptrval ? new VkSparseBufferMemoryBindInfo[data->bufferBindCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->bufferBindCount; i++) {
        VkSparseBufferMemoryBindInfo* tmpUnpacked = vkUtilsUnpack_VkSparseBufferMemoryBindInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSparseBufferMemoryBindInfo)); delete tmpUnpacked; } }
        data->pBufferBinds = tmpArr;
    }
    stream->read(&data->imageOpaqueBindCount, 1 * 4); // u32 imageOpaqueBindCount
    { // VkSparseImageOpaqueMemoryBindInfo pImageOpaqueBinds
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSparseImageOpaqueMemoryBindInfo* tmpArr = ptrval ? new VkSparseImageOpaqueMemoryBindInfo[data->imageOpaqueBindCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->imageOpaqueBindCount; i++) {
        VkSparseImageOpaqueMemoryBindInfo* tmpUnpacked = vkUtilsUnpack_VkSparseImageOpaqueMemoryBindInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSparseImageOpaqueMemoryBindInfo)); delete tmpUnpacked; } }
        data->pImageOpaqueBinds = tmpArr;
    }
    stream->read(&data->imageBindCount, 1 * 4); // u32 imageBindCount
    { // VkSparseImageMemoryBindInfo pImageBinds
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSparseImageMemoryBindInfo* tmpArr = ptrval ? new VkSparseImageMemoryBindInfo[data->imageBindCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->imageBindCount; i++) {
        VkSparseImageMemoryBindInfo* tmpUnpacked = vkUtilsUnpack_VkSparseImageMemoryBindInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSparseImageMemoryBindInfo)); delete tmpUnpacked; } }
        data->pImageBinds = tmpArr;
    }
    stream->read(&data->signalSemaphoreCount, 1 * 4); // u32 signalSemaphoreCount
    data->pSignalSemaphores = data->signalSemaphoreCount ? new VkSemaphore[data->signalSemaphoreCount] : nullptr; // VkSemaphore pSignalSemaphores
    stream->read((char*)data->pSignalSemaphores, data->signalSemaphoreCount * 8); // VkSemaphore pSignalSemaphores
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageSubresourceLayers(const VkImageSubresourceLayers* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkImageAspectFlags aspectMask
    res += 1 * 4; // u32 mipLevel
    res += 1 * 4; // u32 baseArrayLayer
    res += 1 * 4; // u32 layerCount
    return res;
}

void vkUtilsPack_VkImageSubresourceLayers(android::base::InplaceStream* stream, const VkImageSubresourceLayers* data) {
    if (!data) return;
    stream->write(&data->aspectMask, 1 * 4); // VkImageAspectFlags aspectMask
    stream->write(&data->mipLevel, 1 * 4); // u32 mipLevel
    stream->write(&data->baseArrayLayer, 1 * 4); // u32 baseArrayLayer
    stream->write(&data->layerCount, 1 * 4); // u32 layerCount
}

VkImageSubresourceLayers* vkUtilsUnpack_VkImageSubresourceLayers(android::base::InplaceStream* stream) {
    VkImageSubresourceLayers* data = new VkImageSubresourceLayers; memset(data, 0, sizeof(VkImageSubresourceLayers)); 
    stream->read(&data->aspectMask, 1 * 4); // VkImageAspectFlags aspectMask
    stream->read(&data->mipLevel, 1 * 4); // u32 mipLevel
    stream->read(&data->baseArrayLayer, 1 * 4); // u32 baseArrayLayer
    stream->read(&data->layerCount, 1 * 4); // u32 layerCount
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageCopy(const VkImageCopy* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->srcSubresource); // VkImageSubresourceLayers srcSubresource
    res += 1 * vkUtilsEncodingSize_VkOffset3D(&data->srcOffset); // VkOffset3D srcOffset
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->dstSubresource); // VkImageSubresourceLayers dstSubresource
    res += 1 * vkUtilsEncodingSize_VkOffset3D(&data->dstOffset); // VkOffset3D dstOffset
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->extent); // VkExtent3D extent
    return res;
}

void vkUtilsPack_VkImageCopy(android::base::InplaceStream* stream, const VkImageCopy* data) {
    if (!data) return;
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->srcSubresource); // VkImageSubresourceLayers srcSubresource
    vkUtilsPack_VkOffset3D(stream, &data->srcOffset); // VkOffset3D srcOffset
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->dstSubresource); // VkImageSubresourceLayers dstSubresource
    vkUtilsPack_VkOffset3D(stream, &data->dstOffset); // VkOffset3D dstOffset
    vkUtilsPack_VkExtent3D(stream, &data->extent); // VkExtent3D extent
}

VkImageCopy* vkUtilsUnpack_VkImageCopy(android::base::InplaceStream* stream) {
    VkImageCopy* data = new VkImageCopy; memset(data, 0, sizeof(VkImageCopy)); 
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->srcSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers srcSubresource
    { VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); memcpy(&data->srcOffset, tmpUnpacked, sizeof(VkOffset3D)); delete tmpUnpacked; } // VkOffset3D srcOffset
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->dstSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers dstSubresource
    { VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); memcpy(&data->dstOffset, tmpUnpacked, sizeof(VkOffset3D)); delete tmpUnpacked; } // VkOffset3D dstOffset
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->extent, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D extent
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageBlit(const VkImageBlit* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->srcSubresource); // VkImageSubresourceLayers srcSubresource
    res += 2 * vkUtilsEncodingSize_VkOffset3D(data->srcOffsets); // VkOffset3D srcOffsets
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->dstSubresource); // VkImageSubresourceLayers dstSubresource
    res += 2 * vkUtilsEncodingSize_VkOffset3D(data->dstOffsets); // VkOffset3D dstOffsets
    return res;
}

void vkUtilsPack_VkImageBlit(android::base::InplaceStream* stream, const VkImageBlit* data) {
    if (!data) return;
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->srcSubresource); // VkImageSubresourceLayers srcSubresource
    {
    { for (uint32_t i = 0; i < 2; i++) { vkUtilsPack_VkOffset3D(stream, data->srcOffsets + i); } } } // VkOffset3D srcOffsets
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->dstSubresource); // VkImageSubresourceLayers dstSubresource
    {
    { for (uint32_t i = 0; i < 2; i++) { vkUtilsPack_VkOffset3D(stream, data->dstOffsets + i); } } } // VkOffset3D dstOffsets
}

VkImageBlit* vkUtilsUnpack_VkImageBlit(android::base::InplaceStream* stream) {
    VkImageBlit* data = new VkImageBlit; memset(data, 0, sizeof(VkImageBlit)); 
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->srcSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers srcSubresource
    { // VkOffset3D srcOffsets
        { for (uint32_t i = 0; i < 2; i++) {
        VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); data->srcOffsets[i] = *tmpUnpacked; } }
    }
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->dstSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers dstSubresource
    { // VkOffset3D dstOffsets
        { for (uint32_t i = 0; i < 2; i++) {
        VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); data->dstOffsets[i] = *tmpUnpacked; } }
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkBufferImageCopy(const VkBufferImageCopy* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDeviceSize bufferOffset
    res += 1 * 4; // u32 bufferRowLength
    res += 1 * 4; // u32 bufferImageHeight
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->imageSubresource); // VkImageSubresourceLayers imageSubresource
    res += 1 * vkUtilsEncodingSize_VkOffset3D(&data->imageOffset); // VkOffset3D imageOffset
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->imageExtent); // VkExtent3D imageExtent
    return res;
}

void vkUtilsPack_VkBufferImageCopy(android::base::InplaceStream* stream, const VkBufferImageCopy* data) {
    if (!data) return;
    stream->write(&data->bufferOffset, 1 * 8); // VkDeviceSize bufferOffset
    stream->write(&data->bufferRowLength, 1 * 4); // u32 bufferRowLength
    stream->write(&data->bufferImageHeight, 1 * 4); // u32 bufferImageHeight
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->imageSubresource); // VkImageSubresourceLayers imageSubresource
    vkUtilsPack_VkOffset3D(stream, &data->imageOffset); // VkOffset3D imageOffset
    vkUtilsPack_VkExtent3D(stream, &data->imageExtent); // VkExtent3D imageExtent
}

VkBufferImageCopy* vkUtilsUnpack_VkBufferImageCopy(android::base::InplaceStream* stream) {
    VkBufferImageCopy* data = new VkBufferImageCopy; memset(data, 0, sizeof(VkBufferImageCopy)); 
    stream->read(&data->bufferOffset, 1 * 8); // VkDeviceSize bufferOffset
    stream->read(&data->bufferRowLength, 1 * 4); // u32 bufferRowLength
    stream->read(&data->bufferImageHeight, 1 * 4); // u32 bufferImageHeight
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->imageSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers imageSubresource
    { VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); memcpy(&data->imageOffset, tmpUnpacked, sizeof(VkOffset3D)); delete tmpUnpacked; } // VkOffset3D imageOffset
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->imageExtent, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D imageExtent
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageResolve(const VkImageResolve* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->srcSubresource); // VkImageSubresourceLayers srcSubresource
    res += 1 * vkUtilsEncodingSize_VkOffset3D(&data->srcOffset); // VkOffset3D srcOffset
    res += 1 * vkUtilsEncodingSize_VkImageSubresourceLayers(&data->dstSubresource); // VkImageSubresourceLayers dstSubresource
    res += 1 * vkUtilsEncodingSize_VkOffset3D(&data->dstOffset); // VkOffset3D dstOffset
    res += 1 * vkUtilsEncodingSize_VkExtent3D(&data->extent); // VkExtent3D extent
    return res;
}

void vkUtilsPack_VkImageResolve(android::base::InplaceStream* stream, const VkImageResolve* data) {
    if (!data) return;
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->srcSubresource); // VkImageSubresourceLayers srcSubresource
    vkUtilsPack_VkOffset3D(stream, &data->srcOffset); // VkOffset3D srcOffset
    vkUtilsPack_VkImageSubresourceLayers(stream, &data->dstSubresource); // VkImageSubresourceLayers dstSubresource
    vkUtilsPack_VkOffset3D(stream, &data->dstOffset); // VkOffset3D dstOffset
    vkUtilsPack_VkExtent3D(stream, &data->extent); // VkExtent3D extent
}

VkImageResolve* vkUtilsUnpack_VkImageResolve(android::base::InplaceStream* stream) {
    VkImageResolve* data = new VkImageResolve; memset(data, 0, sizeof(VkImageResolve)); 
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->srcSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers srcSubresource
    { VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); memcpy(&data->srcOffset, tmpUnpacked, sizeof(VkOffset3D)); delete tmpUnpacked; } // VkOffset3D srcOffset
    { VkImageSubresourceLayers* tmpUnpacked = vkUtilsUnpack_VkImageSubresourceLayers(stream); memcpy(&data->dstSubresource, tmpUnpacked, sizeof(VkImageSubresourceLayers)); delete tmpUnpacked; } // VkImageSubresourceLayers dstSubresource
    { VkOffset3D* tmpUnpacked = vkUtilsUnpack_VkOffset3D(stream); memcpy(&data->dstOffset, tmpUnpacked, sizeof(VkOffset3D)); delete tmpUnpacked; } // VkOffset3D dstOffset
    { VkExtent3D* tmpUnpacked = vkUtilsUnpack_VkExtent3D(stream); memcpy(&data->extent, tmpUnpacked, sizeof(VkExtent3D)); delete tmpUnpacked; } // VkExtent3D extent
    return data;
}

uint32_t vkUtilsEncodingSize_VkShaderModuleCreateInfo(const VkShaderModuleCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkShaderModuleCreateFlags flags
    res += 1 * 4; // size_t codeSize
    res += data->codeSize * 1; // u32 pCode
    return res;
}

void vkUtilsPack_VkShaderModuleCreateInfo(android::base::InplaceStream* stream, const VkShaderModuleCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkShaderModuleCreateFlags flags
    stream->write(&data->codeSize, 1 * 4); // size_t codeSize
    stream->write(data->pCode, data->codeSize * 1); // u32 pCode
}

VkShaderModuleCreateInfo* vkUtilsUnpack_VkShaderModuleCreateInfo(android::base::InplaceStream* stream) {
    VkShaderModuleCreateInfo* data = new VkShaderModuleCreateInfo; memset(data, 0, sizeof(VkShaderModuleCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkShaderModuleCreateFlags flags
    stream->read(&data->codeSize, 1 * 4); // size_t codeSize
    data->pCode = data->codeSize ? new u32[data->codeSize] : nullptr; // u32 pCode
    stream->read((char*)data->pCode, data->codeSize * 1); // u32 pCode
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorSetLayoutBinding(const VkDescriptorSetLayoutBinding* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 binding
    res += 1 * 4; // VkDescriptorType descriptorType
    res += 1 * 4; // u32 descriptorCount
    res += 1 * 4; // VkShaderStageFlags stageFlags
    res += data->descriptorCount * 8; // VkSampler pImmutableSamplers
    return res;
}

void vkUtilsPack_VkDescriptorSetLayoutBinding(android::base::InplaceStream* stream, const VkDescriptorSetLayoutBinding* data) {
    if (!data) return;
    stream->write(&data->binding, 1 * 4); // u32 binding
    stream->write(&data->descriptorType, 1 * 4); // VkDescriptorType descriptorType
    stream->write(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    stream->write(&data->stageFlags, 1 * 4); // VkShaderStageFlags stageFlags
    stream->write(data->pImmutableSamplers, data->descriptorCount * 8); // VkSampler pImmutableSamplers
}

VkDescriptorSetLayoutBinding* vkUtilsUnpack_VkDescriptorSetLayoutBinding(android::base::InplaceStream* stream) {
    VkDescriptorSetLayoutBinding* data = new VkDescriptorSetLayoutBinding; memset(data, 0, sizeof(VkDescriptorSetLayoutBinding)); 
    stream->read(&data->binding, 1 * 4); // u32 binding
    stream->read(&data->descriptorType, 1 * 4); // VkDescriptorType descriptorType
    stream->read(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    stream->read(&data->stageFlags, 1 * 4); // VkShaderStageFlags stageFlags
    data->pImmutableSamplers = data->descriptorCount ? new VkSampler[data->descriptorCount] : nullptr; // VkSampler pImmutableSamplers
    stream->read((char*)data->pImmutableSamplers, data->descriptorCount * 8); // VkSampler pImmutableSamplers
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDescriptorSetLayoutCreateFlags flags
    res += 1 * 4; // u32 bindingCount
    res += 8; if (data->pBindings) { res += data->bindingCount * vkUtilsEncodingSize_VkDescriptorSetLayoutBinding(data->pBindings); } // VkDescriptorSetLayoutBinding pBindings
    return res;
}

void vkUtilsPack_VkDescriptorSetLayoutCreateInfo(android::base::InplaceStream* stream, const VkDescriptorSetLayoutCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDescriptorSetLayoutCreateFlags flags
    stream->write(&data->bindingCount, 1 * 4); // u32 bindingCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pBindings); stream->write(&ptrval, 8);
    if (data->pBindings) { for (uint32_t i = 0; i < data->bindingCount; i++) { vkUtilsPack_VkDescriptorSetLayoutBinding(stream, data->pBindings + i); } } } // VkDescriptorSetLayoutBinding pBindings
}

VkDescriptorSetLayoutCreateInfo* vkUtilsUnpack_VkDescriptorSetLayoutCreateInfo(android::base::InplaceStream* stream) {
    VkDescriptorSetLayoutCreateInfo* data = new VkDescriptorSetLayoutCreateInfo; memset(data, 0, sizeof(VkDescriptorSetLayoutCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDescriptorSetLayoutCreateFlags flags
    stream->read(&data->bindingCount, 1 * 4); // u32 bindingCount
    { // VkDescriptorSetLayoutBinding pBindings
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkDescriptorSetLayoutBinding* tmpArr = ptrval ? new VkDescriptorSetLayoutBinding[data->bindingCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->bindingCount; i++) {
        VkDescriptorSetLayoutBinding* tmpUnpacked = vkUtilsUnpack_VkDescriptorSetLayoutBinding(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkDescriptorSetLayoutBinding)); delete tmpUnpacked; } }
        data->pBindings = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorPoolSize(const VkDescriptorPoolSize* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkDescriptorType type
    res += 1 * 4; // u32 descriptorCount
    return res;
}

void vkUtilsPack_VkDescriptorPoolSize(android::base::InplaceStream* stream, const VkDescriptorPoolSize* data) {
    if (!data) return;
    stream->write(&data->type, 1 * 4); // VkDescriptorType type
    stream->write(&data->descriptorCount, 1 * 4); // u32 descriptorCount
}

VkDescriptorPoolSize* vkUtilsUnpack_VkDescriptorPoolSize(android::base::InplaceStream* stream) {
    VkDescriptorPoolSize* data = new VkDescriptorPoolSize; memset(data, 0, sizeof(VkDescriptorPoolSize)); 
    stream->read(&data->type, 1 * 4); // VkDescriptorType type
    stream->read(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorPoolCreateInfo(const VkDescriptorPoolCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDescriptorPoolCreateFlags flags
    res += 1 * 4; // u32 maxSets
    res += 1 * 4; // u32 poolSizeCount
    res += 8; if (data->pPoolSizes) { res += data->poolSizeCount * vkUtilsEncodingSize_VkDescriptorPoolSize(data->pPoolSizes); } // VkDescriptorPoolSize pPoolSizes
    return res;
}

void vkUtilsPack_VkDescriptorPoolCreateInfo(android::base::InplaceStream* stream, const VkDescriptorPoolCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDescriptorPoolCreateFlags flags
    stream->write(&data->maxSets, 1 * 4); // u32 maxSets
    stream->write(&data->poolSizeCount, 1 * 4); // u32 poolSizeCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pPoolSizes); stream->write(&ptrval, 8);
    if (data->pPoolSizes) { for (uint32_t i = 0; i < data->poolSizeCount; i++) { vkUtilsPack_VkDescriptorPoolSize(stream, data->pPoolSizes + i); } } } // VkDescriptorPoolSize pPoolSizes
}

VkDescriptorPoolCreateInfo* vkUtilsUnpack_VkDescriptorPoolCreateInfo(android::base::InplaceStream* stream) {
    VkDescriptorPoolCreateInfo* data = new VkDescriptorPoolCreateInfo; memset(data, 0, sizeof(VkDescriptorPoolCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDescriptorPoolCreateFlags flags
    stream->read(&data->maxSets, 1 * 4); // u32 maxSets
    stream->read(&data->poolSizeCount, 1 * 4); // u32 poolSizeCount
    { // VkDescriptorPoolSize pPoolSizes
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkDescriptorPoolSize* tmpArr = ptrval ? new VkDescriptorPoolSize[data->poolSizeCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->poolSizeCount; i++) {
        VkDescriptorPoolSize* tmpUnpacked = vkUtilsUnpack_VkDescriptorPoolSize(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkDescriptorPoolSize)); delete tmpUnpacked; } }
        data->pPoolSizes = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorSetAllocateInfo(const VkDescriptorSetAllocateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkDescriptorPool descriptorPool
    res += 1 * 4; // u32 descriptorSetCount
    res += data->descriptorSetCount * 8; // VkDescriptorSetLayout pSetLayouts
    return res;
}

void vkUtilsPack_VkDescriptorSetAllocateInfo(android::base::InplaceStream* stream, const VkDescriptorSetAllocateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->descriptorPool, 1 * 8); // VkDescriptorPool descriptorPool
    stream->write(&data->descriptorSetCount, 1 * 4); // u32 descriptorSetCount
    stream->write(data->pSetLayouts, data->descriptorSetCount * 8); // VkDescriptorSetLayout pSetLayouts
}

VkDescriptorSetAllocateInfo* vkUtilsUnpack_VkDescriptorSetAllocateInfo(android::base::InplaceStream* stream) {
    VkDescriptorSetAllocateInfo* data = new VkDescriptorSetAllocateInfo; memset(data, 0, sizeof(VkDescriptorSetAllocateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->descriptorPool, 1 * 8); // VkDescriptorPool descriptorPool
    stream->read(&data->descriptorSetCount, 1 * 4); // u32 descriptorSetCount
    data->pSetLayouts = data->descriptorSetCount ? new VkDescriptorSetLayout[data->descriptorSetCount] : nullptr; // VkDescriptorSetLayout pSetLayouts
    stream->read((char*)data->pSetLayouts, data->descriptorSetCount * 8); // VkDescriptorSetLayout pSetLayouts
    return data;
}

uint32_t vkUtilsEncodingSize_VkSpecializationMapEntry(const VkSpecializationMapEntry* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 constantID
    res += 1 * 4; // u32 offset
    res += 1 * 4; // size_t size
    return res;
}

void vkUtilsPack_VkSpecializationMapEntry(android::base::InplaceStream* stream, const VkSpecializationMapEntry* data) {
    if (!data) return;
    stream->write(&data->constantID, 1 * 4); // u32 constantID
    stream->write(&data->offset, 1 * 4); // u32 offset
    stream->write(&data->size, 1 * 4); // size_t size
}

VkSpecializationMapEntry* vkUtilsUnpack_VkSpecializationMapEntry(android::base::InplaceStream* stream) {
    VkSpecializationMapEntry* data = new VkSpecializationMapEntry; memset(data, 0, sizeof(VkSpecializationMapEntry)); 
    stream->read(&data->constantID, 1 * 4); // u32 constantID
    stream->read(&data->offset, 1 * 4); // u32 offset
    stream->read(&data->size, 1 * 4); // size_t size
    return data;
}

uint32_t vkUtilsEncodingSize_VkSpecializationInfo(const VkSpecializationInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 mapEntryCount
    res += 8; if (data->pMapEntries) { res += data->mapEntryCount * vkUtilsEncodingSize_VkSpecializationMapEntry(data->pMapEntries); } // VkSpecializationMapEntry pMapEntries
    res += 1 * 4; // size_t dataSize
    res += data->dataSize * 8; // void pData
    return res;
}

void vkUtilsPack_VkSpecializationInfo(android::base::InplaceStream* stream, const VkSpecializationInfo* data) {
    if (!data) return;
    stream->write(&data->mapEntryCount, 1 * 4); // u32 mapEntryCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pMapEntries); stream->write(&ptrval, 8);
    if (data->pMapEntries) { for (uint32_t i = 0; i < data->mapEntryCount; i++) { vkUtilsPack_VkSpecializationMapEntry(stream, data->pMapEntries + i); } } } // VkSpecializationMapEntry pMapEntries
    stream->write(&data->dataSize, 1 * 4); // size_t dataSize
    stream->write(&data->pData, 8); // void pData
}

VkSpecializationInfo* vkUtilsUnpack_VkSpecializationInfo(android::base::InplaceStream* stream) {
    VkSpecializationInfo* data = new VkSpecializationInfo; memset(data, 0, sizeof(VkSpecializationInfo)); 
    stream->read(&data->mapEntryCount, 1 * 4); // u32 mapEntryCount
    { // VkSpecializationMapEntry pMapEntries
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSpecializationMapEntry* tmpArr = ptrval ? new VkSpecializationMapEntry[data->mapEntryCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->mapEntryCount; i++) {
        VkSpecializationMapEntry* tmpUnpacked = vkUtilsUnpack_VkSpecializationMapEntry(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSpecializationMapEntry)); delete tmpUnpacked; } }
        data->pMapEntries = tmpArr;
    }
    stream->read(&data->dataSize, 1 * 4); // size_t dataSize
    stream->read(&data->pData, 8); // void pData
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineShaderStageCreateInfo(const VkPipelineShaderStageCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineShaderStageCreateFlags flags
    res += 1 * 4; // VkShaderStageFlagBits stage
    res += 1 * 8; // VkShaderModule module
    res += string1d_len(data->pName) * 1; // char pName
    res += 8; if (data->pSpecializationInfo) { res += 1 * vkUtilsEncodingSize_VkSpecializationInfo(data->pSpecializationInfo); } // VkSpecializationInfo pSpecializationInfo
    return res;
}

void vkUtilsPack_VkPipelineShaderStageCreateInfo(android::base::InplaceStream* stream, const VkPipelineShaderStageCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineShaderStageCreateFlags flags
    stream->write(&data->stage, 1 * 4); // VkShaderStageFlagBits stage
    stream->write(&data->module, 1 * 8); // VkShaderModule module
    string1d_pack(stream, data->pName); // char pName
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pSpecializationInfo); stream->write(&ptrval, 8);
    if (data->pSpecializationInfo) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkSpecializationInfo(stream, data->pSpecializationInfo + i); } } } // VkSpecializationInfo pSpecializationInfo
}

VkPipelineShaderStageCreateInfo* vkUtilsUnpack_VkPipelineShaderStageCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineShaderStageCreateInfo* data = new VkPipelineShaderStageCreateInfo; memset(data, 0, sizeof(VkPipelineShaderStageCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineShaderStageCreateFlags flags
    stream->read(&data->stage, 1 * 4); // VkShaderStageFlagBits stage
    stream->read(&data->module, 1 * 8); // VkShaderModule module
    data->pName = string1d_unpack(stream); // char pName
    { // VkSpecializationInfo pSpecializationInfo
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSpecializationInfo* tmpArr = ptrval ? new VkSpecializationInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkSpecializationInfo* tmpUnpacked = vkUtilsUnpack_VkSpecializationInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSpecializationInfo)); delete tmpUnpacked; } }
        data->pSpecializationInfo = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkComputePipelineCreateInfo(const VkComputePipelineCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineCreateFlags flags
    res += 1 * vkUtilsEncodingSize_VkPipelineShaderStageCreateInfo(&data->stage); // VkPipelineShaderStageCreateInfo stage
    res += 1 * 8; // VkPipelineLayout layout
    res += 1 * 8; // VkPipeline basePipelineHandle
    res += 1 * 4; // s32 basePipelineIndex
    return res;
}

void vkUtilsPack_VkComputePipelineCreateInfo(android::base::InplaceStream* stream, const VkComputePipelineCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineCreateFlags flags
    vkUtilsPack_VkPipelineShaderStageCreateInfo(stream, &data->stage); // VkPipelineShaderStageCreateInfo stage
    stream->write(&data->layout, 1 * 8); // VkPipelineLayout layout
    stream->write(&data->basePipelineHandle, 1 * 8); // VkPipeline basePipelineHandle
    stream->write(&data->basePipelineIndex, 1 * 4); // s32 basePipelineIndex
}

VkComputePipelineCreateInfo* vkUtilsUnpack_VkComputePipelineCreateInfo(android::base::InplaceStream* stream) {
    VkComputePipelineCreateInfo* data = new VkComputePipelineCreateInfo; memset(data, 0, sizeof(VkComputePipelineCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineCreateFlags flags
    { VkPipelineShaderStageCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineShaderStageCreateInfo(stream); memcpy(&data->stage, tmpUnpacked, sizeof(VkPipelineShaderStageCreateInfo)); delete tmpUnpacked; } // VkPipelineShaderStageCreateInfo stage
    stream->read(&data->layout, 1 * 8); // VkPipelineLayout layout
    stream->read(&data->basePipelineHandle, 1 * 8); // VkPipeline basePipelineHandle
    stream->read(&data->basePipelineIndex, 1 * 4); // s32 basePipelineIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkVertexInputBindingDescription(const VkVertexInputBindingDescription* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 binding
    res += 1 * 4; // u32 stride
    res += 1 * 4; // VkVertexInputRate inputRate
    return res;
}

void vkUtilsPack_VkVertexInputBindingDescription(android::base::InplaceStream* stream, const VkVertexInputBindingDescription* data) {
    if (!data) return;
    stream->write(&data->binding, 1 * 4); // u32 binding
    stream->write(&data->stride, 1 * 4); // u32 stride
    stream->write(&data->inputRate, 1 * 4); // VkVertexInputRate inputRate
}

VkVertexInputBindingDescription* vkUtilsUnpack_VkVertexInputBindingDescription(android::base::InplaceStream* stream) {
    VkVertexInputBindingDescription* data = new VkVertexInputBindingDescription; memset(data, 0, sizeof(VkVertexInputBindingDescription)); 
    stream->read(&data->binding, 1 * 4); // u32 binding
    stream->read(&data->stride, 1 * 4); // u32 stride
    stream->read(&data->inputRate, 1 * 4); // VkVertexInputRate inputRate
    return data;
}

uint32_t vkUtilsEncodingSize_VkVertexInputAttributeDescription(const VkVertexInputAttributeDescription* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 location
    res += 1 * 4; // u32 binding
    res += 1 * 4; // VkFormat format
    res += 1 * 4; // u32 offset
    return res;
}

void vkUtilsPack_VkVertexInputAttributeDescription(android::base::InplaceStream* stream, const VkVertexInputAttributeDescription* data) {
    if (!data) return;
    stream->write(&data->location, 1 * 4); // u32 location
    stream->write(&data->binding, 1 * 4); // u32 binding
    stream->write(&data->format, 1 * 4); // VkFormat format
    stream->write(&data->offset, 1 * 4); // u32 offset
}

VkVertexInputAttributeDescription* vkUtilsUnpack_VkVertexInputAttributeDescription(android::base::InplaceStream* stream) {
    VkVertexInputAttributeDescription* data = new VkVertexInputAttributeDescription; memset(data, 0, sizeof(VkVertexInputAttributeDescription)); 
    stream->read(&data->location, 1 * 4); // u32 location
    stream->read(&data->binding, 1 * 4); // u32 binding
    stream->read(&data->format, 1 * 4); // VkFormat format
    stream->read(&data->offset, 1 * 4); // u32 offset
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineVertexInputStateCreateInfo(const VkPipelineVertexInputStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineVertexInputStateCreateFlags flags
    res += 1 * 4; // u32 vertexBindingDescriptionCount
    res += 8; if (data->pVertexBindingDescriptions) { res += data->vertexBindingDescriptionCount * vkUtilsEncodingSize_VkVertexInputBindingDescription(data->pVertexBindingDescriptions); } // VkVertexInputBindingDescription pVertexBindingDescriptions
    res += 1 * 4; // u32 vertexAttributeDescriptionCount
    res += 8; if (data->pVertexAttributeDescriptions) { res += data->vertexAttributeDescriptionCount * vkUtilsEncodingSize_VkVertexInputAttributeDescription(data->pVertexAttributeDescriptions); } // VkVertexInputAttributeDescription pVertexAttributeDescriptions
    return res;
}

void vkUtilsPack_VkPipelineVertexInputStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineVertexInputStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineVertexInputStateCreateFlags flags
    stream->write(&data->vertexBindingDescriptionCount, 1 * 4); // u32 vertexBindingDescriptionCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pVertexBindingDescriptions); stream->write(&ptrval, 8);
    if (data->pVertexBindingDescriptions) { for (uint32_t i = 0; i < data->vertexBindingDescriptionCount; i++) { vkUtilsPack_VkVertexInputBindingDescription(stream, data->pVertexBindingDescriptions + i); } } } // VkVertexInputBindingDescription pVertexBindingDescriptions
    stream->write(&data->vertexAttributeDescriptionCount, 1 * 4); // u32 vertexAttributeDescriptionCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pVertexAttributeDescriptions); stream->write(&ptrval, 8);
    if (data->pVertexAttributeDescriptions) { for (uint32_t i = 0; i < data->vertexAttributeDescriptionCount; i++) { vkUtilsPack_VkVertexInputAttributeDescription(stream, data->pVertexAttributeDescriptions + i); } } } // VkVertexInputAttributeDescription pVertexAttributeDescriptions
}

VkPipelineVertexInputStateCreateInfo* vkUtilsUnpack_VkPipelineVertexInputStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineVertexInputStateCreateInfo* data = new VkPipelineVertexInputStateCreateInfo; memset(data, 0, sizeof(VkPipelineVertexInputStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineVertexInputStateCreateFlags flags
    stream->read(&data->vertexBindingDescriptionCount, 1 * 4); // u32 vertexBindingDescriptionCount
    { // VkVertexInputBindingDescription pVertexBindingDescriptions
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkVertexInputBindingDescription* tmpArr = ptrval ? new VkVertexInputBindingDescription[data->vertexBindingDescriptionCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->vertexBindingDescriptionCount; i++) {
        VkVertexInputBindingDescription* tmpUnpacked = vkUtilsUnpack_VkVertexInputBindingDescription(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkVertexInputBindingDescription)); delete tmpUnpacked; } }
        data->pVertexBindingDescriptions = tmpArr;
    }
    stream->read(&data->vertexAttributeDescriptionCount, 1 * 4); // u32 vertexAttributeDescriptionCount
    { // VkVertexInputAttributeDescription pVertexAttributeDescriptions
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkVertexInputAttributeDescription* tmpArr = ptrval ? new VkVertexInputAttributeDescription[data->vertexAttributeDescriptionCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->vertexAttributeDescriptionCount; i++) {
        VkVertexInputAttributeDescription* tmpUnpacked = vkUtilsUnpack_VkVertexInputAttributeDescription(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkVertexInputAttributeDescription)); delete tmpUnpacked; } }
        data->pVertexAttributeDescriptions = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineInputAssemblyStateCreateInfo(const VkPipelineInputAssemblyStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineInputAssemblyStateCreateFlags flags
    res += 1 * 4; // VkPrimitiveTopology topology
    res += 1 * 4; // VkBool32 primitiveRestartEnable
    return res;
}

void vkUtilsPack_VkPipelineInputAssemblyStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineInputAssemblyStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineInputAssemblyStateCreateFlags flags
    stream->write(&data->topology, 1 * 4); // VkPrimitiveTopology topology
    stream->write(&data->primitiveRestartEnable, 1 * 4); // VkBool32 primitiveRestartEnable
}

VkPipelineInputAssemblyStateCreateInfo* vkUtilsUnpack_VkPipelineInputAssemblyStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineInputAssemblyStateCreateInfo* data = new VkPipelineInputAssemblyStateCreateInfo; memset(data, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineInputAssemblyStateCreateFlags flags
    stream->read(&data->topology, 1 * 4); // VkPrimitiveTopology topology
    stream->read(&data->primitiveRestartEnable, 1 * 4); // VkBool32 primitiveRestartEnable
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineTessellationStateCreateInfo(const VkPipelineTessellationStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineTessellationStateCreateFlags flags
    res += 1 * 4; // u32 patchControlPoints
    return res;
}

void vkUtilsPack_VkPipelineTessellationStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineTessellationStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineTessellationStateCreateFlags flags
    stream->write(&data->patchControlPoints, 1 * 4); // u32 patchControlPoints
}

VkPipelineTessellationStateCreateInfo* vkUtilsUnpack_VkPipelineTessellationStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineTessellationStateCreateInfo* data = new VkPipelineTessellationStateCreateInfo; memset(data, 0, sizeof(VkPipelineTessellationStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineTessellationStateCreateFlags flags
    stream->read(&data->patchControlPoints, 1 * 4); // u32 patchControlPoints
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineViewportStateCreateInfo(const VkPipelineViewportStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineViewportStateCreateFlags flags
    res += 1 * 4; // u32 viewportCount
    res += 8; if (data->pViewports) { res += data->viewportCount * vkUtilsEncodingSize_VkViewport(data->pViewports); } // VkViewport pViewports
    res += 1 * 4; // u32 scissorCount
    res += 8; if (data->pScissors) { res += data->scissorCount * vkUtilsEncodingSize_VkRect2D(data->pScissors); } // VkRect2D pScissors
    return res;
}

void vkUtilsPack_VkPipelineViewportStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineViewportStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineViewportStateCreateFlags flags
    stream->write(&data->viewportCount, 1 * 4); // u32 viewportCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pViewports); stream->write(&ptrval, 8);
    if (data->pViewports) { for (uint32_t i = 0; i < data->viewportCount; i++) { vkUtilsPack_VkViewport(stream, data->pViewports + i); } } } // VkViewport pViewports
    stream->write(&data->scissorCount, 1 * 4); // u32 scissorCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pScissors); stream->write(&ptrval, 8);
    if (data->pScissors) { for (uint32_t i = 0; i < data->scissorCount; i++) { vkUtilsPack_VkRect2D(stream, data->pScissors + i); } } } // VkRect2D pScissors
}

VkPipelineViewportStateCreateInfo* vkUtilsUnpack_VkPipelineViewportStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineViewportStateCreateInfo* data = new VkPipelineViewportStateCreateInfo; memset(data, 0, sizeof(VkPipelineViewportStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineViewportStateCreateFlags flags
    stream->read(&data->viewportCount, 1 * 4); // u32 viewportCount
    { // VkViewport pViewports
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkViewport* tmpArr = ptrval ? new VkViewport[data->viewportCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->viewportCount; i++) {
        VkViewport* tmpUnpacked = vkUtilsUnpack_VkViewport(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkViewport)); delete tmpUnpacked; } }
        data->pViewports = tmpArr;
    }
    stream->read(&data->scissorCount, 1 * 4); // u32 scissorCount
    { // VkRect2D pScissors
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkRect2D* tmpArr = ptrval ? new VkRect2D[data->scissorCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->scissorCount; i++) {
        VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } }
        data->pScissors = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineRasterizationStateCreateInfo(const VkPipelineRasterizationStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineRasterizationStateCreateFlags flags
    res += 1 * 4; // VkBool32 depthClampEnable
    res += 1 * 4; // VkBool32 rasterizerDiscardEnable
    res += 1 * 4; // VkPolygonMode polygonMode
    res += 1 * 4; // VkCullModeFlags cullMode
    res += 1 * 4; // VkFrontFace frontFace
    res += 1 * 4; // VkBool32 depthBiasEnable
    res += 1 * 4; // f32 depthBiasConstantFactor
    res += 1 * 4; // f32 depthBiasClamp
    res += 1 * 4; // f32 depthBiasSlopeFactor
    res += 1 * 4; // f32 lineWidth
    return res;
}

void vkUtilsPack_VkPipelineRasterizationStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineRasterizationStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineRasterizationStateCreateFlags flags
    stream->write(&data->depthClampEnable, 1 * 4); // VkBool32 depthClampEnable
    stream->write(&data->rasterizerDiscardEnable, 1 * 4); // VkBool32 rasterizerDiscardEnable
    stream->write(&data->polygonMode, 1 * 4); // VkPolygonMode polygonMode
    stream->write(&data->cullMode, 1 * 4); // VkCullModeFlags cullMode
    stream->write(&data->frontFace, 1 * 4); // VkFrontFace frontFace
    stream->write(&data->depthBiasEnable, 1 * 4); // VkBool32 depthBiasEnable
    stream->write(&data->depthBiasConstantFactor, 1 * 4); // f32 depthBiasConstantFactor
    stream->write(&data->depthBiasClamp, 1 * 4); // f32 depthBiasClamp
    stream->write(&data->depthBiasSlopeFactor, 1 * 4); // f32 depthBiasSlopeFactor
    stream->write(&data->lineWidth, 1 * 4); // f32 lineWidth
}

VkPipelineRasterizationStateCreateInfo* vkUtilsUnpack_VkPipelineRasterizationStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineRasterizationStateCreateInfo* data = new VkPipelineRasterizationStateCreateInfo; memset(data, 0, sizeof(VkPipelineRasterizationStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineRasterizationStateCreateFlags flags
    stream->read(&data->depthClampEnable, 1 * 4); // VkBool32 depthClampEnable
    stream->read(&data->rasterizerDiscardEnable, 1 * 4); // VkBool32 rasterizerDiscardEnable
    stream->read(&data->polygonMode, 1 * 4); // VkPolygonMode polygonMode
    stream->read(&data->cullMode, 1 * 4); // VkCullModeFlags cullMode
    stream->read(&data->frontFace, 1 * 4); // VkFrontFace frontFace
    stream->read(&data->depthBiasEnable, 1 * 4); // VkBool32 depthBiasEnable
    stream->read(&data->depthBiasConstantFactor, 1 * 4); // f32 depthBiasConstantFactor
    stream->read(&data->depthBiasClamp, 1 * 4); // f32 depthBiasClamp
    stream->read(&data->depthBiasSlopeFactor, 1 * 4); // f32 depthBiasSlopeFactor
    stream->read(&data->lineWidth, 1 * 4); // f32 lineWidth
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineMultisampleStateCreateInfo(const VkPipelineMultisampleStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineMultisampleStateCreateFlags flags
    res += 1 * 4; // VkSampleCountFlagBits rasterizationSamples
    res += 1 * 4; // VkBool32 sampleShadingEnable
    res += 1 * 4; // f32 minSampleShading
    res += 1 * 4; // VkSampleMask pSampleMask
    res += 1 * 4; // VkBool32 alphaToCoverageEnable
    res += 1 * 4; // VkBool32 alphaToOneEnable
    return res;
}

void vkUtilsPack_VkPipelineMultisampleStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineMultisampleStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineMultisampleStateCreateFlags flags
    stream->write(&data->rasterizationSamples, 1 * 4); // VkSampleCountFlagBits rasterizationSamples
    stream->write(&data->sampleShadingEnable, 1 * 4); // VkBool32 sampleShadingEnable
    stream->write(&data->minSampleShading, 1 * 4); // f32 minSampleShading
    stream->write(data->pSampleMask, 1 * 4); // VkSampleMask pSampleMask
    stream->write(&data->alphaToCoverageEnable, 1 * 4); // VkBool32 alphaToCoverageEnable
    stream->write(&data->alphaToOneEnable, 1 * 4); // VkBool32 alphaToOneEnable
}

VkPipelineMultisampleStateCreateInfo* vkUtilsUnpack_VkPipelineMultisampleStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineMultisampleStateCreateInfo* data = new VkPipelineMultisampleStateCreateInfo; memset(data, 0, sizeof(VkPipelineMultisampleStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineMultisampleStateCreateFlags flags
    stream->read(&data->rasterizationSamples, 1 * 4); // VkSampleCountFlagBits rasterizationSamples
    stream->read(&data->sampleShadingEnable, 1 * 4); // VkBool32 sampleShadingEnable
    stream->read(&data->minSampleShading, 1 * 4); // f32 minSampleShading
    data->pSampleMask = 1 ? new VkSampleMask[1] : nullptr; // VkSampleMask pSampleMask
    stream->read((char*)data->pSampleMask, 1 * 4); // VkSampleMask pSampleMask
    stream->read(&data->alphaToCoverageEnable, 1 * 4); // VkBool32 alphaToCoverageEnable
    stream->read(&data->alphaToOneEnable, 1 * 4); // VkBool32 alphaToOneEnable
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineColorBlendAttachmentState(const VkPipelineColorBlendAttachmentState* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkBool32 blendEnable
    res += 1 * 4; // VkBlendFactor srcColorBlendFactor
    res += 1 * 4; // VkBlendFactor dstColorBlendFactor
    res += 1 * 4; // VkBlendOp colorBlendOp
    res += 1 * 4; // VkBlendFactor srcAlphaBlendFactor
    res += 1 * 4; // VkBlendFactor dstAlphaBlendFactor
    res += 1 * 4; // VkBlendOp alphaBlendOp
    res += 1 * 4; // VkColorComponentFlags colorWriteMask
    return res;
}

void vkUtilsPack_VkPipelineColorBlendAttachmentState(android::base::InplaceStream* stream, const VkPipelineColorBlendAttachmentState* data) {
    if (!data) return;
    stream->write(&data->blendEnable, 1 * 4); // VkBool32 blendEnable
    stream->write(&data->srcColorBlendFactor, 1 * 4); // VkBlendFactor srcColorBlendFactor
    stream->write(&data->dstColorBlendFactor, 1 * 4); // VkBlendFactor dstColorBlendFactor
    stream->write(&data->colorBlendOp, 1 * 4); // VkBlendOp colorBlendOp
    stream->write(&data->srcAlphaBlendFactor, 1 * 4); // VkBlendFactor srcAlphaBlendFactor
    stream->write(&data->dstAlphaBlendFactor, 1 * 4); // VkBlendFactor dstAlphaBlendFactor
    stream->write(&data->alphaBlendOp, 1 * 4); // VkBlendOp alphaBlendOp
    stream->write(&data->colorWriteMask, 1 * 4); // VkColorComponentFlags colorWriteMask
}

VkPipelineColorBlendAttachmentState* vkUtilsUnpack_VkPipelineColorBlendAttachmentState(android::base::InplaceStream* stream) {
    VkPipelineColorBlendAttachmentState* data = new VkPipelineColorBlendAttachmentState; memset(data, 0, sizeof(VkPipelineColorBlendAttachmentState)); 
    stream->read(&data->blendEnable, 1 * 4); // VkBool32 blendEnable
    stream->read(&data->srcColorBlendFactor, 1 * 4); // VkBlendFactor srcColorBlendFactor
    stream->read(&data->dstColorBlendFactor, 1 * 4); // VkBlendFactor dstColorBlendFactor
    stream->read(&data->colorBlendOp, 1 * 4); // VkBlendOp colorBlendOp
    stream->read(&data->srcAlphaBlendFactor, 1 * 4); // VkBlendFactor srcAlphaBlendFactor
    stream->read(&data->dstAlphaBlendFactor, 1 * 4); // VkBlendFactor dstAlphaBlendFactor
    stream->read(&data->alphaBlendOp, 1 * 4); // VkBlendOp alphaBlendOp
    stream->read(&data->colorWriteMask, 1 * 4); // VkColorComponentFlags colorWriteMask
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineColorBlendStateCreateFlags flags
    res += 1 * 4; // VkBool32 logicOpEnable
    res += 1 * 4; // VkLogicOp logicOp
    res += 1 * 4; // u32 attachmentCount
    res += 8; if (data->pAttachments) { res += data->attachmentCount * vkUtilsEncodingSize_VkPipelineColorBlendAttachmentState(data->pAttachments); } // VkPipelineColorBlendAttachmentState pAttachments
    res += 4 * 4; // f32 blendConstants
    return res;
}

void vkUtilsPack_VkPipelineColorBlendStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineColorBlendStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineColorBlendStateCreateFlags flags
    stream->write(&data->logicOpEnable, 1 * 4); // VkBool32 logicOpEnable
    stream->write(&data->logicOp, 1 * 4); // VkLogicOp logicOp
    stream->write(&data->attachmentCount, 1 * 4); // u32 attachmentCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pAttachments); stream->write(&ptrval, 8);
    if (data->pAttachments) { for (uint32_t i = 0; i < data->attachmentCount; i++) { vkUtilsPack_VkPipelineColorBlendAttachmentState(stream, data->pAttachments + i); } } } // VkPipelineColorBlendAttachmentState pAttachments
    stream->write(&data->blendConstants, 4 * 4); // f32 blendConstants
}

VkPipelineColorBlendStateCreateInfo* vkUtilsUnpack_VkPipelineColorBlendStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineColorBlendStateCreateInfo* data = new VkPipelineColorBlendStateCreateInfo; memset(data, 0, sizeof(VkPipelineColorBlendStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineColorBlendStateCreateFlags flags
    stream->read(&data->logicOpEnable, 1 * 4); // VkBool32 logicOpEnable
    stream->read(&data->logicOp, 1 * 4); // VkLogicOp logicOp
    stream->read(&data->attachmentCount, 1 * 4); // u32 attachmentCount
    { // VkPipelineColorBlendAttachmentState pAttachments
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineColorBlendAttachmentState* tmpArr = ptrval ? new VkPipelineColorBlendAttachmentState[data->attachmentCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->attachmentCount; i++) {
        VkPipelineColorBlendAttachmentState* tmpUnpacked = vkUtilsUnpack_VkPipelineColorBlendAttachmentState(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineColorBlendAttachmentState)); delete tmpUnpacked; } }
        data->pAttachments = tmpArr;
    }
    stream->read(&data->blendConstants, 4 * 4); // f32 blendConstants
    return data;
}

uint32_t vkUtilsEncodingSize_VkStencilOpState(const VkStencilOpState* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStencilOp failOp
    res += 1 * 4; // VkStencilOp passOp
    res += 1 * 4; // VkStencilOp depthFailOp
    res += 1 * 4; // VkCompareOp compareOp
    res += 1 * 4; // u32 compareMask
    res += 1 * 4; // u32 writeMask
    res += 1 * 4; // u32 reference
    return res;
}

void vkUtilsPack_VkStencilOpState(android::base::InplaceStream* stream, const VkStencilOpState* data) {
    if (!data) return;
    stream->write(&data->failOp, 1 * 4); // VkStencilOp failOp
    stream->write(&data->passOp, 1 * 4); // VkStencilOp passOp
    stream->write(&data->depthFailOp, 1 * 4); // VkStencilOp depthFailOp
    stream->write(&data->compareOp, 1 * 4); // VkCompareOp compareOp
    stream->write(&data->compareMask, 1 * 4); // u32 compareMask
    stream->write(&data->writeMask, 1 * 4); // u32 writeMask
    stream->write(&data->reference, 1 * 4); // u32 reference
}

VkStencilOpState* vkUtilsUnpack_VkStencilOpState(android::base::InplaceStream* stream) {
    VkStencilOpState* data = new VkStencilOpState; memset(data, 0, sizeof(VkStencilOpState)); 
    stream->read(&data->failOp, 1 * 4); // VkStencilOp failOp
    stream->read(&data->passOp, 1 * 4); // VkStencilOp passOp
    stream->read(&data->depthFailOp, 1 * 4); // VkStencilOp depthFailOp
    stream->read(&data->compareOp, 1 * 4); // VkCompareOp compareOp
    stream->read(&data->compareMask, 1 * 4); // u32 compareMask
    stream->read(&data->writeMask, 1 * 4); // u32 writeMask
    stream->read(&data->reference, 1 * 4); // u32 reference
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineDepthStencilStateCreateInfo(const VkPipelineDepthStencilStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineDepthStencilStateCreateFlags flags
    res += 1 * 4; // VkBool32 depthTestEnable
    res += 1 * 4; // VkBool32 depthWriteEnable
    res += 1 * 4; // VkCompareOp depthCompareOp
    res += 1 * 4; // VkBool32 depthBoundsTestEnable
    res += 1 * 4; // VkBool32 stencilTestEnable
    res += 1 * vkUtilsEncodingSize_VkStencilOpState(&data->front); // VkStencilOpState front
    res += 1 * vkUtilsEncodingSize_VkStencilOpState(&data->back); // VkStencilOpState back
    res += 1 * 4; // f32 minDepthBounds
    res += 1 * 4; // f32 maxDepthBounds
    return res;
}

void vkUtilsPack_VkPipelineDepthStencilStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineDepthStencilStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineDepthStencilStateCreateFlags flags
    stream->write(&data->depthTestEnable, 1 * 4); // VkBool32 depthTestEnable
    stream->write(&data->depthWriteEnable, 1 * 4); // VkBool32 depthWriteEnable
    stream->write(&data->depthCompareOp, 1 * 4); // VkCompareOp depthCompareOp
    stream->write(&data->depthBoundsTestEnable, 1 * 4); // VkBool32 depthBoundsTestEnable
    stream->write(&data->stencilTestEnable, 1 * 4); // VkBool32 stencilTestEnable
    vkUtilsPack_VkStencilOpState(stream, &data->front); // VkStencilOpState front
    vkUtilsPack_VkStencilOpState(stream, &data->back); // VkStencilOpState back
    stream->write(&data->minDepthBounds, 1 * 4); // f32 minDepthBounds
    stream->write(&data->maxDepthBounds, 1 * 4); // f32 maxDepthBounds
}

VkPipelineDepthStencilStateCreateInfo* vkUtilsUnpack_VkPipelineDepthStencilStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineDepthStencilStateCreateInfo* data = new VkPipelineDepthStencilStateCreateInfo; memset(data, 0, sizeof(VkPipelineDepthStencilStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineDepthStencilStateCreateFlags flags
    stream->read(&data->depthTestEnable, 1 * 4); // VkBool32 depthTestEnable
    stream->read(&data->depthWriteEnable, 1 * 4); // VkBool32 depthWriteEnable
    stream->read(&data->depthCompareOp, 1 * 4); // VkCompareOp depthCompareOp
    stream->read(&data->depthBoundsTestEnable, 1 * 4); // VkBool32 depthBoundsTestEnable
    stream->read(&data->stencilTestEnable, 1 * 4); // VkBool32 stencilTestEnable
    { VkStencilOpState* tmpUnpacked = vkUtilsUnpack_VkStencilOpState(stream); memcpy(&data->front, tmpUnpacked, sizeof(VkStencilOpState)); delete tmpUnpacked; } // VkStencilOpState front
    { VkStencilOpState* tmpUnpacked = vkUtilsUnpack_VkStencilOpState(stream); memcpy(&data->back, tmpUnpacked, sizeof(VkStencilOpState)); delete tmpUnpacked; } // VkStencilOpState back
    stream->read(&data->minDepthBounds, 1 * 4); // f32 minDepthBounds
    stream->read(&data->maxDepthBounds, 1 * 4); // f32 maxDepthBounds
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineDynamicStateCreateInfo(const VkPipelineDynamicStateCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineDynamicStateCreateFlags flags
    res += 1 * 4; // u32 dynamicStateCount
    res += data->dynamicStateCount * 4; // VkDynamicState pDynamicStates
    return res;
}

void vkUtilsPack_VkPipelineDynamicStateCreateInfo(android::base::InplaceStream* stream, const VkPipelineDynamicStateCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineDynamicStateCreateFlags flags
    stream->write(&data->dynamicStateCount, 1 * 4); // u32 dynamicStateCount
    stream->write(data->pDynamicStates, data->dynamicStateCount * 4); // VkDynamicState pDynamicStates
}

VkPipelineDynamicStateCreateInfo* vkUtilsUnpack_VkPipelineDynamicStateCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineDynamicStateCreateInfo* data = new VkPipelineDynamicStateCreateInfo; memset(data, 0, sizeof(VkPipelineDynamicStateCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineDynamicStateCreateFlags flags
    stream->read(&data->dynamicStateCount, 1 * 4); // u32 dynamicStateCount
    data->pDynamicStates = data->dynamicStateCount ? new VkDynamicState[data->dynamicStateCount] : nullptr; // VkDynamicState pDynamicStates
    stream->read((char*)data->pDynamicStates, data->dynamicStateCount * 4); // VkDynamicState pDynamicStates
    return data;
}

uint32_t vkUtilsEncodingSize_VkGraphicsPipelineCreateInfo(const VkGraphicsPipelineCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineCreateFlags flags
    res += 1 * 4; // u32 stageCount
    res += 8; if (data->pStages) { res += data->stageCount * vkUtilsEncodingSize_VkPipelineShaderStageCreateInfo(data->pStages); } // VkPipelineShaderStageCreateInfo pStages
    res += 8; if (data->pVertexInputState) { res += 1 * vkUtilsEncodingSize_VkPipelineVertexInputStateCreateInfo(data->pVertexInputState); } // VkPipelineVertexInputStateCreateInfo pVertexInputState
    res += 8; if (data->pInputAssemblyState) { res += 1 * vkUtilsEncodingSize_VkPipelineInputAssemblyStateCreateInfo(data->pInputAssemblyState); } // VkPipelineInputAssemblyStateCreateInfo pInputAssemblyState
    res += 8; if (data->pTessellationState) { res += 1 * vkUtilsEncodingSize_VkPipelineTessellationStateCreateInfo(data->pTessellationState); } // VkPipelineTessellationStateCreateInfo pTessellationState
    res += 8; if (data->pViewportState) { res += 1 * vkUtilsEncodingSize_VkPipelineViewportStateCreateInfo(data->pViewportState); } // VkPipelineViewportStateCreateInfo pViewportState
    res += 8; if (data->pRasterizationState) { res += 1 * vkUtilsEncodingSize_VkPipelineRasterizationStateCreateInfo(data->pRasterizationState); } // VkPipelineRasterizationStateCreateInfo pRasterizationState
    res += 8; if (data->pMultisampleState) { res += 1 * vkUtilsEncodingSize_VkPipelineMultisampleStateCreateInfo(data->pMultisampleState); } // VkPipelineMultisampleStateCreateInfo pMultisampleState
    res += 8; if (data->pDepthStencilState) { res += 1 * vkUtilsEncodingSize_VkPipelineDepthStencilStateCreateInfo(data->pDepthStencilState); } // VkPipelineDepthStencilStateCreateInfo pDepthStencilState
    res += 8; if (data->pColorBlendState) { res += 1 * vkUtilsEncodingSize_VkPipelineColorBlendStateCreateInfo(data->pColorBlendState); } // VkPipelineColorBlendStateCreateInfo pColorBlendState
    res += 8; if (data->pDynamicState) { res += 1 * vkUtilsEncodingSize_VkPipelineDynamicStateCreateInfo(data->pDynamicState); } // VkPipelineDynamicStateCreateInfo pDynamicState
    res += 1 * 8; // VkPipelineLayout layout
    res += 1 * 8; // VkRenderPass renderPass
    res += 1 * 4; // u32 subpass
    res += 1 * 8; // VkPipeline basePipelineHandle
    res += 1 * 4; // s32 basePipelineIndex
    return res;
}

void vkUtilsPack_VkGraphicsPipelineCreateInfo(android::base::InplaceStream* stream, const VkGraphicsPipelineCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineCreateFlags flags
    stream->write(&data->stageCount, 1 * 4); // u32 stageCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pStages); stream->write(&ptrval, 8);
    if (data->pStages) { for (uint32_t i = 0; i < data->stageCount; i++) { vkUtilsPack_VkPipelineShaderStageCreateInfo(stream, data->pStages + i); } } } // VkPipelineShaderStageCreateInfo pStages
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pVertexInputState); stream->write(&ptrval, 8);
    if (data->pVertexInputState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineVertexInputStateCreateInfo(stream, data->pVertexInputState + i); } } } // VkPipelineVertexInputStateCreateInfo pVertexInputState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pInputAssemblyState); stream->write(&ptrval, 8);
    if (data->pInputAssemblyState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineInputAssemblyStateCreateInfo(stream, data->pInputAssemblyState + i); } } } // VkPipelineInputAssemblyStateCreateInfo pInputAssemblyState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pTessellationState); stream->write(&ptrval, 8);
    if (data->pTessellationState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineTessellationStateCreateInfo(stream, data->pTessellationState + i); } } } // VkPipelineTessellationStateCreateInfo pTessellationState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pViewportState); stream->write(&ptrval, 8);
    if (data->pViewportState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineViewportStateCreateInfo(stream, data->pViewportState + i); } } } // VkPipelineViewportStateCreateInfo pViewportState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pRasterizationState); stream->write(&ptrval, 8);
    if (data->pRasterizationState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineRasterizationStateCreateInfo(stream, data->pRasterizationState + i); } } } // VkPipelineRasterizationStateCreateInfo pRasterizationState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pMultisampleState); stream->write(&ptrval, 8);
    if (data->pMultisampleState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineMultisampleStateCreateInfo(stream, data->pMultisampleState + i); } } } // VkPipelineMultisampleStateCreateInfo pMultisampleState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDepthStencilState); stream->write(&ptrval, 8);
    if (data->pDepthStencilState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineDepthStencilStateCreateInfo(stream, data->pDepthStencilState + i); } } } // VkPipelineDepthStencilStateCreateInfo pDepthStencilState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pColorBlendState); stream->write(&ptrval, 8);
    if (data->pColorBlendState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineColorBlendStateCreateInfo(stream, data->pColorBlendState + i); } } } // VkPipelineColorBlendStateCreateInfo pColorBlendState
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDynamicState); stream->write(&ptrval, 8);
    if (data->pDynamicState) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkPipelineDynamicStateCreateInfo(stream, data->pDynamicState + i); } } } // VkPipelineDynamicStateCreateInfo pDynamicState
    stream->write(&data->layout, 1 * 8); // VkPipelineLayout layout
    stream->write(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->write(&data->subpass, 1 * 4); // u32 subpass
    stream->write(&data->basePipelineHandle, 1 * 8); // VkPipeline basePipelineHandle
    stream->write(&data->basePipelineIndex, 1 * 4); // s32 basePipelineIndex
}

VkGraphicsPipelineCreateInfo* vkUtilsUnpack_VkGraphicsPipelineCreateInfo(android::base::InplaceStream* stream) {
    VkGraphicsPipelineCreateInfo* data = new VkGraphicsPipelineCreateInfo; memset(data, 0, sizeof(VkGraphicsPipelineCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineCreateFlags flags
    stream->read(&data->stageCount, 1 * 4); // u32 stageCount
    { // VkPipelineShaderStageCreateInfo pStages
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineShaderStageCreateInfo* tmpArr = ptrval ? new VkPipelineShaderStageCreateInfo[data->stageCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->stageCount; i++) {
        VkPipelineShaderStageCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineShaderStageCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineShaderStageCreateInfo)); delete tmpUnpacked; } }
        data->pStages = tmpArr;
    }
    { // VkPipelineVertexInputStateCreateInfo pVertexInputState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineVertexInputStateCreateInfo* tmpArr = ptrval ? new VkPipelineVertexInputStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineVertexInputStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineVertexInputStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineVertexInputStateCreateInfo)); delete tmpUnpacked; } }
        data->pVertexInputState = tmpArr;
    }
    { // VkPipelineInputAssemblyStateCreateInfo pInputAssemblyState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineInputAssemblyStateCreateInfo* tmpArr = ptrval ? new VkPipelineInputAssemblyStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineInputAssemblyStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineInputAssemblyStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineInputAssemblyStateCreateInfo)); delete tmpUnpacked; } }
        data->pInputAssemblyState = tmpArr;
    }
    { // VkPipelineTessellationStateCreateInfo pTessellationState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineTessellationStateCreateInfo* tmpArr = ptrval ? new VkPipelineTessellationStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineTessellationStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineTessellationStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineTessellationStateCreateInfo)); delete tmpUnpacked; } }
        data->pTessellationState = tmpArr;
    }
    { // VkPipelineViewportStateCreateInfo pViewportState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineViewportStateCreateInfo* tmpArr = ptrval ? new VkPipelineViewportStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineViewportStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineViewportStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineViewportStateCreateInfo)); delete tmpUnpacked; } }
        data->pViewportState = tmpArr;
    }
    { // VkPipelineRasterizationStateCreateInfo pRasterizationState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineRasterizationStateCreateInfo* tmpArr = ptrval ? new VkPipelineRasterizationStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineRasterizationStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineRasterizationStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineRasterizationStateCreateInfo)); delete tmpUnpacked; } }
        data->pRasterizationState = tmpArr;
    }
    { // VkPipelineMultisampleStateCreateInfo pMultisampleState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineMultisampleStateCreateInfo* tmpArr = ptrval ? new VkPipelineMultisampleStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineMultisampleStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineMultisampleStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineMultisampleStateCreateInfo)); delete tmpUnpacked; } }
        data->pMultisampleState = tmpArr;
    }
    { // VkPipelineDepthStencilStateCreateInfo pDepthStencilState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineDepthStencilStateCreateInfo* tmpArr = ptrval ? new VkPipelineDepthStencilStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineDepthStencilStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineDepthStencilStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineDepthStencilStateCreateInfo)); delete tmpUnpacked; } }
        data->pDepthStencilState = tmpArr;
    }
    { // VkPipelineColorBlendStateCreateInfo pColorBlendState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineColorBlendStateCreateInfo* tmpArr = ptrval ? new VkPipelineColorBlendStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineColorBlendStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineColorBlendStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineColorBlendStateCreateInfo)); delete tmpUnpacked; } }
        data->pColorBlendState = tmpArr;
    }
    { // VkPipelineDynamicStateCreateInfo pDynamicState
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPipelineDynamicStateCreateInfo* tmpArr = ptrval ? new VkPipelineDynamicStateCreateInfo[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkPipelineDynamicStateCreateInfo* tmpUnpacked = vkUtilsUnpack_VkPipelineDynamicStateCreateInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPipelineDynamicStateCreateInfo)); delete tmpUnpacked; } }
        data->pDynamicState = tmpArr;
    }
    stream->read(&data->layout, 1 * 8); // VkPipelineLayout layout
    stream->read(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->read(&data->subpass, 1 * 4); // u32 subpass
    stream->read(&data->basePipelineHandle, 1 * 8); // VkPipeline basePipelineHandle
    stream->read(&data->basePipelineIndex, 1 * 4); // s32 basePipelineIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineCacheCreateInfo(const VkPipelineCacheCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineCacheCreateFlags flags
    res += 1 * 4; // size_t initialDataSize
    res += data->initialDataSize * 8; // void pInitialData
    return res;
}

void vkUtilsPack_VkPipelineCacheCreateInfo(android::base::InplaceStream* stream, const VkPipelineCacheCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineCacheCreateFlags flags
    stream->write(&data->initialDataSize, 1 * 4); // size_t initialDataSize
    stream->write(&data->pInitialData, 8); // void pInitialData
}

VkPipelineCacheCreateInfo* vkUtilsUnpack_VkPipelineCacheCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineCacheCreateInfo* data = new VkPipelineCacheCreateInfo; memset(data, 0, sizeof(VkPipelineCacheCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineCacheCreateFlags flags
    stream->read(&data->initialDataSize, 1 * 4); // size_t initialDataSize
    stream->read(&data->pInitialData, 8); // void pInitialData
    return data;
}

uint32_t vkUtilsEncodingSize_VkPushConstantRange(const VkPushConstantRange* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkShaderStageFlags stageFlags
    res += 1 * 4; // u32 offset
    res += 1 * 4; // u32 size
    return res;
}

void vkUtilsPack_VkPushConstantRange(android::base::InplaceStream* stream, const VkPushConstantRange* data) {
    if (!data) return;
    stream->write(&data->stageFlags, 1 * 4); // VkShaderStageFlags stageFlags
    stream->write(&data->offset, 1 * 4); // u32 offset
    stream->write(&data->size, 1 * 4); // u32 size
}

VkPushConstantRange* vkUtilsUnpack_VkPushConstantRange(android::base::InplaceStream* stream) {
    VkPushConstantRange* data = new VkPushConstantRange; memset(data, 0, sizeof(VkPushConstantRange)); 
    stream->read(&data->stageFlags, 1 * 4); // VkShaderStageFlags stageFlags
    stream->read(&data->offset, 1 * 4); // u32 offset
    stream->read(&data->size, 1 * 4); // u32 size
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineLayoutCreateFlags flags
    res += 1 * 4; // u32 setLayoutCount
    res += data->setLayoutCount * 8; // VkDescriptorSetLayout pSetLayouts
    res += 1 * 4; // u32 pushConstantRangeCount
    res += 8; if (data->pPushConstantRanges) { res += data->pushConstantRangeCount * vkUtilsEncodingSize_VkPushConstantRange(data->pPushConstantRanges); } // VkPushConstantRange pPushConstantRanges
    return res;
}

void vkUtilsPack_VkPipelineLayoutCreateInfo(android::base::InplaceStream* stream, const VkPipelineLayoutCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineLayoutCreateFlags flags
    stream->write(&data->setLayoutCount, 1 * 4); // u32 setLayoutCount
    stream->write(data->pSetLayouts, data->setLayoutCount * 8); // VkDescriptorSetLayout pSetLayouts
    stream->write(&data->pushConstantRangeCount, 1 * 4); // u32 pushConstantRangeCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pPushConstantRanges); stream->write(&ptrval, 8);
    if (data->pPushConstantRanges) { for (uint32_t i = 0; i < data->pushConstantRangeCount; i++) { vkUtilsPack_VkPushConstantRange(stream, data->pPushConstantRanges + i); } } } // VkPushConstantRange pPushConstantRanges
}

VkPipelineLayoutCreateInfo* vkUtilsUnpack_VkPipelineLayoutCreateInfo(android::base::InplaceStream* stream) {
    VkPipelineLayoutCreateInfo* data = new VkPipelineLayoutCreateInfo; memset(data, 0, sizeof(VkPipelineLayoutCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineLayoutCreateFlags flags
    stream->read(&data->setLayoutCount, 1 * 4); // u32 setLayoutCount
    data->pSetLayouts = data->setLayoutCount ? new VkDescriptorSetLayout[data->setLayoutCount] : nullptr; // VkDescriptorSetLayout pSetLayouts
    stream->read((char*)data->pSetLayouts, data->setLayoutCount * 8); // VkDescriptorSetLayout pSetLayouts
    stream->read(&data->pushConstantRangeCount, 1 * 4); // u32 pushConstantRangeCount
    { // VkPushConstantRange pPushConstantRanges
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPushConstantRange* tmpArr = ptrval ? new VkPushConstantRange[data->pushConstantRangeCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->pushConstantRangeCount; i++) {
        VkPushConstantRange* tmpUnpacked = vkUtilsUnpack_VkPushConstantRange(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPushConstantRange)); delete tmpUnpacked; } }
        data->pPushConstantRanges = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkSamplerCreateInfo(const VkSamplerCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkSamplerCreateFlags flags
    res += 1 * 4; // VkFilter magFilter
    res += 1 * 4; // VkFilter minFilter
    res += 1 * 4; // VkSamplerMipmapMode mipmapMode
    res += 1 * 4; // VkSamplerAddressMode addressModeU
    res += 1 * 4; // VkSamplerAddressMode addressModeV
    res += 1 * 4; // VkSamplerAddressMode addressModeW
    res += 1 * 4; // f32 mipLodBias
    res += 1 * 4; // VkBool32 anisotropyEnable
    res += 1 * 4; // f32 maxAnisotropy
    res += 1 * 4; // VkBool32 compareEnable
    res += 1 * 4; // VkCompareOp compareOp
    res += 1 * 4; // f32 minLod
    res += 1 * 4; // f32 maxLod
    res += 1 * 4; // VkBorderColor borderColor
    res += 1 * 4; // VkBool32 unnormalizedCoordinates
    return res;
}

void vkUtilsPack_VkSamplerCreateInfo(android::base::InplaceStream* stream, const VkSamplerCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkSamplerCreateFlags flags
    stream->write(&data->magFilter, 1 * 4); // VkFilter magFilter
    stream->write(&data->minFilter, 1 * 4); // VkFilter minFilter
    stream->write(&data->mipmapMode, 1 * 4); // VkSamplerMipmapMode mipmapMode
    stream->write(&data->addressModeU, 1 * 4); // VkSamplerAddressMode addressModeU
    stream->write(&data->addressModeV, 1 * 4); // VkSamplerAddressMode addressModeV
    stream->write(&data->addressModeW, 1 * 4); // VkSamplerAddressMode addressModeW
    stream->write(&data->mipLodBias, 1 * 4); // f32 mipLodBias
    stream->write(&data->anisotropyEnable, 1 * 4); // VkBool32 anisotropyEnable
    stream->write(&data->maxAnisotropy, 1 * 4); // f32 maxAnisotropy
    stream->write(&data->compareEnable, 1 * 4); // VkBool32 compareEnable
    stream->write(&data->compareOp, 1 * 4); // VkCompareOp compareOp
    stream->write(&data->minLod, 1 * 4); // f32 minLod
    stream->write(&data->maxLod, 1 * 4); // f32 maxLod
    stream->write(&data->borderColor, 1 * 4); // VkBorderColor borderColor
    stream->write(&data->unnormalizedCoordinates, 1 * 4); // VkBool32 unnormalizedCoordinates
}

VkSamplerCreateInfo* vkUtilsUnpack_VkSamplerCreateInfo(android::base::InplaceStream* stream) {
    VkSamplerCreateInfo* data = new VkSamplerCreateInfo; memset(data, 0, sizeof(VkSamplerCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkSamplerCreateFlags flags
    stream->read(&data->magFilter, 1 * 4); // VkFilter magFilter
    stream->read(&data->minFilter, 1 * 4); // VkFilter minFilter
    stream->read(&data->mipmapMode, 1 * 4); // VkSamplerMipmapMode mipmapMode
    stream->read(&data->addressModeU, 1 * 4); // VkSamplerAddressMode addressModeU
    stream->read(&data->addressModeV, 1 * 4); // VkSamplerAddressMode addressModeV
    stream->read(&data->addressModeW, 1 * 4); // VkSamplerAddressMode addressModeW
    stream->read(&data->mipLodBias, 1 * 4); // f32 mipLodBias
    stream->read(&data->anisotropyEnable, 1 * 4); // VkBool32 anisotropyEnable
    stream->read(&data->maxAnisotropy, 1 * 4); // f32 maxAnisotropy
    stream->read(&data->compareEnable, 1 * 4); // VkBool32 compareEnable
    stream->read(&data->compareOp, 1 * 4); // VkCompareOp compareOp
    stream->read(&data->minLod, 1 * 4); // f32 minLod
    stream->read(&data->maxLod, 1 * 4); // f32 maxLod
    stream->read(&data->borderColor, 1 * 4); // VkBorderColor borderColor
    stream->read(&data->unnormalizedCoordinates, 1 * 4); // VkBool32 unnormalizedCoordinates
    return data;
}

uint32_t vkUtilsEncodingSize_VkCommandPoolCreateInfo(const VkCommandPoolCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkCommandPoolCreateFlags flags
    res += 1 * 4; // u32 queueFamilyIndex
    return res;
}

void vkUtilsPack_VkCommandPoolCreateInfo(android::base::InplaceStream* stream, const VkCommandPoolCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkCommandPoolCreateFlags flags
    stream->write(&data->queueFamilyIndex, 1 * 4); // u32 queueFamilyIndex
}

VkCommandPoolCreateInfo* vkUtilsUnpack_VkCommandPoolCreateInfo(android::base::InplaceStream* stream) {
    VkCommandPoolCreateInfo* data = new VkCommandPoolCreateInfo; memset(data, 0, sizeof(VkCommandPoolCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkCommandPoolCreateFlags flags
    stream->read(&data->queueFamilyIndex, 1 * 4); // u32 queueFamilyIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkCommandBufferAllocateInfo(const VkCommandBufferAllocateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkCommandPool commandPool
    res += 1 * 4; // VkCommandBufferLevel level
    res += 1 * 4; // u32 commandBufferCount
    return res;
}

void vkUtilsPack_VkCommandBufferAllocateInfo(android::base::InplaceStream* stream, const VkCommandBufferAllocateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->commandPool, 1 * 8); // VkCommandPool commandPool
    stream->write(&data->level, 1 * 4); // VkCommandBufferLevel level
    stream->write(&data->commandBufferCount, 1 * 4); // u32 commandBufferCount
}

VkCommandBufferAllocateInfo* vkUtilsUnpack_VkCommandBufferAllocateInfo(android::base::InplaceStream* stream) {
    VkCommandBufferAllocateInfo* data = new VkCommandBufferAllocateInfo; memset(data, 0, sizeof(VkCommandBufferAllocateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->commandPool, 1 * 8); // VkCommandPool commandPool
    stream->read(&data->level, 1 * 4); // VkCommandBufferLevel level
    stream->read(&data->commandBufferCount, 1 * 4); // u32 commandBufferCount
    return data;
}

uint32_t vkUtilsEncodingSize_VkCommandBufferInheritanceInfo(const VkCommandBufferInheritanceInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkRenderPass renderPass
    res += 1 * 4; // u32 subpass
    res += 1 * 8; // VkFramebuffer framebuffer
    res += 1 * 4; // VkBool32 occlusionQueryEnable
    res += 1 * 4; // VkQueryControlFlags queryFlags
    res += 1 * 4; // VkQueryPipelineStatisticFlags pipelineStatistics
    return res;
}

void vkUtilsPack_VkCommandBufferInheritanceInfo(android::base::InplaceStream* stream, const VkCommandBufferInheritanceInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->write(&data->subpass, 1 * 4); // u32 subpass
    stream->write(&data->framebuffer, 1 * 8); // VkFramebuffer framebuffer
    stream->write(&data->occlusionQueryEnable, 1 * 4); // VkBool32 occlusionQueryEnable
    stream->write(&data->queryFlags, 1 * 4); // VkQueryControlFlags queryFlags
    stream->write(&data->pipelineStatistics, 1 * 4); // VkQueryPipelineStatisticFlags pipelineStatistics
}

VkCommandBufferInheritanceInfo* vkUtilsUnpack_VkCommandBufferInheritanceInfo(android::base::InplaceStream* stream) {
    VkCommandBufferInheritanceInfo* data = new VkCommandBufferInheritanceInfo; memset(data, 0, sizeof(VkCommandBufferInheritanceInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->read(&data->subpass, 1 * 4); // u32 subpass
    stream->read(&data->framebuffer, 1 * 8); // VkFramebuffer framebuffer
    stream->read(&data->occlusionQueryEnable, 1 * 4); // VkBool32 occlusionQueryEnable
    stream->read(&data->queryFlags, 1 * 4); // VkQueryControlFlags queryFlags
    stream->read(&data->pipelineStatistics, 1 * 4); // VkQueryPipelineStatisticFlags pipelineStatistics
    return data;
}

uint32_t vkUtilsEncodingSize_VkCommandBufferBeginInfo(const VkCommandBufferBeginInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkCommandBufferUsageFlags flags
    res += 8; if (data->pInheritanceInfo) { res += customCount_VkCommandBufferBeginInfo_pInheritanceInfo(data) * vkUtilsEncodingSize_VkCommandBufferInheritanceInfo(data->pInheritanceInfo); } // VkCommandBufferInheritanceInfo pInheritanceInfo
    return res;
}

void vkUtilsPack_VkCommandBufferBeginInfo(android::base::InplaceStream* stream, const VkCommandBufferBeginInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkCommandBufferUsageFlags flags
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pInheritanceInfo); stream->write(&ptrval, 8);
    if (data->pInheritanceInfo) { for (uint32_t i = 0; i < customCount_VkCommandBufferBeginInfo_pInheritanceInfo(data); i++) { vkUtilsPack_VkCommandBufferInheritanceInfo(stream, data->pInheritanceInfo + i); } } } // VkCommandBufferInheritanceInfo pInheritanceInfo
}

VkCommandBufferBeginInfo* vkUtilsUnpack_VkCommandBufferBeginInfo(android::base::InplaceStream* stream) {
    VkCommandBufferBeginInfo* data = new VkCommandBufferBeginInfo; memset(data, 0, sizeof(VkCommandBufferBeginInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkCommandBufferUsageFlags flags
    { // VkCommandBufferInheritanceInfo pInheritanceInfo
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkCommandBufferInheritanceInfo* tmpArr = ptrval ? new VkCommandBufferInheritanceInfo[customCount_VkCommandBufferBeginInfo_pInheritanceInfo(data)] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < customCount_VkCommandBufferBeginInfo_pInheritanceInfo(data); i++) {
        VkCommandBufferInheritanceInfo* tmpUnpacked = vkUtilsUnpack_VkCommandBufferInheritanceInfo(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkCommandBufferInheritanceInfo)); delete tmpUnpacked; } }
        data->pInheritanceInfo = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkRenderPassBeginInfo(const VkRenderPassBeginInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkRenderPass renderPass
    res += 1 * 8; // VkFramebuffer framebuffer
    res += 1 * vkUtilsEncodingSize_VkRect2D(&data->renderArea); // VkRect2D renderArea
    res += 1 * 4; // u32 clearValueCount
    res += 8; if (data->pClearValues) { res += data->clearValueCount * vkUtilsEncodingSize_VkClearValue(data->pClearValues); } // VkClearValue pClearValues
    return res;
}

void vkUtilsPack_VkRenderPassBeginInfo(android::base::InplaceStream* stream, const VkRenderPassBeginInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->write(&data->framebuffer, 1 * 8); // VkFramebuffer framebuffer
    vkUtilsPack_VkRect2D(stream, &data->renderArea); // VkRect2D renderArea
    stream->write(&data->clearValueCount, 1 * 4); // u32 clearValueCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pClearValues); stream->write(&ptrval, 8);
    if (data->pClearValues) { for (uint32_t i = 0; i < data->clearValueCount; i++) { vkUtilsPack_VkClearValue(stream, data->pClearValues + i); } } } // VkClearValue pClearValues
}

VkRenderPassBeginInfo* vkUtilsUnpack_VkRenderPassBeginInfo(android::base::InplaceStream* stream) {
    VkRenderPassBeginInfo* data = new VkRenderPassBeginInfo; memset(data, 0, sizeof(VkRenderPassBeginInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->read(&data->framebuffer, 1 * 8); // VkFramebuffer framebuffer
    { VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&data->renderArea, tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } // VkRect2D renderArea
    stream->read(&data->clearValueCount, 1 * 4); // u32 clearValueCount
    { // VkClearValue pClearValues
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkClearValue* tmpArr = ptrval ? new VkClearValue[data->clearValueCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->clearValueCount; i++) {
        VkClearValue* tmpUnpacked = vkUtilsUnpack_VkClearValue(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkClearValue)); delete tmpUnpacked; } }
        data->pClearValues = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkClearColorValue(const VkClearColorValue* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 4 * 4; // f32 float32
    res += 4 * 4; // s32 int32
    res += 4 * 4; // u32 uint32
    return res;
}

void vkUtilsPack_VkClearColorValue(android::base::InplaceStream* stream, const VkClearColorValue* data) {
    if (!data) return;
    stream->write(&data->float32, 4 * 4); // f32 float32
    stream->write(&data->int32, 4 * 4); // s32 int32
    stream->write(&data->uint32, 4 * 4); // u32 uint32
}

VkClearColorValue* vkUtilsUnpack_VkClearColorValue(android::base::InplaceStream* stream) {
    VkClearColorValue* data = new VkClearColorValue; memset(data, 0, sizeof(VkClearColorValue)); 
    stream->read(&data->float32, 4 * 4); // f32 float32
    stream->read(&data->int32, 4 * 4); // s32 int32
    stream->read(&data->uint32, 4 * 4); // u32 uint32
    return data;
}

uint32_t vkUtilsEncodingSize_VkClearDepthStencilValue(const VkClearDepthStencilValue* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // f32 depth
    res += 1 * 4; // u32 stencil
    return res;
}

void vkUtilsPack_VkClearDepthStencilValue(android::base::InplaceStream* stream, const VkClearDepthStencilValue* data) {
    if (!data) return;
    stream->write(&data->depth, 1 * 4); // f32 depth
    stream->write(&data->stencil, 1 * 4); // u32 stencil
}

VkClearDepthStencilValue* vkUtilsUnpack_VkClearDepthStencilValue(android::base::InplaceStream* stream) {
    VkClearDepthStencilValue* data = new VkClearDepthStencilValue; memset(data, 0, sizeof(VkClearDepthStencilValue)); 
    stream->read(&data->depth, 1 * 4); // f32 depth
    stream->read(&data->stencil, 1 * 4); // u32 stencil
    return data;
}

uint32_t vkUtilsEncodingSize_VkClearValue(const VkClearValue* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkClearColorValue(&data->color); // VkClearColorValue color
    res += 1 * vkUtilsEncodingSize_VkClearDepthStencilValue(&data->depthStencil); // VkClearDepthStencilValue depthStencil
    return res;
}

void vkUtilsPack_VkClearValue(android::base::InplaceStream* stream, const VkClearValue* data) {
    if (!data) return;
    vkUtilsPack_VkClearColorValue(stream, &data->color); // VkClearColorValue color
    vkUtilsPack_VkClearDepthStencilValue(stream, &data->depthStencil); // VkClearDepthStencilValue depthStencil
}

VkClearValue* vkUtilsUnpack_VkClearValue(android::base::InplaceStream* stream) {
    VkClearValue* data = new VkClearValue; memset(data, 0, sizeof(VkClearValue)); 
    { VkClearColorValue* tmpUnpacked = vkUtilsUnpack_VkClearColorValue(stream); memcpy(&data->color, tmpUnpacked, sizeof(VkClearColorValue)); delete tmpUnpacked; } // VkClearColorValue color
    { VkClearDepthStencilValue* tmpUnpacked = vkUtilsUnpack_VkClearDepthStencilValue(stream); memcpy(&data->depthStencil, tmpUnpacked, sizeof(VkClearDepthStencilValue)); delete tmpUnpacked; } // VkClearDepthStencilValue depthStencil
    return data;
}

uint32_t vkUtilsEncodingSize_VkClearAttachment(const VkClearAttachment* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkImageAspectFlags aspectMask
    res += 1 * 4; // u32 colorAttachment
    res += 1 * vkUtilsEncodingSize_VkClearValue(&data->clearValue); // VkClearValue clearValue
    return res;
}

void vkUtilsPack_VkClearAttachment(android::base::InplaceStream* stream, const VkClearAttachment* data) {
    if (!data) return;
    stream->write(&data->aspectMask, 1 * 4); // VkImageAspectFlags aspectMask
    stream->write(&data->colorAttachment, 1 * 4); // u32 colorAttachment
    vkUtilsPack_VkClearValue(stream, &data->clearValue); // VkClearValue clearValue
}

VkClearAttachment* vkUtilsUnpack_VkClearAttachment(android::base::InplaceStream* stream) {
    VkClearAttachment* data = new VkClearAttachment; memset(data, 0, sizeof(VkClearAttachment)); 
    stream->read(&data->aspectMask, 1 * 4); // VkImageAspectFlags aspectMask
    stream->read(&data->colorAttachment, 1 * 4); // u32 colorAttachment
    { VkClearValue* tmpUnpacked = vkUtilsUnpack_VkClearValue(stream); memcpy(&data->clearValue, tmpUnpacked, sizeof(VkClearValue)); delete tmpUnpacked; } // VkClearValue clearValue
    return data;
}

uint32_t vkUtilsEncodingSize_VkAttachmentDescription(const VkAttachmentDescription* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkAttachmentDescriptionFlags flags
    res += 1 * 4; // VkFormat format
    res += 1 * 4; // VkSampleCountFlagBits samples
    res += 1 * 4; // VkAttachmentLoadOp loadOp
    res += 1 * 4; // VkAttachmentStoreOp storeOp
    res += 1 * 4; // VkAttachmentLoadOp stencilLoadOp
    res += 1 * 4; // VkAttachmentStoreOp stencilStoreOp
    res += 1 * 4; // VkImageLayout initialLayout
    res += 1 * 4; // VkImageLayout finalLayout
    return res;
}

void vkUtilsPack_VkAttachmentDescription(android::base::InplaceStream* stream, const VkAttachmentDescription* data) {
    if (!data) return;
    stream->write(&data->flags, 1 * 4); // VkAttachmentDescriptionFlags flags
    stream->write(&data->format, 1 * 4); // VkFormat format
    stream->write(&data->samples, 1 * 4); // VkSampleCountFlagBits samples
    stream->write(&data->loadOp, 1 * 4); // VkAttachmentLoadOp loadOp
    stream->write(&data->storeOp, 1 * 4); // VkAttachmentStoreOp storeOp
    stream->write(&data->stencilLoadOp, 1 * 4); // VkAttachmentLoadOp stencilLoadOp
    stream->write(&data->stencilStoreOp, 1 * 4); // VkAttachmentStoreOp stencilStoreOp
    stream->write(&data->initialLayout, 1 * 4); // VkImageLayout initialLayout
    stream->write(&data->finalLayout, 1 * 4); // VkImageLayout finalLayout
}

VkAttachmentDescription* vkUtilsUnpack_VkAttachmentDescription(android::base::InplaceStream* stream) {
    VkAttachmentDescription* data = new VkAttachmentDescription; memset(data, 0, sizeof(VkAttachmentDescription)); 
    stream->read(&data->flags, 1 * 4); // VkAttachmentDescriptionFlags flags
    stream->read(&data->format, 1 * 4); // VkFormat format
    stream->read(&data->samples, 1 * 4); // VkSampleCountFlagBits samples
    stream->read(&data->loadOp, 1 * 4); // VkAttachmentLoadOp loadOp
    stream->read(&data->storeOp, 1 * 4); // VkAttachmentStoreOp storeOp
    stream->read(&data->stencilLoadOp, 1 * 4); // VkAttachmentLoadOp stencilLoadOp
    stream->read(&data->stencilStoreOp, 1 * 4); // VkAttachmentStoreOp stencilStoreOp
    stream->read(&data->initialLayout, 1 * 4); // VkImageLayout initialLayout
    stream->read(&data->finalLayout, 1 * 4); // VkImageLayout finalLayout
    return data;
}

uint32_t vkUtilsEncodingSize_VkAttachmentReference(const VkAttachmentReference* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 attachment
    res += 1 * 4; // VkImageLayout layout
    return res;
}

void vkUtilsPack_VkAttachmentReference(android::base::InplaceStream* stream, const VkAttachmentReference* data) {
    if (!data) return;
    stream->write(&data->attachment, 1 * 4); // u32 attachment
    stream->write(&data->layout, 1 * 4); // VkImageLayout layout
}

VkAttachmentReference* vkUtilsUnpack_VkAttachmentReference(android::base::InplaceStream* stream) {
    VkAttachmentReference* data = new VkAttachmentReference; memset(data, 0, sizeof(VkAttachmentReference)); 
    stream->read(&data->attachment, 1 * 4); // u32 attachment
    stream->read(&data->layout, 1 * 4); // VkImageLayout layout
    return data;
}

uint32_t vkUtilsEncodingSize_VkSubpassDescription(const VkSubpassDescription* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkSubpassDescriptionFlags flags
    res += 1 * 4; // VkPipelineBindPoint pipelineBindPoint
    res += 1 * 4; // u32 inputAttachmentCount
    res += 8; if (data->pInputAttachments) { res += data->inputAttachmentCount * vkUtilsEncodingSize_VkAttachmentReference(data->pInputAttachments); } // VkAttachmentReference pInputAttachments
    res += 1 * 4; // u32 colorAttachmentCount
    res += 8; if (data->pColorAttachments) { res += data->colorAttachmentCount * vkUtilsEncodingSize_VkAttachmentReference(data->pColorAttachments); } // VkAttachmentReference pColorAttachments
    res += 8; if (data->pResolveAttachments) { res += data->colorAttachmentCount * vkUtilsEncodingSize_VkAttachmentReference(data->pResolveAttachments); } // VkAttachmentReference pResolveAttachments
    res += 8; if (data->pDepthStencilAttachment) { res += 1 * vkUtilsEncodingSize_VkAttachmentReference(data->pDepthStencilAttachment); } // VkAttachmentReference pDepthStencilAttachment
    res += 1 * 4; // u32 preserveAttachmentCount
    res += data->preserveAttachmentCount * 4; // u32 pPreserveAttachments
    return res;
}

void vkUtilsPack_VkSubpassDescription(android::base::InplaceStream* stream, const VkSubpassDescription* data) {
    if (!data) return;
    stream->write(&data->flags, 1 * 4); // VkSubpassDescriptionFlags flags
    stream->write(&data->pipelineBindPoint, 1 * 4); // VkPipelineBindPoint pipelineBindPoint
    stream->write(&data->inputAttachmentCount, 1 * 4); // u32 inputAttachmentCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pInputAttachments); stream->write(&ptrval, 8);
    if (data->pInputAttachments) { for (uint32_t i = 0; i < data->inputAttachmentCount; i++) { vkUtilsPack_VkAttachmentReference(stream, data->pInputAttachments + i); } } } // VkAttachmentReference pInputAttachments
    stream->write(&data->colorAttachmentCount, 1 * 4); // u32 colorAttachmentCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pColorAttachments); stream->write(&ptrval, 8);
    if (data->pColorAttachments) { for (uint32_t i = 0; i < data->colorAttachmentCount; i++) { vkUtilsPack_VkAttachmentReference(stream, data->pColorAttachments + i); } } } // VkAttachmentReference pColorAttachments
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pResolveAttachments); stream->write(&ptrval, 8);
    if (data->pResolveAttachments) { for (uint32_t i = 0; i < data->colorAttachmentCount; i++) { vkUtilsPack_VkAttachmentReference(stream, data->pResolveAttachments + i); } } } // VkAttachmentReference pResolveAttachments
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDepthStencilAttachment); stream->write(&ptrval, 8);
    if (data->pDepthStencilAttachment) { for (uint32_t i = 0; i < 1; i++) { vkUtilsPack_VkAttachmentReference(stream, data->pDepthStencilAttachment + i); } } } // VkAttachmentReference pDepthStencilAttachment
    stream->write(&data->preserveAttachmentCount, 1 * 4); // u32 preserveAttachmentCount
    stream->write(data->pPreserveAttachments, data->preserveAttachmentCount * 4); // u32 pPreserveAttachments
}

VkSubpassDescription* vkUtilsUnpack_VkSubpassDescription(android::base::InplaceStream* stream) {
    VkSubpassDescription* data = new VkSubpassDescription; memset(data, 0, sizeof(VkSubpassDescription)); 
    stream->read(&data->flags, 1 * 4); // VkSubpassDescriptionFlags flags
    stream->read(&data->pipelineBindPoint, 1 * 4); // VkPipelineBindPoint pipelineBindPoint
    stream->read(&data->inputAttachmentCount, 1 * 4); // u32 inputAttachmentCount
    { // VkAttachmentReference pInputAttachments
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkAttachmentReference* tmpArr = ptrval ? new VkAttachmentReference[data->inputAttachmentCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->inputAttachmentCount; i++) {
        VkAttachmentReference* tmpUnpacked = vkUtilsUnpack_VkAttachmentReference(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkAttachmentReference)); delete tmpUnpacked; } }
        data->pInputAttachments = tmpArr;
    }
    stream->read(&data->colorAttachmentCount, 1 * 4); // u32 colorAttachmentCount
    { // VkAttachmentReference pColorAttachments
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkAttachmentReference* tmpArr = ptrval ? new VkAttachmentReference[data->colorAttachmentCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->colorAttachmentCount; i++) {
        VkAttachmentReference* tmpUnpacked = vkUtilsUnpack_VkAttachmentReference(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkAttachmentReference)); delete tmpUnpacked; } }
        data->pColorAttachments = tmpArr;
    }
    { // VkAttachmentReference pResolveAttachments
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkAttachmentReference* tmpArr = ptrval ? new VkAttachmentReference[data->colorAttachmentCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->colorAttachmentCount; i++) {
        VkAttachmentReference* tmpUnpacked = vkUtilsUnpack_VkAttachmentReference(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkAttachmentReference)); delete tmpUnpacked; } }
        data->pResolveAttachments = tmpArr;
    }
    { // VkAttachmentReference pDepthStencilAttachment
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkAttachmentReference* tmpArr = ptrval ? new VkAttachmentReference[1] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < 1; i++) {
        VkAttachmentReference* tmpUnpacked = vkUtilsUnpack_VkAttachmentReference(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkAttachmentReference)); delete tmpUnpacked; } }
        data->pDepthStencilAttachment = tmpArr;
    }
    stream->read(&data->preserveAttachmentCount, 1 * 4); // u32 preserveAttachmentCount
    data->pPreserveAttachments = data->preserveAttachmentCount ? new u32[data->preserveAttachmentCount] : nullptr; // u32 pPreserveAttachments
    stream->read((char*)data->pPreserveAttachments, data->preserveAttachmentCount * 4); // u32 pPreserveAttachments
    return data;
}

uint32_t vkUtilsEncodingSize_VkSubpassDependency(const VkSubpassDependency* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 srcSubpass
    res += 1 * 4; // u32 dstSubpass
    res += 1 * 4; // VkPipelineStageFlags srcStageMask
    res += 1 * 4; // VkPipelineStageFlags dstStageMask
    res += 1 * 4; // VkAccessFlags srcAccessMask
    res += 1 * 4; // VkAccessFlags dstAccessMask
    res += 1 * 4; // VkDependencyFlags dependencyFlags
    return res;
}

void vkUtilsPack_VkSubpassDependency(android::base::InplaceStream* stream, const VkSubpassDependency* data) {
    if (!data) return;
    stream->write(&data->srcSubpass, 1 * 4); // u32 srcSubpass
    stream->write(&data->dstSubpass, 1 * 4); // u32 dstSubpass
    stream->write(&data->srcStageMask, 1 * 4); // VkPipelineStageFlags srcStageMask
    stream->write(&data->dstStageMask, 1 * 4); // VkPipelineStageFlags dstStageMask
    stream->write(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->write(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    stream->write(&data->dependencyFlags, 1 * 4); // VkDependencyFlags dependencyFlags
}

VkSubpassDependency* vkUtilsUnpack_VkSubpassDependency(android::base::InplaceStream* stream) {
    VkSubpassDependency* data = new VkSubpassDependency; memset(data, 0, sizeof(VkSubpassDependency)); 
    stream->read(&data->srcSubpass, 1 * 4); // u32 srcSubpass
    stream->read(&data->dstSubpass, 1 * 4); // u32 dstSubpass
    stream->read(&data->srcStageMask, 1 * 4); // VkPipelineStageFlags srcStageMask
    stream->read(&data->dstStageMask, 1 * 4); // VkPipelineStageFlags dstStageMask
    stream->read(&data->srcAccessMask, 1 * 4); // VkAccessFlags srcAccessMask
    stream->read(&data->dstAccessMask, 1 * 4); // VkAccessFlags dstAccessMask
    stream->read(&data->dependencyFlags, 1 * 4); // VkDependencyFlags dependencyFlags
    return data;
}

uint32_t vkUtilsEncodingSize_VkRenderPassCreateInfo(const VkRenderPassCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkRenderPassCreateFlags flags
    res += 1 * 4; // u32 attachmentCount
    res += 8; if (data->pAttachments) { res += data->attachmentCount * vkUtilsEncodingSize_VkAttachmentDescription(data->pAttachments); } // VkAttachmentDescription pAttachments
    res += 1 * 4; // u32 subpassCount
    res += 8; if (data->pSubpasses) { res += data->subpassCount * vkUtilsEncodingSize_VkSubpassDescription(data->pSubpasses); } // VkSubpassDescription pSubpasses
    res += 1 * 4; // u32 dependencyCount
    res += 8; if (data->pDependencies) { res += data->dependencyCount * vkUtilsEncodingSize_VkSubpassDependency(data->pDependencies); } // VkSubpassDependency pDependencies
    return res;
}

void vkUtilsPack_VkRenderPassCreateInfo(android::base::InplaceStream* stream, const VkRenderPassCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkRenderPassCreateFlags flags
    stream->write(&data->attachmentCount, 1 * 4); // u32 attachmentCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pAttachments); stream->write(&ptrval, 8);
    if (data->pAttachments) { for (uint32_t i = 0; i < data->attachmentCount; i++) { vkUtilsPack_VkAttachmentDescription(stream, data->pAttachments + i); } } } // VkAttachmentDescription pAttachments
    stream->write(&data->subpassCount, 1 * 4); // u32 subpassCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pSubpasses); stream->write(&ptrval, 8);
    if (data->pSubpasses) { for (uint32_t i = 0; i < data->subpassCount; i++) { vkUtilsPack_VkSubpassDescription(stream, data->pSubpasses + i); } } } // VkSubpassDescription pSubpasses
    stream->write(&data->dependencyCount, 1 * 4); // u32 dependencyCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDependencies); stream->write(&ptrval, 8);
    if (data->pDependencies) { for (uint32_t i = 0; i < data->dependencyCount; i++) { vkUtilsPack_VkSubpassDependency(stream, data->pDependencies + i); } } } // VkSubpassDependency pDependencies
}

VkRenderPassCreateInfo* vkUtilsUnpack_VkRenderPassCreateInfo(android::base::InplaceStream* stream) {
    VkRenderPassCreateInfo* data = new VkRenderPassCreateInfo; memset(data, 0, sizeof(VkRenderPassCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkRenderPassCreateFlags flags
    stream->read(&data->attachmentCount, 1 * 4); // u32 attachmentCount
    { // VkAttachmentDescription pAttachments
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkAttachmentDescription* tmpArr = ptrval ? new VkAttachmentDescription[data->attachmentCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->attachmentCount; i++) {
        VkAttachmentDescription* tmpUnpacked = vkUtilsUnpack_VkAttachmentDescription(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkAttachmentDescription)); delete tmpUnpacked; } }
        data->pAttachments = tmpArr;
    }
    stream->read(&data->subpassCount, 1 * 4); // u32 subpassCount
    { // VkSubpassDescription pSubpasses
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSubpassDescription* tmpArr = ptrval ? new VkSubpassDescription[data->subpassCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->subpassCount; i++) {
        VkSubpassDescription* tmpUnpacked = vkUtilsUnpack_VkSubpassDescription(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSubpassDescription)); delete tmpUnpacked; } }
        data->pSubpasses = tmpArr;
    }
    stream->read(&data->dependencyCount, 1 * 4); // u32 dependencyCount
    { // VkSubpassDependency pDependencies
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkSubpassDependency* tmpArr = ptrval ? new VkSubpassDependency[data->dependencyCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->dependencyCount; i++) {
        VkSubpassDependency* tmpUnpacked = vkUtilsUnpack_VkSubpassDependency(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkSubpassDependency)); delete tmpUnpacked; } }
        data->pDependencies = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkEventCreateInfo(const VkEventCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkEventCreateFlags flags
    return res;
}

void vkUtilsPack_VkEventCreateInfo(android::base::InplaceStream* stream, const VkEventCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkEventCreateFlags flags
}

VkEventCreateInfo* vkUtilsUnpack_VkEventCreateInfo(android::base::InplaceStream* stream) {
    VkEventCreateInfo* data = new VkEventCreateInfo; memset(data, 0, sizeof(VkEventCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkEventCreateFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkFenceCreateInfo(const VkFenceCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkFenceCreateFlags flags
    return res;
}

void vkUtilsPack_VkFenceCreateInfo(android::base::InplaceStream* stream, const VkFenceCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkFenceCreateFlags flags
}

VkFenceCreateInfo* vkUtilsUnpack_VkFenceCreateInfo(android::base::InplaceStream* stream) {
    VkFenceCreateInfo* data = new VkFenceCreateInfo; memset(data, 0, sizeof(VkFenceCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkFenceCreateFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkBool32 robustBufferAccess
    res += 1 * 4; // VkBool32 fullDrawIndexUint32
    res += 1 * 4; // VkBool32 imageCubeArray
    res += 1 * 4; // VkBool32 independentBlend
    res += 1 * 4; // VkBool32 geometryShader
    res += 1 * 4; // VkBool32 tessellationShader
    res += 1 * 4; // VkBool32 sampleRateShading
    res += 1 * 4; // VkBool32 dualSrcBlend
    res += 1 * 4; // VkBool32 logicOp
    res += 1 * 4; // VkBool32 multiDrawIndirect
    res += 1 * 4; // VkBool32 drawIndirectFirstInstance
    res += 1 * 4; // VkBool32 depthClamp
    res += 1 * 4; // VkBool32 depthBiasClamp
    res += 1 * 4; // VkBool32 fillModeNonSolid
    res += 1 * 4; // VkBool32 depthBounds
    res += 1 * 4; // VkBool32 wideLines
    res += 1 * 4; // VkBool32 largePoints
    res += 1 * 4; // VkBool32 alphaToOne
    res += 1 * 4; // VkBool32 multiViewport
    res += 1 * 4; // VkBool32 samplerAnisotropy
    res += 1 * 4; // VkBool32 textureCompressionETC2
    res += 1 * 4; // VkBool32 textureCompressionASTC_LDR
    res += 1 * 4; // VkBool32 textureCompressionBC
    res += 1 * 4; // VkBool32 occlusionQueryPrecise
    res += 1 * 4; // VkBool32 pipelineStatisticsQuery
    res += 1 * 4; // VkBool32 vertexPipelineStoresAndAtomics
    res += 1 * 4; // VkBool32 fragmentStoresAndAtomics
    res += 1 * 4; // VkBool32 shaderTessellationAndGeometryPointSize
    res += 1 * 4; // VkBool32 shaderImageGatherExtended
    res += 1 * 4; // VkBool32 shaderStorageImageExtendedFormats
    res += 1 * 4; // VkBool32 shaderStorageImageMultisample
    res += 1 * 4; // VkBool32 shaderStorageImageReadWithoutFormat
    res += 1 * 4; // VkBool32 shaderStorageImageWriteWithoutFormat
    res += 1 * 4; // VkBool32 shaderUniformBufferArrayDynamicIndexing
    res += 1 * 4; // VkBool32 shaderSampledImageArrayDynamicIndexing
    res += 1 * 4; // VkBool32 shaderStorageBufferArrayDynamicIndexing
    res += 1 * 4; // VkBool32 shaderStorageImageArrayDynamicIndexing
    res += 1 * 4; // VkBool32 shaderClipDistance
    res += 1 * 4; // VkBool32 shaderCullDistance
    res += 1 * 4; // VkBool32 shaderFloat64
    res += 1 * 4; // VkBool32 shaderInt64
    res += 1 * 4; // VkBool32 shaderInt16
    res += 1 * 4; // VkBool32 shaderResourceResidency
    res += 1 * 4; // VkBool32 shaderResourceMinLod
    res += 1 * 4; // VkBool32 sparseBinding
    res += 1 * 4; // VkBool32 sparseResidencyBuffer
    res += 1 * 4; // VkBool32 sparseResidencyImage2D
    res += 1 * 4; // VkBool32 sparseResidencyImage3D
    res += 1 * 4; // VkBool32 sparseResidency2Samples
    res += 1 * 4; // VkBool32 sparseResidency4Samples
    res += 1 * 4; // VkBool32 sparseResidency8Samples
    res += 1 * 4; // VkBool32 sparseResidency16Samples
    res += 1 * 4; // VkBool32 sparseResidencyAliased
    res += 1 * 4; // VkBool32 variableMultisampleRate
    res += 1 * 4; // VkBool32 inheritedQueries
    return res;
}

void vkUtilsPack_VkPhysicalDeviceFeatures(android::base::InplaceStream* stream, const VkPhysicalDeviceFeatures* data) {
    if (!data) return;
    stream->write(&data->robustBufferAccess, 1 * 4); // VkBool32 robustBufferAccess
    stream->write(&data->fullDrawIndexUint32, 1 * 4); // VkBool32 fullDrawIndexUint32
    stream->write(&data->imageCubeArray, 1 * 4); // VkBool32 imageCubeArray
    stream->write(&data->independentBlend, 1 * 4); // VkBool32 independentBlend
    stream->write(&data->geometryShader, 1 * 4); // VkBool32 geometryShader
    stream->write(&data->tessellationShader, 1 * 4); // VkBool32 tessellationShader
    stream->write(&data->sampleRateShading, 1 * 4); // VkBool32 sampleRateShading
    stream->write(&data->dualSrcBlend, 1 * 4); // VkBool32 dualSrcBlend
    stream->write(&data->logicOp, 1 * 4); // VkBool32 logicOp
    stream->write(&data->multiDrawIndirect, 1 * 4); // VkBool32 multiDrawIndirect
    stream->write(&data->drawIndirectFirstInstance, 1 * 4); // VkBool32 drawIndirectFirstInstance
    stream->write(&data->depthClamp, 1 * 4); // VkBool32 depthClamp
    stream->write(&data->depthBiasClamp, 1 * 4); // VkBool32 depthBiasClamp
    stream->write(&data->fillModeNonSolid, 1 * 4); // VkBool32 fillModeNonSolid
    stream->write(&data->depthBounds, 1 * 4); // VkBool32 depthBounds
    stream->write(&data->wideLines, 1 * 4); // VkBool32 wideLines
    stream->write(&data->largePoints, 1 * 4); // VkBool32 largePoints
    stream->write(&data->alphaToOne, 1 * 4); // VkBool32 alphaToOne
    stream->write(&data->multiViewport, 1 * 4); // VkBool32 multiViewport
    stream->write(&data->samplerAnisotropy, 1 * 4); // VkBool32 samplerAnisotropy
    stream->write(&data->textureCompressionETC2, 1 * 4); // VkBool32 textureCompressionETC2
    stream->write(&data->textureCompressionASTC_LDR, 1 * 4); // VkBool32 textureCompressionASTC_LDR
    stream->write(&data->textureCompressionBC, 1 * 4); // VkBool32 textureCompressionBC
    stream->write(&data->occlusionQueryPrecise, 1 * 4); // VkBool32 occlusionQueryPrecise
    stream->write(&data->pipelineStatisticsQuery, 1 * 4); // VkBool32 pipelineStatisticsQuery
    stream->write(&data->vertexPipelineStoresAndAtomics, 1 * 4); // VkBool32 vertexPipelineStoresAndAtomics
    stream->write(&data->fragmentStoresAndAtomics, 1 * 4); // VkBool32 fragmentStoresAndAtomics
    stream->write(&data->shaderTessellationAndGeometryPointSize, 1 * 4); // VkBool32 shaderTessellationAndGeometryPointSize
    stream->write(&data->shaderImageGatherExtended, 1 * 4); // VkBool32 shaderImageGatherExtended
    stream->write(&data->shaderStorageImageExtendedFormats, 1 * 4); // VkBool32 shaderStorageImageExtendedFormats
    stream->write(&data->shaderStorageImageMultisample, 1 * 4); // VkBool32 shaderStorageImageMultisample
    stream->write(&data->shaderStorageImageReadWithoutFormat, 1 * 4); // VkBool32 shaderStorageImageReadWithoutFormat
    stream->write(&data->shaderStorageImageWriteWithoutFormat, 1 * 4); // VkBool32 shaderStorageImageWriteWithoutFormat
    stream->write(&data->shaderUniformBufferArrayDynamicIndexing, 1 * 4); // VkBool32 shaderUniformBufferArrayDynamicIndexing
    stream->write(&data->shaderSampledImageArrayDynamicIndexing, 1 * 4); // VkBool32 shaderSampledImageArrayDynamicIndexing
    stream->write(&data->shaderStorageBufferArrayDynamicIndexing, 1 * 4); // VkBool32 shaderStorageBufferArrayDynamicIndexing
    stream->write(&data->shaderStorageImageArrayDynamicIndexing, 1 * 4); // VkBool32 shaderStorageImageArrayDynamicIndexing
    stream->write(&data->shaderClipDistance, 1 * 4); // VkBool32 shaderClipDistance
    stream->write(&data->shaderCullDistance, 1 * 4); // VkBool32 shaderCullDistance
    stream->write(&data->shaderFloat64, 1 * 4); // VkBool32 shaderFloat64
    stream->write(&data->shaderInt64, 1 * 4); // VkBool32 shaderInt64
    stream->write(&data->shaderInt16, 1 * 4); // VkBool32 shaderInt16
    stream->write(&data->shaderResourceResidency, 1 * 4); // VkBool32 shaderResourceResidency
    stream->write(&data->shaderResourceMinLod, 1 * 4); // VkBool32 shaderResourceMinLod
    stream->write(&data->sparseBinding, 1 * 4); // VkBool32 sparseBinding
    stream->write(&data->sparseResidencyBuffer, 1 * 4); // VkBool32 sparseResidencyBuffer
    stream->write(&data->sparseResidencyImage2D, 1 * 4); // VkBool32 sparseResidencyImage2D
    stream->write(&data->sparseResidencyImage3D, 1 * 4); // VkBool32 sparseResidencyImage3D
    stream->write(&data->sparseResidency2Samples, 1 * 4); // VkBool32 sparseResidency2Samples
    stream->write(&data->sparseResidency4Samples, 1 * 4); // VkBool32 sparseResidency4Samples
    stream->write(&data->sparseResidency8Samples, 1 * 4); // VkBool32 sparseResidency8Samples
    stream->write(&data->sparseResidency16Samples, 1 * 4); // VkBool32 sparseResidency16Samples
    stream->write(&data->sparseResidencyAliased, 1 * 4); // VkBool32 sparseResidencyAliased
    stream->write(&data->variableMultisampleRate, 1 * 4); // VkBool32 variableMultisampleRate
    stream->write(&data->inheritedQueries, 1 * 4); // VkBool32 inheritedQueries
}

VkPhysicalDeviceFeatures* vkUtilsUnpack_VkPhysicalDeviceFeatures(android::base::InplaceStream* stream) {
    VkPhysicalDeviceFeatures* data = new VkPhysicalDeviceFeatures; memset(data, 0, sizeof(VkPhysicalDeviceFeatures)); 
    stream->read(&data->robustBufferAccess, 1 * 4); // VkBool32 robustBufferAccess
    stream->read(&data->fullDrawIndexUint32, 1 * 4); // VkBool32 fullDrawIndexUint32
    stream->read(&data->imageCubeArray, 1 * 4); // VkBool32 imageCubeArray
    stream->read(&data->independentBlend, 1 * 4); // VkBool32 independentBlend
    stream->read(&data->geometryShader, 1 * 4); // VkBool32 geometryShader
    stream->read(&data->tessellationShader, 1 * 4); // VkBool32 tessellationShader
    stream->read(&data->sampleRateShading, 1 * 4); // VkBool32 sampleRateShading
    stream->read(&data->dualSrcBlend, 1 * 4); // VkBool32 dualSrcBlend
    stream->read(&data->logicOp, 1 * 4); // VkBool32 logicOp
    stream->read(&data->multiDrawIndirect, 1 * 4); // VkBool32 multiDrawIndirect
    stream->read(&data->drawIndirectFirstInstance, 1 * 4); // VkBool32 drawIndirectFirstInstance
    stream->read(&data->depthClamp, 1 * 4); // VkBool32 depthClamp
    stream->read(&data->depthBiasClamp, 1 * 4); // VkBool32 depthBiasClamp
    stream->read(&data->fillModeNonSolid, 1 * 4); // VkBool32 fillModeNonSolid
    stream->read(&data->depthBounds, 1 * 4); // VkBool32 depthBounds
    stream->read(&data->wideLines, 1 * 4); // VkBool32 wideLines
    stream->read(&data->largePoints, 1 * 4); // VkBool32 largePoints
    stream->read(&data->alphaToOne, 1 * 4); // VkBool32 alphaToOne
    stream->read(&data->multiViewport, 1 * 4); // VkBool32 multiViewport
    stream->read(&data->samplerAnisotropy, 1 * 4); // VkBool32 samplerAnisotropy
    stream->read(&data->textureCompressionETC2, 1 * 4); // VkBool32 textureCompressionETC2
    stream->read(&data->textureCompressionASTC_LDR, 1 * 4); // VkBool32 textureCompressionASTC_LDR
    stream->read(&data->textureCompressionBC, 1 * 4); // VkBool32 textureCompressionBC
    stream->read(&data->occlusionQueryPrecise, 1 * 4); // VkBool32 occlusionQueryPrecise
    stream->read(&data->pipelineStatisticsQuery, 1 * 4); // VkBool32 pipelineStatisticsQuery
    stream->read(&data->vertexPipelineStoresAndAtomics, 1 * 4); // VkBool32 vertexPipelineStoresAndAtomics
    stream->read(&data->fragmentStoresAndAtomics, 1 * 4); // VkBool32 fragmentStoresAndAtomics
    stream->read(&data->shaderTessellationAndGeometryPointSize, 1 * 4); // VkBool32 shaderTessellationAndGeometryPointSize
    stream->read(&data->shaderImageGatherExtended, 1 * 4); // VkBool32 shaderImageGatherExtended
    stream->read(&data->shaderStorageImageExtendedFormats, 1 * 4); // VkBool32 shaderStorageImageExtendedFormats
    stream->read(&data->shaderStorageImageMultisample, 1 * 4); // VkBool32 shaderStorageImageMultisample
    stream->read(&data->shaderStorageImageReadWithoutFormat, 1 * 4); // VkBool32 shaderStorageImageReadWithoutFormat
    stream->read(&data->shaderStorageImageWriteWithoutFormat, 1 * 4); // VkBool32 shaderStorageImageWriteWithoutFormat
    stream->read(&data->shaderUniformBufferArrayDynamicIndexing, 1 * 4); // VkBool32 shaderUniformBufferArrayDynamicIndexing
    stream->read(&data->shaderSampledImageArrayDynamicIndexing, 1 * 4); // VkBool32 shaderSampledImageArrayDynamicIndexing
    stream->read(&data->shaderStorageBufferArrayDynamicIndexing, 1 * 4); // VkBool32 shaderStorageBufferArrayDynamicIndexing
    stream->read(&data->shaderStorageImageArrayDynamicIndexing, 1 * 4); // VkBool32 shaderStorageImageArrayDynamicIndexing
    stream->read(&data->shaderClipDistance, 1 * 4); // VkBool32 shaderClipDistance
    stream->read(&data->shaderCullDistance, 1 * 4); // VkBool32 shaderCullDistance
    stream->read(&data->shaderFloat64, 1 * 4); // VkBool32 shaderFloat64
    stream->read(&data->shaderInt64, 1 * 4); // VkBool32 shaderInt64
    stream->read(&data->shaderInt16, 1 * 4); // VkBool32 shaderInt16
    stream->read(&data->shaderResourceResidency, 1 * 4); // VkBool32 shaderResourceResidency
    stream->read(&data->shaderResourceMinLod, 1 * 4); // VkBool32 shaderResourceMinLod
    stream->read(&data->sparseBinding, 1 * 4); // VkBool32 sparseBinding
    stream->read(&data->sparseResidencyBuffer, 1 * 4); // VkBool32 sparseResidencyBuffer
    stream->read(&data->sparseResidencyImage2D, 1 * 4); // VkBool32 sparseResidencyImage2D
    stream->read(&data->sparseResidencyImage3D, 1 * 4); // VkBool32 sparseResidencyImage3D
    stream->read(&data->sparseResidency2Samples, 1 * 4); // VkBool32 sparseResidency2Samples
    stream->read(&data->sparseResidency4Samples, 1 * 4); // VkBool32 sparseResidency4Samples
    stream->read(&data->sparseResidency8Samples, 1 * 4); // VkBool32 sparseResidency8Samples
    stream->read(&data->sparseResidency16Samples, 1 * 4); // VkBool32 sparseResidency16Samples
    stream->read(&data->sparseResidencyAliased, 1 * 4); // VkBool32 sparseResidencyAliased
    stream->read(&data->variableMultisampleRate, 1 * 4); // VkBool32 variableMultisampleRate
    stream->read(&data->inheritedQueries, 1 * 4); // VkBool32 inheritedQueries
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceLimits(const VkPhysicalDeviceLimits* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 maxImageDimension1D
    res += 1 * 4; // u32 maxImageDimension2D
    res += 1 * 4; // u32 maxImageDimension3D
    res += 1 * 4; // u32 maxImageDimensionCube
    res += 1 * 4; // u32 maxImageArrayLayers
    res += 1 * 4; // u32 maxTexelBufferElements
    res += 1 * 4; // u32 maxUniformBufferRange
    res += 1 * 4; // u32 maxStorageBufferRange
    res += 1 * 4; // u32 maxPushConstantsSize
    res += 1 * 4; // u32 maxMemoryAllocationCount
    res += 1 * 4; // u32 maxSamplerAllocationCount
    res += 1 * 8; // VkDeviceSize bufferImageGranularity
    res += 1 * 8; // VkDeviceSize sparseAddressSpaceSize
    res += 1 * 4; // u32 maxBoundDescriptorSets
    res += 1 * 4; // u32 maxPerStageDescriptorSamplers
    res += 1 * 4; // u32 maxPerStageDescriptorUniformBuffers
    res += 1 * 4; // u32 maxPerStageDescriptorStorageBuffers
    res += 1 * 4; // u32 maxPerStageDescriptorSampledImages
    res += 1 * 4; // u32 maxPerStageDescriptorStorageImages
    res += 1 * 4; // u32 maxPerStageDescriptorInputAttachments
    res += 1 * 4; // u32 maxPerStageResources
    res += 1 * 4; // u32 maxDescriptorSetSamplers
    res += 1 * 4; // u32 maxDescriptorSetUniformBuffers
    res += 1 * 4; // u32 maxDescriptorSetUniformBuffersDynamic
    res += 1 * 4; // u32 maxDescriptorSetStorageBuffers
    res += 1 * 4; // u32 maxDescriptorSetStorageBuffersDynamic
    res += 1 * 4; // u32 maxDescriptorSetSampledImages
    res += 1 * 4; // u32 maxDescriptorSetStorageImages
    res += 1 * 4; // u32 maxDescriptorSetInputAttachments
    res += 1 * 4; // u32 maxVertexInputAttributes
    res += 1 * 4; // u32 maxVertexInputBindings
    res += 1 * 4; // u32 maxVertexInputAttributeOffset
    res += 1 * 4; // u32 maxVertexInputBindingStride
    res += 1 * 4; // u32 maxVertexOutputComponents
    res += 1 * 4; // u32 maxTessellationGenerationLevel
    res += 1 * 4; // u32 maxTessellationPatchSize
    res += 1 * 4; // u32 maxTessellationControlPerVertexInputComponents
    res += 1 * 4; // u32 maxTessellationControlPerVertexOutputComponents
    res += 1 * 4; // u32 maxTessellationControlPerPatchOutputComponents
    res += 1 * 4; // u32 maxTessellationControlTotalOutputComponents
    res += 1 * 4; // u32 maxTessellationEvaluationInputComponents
    res += 1 * 4; // u32 maxTessellationEvaluationOutputComponents
    res += 1 * 4; // u32 maxGeometryShaderInvocations
    res += 1 * 4; // u32 maxGeometryInputComponents
    res += 1 * 4; // u32 maxGeometryOutputComponents
    res += 1 * 4; // u32 maxGeometryOutputVertices
    res += 1 * 4; // u32 maxGeometryTotalOutputComponents
    res += 1 * 4; // u32 maxFragmentInputComponents
    res += 1 * 4; // u32 maxFragmentOutputAttachments
    res += 1 * 4; // u32 maxFragmentDualSrcAttachments
    res += 1 * 4; // u32 maxFragmentCombinedOutputResources
    res += 1 * 4; // u32 maxComputeSharedMemorySize
    res += 3 * 4; // u32 maxComputeWorkGroupCount
    res += 1 * 4; // u32 maxComputeWorkGroupInvocations
    res += 3 * 4; // u32 maxComputeWorkGroupSize
    res += 1 * 4; // u32 subPixelPrecisionBits
    res += 1 * 4; // u32 subTexelPrecisionBits
    res += 1 * 4; // u32 mipmapPrecisionBits
    res += 1 * 4; // u32 maxDrawIndexedIndexValue
    res += 1 * 4; // u32 maxDrawIndirectCount
    res += 1 * 4; // f32 maxSamplerLodBias
    res += 1 * 4; // f32 maxSamplerAnisotropy
    res += 1 * 4; // u32 maxViewports
    res += 2 * 4; // u32 maxViewportDimensions
    res += 2 * 4; // f32 viewportBoundsRange
    res += 1 * 4; // u32 viewportSubPixelBits
    res += 1 * 4; // size_t minMemoryMapAlignment
    res += 1 * 8; // VkDeviceSize minTexelBufferOffsetAlignment
    res += 1 * 8; // VkDeviceSize minUniformBufferOffsetAlignment
    res += 1 * 8; // VkDeviceSize minStorageBufferOffsetAlignment
    res += 1 * 4; // s32 minTexelOffset
    res += 1 * 4; // u32 maxTexelOffset
    res += 1 * 4; // s32 minTexelGatherOffset
    res += 1 * 4; // u32 maxTexelGatherOffset
    res += 1 * 4; // f32 minInterpolationOffset
    res += 1 * 4; // f32 maxInterpolationOffset
    res += 1 * 4; // u32 subPixelInterpolationOffsetBits
    res += 1 * 4; // u32 maxFramebufferWidth
    res += 1 * 4; // u32 maxFramebufferHeight
    res += 1 * 4; // u32 maxFramebufferLayers
    res += 1 * 4; // VkSampleCountFlags framebufferColorSampleCounts
    res += 1 * 4; // VkSampleCountFlags framebufferDepthSampleCounts
    res += 1 * 4; // VkSampleCountFlags framebufferStencilSampleCounts
    res += 1 * 4; // VkSampleCountFlags framebufferNoAttachmentsSampleCounts
    res += 1 * 4; // u32 maxColorAttachments
    res += 1 * 4; // VkSampleCountFlags sampledImageColorSampleCounts
    res += 1 * 4; // VkSampleCountFlags sampledImageIntegerSampleCounts
    res += 1 * 4; // VkSampleCountFlags sampledImageDepthSampleCounts
    res += 1 * 4; // VkSampleCountFlags sampledImageStencilSampleCounts
    res += 1 * 4; // VkSampleCountFlags storageImageSampleCounts
    res += 1 * 4; // u32 maxSampleMaskWords
    res += 1 * 4; // VkBool32 timestampComputeAndGraphics
    res += 1 * 4; // f32 timestampPeriod
    res += 1 * 4; // u32 maxClipDistances
    res += 1 * 4; // u32 maxCullDistances
    res += 1 * 4; // u32 maxCombinedClipAndCullDistances
    res += 1 * 4; // u32 discreteQueuePriorities
    res += 2 * 4; // f32 pointSizeRange
    res += 2 * 4; // f32 lineWidthRange
    res += 1 * 4; // f32 pointSizeGranularity
    res += 1 * 4; // f32 lineWidthGranularity
    res += 1 * 4; // VkBool32 strictLines
    res += 1 * 4; // VkBool32 standardSampleLocations
    res += 1 * 8; // VkDeviceSize optimalBufferCopyOffsetAlignment
    res += 1 * 8; // VkDeviceSize optimalBufferCopyRowPitchAlignment
    res += 1 * 8; // VkDeviceSize nonCoherentAtomSize
    return res;
}

void vkUtilsPack_VkPhysicalDeviceLimits(android::base::InplaceStream* stream, const VkPhysicalDeviceLimits* data) {
    if (!data) return;
    stream->write(&data->maxImageDimension1D, 1 * 4); // u32 maxImageDimension1D
    stream->write(&data->maxImageDimension2D, 1 * 4); // u32 maxImageDimension2D
    stream->write(&data->maxImageDimension3D, 1 * 4); // u32 maxImageDimension3D
    stream->write(&data->maxImageDimensionCube, 1 * 4); // u32 maxImageDimensionCube
    stream->write(&data->maxImageArrayLayers, 1 * 4); // u32 maxImageArrayLayers
    stream->write(&data->maxTexelBufferElements, 1 * 4); // u32 maxTexelBufferElements
    stream->write(&data->maxUniformBufferRange, 1 * 4); // u32 maxUniformBufferRange
    stream->write(&data->maxStorageBufferRange, 1 * 4); // u32 maxStorageBufferRange
    stream->write(&data->maxPushConstantsSize, 1 * 4); // u32 maxPushConstantsSize
    stream->write(&data->maxMemoryAllocationCount, 1 * 4); // u32 maxMemoryAllocationCount
    stream->write(&data->maxSamplerAllocationCount, 1 * 4); // u32 maxSamplerAllocationCount
    stream->write(&data->bufferImageGranularity, 1 * 8); // VkDeviceSize bufferImageGranularity
    stream->write(&data->sparseAddressSpaceSize, 1 * 8); // VkDeviceSize sparseAddressSpaceSize
    stream->write(&data->maxBoundDescriptorSets, 1 * 4); // u32 maxBoundDescriptorSets
    stream->write(&data->maxPerStageDescriptorSamplers, 1 * 4); // u32 maxPerStageDescriptorSamplers
    stream->write(&data->maxPerStageDescriptorUniformBuffers, 1 * 4); // u32 maxPerStageDescriptorUniformBuffers
    stream->write(&data->maxPerStageDescriptorStorageBuffers, 1 * 4); // u32 maxPerStageDescriptorStorageBuffers
    stream->write(&data->maxPerStageDescriptorSampledImages, 1 * 4); // u32 maxPerStageDescriptorSampledImages
    stream->write(&data->maxPerStageDescriptorStorageImages, 1 * 4); // u32 maxPerStageDescriptorStorageImages
    stream->write(&data->maxPerStageDescriptorInputAttachments, 1 * 4); // u32 maxPerStageDescriptorInputAttachments
    stream->write(&data->maxPerStageResources, 1 * 4); // u32 maxPerStageResources
    stream->write(&data->maxDescriptorSetSamplers, 1 * 4); // u32 maxDescriptorSetSamplers
    stream->write(&data->maxDescriptorSetUniformBuffers, 1 * 4); // u32 maxDescriptorSetUniformBuffers
    stream->write(&data->maxDescriptorSetUniformBuffersDynamic, 1 * 4); // u32 maxDescriptorSetUniformBuffersDynamic
    stream->write(&data->maxDescriptorSetStorageBuffers, 1 * 4); // u32 maxDescriptorSetStorageBuffers
    stream->write(&data->maxDescriptorSetStorageBuffersDynamic, 1 * 4); // u32 maxDescriptorSetStorageBuffersDynamic
    stream->write(&data->maxDescriptorSetSampledImages, 1 * 4); // u32 maxDescriptorSetSampledImages
    stream->write(&data->maxDescriptorSetStorageImages, 1 * 4); // u32 maxDescriptorSetStorageImages
    stream->write(&data->maxDescriptorSetInputAttachments, 1 * 4); // u32 maxDescriptorSetInputAttachments
    stream->write(&data->maxVertexInputAttributes, 1 * 4); // u32 maxVertexInputAttributes
    stream->write(&data->maxVertexInputBindings, 1 * 4); // u32 maxVertexInputBindings
    stream->write(&data->maxVertexInputAttributeOffset, 1 * 4); // u32 maxVertexInputAttributeOffset
    stream->write(&data->maxVertexInputBindingStride, 1 * 4); // u32 maxVertexInputBindingStride
    stream->write(&data->maxVertexOutputComponents, 1 * 4); // u32 maxVertexOutputComponents
    stream->write(&data->maxTessellationGenerationLevel, 1 * 4); // u32 maxTessellationGenerationLevel
    stream->write(&data->maxTessellationPatchSize, 1 * 4); // u32 maxTessellationPatchSize
    stream->write(&data->maxTessellationControlPerVertexInputComponents, 1 * 4); // u32 maxTessellationControlPerVertexInputComponents
    stream->write(&data->maxTessellationControlPerVertexOutputComponents, 1 * 4); // u32 maxTessellationControlPerVertexOutputComponents
    stream->write(&data->maxTessellationControlPerPatchOutputComponents, 1 * 4); // u32 maxTessellationControlPerPatchOutputComponents
    stream->write(&data->maxTessellationControlTotalOutputComponents, 1 * 4); // u32 maxTessellationControlTotalOutputComponents
    stream->write(&data->maxTessellationEvaluationInputComponents, 1 * 4); // u32 maxTessellationEvaluationInputComponents
    stream->write(&data->maxTessellationEvaluationOutputComponents, 1 * 4); // u32 maxTessellationEvaluationOutputComponents
    stream->write(&data->maxGeometryShaderInvocations, 1 * 4); // u32 maxGeometryShaderInvocations
    stream->write(&data->maxGeometryInputComponents, 1 * 4); // u32 maxGeometryInputComponents
    stream->write(&data->maxGeometryOutputComponents, 1 * 4); // u32 maxGeometryOutputComponents
    stream->write(&data->maxGeometryOutputVertices, 1 * 4); // u32 maxGeometryOutputVertices
    stream->write(&data->maxGeometryTotalOutputComponents, 1 * 4); // u32 maxGeometryTotalOutputComponents
    stream->write(&data->maxFragmentInputComponents, 1 * 4); // u32 maxFragmentInputComponents
    stream->write(&data->maxFragmentOutputAttachments, 1 * 4); // u32 maxFragmentOutputAttachments
    stream->write(&data->maxFragmentDualSrcAttachments, 1 * 4); // u32 maxFragmentDualSrcAttachments
    stream->write(&data->maxFragmentCombinedOutputResources, 1 * 4); // u32 maxFragmentCombinedOutputResources
    stream->write(&data->maxComputeSharedMemorySize, 1 * 4); // u32 maxComputeSharedMemorySize
    stream->write(&data->maxComputeWorkGroupCount, 3 * 4); // u32 maxComputeWorkGroupCount
    stream->write(&data->maxComputeWorkGroupInvocations, 1 * 4); // u32 maxComputeWorkGroupInvocations
    stream->write(&data->maxComputeWorkGroupSize, 3 * 4); // u32 maxComputeWorkGroupSize
    stream->write(&data->subPixelPrecisionBits, 1 * 4); // u32 subPixelPrecisionBits
    stream->write(&data->subTexelPrecisionBits, 1 * 4); // u32 subTexelPrecisionBits
    stream->write(&data->mipmapPrecisionBits, 1 * 4); // u32 mipmapPrecisionBits
    stream->write(&data->maxDrawIndexedIndexValue, 1 * 4); // u32 maxDrawIndexedIndexValue
    stream->write(&data->maxDrawIndirectCount, 1 * 4); // u32 maxDrawIndirectCount
    stream->write(&data->maxSamplerLodBias, 1 * 4); // f32 maxSamplerLodBias
    stream->write(&data->maxSamplerAnisotropy, 1 * 4); // f32 maxSamplerAnisotropy
    stream->write(&data->maxViewports, 1 * 4); // u32 maxViewports
    stream->write(&data->maxViewportDimensions, 2 * 4); // u32 maxViewportDimensions
    stream->write(&data->viewportBoundsRange, 2 * 4); // f32 viewportBoundsRange
    stream->write(&data->viewportSubPixelBits, 1 * 4); // u32 viewportSubPixelBits
    stream->write(&data->minMemoryMapAlignment, 1 * 4); // size_t minMemoryMapAlignment
    stream->write(&data->minTexelBufferOffsetAlignment, 1 * 8); // VkDeviceSize minTexelBufferOffsetAlignment
    stream->write(&data->minUniformBufferOffsetAlignment, 1 * 8); // VkDeviceSize minUniformBufferOffsetAlignment
    stream->write(&data->minStorageBufferOffsetAlignment, 1 * 8); // VkDeviceSize minStorageBufferOffsetAlignment
    stream->write(&data->minTexelOffset, 1 * 4); // s32 minTexelOffset
    stream->write(&data->maxTexelOffset, 1 * 4); // u32 maxTexelOffset
    stream->write(&data->minTexelGatherOffset, 1 * 4); // s32 minTexelGatherOffset
    stream->write(&data->maxTexelGatherOffset, 1 * 4); // u32 maxTexelGatherOffset
    stream->write(&data->minInterpolationOffset, 1 * 4); // f32 minInterpolationOffset
    stream->write(&data->maxInterpolationOffset, 1 * 4); // f32 maxInterpolationOffset
    stream->write(&data->subPixelInterpolationOffsetBits, 1 * 4); // u32 subPixelInterpolationOffsetBits
    stream->write(&data->maxFramebufferWidth, 1 * 4); // u32 maxFramebufferWidth
    stream->write(&data->maxFramebufferHeight, 1 * 4); // u32 maxFramebufferHeight
    stream->write(&data->maxFramebufferLayers, 1 * 4); // u32 maxFramebufferLayers
    stream->write(&data->framebufferColorSampleCounts, 1 * 4); // VkSampleCountFlags framebufferColorSampleCounts
    stream->write(&data->framebufferDepthSampleCounts, 1 * 4); // VkSampleCountFlags framebufferDepthSampleCounts
    stream->write(&data->framebufferStencilSampleCounts, 1 * 4); // VkSampleCountFlags framebufferStencilSampleCounts
    stream->write(&data->framebufferNoAttachmentsSampleCounts, 1 * 4); // VkSampleCountFlags framebufferNoAttachmentsSampleCounts
    stream->write(&data->maxColorAttachments, 1 * 4); // u32 maxColorAttachments
    stream->write(&data->sampledImageColorSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageColorSampleCounts
    stream->write(&data->sampledImageIntegerSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageIntegerSampleCounts
    stream->write(&data->sampledImageDepthSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageDepthSampleCounts
    stream->write(&data->sampledImageStencilSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageStencilSampleCounts
    stream->write(&data->storageImageSampleCounts, 1 * 4); // VkSampleCountFlags storageImageSampleCounts
    stream->write(&data->maxSampleMaskWords, 1 * 4); // u32 maxSampleMaskWords
    stream->write(&data->timestampComputeAndGraphics, 1 * 4); // VkBool32 timestampComputeAndGraphics
    stream->write(&data->timestampPeriod, 1 * 4); // f32 timestampPeriod
    stream->write(&data->maxClipDistances, 1 * 4); // u32 maxClipDistances
    stream->write(&data->maxCullDistances, 1 * 4); // u32 maxCullDistances
    stream->write(&data->maxCombinedClipAndCullDistances, 1 * 4); // u32 maxCombinedClipAndCullDistances
    stream->write(&data->discreteQueuePriorities, 1 * 4); // u32 discreteQueuePriorities
    stream->write(&data->pointSizeRange, 2 * 4); // f32 pointSizeRange
    stream->write(&data->lineWidthRange, 2 * 4); // f32 lineWidthRange
    stream->write(&data->pointSizeGranularity, 1 * 4); // f32 pointSizeGranularity
    stream->write(&data->lineWidthGranularity, 1 * 4); // f32 lineWidthGranularity
    stream->write(&data->strictLines, 1 * 4); // VkBool32 strictLines
    stream->write(&data->standardSampleLocations, 1 * 4); // VkBool32 standardSampleLocations
    stream->write(&data->optimalBufferCopyOffsetAlignment, 1 * 8); // VkDeviceSize optimalBufferCopyOffsetAlignment
    stream->write(&data->optimalBufferCopyRowPitchAlignment, 1 * 8); // VkDeviceSize optimalBufferCopyRowPitchAlignment
    stream->write(&data->nonCoherentAtomSize, 1 * 8); // VkDeviceSize nonCoherentAtomSize
}

VkPhysicalDeviceLimits* vkUtilsUnpack_VkPhysicalDeviceLimits(android::base::InplaceStream* stream) {
    VkPhysicalDeviceLimits* data = new VkPhysicalDeviceLimits; memset(data, 0, sizeof(VkPhysicalDeviceLimits)); 
    stream->read(&data->maxImageDimension1D, 1 * 4); // u32 maxImageDimension1D
    stream->read(&data->maxImageDimension2D, 1 * 4); // u32 maxImageDimension2D
    stream->read(&data->maxImageDimension3D, 1 * 4); // u32 maxImageDimension3D
    stream->read(&data->maxImageDimensionCube, 1 * 4); // u32 maxImageDimensionCube
    stream->read(&data->maxImageArrayLayers, 1 * 4); // u32 maxImageArrayLayers
    stream->read(&data->maxTexelBufferElements, 1 * 4); // u32 maxTexelBufferElements
    stream->read(&data->maxUniformBufferRange, 1 * 4); // u32 maxUniformBufferRange
    stream->read(&data->maxStorageBufferRange, 1 * 4); // u32 maxStorageBufferRange
    stream->read(&data->maxPushConstantsSize, 1 * 4); // u32 maxPushConstantsSize
    stream->read(&data->maxMemoryAllocationCount, 1 * 4); // u32 maxMemoryAllocationCount
    stream->read(&data->maxSamplerAllocationCount, 1 * 4); // u32 maxSamplerAllocationCount
    stream->read(&data->bufferImageGranularity, 1 * 8); // VkDeviceSize bufferImageGranularity
    stream->read(&data->sparseAddressSpaceSize, 1 * 8); // VkDeviceSize sparseAddressSpaceSize
    stream->read(&data->maxBoundDescriptorSets, 1 * 4); // u32 maxBoundDescriptorSets
    stream->read(&data->maxPerStageDescriptorSamplers, 1 * 4); // u32 maxPerStageDescriptorSamplers
    stream->read(&data->maxPerStageDescriptorUniformBuffers, 1 * 4); // u32 maxPerStageDescriptorUniformBuffers
    stream->read(&data->maxPerStageDescriptorStorageBuffers, 1 * 4); // u32 maxPerStageDescriptorStorageBuffers
    stream->read(&data->maxPerStageDescriptorSampledImages, 1 * 4); // u32 maxPerStageDescriptorSampledImages
    stream->read(&data->maxPerStageDescriptorStorageImages, 1 * 4); // u32 maxPerStageDescriptorStorageImages
    stream->read(&data->maxPerStageDescriptorInputAttachments, 1 * 4); // u32 maxPerStageDescriptorInputAttachments
    stream->read(&data->maxPerStageResources, 1 * 4); // u32 maxPerStageResources
    stream->read(&data->maxDescriptorSetSamplers, 1 * 4); // u32 maxDescriptorSetSamplers
    stream->read(&data->maxDescriptorSetUniformBuffers, 1 * 4); // u32 maxDescriptorSetUniformBuffers
    stream->read(&data->maxDescriptorSetUniformBuffersDynamic, 1 * 4); // u32 maxDescriptorSetUniformBuffersDynamic
    stream->read(&data->maxDescriptorSetStorageBuffers, 1 * 4); // u32 maxDescriptorSetStorageBuffers
    stream->read(&data->maxDescriptorSetStorageBuffersDynamic, 1 * 4); // u32 maxDescriptorSetStorageBuffersDynamic
    stream->read(&data->maxDescriptorSetSampledImages, 1 * 4); // u32 maxDescriptorSetSampledImages
    stream->read(&data->maxDescriptorSetStorageImages, 1 * 4); // u32 maxDescriptorSetStorageImages
    stream->read(&data->maxDescriptorSetInputAttachments, 1 * 4); // u32 maxDescriptorSetInputAttachments
    stream->read(&data->maxVertexInputAttributes, 1 * 4); // u32 maxVertexInputAttributes
    stream->read(&data->maxVertexInputBindings, 1 * 4); // u32 maxVertexInputBindings
    stream->read(&data->maxVertexInputAttributeOffset, 1 * 4); // u32 maxVertexInputAttributeOffset
    stream->read(&data->maxVertexInputBindingStride, 1 * 4); // u32 maxVertexInputBindingStride
    stream->read(&data->maxVertexOutputComponents, 1 * 4); // u32 maxVertexOutputComponents
    stream->read(&data->maxTessellationGenerationLevel, 1 * 4); // u32 maxTessellationGenerationLevel
    stream->read(&data->maxTessellationPatchSize, 1 * 4); // u32 maxTessellationPatchSize
    stream->read(&data->maxTessellationControlPerVertexInputComponents, 1 * 4); // u32 maxTessellationControlPerVertexInputComponents
    stream->read(&data->maxTessellationControlPerVertexOutputComponents, 1 * 4); // u32 maxTessellationControlPerVertexOutputComponents
    stream->read(&data->maxTessellationControlPerPatchOutputComponents, 1 * 4); // u32 maxTessellationControlPerPatchOutputComponents
    stream->read(&data->maxTessellationControlTotalOutputComponents, 1 * 4); // u32 maxTessellationControlTotalOutputComponents
    stream->read(&data->maxTessellationEvaluationInputComponents, 1 * 4); // u32 maxTessellationEvaluationInputComponents
    stream->read(&data->maxTessellationEvaluationOutputComponents, 1 * 4); // u32 maxTessellationEvaluationOutputComponents
    stream->read(&data->maxGeometryShaderInvocations, 1 * 4); // u32 maxGeometryShaderInvocations
    stream->read(&data->maxGeometryInputComponents, 1 * 4); // u32 maxGeometryInputComponents
    stream->read(&data->maxGeometryOutputComponents, 1 * 4); // u32 maxGeometryOutputComponents
    stream->read(&data->maxGeometryOutputVertices, 1 * 4); // u32 maxGeometryOutputVertices
    stream->read(&data->maxGeometryTotalOutputComponents, 1 * 4); // u32 maxGeometryTotalOutputComponents
    stream->read(&data->maxFragmentInputComponents, 1 * 4); // u32 maxFragmentInputComponents
    stream->read(&data->maxFragmentOutputAttachments, 1 * 4); // u32 maxFragmentOutputAttachments
    stream->read(&data->maxFragmentDualSrcAttachments, 1 * 4); // u32 maxFragmentDualSrcAttachments
    stream->read(&data->maxFragmentCombinedOutputResources, 1 * 4); // u32 maxFragmentCombinedOutputResources
    stream->read(&data->maxComputeSharedMemorySize, 1 * 4); // u32 maxComputeSharedMemorySize
    stream->read(&data->maxComputeWorkGroupCount, 3 * 4); // u32 maxComputeWorkGroupCount
    stream->read(&data->maxComputeWorkGroupInvocations, 1 * 4); // u32 maxComputeWorkGroupInvocations
    stream->read(&data->maxComputeWorkGroupSize, 3 * 4); // u32 maxComputeWorkGroupSize
    stream->read(&data->subPixelPrecisionBits, 1 * 4); // u32 subPixelPrecisionBits
    stream->read(&data->subTexelPrecisionBits, 1 * 4); // u32 subTexelPrecisionBits
    stream->read(&data->mipmapPrecisionBits, 1 * 4); // u32 mipmapPrecisionBits
    stream->read(&data->maxDrawIndexedIndexValue, 1 * 4); // u32 maxDrawIndexedIndexValue
    stream->read(&data->maxDrawIndirectCount, 1 * 4); // u32 maxDrawIndirectCount
    stream->read(&data->maxSamplerLodBias, 1 * 4); // f32 maxSamplerLodBias
    stream->read(&data->maxSamplerAnisotropy, 1 * 4); // f32 maxSamplerAnisotropy
    stream->read(&data->maxViewports, 1 * 4); // u32 maxViewports
    stream->read(&data->maxViewportDimensions, 2 * 4); // u32 maxViewportDimensions
    stream->read(&data->viewportBoundsRange, 2 * 4); // f32 viewportBoundsRange
    stream->read(&data->viewportSubPixelBits, 1 * 4); // u32 viewportSubPixelBits
    stream->read(&data->minMemoryMapAlignment, 1 * 4); // size_t minMemoryMapAlignment
    stream->read(&data->minTexelBufferOffsetAlignment, 1 * 8); // VkDeviceSize minTexelBufferOffsetAlignment
    stream->read(&data->minUniformBufferOffsetAlignment, 1 * 8); // VkDeviceSize minUniformBufferOffsetAlignment
    stream->read(&data->minStorageBufferOffsetAlignment, 1 * 8); // VkDeviceSize minStorageBufferOffsetAlignment
    stream->read(&data->minTexelOffset, 1 * 4); // s32 minTexelOffset
    stream->read(&data->maxTexelOffset, 1 * 4); // u32 maxTexelOffset
    stream->read(&data->minTexelGatherOffset, 1 * 4); // s32 minTexelGatherOffset
    stream->read(&data->maxTexelGatherOffset, 1 * 4); // u32 maxTexelGatherOffset
    stream->read(&data->minInterpolationOffset, 1 * 4); // f32 minInterpolationOffset
    stream->read(&data->maxInterpolationOffset, 1 * 4); // f32 maxInterpolationOffset
    stream->read(&data->subPixelInterpolationOffsetBits, 1 * 4); // u32 subPixelInterpolationOffsetBits
    stream->read(&data->maxFramebufferWidth, 1 * 4); // u32 maxFramebufferWidth
    stream->read(&data->maxFramebufferHeight, 1 * 4); // u32 maxFramebufferHeight
    stream->read(&data->maxFramebufferLayers, 1 * 4); // u32 maxFramebufferLayers
    stream->read(&data->framebufferColorSampleCounts, 1 * 4); // VkSampleCountFlags framebufferColorSampleCounts
    stream->read(&data->framebufferDepthSampleCounts, 1 * 4); // VkSampleCountFlags framebufferDepthSampleCounts
    stream->read(&data->framebufferStencilSampleCounts, 1 * 4); // VkSampleCountFlags framebufferStencilSampleCounts
    stream->read(&data->framebufferNoAttachmentsSampleCounts, 1 * 4); // VkSampleCountFlags framebufferNoAttachmentsSampleCounts
    stream->read(&data->maxColorAttachments, 1 * 4); // u32 maxColorAttachments
    stream->read(&data->sampledImageColorSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageColorSampleCounts
    stream->read(&data->sampledImageIntegerSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageIntegerSampleCounts
    stream->read(&data->sampledImageDepthSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageDepthSampleCounts
    stream->read(&data->sampledImageStencilSampleCounts, 1 * 4); // VkSampleCountFlags sampledImageStencilSampleCounts
    stream->read(&data->storageImageSampleCounts, 1 * 4); // VkSampleCountFlags storageImageSampleCounts
    stream->read(&data->maxSampleMaskWords, 1 * 4); // u32 maxSampleMaskWords
    stream->read(&data->timestampComputeAndGraphics, 1 * 4); // VkBool32 timestampComputeAndGraphics
    stream->read(&data->timestampPeriod, 1 * 4); // f32 timestampPeriod
    stream->read(&data->maxClipDistances, 1 * 4); // u32 maxClipDistances
    stream->read(&data->maxCullDistances, 1 * 4); // u32 maxCullDistances
    stream->read(&data->maxCombinedClipAndCullDistances, 1 * 4); // u32 maxCombinedClipAndCullDistances
    stream->read(&data->discreteQueuePriorities, 1 * 4); // u32 discreteQueuePriorities
    stream->read(&data->pointSizeRange, 2 * 4); // f32 pointSizeRange
    stream->read(&data->lineWidthRange, 2 * 4); // f32 lineWidthRange
    stream->read(&data->pointSizeGranularity, 1 * 4); // f32 pointSizeGranularity
    stream->read(&data->lineWidthGranularity, 1 * 4); // f32 lineWidthGranularity
    stream->read(&data->strictLines, 1 * 4); // VkBool32 strictLines
    stream->read(&data->standardSampleLocations, 1 * 4); // VkBool32 standardSampleLocations
    stream->read(&data->optimalBufferCopyOffsetAlignment, 1 * 8); // VkDeviceSize optimalBufferCopyOffsetAlignment
    stream->read(&data->optimalBufferCopyRowPitchAlignment, 1 * 8); // VkDeviceSize optimalBufferCopyRowPitchAlignment
    stream->read(&data->nonCoherentAtomSize, 1 * 8); // VkDeviceSize nonCoherentAtomSize
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceSparseProperties(const VkPhysicalDeviceSparseProperties* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkBool32 residencyStandard2DBlockShape
    res += 1 * 4; // VkBool32 residencyStandard2DMultisampleBlockShape
    res += 1 * 4; // VkBool32 residencyStandard3DBlockShape
    res += 1 * 4; // VkBool32 residencyAlignedMipSize
    res += 1 * 4; // VkBool32 residencyNonResidentStrict
    return res;
}

void vkUtilsPack_VkPhysicalDeviceSparseProperties(android::base::InplaceStream* stream, const VkPhysicalDeviceSparseProperties* data) {
    if (!data) return;
    stream->write(&data->residencyStandard2DBlockShape, 1 * 4); // VkBool32 residencyStandard2DBlockShape
    stream->write(&data->residencyStandard2DMultisampleBlockShape, 1 * 4); // VkBool32 residencyStandard2DMultisampleBlockShape
    stream->write(&data->residencyStandard3DBlockShape, 1 * 4); // VkBool32 residencyStandard3DBlockShape
    stream->write(&data->residencyAlignedMipSize, 1 * 4); // VkBool32 residencyAlignedMipSize
    stream->write(&data->residencyNonResidentStrict, 1 * 4); // VkBool32 residencyNonResidentStrict
}

VkPhysicalDeviceSparseProperties* vkUtilsUnpack_VkPhysicalDeviceSparseProperties(android::base::InplaceStream* stream) {
    VkPhysicalDeviceSparseProperties* data = new VkPhysicalDeviceSparseProperties; memset(data, 0, sizeof(VkPhysicalDeviceSparseProperties)); 
    stream->read(&data->residencyStandard2DBlockShape, 1 * 4); // VkBool32 residencyStandard2DBlockShape
    stream->read(&data->residencyStandard2DMultisampleBlockShape, 1 * 4); // VkBool32 residencyStandard2DMultisampleBlockShape
    stream->read(&data->residencyStandard3DBlockShape, 1 * 4); // VkBool32 residencyStandard3DBlockShape
    stream->read(&data->residencyAlignedMipSize, 1 * 4); // VkBool32 residencyAlignedMipSize
    stream->read(&data->residencyNonResidentStrict, 1 * 4); // VkBool32 residencyNonResidentStrict
    return data;
}

uint32_t vkUtilsEncodingSize_VkSemaphoreCreateInfo(const VkSemaphoreCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkSemaphoreCreateFlags flags
    return res;
}

void vkUtilsPack_VkSemaphoreCreateInfo(android::base::InplaceStream* stream, const VkSemaphoreCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkSemaphoreCreateFlags flags
}

VkSemaphoreCreateInfo* vkUtilsUnpack_VkSemaphoreCreateInfo(android::base::InplaceStream* stream) {
    VkSemaphoreCreateInfo* data = new VkSemaphoreCreateInfo; memset(data, 0, sizeof(VkSemaphoreCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkSemaphoreCreateFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkQueryPoolCreateInfo(const VkQueryPoolCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkQueryPoolCreateFlags flags
    res += 1 * 4; // VkQueryType queryType
    res += 1 * 4; // u32 queryCount
    res += 1 * 4; // VkQueryPipelineStatisticFlags pipelineStatistics
    return res;
}

void vkUtilsPack_VkQueryPoolCreateInfo(android::base::InplaceStream* stream, const VkQueryPoolCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkQueryPoolCreateFlags flags
    stream->write(&data->queryType, 1 * 4); // VkQueryType queryType
    stream->write(&data->queryCount, 1 * 4); // u32 queryCount
    stream->write(&data->pipelineStatistics, 1 * 4); // VkQueryPipelineStatisticFlags pipelineStatistics
}

VkQueryPoolCreateInfo* vkUtilsUnpack_VkQueryPoolCreateInfo(android::base::InplaceStream* stream) {
    VkQueryPoolCreateInfo* data = new VkQueryPoolCreateInfo; memset(data, 0, sizeof(VkQueryPoolCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkQueryPoolCreateFlags flags
    stream->read(&data->queryType, 1 * 4); // VkQueryType queryType
    stream->read(&data->queryCount, 1 * 4); // u32 queryCount
    stream->read(&data->pipelineStatistics, 1 * 4); // VkQueryPipelineStatisticFlags pipelineStatistics
    return data;
}

uint32_t vkUtilsEncodingSize_VkFramebufferCreateInfo(const VkFramebufferCreateInfo* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkFramebufferCreateFlags flags
    res += 1 * 8; // VkRenderPass renderPass
    res += 1 * 4; // u32 attachmentCount
    res += data->attachmentCount * 8; // VkImageView pAttachments
    res += 1 * 4; // u32 width
    res += 1 * 4; // u32 height
    res += 1 * 4; // u32 layers
    return res;
}

void vkUtilsPack_VkFramebufferCreateInfo(android::base::InplaceStream* stream, const VkFramebufferCreateInfo* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkFramebufferCreateFlags flags
    stream->write(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->write(&data->attachmentCount, 1 * 4); // u32 attachmentCount
    stream->write(data->pAttachments, data->attachmentCount * 8); // VkImageView pAttachments
    stream->write(&data->width, 1 * 4); // u32 width
    stream->write(&data->height, 1 * 4); // u32 height
    stream->write(&data->layers, 1 * 4); // u32 layers
}

VkFramebufferCreateInfo* vkUtilsUnpack_VkFramebufferCreateInfo(android::base::InplaceStream* stream) {
    VkFramebufferCreateInfo* data = new VkFramebufferCreateInfo; memset(data, 0, sizeof(VkFramebufferCreateInfo)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkFramebufferCreateFlags flags
    stream->read(&data->renderPass, 1 * 8); // VkRenderPass renderPass
    stream->read(&data->attachmentCount, 1 * 4); // u32 attachmentCount
    data->pAttachments = data->attachmentCount ? new VkImageView[data->attachmentCount] : nullptr; // VkImageView pAttachments
    stream->read((char*)data->pAttachments, data->attachmentCount * 8); // VkImageView pAttachments
    stream->read(&data->width, 1 * 4); // u32 width
    stream->read(&data->height, 1 * 4); // u32 height
    stream->read(&data->layers, 1 * 4); // u32 layers
    return data;
}

uint32_t vkUtilsEncodingSize_VkDrawIndirectCommand(const VkDrawIndirectCommand* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 vertexCount
    res += 1 * 4; // u32 instanceCount
    res += 1 * 4; // u32 firstVertex
    res += 1 * 4; // u32 firstInstance
    return res;
}

void vkUtilsPack_VkDrawIndirectCommand(android::base::InplaceStream* stream, const VkDrawIndirectCommand* data) {
    if (!data) return;
    stream->write(&data->vertexCount, 1 * 4); // u32 vertexCount
    stream->write(&data->instanceCount, 1 * 4); // u32 instanceCount
    stream->write(&data->firstVertex, 1 * 4); // u32 firstVertex
    stream->write(&data->firstInstance, 1 * 4); // u32 firstInstance
}

VkDrawIndirectCommand* vkUtilsUnpack_VkDrawIndirectCommand(android::base::InplaceStream* stream) {
    VkDrawIndirectCommand* data = new VkDrawIndirectCommand; memset(data, 0, sizeof(VkDrawIndirectCommand)); 
    stream->read(&data->vertexCount, 1 * 4); // u32 vertexCount
    stream->read(&data->instanceCount, 1 * 4); // u32 instanceCount
    stream->read(&data->firstVertex, 1 * 4); // u32 firstVertex
    stream->read(&data->firstInstance, 1 * 4); // u32 firstInstance
    return data;
}

uint32_t vkUtilsEncodingSize_VkDrawIndexedIndirectCommand(const VkDrawIndexedIndirectCommand* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 indexCount
    res += 1 * 4; // u32 instanceCount
    res += 1 * 4; // u32 firstIndex
    res += 1 * 4; // s32 vertexOffset
    res += 1 * 4; // u32 firstInstance
    return res;
}

void vkUtilsPack_VkDrawIndexedIndirectCommand(android::base::InplaceStream* stream, const VkDrawIndexedIndirectCommand* data) {
    if (!data) return;
    stream->write(&data->indexCount, 1 * 4); // u32 indexCount
    stream->write(&data->instanceCount, 1 * 4); // u32 instanceCount
    stream->write(&data->firstIndex, 1 * 4); // u32 firstIndex
    stream->write(&data->vertexOffset, 1 * 4); // s32 vertexOffset
    stream->write(&data->firstInstance, 1 * 4); // u32 firstInstance
}

VkDrawIndexedIndirectCommand* vkUtilsUnpack_VkDrawIndexedIndirectCommand(android::base::InplaceStream* stream) {
    VkDrawIndexedIndirectCommand* data = new VkDrawIndexedIndirectCommand; memset(data, 0, sizeof(VkDrawIndexedIndirectCommand)); 
    stream->read(&data->indexCount, 1 * 4); // u32 indexCount
    stream->read(&data->instanceCount, 1 * 4); // u32 instanceCount
    stream->read(&data->firstIndex, 1 * 4); // u32 firstIndex
    stream->read(&data->vertexOffset, 1 * 4); // s32 vertexOffset
    stream->read(&data->firstInstance, 1 * 4); // u32 firstInstance
    return data;
}

uint32_t vkUtilsEncodingSize_VkDispatchIndirectCommand(const VkDispatchIndirectCommand* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 x
    res += 1 * 4; // u32 y
    res += 1 * 4; // u32 z
    return res;
}

void vkUtilsPack_VkDispatchIndirectCommand(android::base::InplaceStream* stream, const VkDispatchIndirectCommand* data) {
    if (!data) return;
    stream->write(&data->x, 1 * 4); // u32 x
    stream->write(&data->y, 1 * 4); // u32 y
    stream->write(&data->z, 1 * 4); // u32 z
}

VkDispatchIndirectCommand* vkUtilsUnpack_VkDispatchIndirectCommand(android::base::InplaceStream* stream) {
    VkDispatchIndirectCommand* data = new VkDispatchIndirectCommand; memset(data, 0, sizeof(VkDispatchIndirectCommand)); 
    stream->read(&data->x, 1 * 4); // u32 x
    stream->read(&data->y, 1 * 4); // u32 y
    stream->read(&data->z, 1 * 4); // u32 z
    return data;
}

uint32_t vkUtilsEncodingSize_VkSurfaceCapabilitiesKHR(const VkSurfaceCapabilitiesKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 minImageCount
    res += 1 * 4; // u32 maxImageCount
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->currentExtent); // VkExtent2D currentExtent
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->minImageExtent); // VkExtent2D minImageExtent
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->maxImageExtent); // VkExtent2D maxImageExtent
    res += 1 * 4; // u32 maxImageArrayLayers
    res += 1 * 4; // VkSurfaceTransformFlagsKHR supportedTransforms
    res += 1 * 4; // VkSurfaceTransformFlagBitsKHR currentTransform
    res += 1 * 4; // VkCompositeAlphaFlagsKHR supportedCompositeAlpha
    res += 1 * 4; // VkImageUsageFlags supportedUsageFlags
    return res;
}

void vkUtilsPack_VkSurfaceCapabilitiesKHR(android::base::InplaceStream* stream, const VkSurfaceCapabilitiesKHR* data) {
    if (!data) return;
    stream->write(&data->minImageCount, 1 * 4); // u32 minImageCount
    stream->write(&data->maxImageCount, 1 * 4); // u32 maxImageCount
    vkUtilsPack_VkExtent2D(stream, &data->currentExtent); // VkExtent2D currentExtent
    vkUtilsPack_VkExtent2D(stream, &data->minImageExtent); // VkExtent2D minImageExtent
    vkUtilsPack_VkExtent2D(stream, &data->maxImageExtent); // VkExtent2D maxImageExtent
    stream->write(&data->maxImageArrayLayers, 1 * 4); // u32 maxImageArrayLayers
    stream->write(&data->supportedTransforms, 1 * 4); // VkSurfaceTransformFlagsKHR supportedTransforms
    stream->write(&data->currentTransform, 1 * 4); // VkSurfaceTransformFlagBitsKHR currentTransform
    stream->write(&data->supportedCompositeAlpha, 1 * 4); // VkCompositeAlphaFlagsKHR supportedCompositeAlpha
    stream->write(&data->supportedUsageFlags, 1 * 4); // VkImageUsageFlags supportedUsageFlags
}

VkSurfaceCapabilitiesKHR* vkUtilsUnpack_VkSurfaceCapabilitiesKHR(android::base::InplaceStream* stream) {
    VkSurfaceCapabilitiesKHR* data = new VkSurfaceCapabilitiesKHR; memset(data, 0, sizeof(VkSurfaceCapabilitiesKHR)); 
    stream->read(&data->minImageCount, 1 * 4); // u32 minImageCount
    stream->read(&data->maxImageCount, 1 * 4); // u32 maxImageCount
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->currentExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D currentExtent
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->minImageExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D minImageExtent
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->maxImageExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D maxImageExtent
    stream->read(&data->maxImageArrayLayers, 1 * 4); // u32 maxImageArrayLayers
    stream->read(&data->supportedTransforms, 1 * 4); // VkSurfaceTransformFlagsKHR supportedTransforms
    stream->read(&data->currentTransform, 1 * 4); // VkSurfaceTransformFlagBitsKHR currentTransform
    stream->read(&data->supportedCompositeAlpha, 1 * 4); // VkCompositeAlphaFlagsKHR supportedCompositeAlpha
    stream->read(&data->supportedUsageFlags, 1 * 4); // VkImageUsageFlags supportedUsageFlags
    return data;
}

uint32_t vkUtilsEncodingSize_VkSurfaceFormatKHR(const VkSurfaceFormatKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkFormat format
    res += 1 * 4; // VkColorSpaceKHR colorSpace
    return res;
}

void vkUtilsPack_VkSurfaceFormatKHR(android::base::InplaceStream* stream, const VkSurfaceFormatKHR* data) {
    if (!data) return;
    stream->write(&data->format, 1 * 4); // VkFormat format
    stream->write(&data->colorSpace, 1 * 4); // VkColorSpaceKHR colorSpace
}

VkSurfaceFormatKHR* vkUtilsUnpack_VkSurfaceFormatKHR(android::base::InplaceStream* stream) {
    VkSurfaceFormatKHR* data = new VkSurfaceFormatKHR; memset(data, 0, sizeof(VkSurfaceFormatKHR)); 
    stream->read(&data->format, 1 * 4); // VkFormat format
    stream->read(&data->colorSpace, 1 * 4); // VkColorSpaceKHR colorSpace
    return data;
}

uint32_t vkUtilsEncodingSize_VkSwapchainCreateInfoKHR(const VkSwapchainCreateInfoKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkSwapchainCreateFlagsKHR flags
    res += 1 * 8; // VkSurfaceKHR surface
    res += 1 * 4; // u32 minImageCount
    res += 1 * 4; // VkFormat imageFormat
    res += 1 * 4; // VkColorSpaceKHR imageColorSpace
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->imageExtent); // VkExtent2D imageExtent
    res += 1 * 4; // u32 imageArrayLayers
    res += 1 * 4; // VkImageUsageFlags imageUsage
    res += 1 * 4; // VkSharingMode imageSharingMode
    res += 1 * 4; // u32 queueFamilyIndexCount
    res += data->queueFamilyIndexCount * 4; // u32 pQueueFamilyIndices
    res += 1 * 4; // VkSurfaceTransformFlagBitsKHR preTransform
    res += 1 * 4; // VkCompositeAlphaFlagBitsKHR compositeAlpha
    res += 1 * 4; // VkPresentModeKHR presentMode
    res += 1 * 4; // VkBool32 clipped
    res += 1 * 8; // VkSwapchainKHR oldSwapchain
    return res;
}

void vkUtilsPack_VkSwapchainCreateInfoKHR(android::base::InplaceStream* stream, const VkSwapchainCreateInfoKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkSwapchainCreateFlagsKHR flags
    stream->write(&data->surface, 1 * 8); // VkSurfaceKHR surface
    stream->write(&data->minImageCount, 1 * 4); // u32 minImageCount
    stream->write(&data->imageFormat, 1 * 4); // VkFormat imageFormat
    stream->write(&data->imageColorSpace, 1 * 4); // VkColorSpaceKHR imageColorSpace
    vkUtilsPack_VkExtent2D(stream, &data->imageExtent); // VkExtent2D imageExtent
    stream->write(&data->imageArrayLayers, 1 * 4); // u32 imageArrayLayers
    stream->write(&data->imageUsage, 1 * 4); // VkImageUsageFlags imageUsage
    stream->write(&data->imageSharingMode, 1 * 4); // VkSharingMode imageSharingMode
    stream->write(&data->queueFamilyIndexCount, 1 * 4); // u32 queueFamilyIndexCount
    stream->write(data->pQueueFamilyIndices, data->queueFamilyIndexCount * 4); // u32 pQueueFamilyIndices
    stream->write(&data->preTransform, 1 * 4); // VkSurfaceTransformFlagBitsKHR preTransform
    stream->write(&data->compositeAlpha, 1 * 4); // VkCompositeAlphaFlagBitsKHR compositeAlpha
    stream->write(&data->presentMode, 1 * 4); // VkPresentModeKHR presentMode
    stream->write(&data->clipped, 1 * 4); // VkBool32 clipped
    stream->write(&data->oldSwapchain, 1 * 8); // VkSwapchainKHR oldSwapchain
}

VkSwapchainCreateInfoKHR* vkUtilsUnpack_VkSwapchainCreateInfoKHR(android::base::InplaceStream* stream) {
    VkSwapchainCreateInfoKHR* data = new VkSwapchainCreateInfoKHR; memset(data, 0, sizeof(VkSwapchainCreateInfoKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkSwapchainCreateFlagsKHR flags
    stream->read(&data->surface, 1 * 8); // VkSurfaceKHR surface
    stream->read(&data->minImageCount, 1 * 4); // u32 minImageCount
    stream->read(&data->imageFormat, 1 * 4); // VkFormat imageFormat
    stream->read(&data->imageColorSpace, 1 * 4); // VkColorSpaceKHR imageColorSpace
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->imageExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D imageExtent
    stream->read(&data->imageArrayLayers, 1 * 4); // u32 imageArrayLayers
    stream->read(&data->imageUsage, 1 * 4); // VkImageUsageFlags imageUsage
    stream->read(&data->imageSharingMode, 1 * 4); // VkSharingMode imageSharingMode
    stream->read(&data->queueFamilyIndexCount, 1 * 4); // u32 queueFamilyIndexCount
    data->pQueueFamilyIndices = data->queueFamilyIndexCount ? new u32[data->queueFamilyIndexCount] : nullptr; // u32 pQueueFamilyIndices
    stream->read((char*)data->pQueueFamilyIndices, data->queueFamilyIndexCount * 4); // u32 pQueueFamilyIndices
    stream->read(&data->preTransform, 1 * 4); // VkSurfaceTransformFlagBitsKHR preTransform
    stream->read(&data->compositeAlpha, 1 * 4); // VkCompositeAlphaFlagBitsKHR compositeAlpha
    stream->read(&data->presentMode, 1 * 4); // VkPresentModeKHR presentMode
    stream->read(&data->clipped, 1 * 4); // VkBool32 clipped
    stream->read(&data->oldSwapchain, 1 * 8); // VkSwapchainKHR oldSwapchain
    return data;
}

uint32_t vkUtilsEncodingSize_VkPresentInfoKHR(const VkPresentInfoKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 waitSemaphoreCount
    res += data->waitSemaphoreCount * 8; // VkSemaphore pWaitSemaphores
    res += 1 * 4; // u32 swapchainCount
    res += data->swapchainCount * 8; // VkSwapchainKHR pSwapchains
    res += 1 * 4; // u32 pImageIndices
    res += 1 * 4; // VkResult pResults
    return res;
}

void vkUtilsPack_VkPresentInfoKHR(android::base::InplaceStream* stream, const VkPresentInfoKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    stream->write(data->pWaitSemaphores, data->waitSemaphoreCount * 8); // VkSemaphore pWaitSemaphores
    stream->write(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    stream->write(data->pSwapchains, data->swapchainCount * 8); // VkSwapchainKHR pSwapchains
    stream->write(data->pImageIndices, 1 * 4); // u32 pImageIndices
    stream->write(data->pResults, 1 * 4); // VkResult pResults
}

VkPresentInfoKHR* vkUtilsUnpack_VkPresentInfoKHR(android::base::InplaceStream* stream) {
    VkPresentInfoKHR* data = new VkPresentInfoKHR; memset(data, 0, sizeof(VkPresentInfoKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    data->pWaitSemaphores = data->waitSemaphoreCount ? new VkSemaphore[data->waitSemaphoreCount] : nullptr; // VkSemaphore pWaitSemaphores
    stream->read((char*)data->pWaitSemaphores, data->waitSemaphoreCount * 8); // VkSemaphore pWaitSemaphores
    stream->read(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    data->pSwapchains = data->swapchainCount ? new VkSwapchainKHR[data->swapchainCount] : nullptr; // VkSwapchainKHR pSwapchains
    stream->read((char*)data->pSwapchains, data->swapchainCount * 8); // VkSwapchainKHR pSwapchains
    data->pImageIndices = 1 ? new u32[1] : nullptr; // u32 pImageIndices
    stream->read((char*)data->pImageIndices, 1 * 4); // u32 pImageIndices
    data->pResults = 1 ? new VkResult[1] : nullptr; // VkResult pResults
    stream->read((char*)data->pResults, 1 * 4); // VkResult pResults
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayPropertiesKHR(const VkDisplayPropertiesKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDisplayKHR display
    res += 1 * 1; // char displayName
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->physicalDimensions); // VkExtent2D physicalDimensions
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->physicalResolution); // VkExtent2D physicalResolution
    res += 1 * 4; // VkSurfaceTransformFlagsKHR supportedTransforms
    res += 1 * 4; // VkBool32 planeReorderPossible
    res += 1 * 4; // VkBool32 persistentContent
    return res;
}

void vkUtilsPack_VkDisplayPropertiesKHR(android::base::InplaceStream* stream, const VkDisplayPropertiesKHR* data) {
    if (!data) return;
    stream->write(&data->display, 1 * 8); // VkDisplayKHR display
    stream->write(data->displayName, 1 * 1); // char displayName
    vkUtilsPack_VkExtent2D(stream, &data->physicalDimensions); // VkExtent2D physicalDimensions
    vkUtilsPack_VkExtent2D(stream, &data->physicalResolution); // VkExtent2D physicalResolution
    stream->write(&data->supportedTransforms, 1 * 4); // VkSurfaceTransformFlagsKHR supportedTransforms
    stream->write(&data->planeReorderPossible, 1 * 4); // VkBool32 planeReorderPossible
    stream->write(&data->persistentContent, 1 * 4); // VkBool32 persistentContent
}

VkDisplayPropertiesKHR* vkUtilsUnpack_VkDisplayPropertiesKHR(android::base::InplaceStream* stream) {
    VkDisplayPropertiesKHR* data = new VkDisplayPropertiesKHR; memset(data, 0, sizeof(VkDisplayPropertiesKHR)); 
    stream->read(&data->display, 1 * 8); // VkDisplayKHR display
    data->displayName = 1 ? new char[1] : nullptr; // char displayName
    stream->read((char*)data->displayName, 1 * 1); // char displayName
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->physicalDimensions, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D physicalDimensions
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->physicalResolution, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D physicalResolution
    stream->read(&data->supportedTransforms, 1 * 4); // VkSurfaceTransformFlagsKHR supportedTransforms
    stream->read(&data->planeReorderPossible, 1 * 4); // VkBool32 planeReorderPossible
    stream->read(&data->persistentContent, 1 * 4); // VkBool32 persistentContent
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayModeParametersKHR(const VkDisplayModeParametersKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->visibleRegion); // VkExtent2D visibleRegion
    res += 1 * 4; // u32 refreshRate
    return res;
}

void vkUtilsPack_VkDisplayModeParametersKHR(android::base::InplaceStream* stream, const VkDisplayModeParametersKHR* data) {
    if (!data) return;
    vkUtilsPack_VkExtent2D(stream, &data->visibleRegion); // VkExtent2D visibleRegion
    stream->write(&data->refreshRate, 1 * 4); // u32 refreshRate
}

VkDisplayModeParametersKHR* vkUtilsUnpack_VkDisplayModeParametersKHR(android::base::InplaceStream* stream) {
    VkDisplayModeParametersKHR* data = new VkDisplayModeParametersKHR; memset(data, 0, sizeof(VkDisplayModeParametersKHR)); 
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->visibleRegion, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D visibleRegion
    stream->read(&data->refreshRate, 1 * 4); // u32 refreshRate
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayModePropertiesKHR(const VkDisplayModePropertiesKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDisplayModeKHR displayMode
    res += 1 * vkUtilsEncodingSize_VkDisplayModeParametersKHR(&data->parameters); // VkDisplayModeParametersKHR parameters
    return res;
}

void vkUtilsPack_VkDisplayModePropertiesKHR(android::base::InplaceStream* stream, const VkDisplayModePropertiesKHR* data) {
    if (!data) return;
    stream->write(&data->displayMode, 1 * 8); // VkDisplayModeKHR displayMode
    vkUtilsPack_VkDisplayModeParametersKHR(stream, &data->parameters); // VkDisplayModeParametersKHR parameters
}

VkDisplayModePropertiesKHR* vkUtilsUnpack_VkDisplayModePropertiesKHR(android::base::InplaceStream* stream) {
    VkDisplayModePropertiesKHR* data = new VkDisplayModePropertiesKHR; memset(data, 0, sizeof(VkDisplayModePropertiesKHR)); 
    stream->read(&data->displayMode, 1 * 8); // VkDisplayModeKHR displayMode
    { VkDisplayModeParametersKHR* tmpUnpacked = vkUtilsUnpack_VkDisplayModeParametersKHR(stream); memcpy(&data->parameters, tmpUnpacked, sizeof(VkDisplayModeParametersKHR)); delete tmpUnpacked; } // VkDisplayModeParametersKHR parameters
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayModeCreateInfoKHR(const VkDisplayModeCreateInfoKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDisplayModeCreateFlagsKHR flags
    res += 1 * vkUtilsEncodingSize_VkDisplayModeParametersKHR(&data->parameters); // VkDisplayModeParametersKHR parameters
    return res;
}

void vkUtilsPack_VkDisplayModeCreateInfoKHR(android::base::InplaceStream* stream, const VkDisplayModeCreateInfoKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDisplayModeCreateFlagsKHR flags
    vkUtilsPack_VkDisplayModeParametersKHR(stream, &data->parameters); // VkDisplayModeParametersKHR parameters
}

VkDisplayModeCreateInfoKHR* vkUtilsUnpack_VkDisplayModeCreateInfoKHR(android::base::InplaceStream* stream) {
    VkDisplayModeCreateInfoKHR* data = new VkDisplayModeCreateInfoKHR; memset(data, 0, sizeof(VkDisplayModeCreateInfoKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDisplayModeCreateFlagsKHR flags
    { VkDisplayModeParametersKHR* tmpUnpacked = vkUtilsUnpack_VkDisplayModeParametersKHR(stream); memcpy(&data->parameters, tmpUnpacked, sizeof(VkDisplayModeParametersKHR)); delete tmpUnpacked; } // VkDisplayModeParametersKHR parameters
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayPlanePropertiesKHR(const VkDisplayPlanePropertiesKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // VkDisplayKHR currentDisplay
    res += 1 * 4; // u32 currentStackIndex
    return res;
}

void vkUtilsPack_VkDisplayPlanePropertiesKHR(android::base::InplaceStream* stream, const VkDisplayPlanePropertiesKHR* data) {
    if (!data) return;
    stream->write(&data->currentDisplay, 1 * 8); // VkDisplayKHR currentDisplay
    stream->write(&data->currentStackIndex, 1 * 4); // u32 currentStackIndex
}

VkDisplayPlanePropertiesKHR* vkUtilsUnpack_VkDisplayPlanePropertiesKHR(android::base::InplaceStream* stream) {
    VkDisplayPlanePropertiesKHR* data = new VkDisplayPlanePropertiesKHR; memset(data, 0, sizeof(VkDisplayPlanePropertiesKHR)); 
    stream->read(&data->currentDisplay, 1 * 8); // VkDisplayKHR currentDisplay
    stream->read(&data->currentStackIndex, 1 * 4); // u32 currentStackIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayPlaneCapabilitiesKHR(const VkDisplayPlaneCapabilitiesKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkDisplayPlaneAlphaFlagsKHR supportedAlpha
    res += 1 * vkUtilsEncodingSize_VkOffset2D(&data->minSrcPosition); // VkOffset2D minSrcPosition
    res += 1 * vkUtilsEncodingSize_VkOffset2D(&data->maxSrcPosition); // VkOffset2D maxSrcPosition
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->minSrcExtent); // VkExtent2D minSrcExtent
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->maxSrcExtent); // VkExtent2D maxSrcExtent
    res += 1 * vkUtilsEncodingSize_VkOffset2D(&data->minDstPosition); // VkOffset2D minDstPosition
    res += 1 * vkUtilsEncodingSize_VkOffset2D(&data->maxDstPosition); // VkOffset2D maxDstPosition
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->minDstExtent); // VkExtent2D minDstExtent
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->maxDstExtent); // VkExtent2D maxDstExtent
    return res;
}

void vkUtilsPack_VkDisplayPlaneCapabilitiesKHR(android::base::InplaceStream* stream, const VkDisplayPlaneCapabilitiesKHR* data) {
    if (!data) return;
    stream->write(&data->supportedAlpha, 1 * 4); // VkDisplayPlaneAlphaFlagsKHR supportedAlpha
    vkUtilsPack_VkOffset2D(stream, &data->minSrcPosition); // VkOffset2D minSrcPosition
    vkUtilsPack_VkOffset2D(stream, &data->maxSrcPosition); // VkOffset2D maxSrcPosition
    vkUtilsPack_VkExtent2D(stream, &data->minSrcExtent); // VkExtent2D minSrcExtent
    vkUtilsPack_VkExtent2D(stream, &data->maxSrcExtent); // VkExtent2D maxSrcExtent
    vkUtilsPack_VkOffset2D(stream, &data->minDstPosition); // VkOffset2D minDstPosition
    vkUtilsPack_VkOffset2D(stream, &data->maxDstPosition); // VkOffset2D maxDstPosition
    vkUtilsPack_VkExtent2D(stream, &data->minDstExtent); // VkExtent2D minDstExtent
    vkUtilsPack_VkExtent2D(stream, &data->maxDstExtent); // VkExtent2D maxDstExtent
}

VkDisplayPlaneCapabilitiesKHR* vkUtilsUnpack_VkDisplayPlaneCapabilitiesKHR(android::base::InplaceStream* stream) {
    VkDisplayPlaneCapabilitiesKHR* data = new VkDisplayPlaneCapabilitiesKHR; memset(data, 0, sizeof(VkDisplayPlaneCapabilitiesKHR)); 
    stream->read(&data->supportedAlpha, 1 * 4); // VkDisplayPlaneAlphaFlagsKHR supportedAlpha
    { VkOffset2D* tmpUnpacked = vkUtilsUnpack_VkOffset2D(stream); memcpy(&data->minSrcPosition, tmpUnpacked, sizeof(VkOffset2D)); delete tmpUnpacked; } // VkOffset2D minSrcPosition
    { VkOffset2D* tmpUnpacked = vkUtilsUnpack_VkOffset2D(stream); memcpy(&data->maxSrcPosition, tmpUnpacked, sizeof(VkOffset2D)); delete tmpUnpacked; } // VkOffset2D maxSrcPosition
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->minSrcExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D minSrcExtent
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->maxSrcExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D maxSrcExtent
    { VkOffset2D* tmpUnpacked = vkUtilsUnpack_VkOffset2D(stream); memcpy(&data->minDstPosition, tmpUnpacked, sizeof(VkOffset2D)); delete tmpUnpacked; } // VkOffset2D minDstPosition
    { VkOffset2D* tmpUnpacked = vkUtilsUnpack_VkOffset2D(stream); memcpy(&data->maxDstPosition, tmpUnpacked, sizeof(VkOffset2D)); delete tmpUnpacked; } // VkOffset2D maxDstPosition
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->minDstExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D minDstExtent
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->maxDstExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D maxDstExtent
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplaySurfaceCreateInfoKHR(const VkDisplaySurfaceCreateInfoKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDisplaySurfaceCreateFlagsKHR flags
    res += 1 * 8; // VkDisplayModeKHR displayMode
    res += 1 * 4; // u32 planeIndex
    res += 1 * 4; // u32 planeStackIndex
    res += 1 * 4; // VkSurfaceTransformFlagBitsKHR transform
    res += 1 * 4; // f32 globalAlpha
    res += 1 * 4; // VkDisplayPlaneAlphaFlagBitsKHR alphaMode
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->imageExtent); // VkExtent2D imageExtent
    return res;
}

void vkUtilsPack_VkDisplaySurfaceCreateInfoKHR(android::base::InplaceStream* stream, const VkDisplaySurfaceCreateInfoKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDisplaySurfaceCreateFlagsKHR flags
    stream->write(&data->displayMode, 1 * 8); // VkDisplayModeKHR displayMode
    stream->write(&data->planeIndex, 1 * 4); // u32 planeIndex
    stream->write(&data->planeStackIndex, 1 * 4); // u32 planeStackIndex
    stream->write(&data->transform, 1 * 4); // VkSurfaceTransformFlagBitsKHR transform
    stream->write(&data->globalAlpha, 1 * 4); // f32 globalAlpha
    stream->write(&data->alphaMode, 1 * 4); // VkDisplayPlaneAlphaFlagBitsKHR alphaMode
    vkUtilsPack_VkExtent2D(stream, &data->imageExtent); // VkExtent2D imageExtent
}

VkDisplaySurfaceCreateInfoKHR* vkUtilsUnpack_VkDisplaySurfaceCreateInfoKHR(android::base::InplaceStream* stream) {
    VkDisplaySurfaceCreateInfoKHR* data = new VkDisplaySurfaceCreateInfoKHR; memset(data, 0, sizeof(VkDisplaySurfaceCreateInfoKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDisplaySurfaceCreateFlagsKHR flags
    stream->read(&data->displayMode, 1 * 8); // VkDisplayModeKHR displayMode
    stream->read(&data->planeIndex, 1 * 4); // u32 planeIndex
    stream->read(&data->planeStackIndex, 1 * 4); // u32 planeStackIndex
    stream->read(&data->transform, 1 * 4); // VkSurfaceTransformFlagBitsKHR transform
    stream->read(&data->globalAlpha, 1 * 4); // f32 globalAlpha
    stream->read(&data->alphaMode, 1 * 4); // VkDisplayPlaneAlphaFlagBitsKHR alphaMode
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->imageExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D imageExtent
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayPresentInfoKHR(const VkDisplayPresentInfoKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkRect2D(&data->srcRect); // VkRect2D srcRect
    res += 1 * vkUtilsEncodingSize_VkRect2D(&data->dstRect); // VkRect2D dstRect
    res += 1 * 4; // VkBool32 persistent
    return res;
}

void vkUtilsPack_VkDisplayPresentInfoKHR(android::base::InplaceStream* stream, const VkDisplayPresentInfoKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkRect2D(stream, &data->srcRect); // VkRect2D srcRect
    vkUtilsPack_VkRect2D(stream, &data->dstRect); // VkRect2D dstRect
    stream->write(&data->persistent, 1 * 4); // VkBool32 persistent
}

VkDisplayPresentInfoKHR* vkUtilsUnpack_VkDisplayPresentInfoKHR(android::base::InplaceStream* stream) {
    VkDisplayPresentInfoKHR* data = new VkDisplayPresentInfoKHR; memset(data, 0, sizeof(VkDisplayPresentInfoKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&data->srcRect, tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } // VkRect2D srcRect
    { VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&data->dstRect, tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } // VkRect2D dstRect
    stream->read(&data->persistent, 1 * 4); // VkBool32 persistent
    return data;
}

uint32_t vkUtilsEncodingSize_VkDebugReportCallbackCreateInfoEXT(const VkDebugReportCallbackCreateInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDebugReportFlagsEXT flags
    res += 1 * 8; // PFN_vkDebugReportCallbackEXT pfnCallback
    res += 1 * 8; // void pUserData
    return res;
}

void vkUtilsPack_VkDebugReportCallbackCreateInfoEXT(android::base::InplaceStream* stream, const VkDebugReportCallbackCreateInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDebugReportFlagsEXT flags
    stream->write(&data->pfnCallback, 1 * 8); // PFN_vkDebugReportCallbackEXT pfnCallback
    stream->write(&data->pUserData, 8); // void pUserData
}

VkDebugReportCallbackCreateInfoEXT* vkUtilsUnpack_VkDebugReportCallbackCreateInfoEXT(android::base::InplaceStream* stream) {
    VkDebugReportCallbackCreateInfoEXT* data = new VkDebugReportCallbackCreateInfoEXT; memset(data, 0, sizeof(VkDebugReportCallbackCreateInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDebugReportFlagsEXT flags
    stream->read(&data->pfnCallback, 1 * 8); // PFN_vkDebugReportCallbackEXT pfnCallback
    stream->read(&data->pUserData, 8); // void pUserData
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineRasterizationStateRasterizationOrderAMD(const VkPipelineRasterizationStateRasterizationOrderAMD* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkRasterizationOrderAMD rasterizationOrder
    return res;
}

void vkUtilsPack_VkPipelineRasterizationStateRasterizationOrderAMD(android::base::InplaceStream* stream, const VkPipelineRasterizationStateRasterizationOrderAMD* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->rasterizationOrder, 1 * 4); // VkRasterizationOrderAMD rasterizationOrder
}

VkPipelineRasterizationStateRasterizationOrderAMD* vkUtilsUnpack_VkPipelineRasterizationStateRasterizationOrderAMD(android::base::InplaceStream* stream) {
    VkPipelineRasterizationStateRasterizationOrderAMD* data = new VkPipelineRasterizationStateRasterizationOrderAMD; memset(data, 0, sizeof(VkPipelineRasterizationStateRasterizationOrderAMD)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->rasterizationOrder, 1 * 4); // VkRasterizationOrderAMD rasterizationOrder
    return data;
}

uint32_t vkUtilsEncodingSize_VkDebugMarkerObjectNameInfoEXT(const VkDebugMarkerObjectNameInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDebugReportObjectTypeEXT objectType
    res += 1 * 8; // u64 object
    res += 1 * 1; // char pObjectName
    return res;
}

void vkUtilsPack_VkDebugMarkerObjectNameInfoEXT(android::base::InplaceStream* stream, const VkDebugMarkerObjectNameInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->objectType, 1 * 4); // VkDebugReportObjectTypeEXT objectType
    stream->write(&data->object, 1 * 8); // u64 object
    stream->write(data->pObjectName, 1 * 1); // char pObjectName
}

VkDebugMarkerObjectNameInfoEXT* vkUtilsUnpack_VkDebugMarkerObjectNameInfoEXT(android::base::InplaceStream* stream) {
    VkDebugMarkerObjectNameInfoEXT* data = new VkDebugMarkerObjectNameInfoEXT; memset(data, 0, sizeof(VkDebugMarkerObjectNameInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->objectType, 1 * 4); // VkDebugReportObjectTypeEXT objectType
    stream->read(&data->object, 1 * 8); // u64 object
    data->pObjectName = 1 ? new char[1] : nullptr; // char pObjectName
    stream->read((char*)data->pObjectName, 1 * 1); // char pObjectName
    return data;
}

uint32_t vkUtilsEncodingSize_VkDebugMarkerObjectTagInfoEXT(const VkDebugMarkerObjectTagInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDebugReportObjectTypeEXT objectType
    res += 1 * 8; // u64 object
    res += 1 * 8; // u64 tagName
    res += 1 * 4; // size_t tagSize
    res += 1 * 8; // void pTag
    return res;
}

void vkUtilsPack_VkDebugMarkerObjectTagInfoEXT(android::base::InplaceStream* stream, const VkDebugMarkerObjectTagInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->objectType, 1 * 4); // VkDebugReportObjectTypeEXT objectType
    stream->write(&data->object, 1 * 8); // u64 object
    stream->write(&data->tagName, 1 * 8); // u64 tagName
    stream->write(&data->tagSize, 1 * 4); // size_t tagSize
    stream->write(&data->pTag, 8); // void pTag
}

VkDebugMarkerObjectTagInfoEXT* vkUtilsUnpack_VkDebugMarkerObjectTagInfoEXT(android::base::InplaceStream* stream) {
    VkDebugMarkerObjectTagInfoEXT* data = new VkDebugMarkerObjectTagInfoEXT; memset(data, 0, sizeof(VkDebugMarkerObjectTagInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->objectType, 1 * 4); // VkDebugReportObjectTypeEXT objectType
    stream->read(&data->object, 1 * 8); // u64 object
    stream->read(&data->tagName, 1 * 8); // u64 tagName
    stream->read(&data->tagSize, 1 * 4); // size_t tagSize
    stream->read(&data->pTag, 8); // void pTag
    return data;
}

uint32_t vkUtilsEncodingSize_VkDebugMarkerMarkerInfoEXT(const VkDebugMarkerMarkerInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 1; // char pMarkerName
    res += 4 * 4; // f32 color
    return res;
}

void vkUtilsPack_VkDebugMarkerMarkerInfoEXT(android::base::InplaceStream* stream, const VkDebugMarkerMarkerInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(data->pMarkerName, 1 * 1); // char pMarkerName
    stream->write(&data->color, 4 * 4); // f32 color
}

VkDebugMarkerMarkerInfoEXT* vkUtilsUnpack_VkDebugMarkerMarkerInfoEXT(android::base::InplaceStream* stream) {
    VkDebugMarkerMarkerInfoEXT* data = new VkDebugMarkerMarkerInfoEXT; memset(data, 0, sizeof(VkDebugMarkerMarkerInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    data->pMarkerName = 1 ? new char[1] : nullptr; // char pMarkerName
    stream->read((char*)data->pMarkerName, 1 * 1); // char pMarkerName
    stream->read(&data->color, 4 * 4); // f32 color
    return data;
}

uint32_t vkUtilsEncodingSize_VkDedicatedAllocationImageCreateInfoNV(const VkDedicatedAllocationImageCreateInfoNV* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBool32 dedicatedAllocation
    return res;
}

void vkUtilsPack_VkDedicatedAllocationImageCreateInfoNV(android::base::InplaceStream* stream, const VkDedicatedAllocationImageCreateInfoNV* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->dedicatedAllocation, 1 * 4); // VkBool32 dedicatedAllocation
}

VkDedicatedAllocationImageCreateInfoNV* vkUtilsUnpack_VkDedicatedAllocationImageCreateInfoNV(android::base::InplaceStream* stream) {
    VkDedicatedAllocationImageCreateInfoNV* data = new VkDedicatedAllocationImageCreateInfoNV; memset(data, 0, sizeof(VkDedicatedAllocationImageCreateInfoNV)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->dedicatedAllocation, 1 * 4); // VkBool32 dedicatedAllocation
    return data;
}

uint32_t vkUtilsEncodingSize_VkDedicatedAllocationBufferCreateInfoNV(const VkDedicatedAllocationBufferCreateInfoNV* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBool32 dedicatedAllocation
    return res;
}

void vkUtilsPack_VkDedicatedAllocationBufferCreateInfoNV(android::base::InplaceStream* stream, const VkDedicatedAllocationBufferCreateInfoNV* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->dedicatedAllocation, 1 * 4); // VkBool32 dedicatedAllocation
}

VkDedicatedAllocationBufferCreateInfoNV* vkUtilsUnpack_VkDedicatedAllocationBufferCreateInfoNV(android::base::InplaceStream* stream) {
    VkDedicatedAllocationBufferCreateInfoNV* data = new VkDedicatedAllocationBufferCreateInfoNV; memset(data, 0, sizeof(VkDedicatedAllocationBufferCreateInfoNV)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->dedicatedAllocation, 1 * 4); // VkBool32 dedicatedAllocation
    return data;
}

uint32_t vkUtilsEncodingSize_VkDedicatedAllocationMemoryAllocateInfoNV(const VkDedicatedAllocationMemoryAllocateInfoNV* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkImage image
    res += 1 * 8; // VkBuffer buffer
    return res;
}

void vkUtilsPack_VkDedicatedAllocationMemoryAllocateInfoNV(android::base::InplaceStream* stream, const VkDedicatedAllocationMemoryAllocateInfoNV* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->image, 1 * 8); // VkImage image
    stream->write(&data->buffer, 1 * 8); // VkBuffer buffer
}

VkDedicatedAllocationMemoryAllocateInfoNV* vkUtilsUnpack_VkDedicatedAllocationMemoryAllocateInfoNV(android::base::InplaceStream* stream) {
    VkDedicatedAllocationMemoryAllocateInfoNV* data = new VkDedicatedAllocationMemoryAllocateInfoNV; memset(data, 0, sizeof(VkDedicatedAllocationMemoryAllocateInfoNV)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->image, 1 * 8); // VkImage image
    stream->read(&data->buffer, 1 * 8); // VkBuffer buffer
    return data;
}

uint32_t vkUtilsEncodingSize_VkRenderPassMultiviewCreateInfoKHX(const VkRenderPassMultiviewCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 subpassCount
    res += data->subpassCount * 4; // u32 pViewMasks
    res += 1 * 4; // u32 dependencyCount
    res += data->dependencyCount * 4; // s32 pViewOffsets
    res += 1 * 4; // u32 correlationMaskCount
    res += data->correlationMaskCount * 4; // u32 pCorrelationMasks
    return res;
}

void vkUtilsPack_VkRenderPassMultiviewCreateInfoKHX(android::base::InplaceStream* stream, const VkRenderPassMultiviewCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->subpassCount, 1 * 4); // u32 subpassCount
    stream->write(data->pViewMasks, data->subpassCount * 4); // u32 pViewMasks
    stream->write(&data->dependencyCount, 1 * 4); // u32 dependencyCount
    stream->write(data->pViewOffsets, data->dependencyCount * 4); // s32 pViewOffsets
    stream->write(&data->correlationMaskCount, 1 * 4); // u32 correlationMaskCount
    stream->write(data->pCorrelationMasks, data->correlationMaskCount * 4); // u32 pCorrelationMasks
}

VkRenderPassMultiviewCreateInfoKHX* vkUtilsUnpack_VkRenderPassMultiviewCreateInfoKHX(android::base::InplaceStream* stream) {
    VkRenderPassMultiviewCreateInfoKHX* data = new VkRenderPassMultiviewCreateInfoKHX; memset(data, 0, sizeof(VkRenderPassMultiviewCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->subpassCount, 1 * 4); // u32 subpassCount
    data->pViewMasks = data->subpassCount ? new u32[data->subpassCount] : nullptr; // u32 pViewMasks
    stream->read((char*)data->pViewMasks, data->subpassCount * 4); // u32 pViewMasks
    stream->read(&data->dependencyCount, 1 * 4); // u32 dependencyCount
    data->pViewOffsets = data->dependencyCount ? new s32[data->dependencyCount] : nullptr; // s32 pViewOffsets
    stream->read((char*)data->pViewOffsets, data->dependencyCount * 4); // s32 pViewOffsets
    stream->read(&data->correlationMaskCount, 1 * 4); // u32 correlationMaskCount
    data->pCorrelationMasks = data->correlationMaskCount ? new u32[data->correlationMaskCount] : nullptr; // u32 pCorrelationMasks
    stream->read((char*)data->pCorrelationMasks, data->correlationMaskCount * 4); // u32 pCorrelationMasks
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMultiviewFeaturesKHX(const VkPhysicalDeviceMultiviewFeaturesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBool32 multiview
    res += 1 * 4; // VkBool32 multiviewGeometryShader
    res += 1 * 4; // VkBool32 multiviewTessellationShader
    return res;
}

void vkUtilsPack_VkPhysicalDeviceMultiviewFeaturesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceMultiviewFeaturesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->multiview, 1 * 4); // VkBool32 multiview
    stream->write(&data->multiviewGeometryShader, 1 * 4); // VkBool32 multiviewGeometryShader
    stream->write(&data->multiviewTessellationShader, 1 * 4); // VkBool32 multiviewTessellationShader
}

VkPhysicalDeviceMultiviewFeaturesKHX* vkUtilsUnpack_VkPhysicalDeviceMultiviewFeaturesKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceMultiviewFeaturesKHX* data = new VkPhysicalDeviceMultiviewFeaturesKHX; memset(data, 0, sizeof(VkPhysicalDeviceMultiviewFeaturesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->multiview, 1 * 4); // VkBool32 multiview
    stream->read(&data->multiviewGeometryShader, 1 * 4); // VkBool32 multiviewGeometryShader
    stream->read(&data->multiviewTessellationShader, 1 * 4); // VkBool32 multiviewTessellationShader
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMultiviewPropertiesKHX(const VkPhysicalDeviceMultiviewPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 maxMultiviewViewCount
    res += 1 * 4; // u32 maxMultiviewInstanceIndex
    return res;
}

void vkUtilsPack_VkPhysicalDeviceMultiviewPropertiesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceMultiviewPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->maxMultiviewViewCount, 1 * 4); // u32 maxMultiviewViewCount
    stream->write(&data->maxMultiviewInstanceIndex, 1 * 4); // u32 maxMultiviewInstanceIndex
}

VkPhysicalDeviceMultiviewPropertiesKHX* vkUtilsUnpack_VkPhysicalDeviceMultiviewPropertiesKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceMultiviewPropertiesKHX* data = new VkPhysicalDeviceMultiviewPropertiesKHX; memset(data, 0, sizeof(VkPhysicalDeviceMultiviewPropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->maxMultiviewViewCount, 1 * 4); // u32 maxMultiviewViewCount
    stream->read(&data->maxMultiviewInstanceIndex, 1 * 4); // u32 maxMultiviewInstanceIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalImageFormatPropertiesNV(const VkExternalImageFormatPropertiesNV* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkImageFormatProperties(&data->imageFormatProperties); // VkImageFormatProperties imageFormatProperties
    res += 1 * 4; // VkExternalMemoryFeatureFlagsNV externalMemoryFeatures
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsNV exportFromImportedHandleTypes
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsNV compatibleHandleTypes
    return res;
}

void vkUtilsPack_VkExternalImageFormatPropertiesNV(android::base::InplaceStream* stream, const VkExternalImageFormatPropertiesNV* data) {
    if (!data) return;
    vkUtilsPack_VkImageFormatProperties(stream, &data->imageFormatProperties); // VkImageFormatProperties imageFormatProperties
    stream->write(&data->externalMemoryFeatures, 1 * 4); // VkExternalMemoryFeatureFlagsNV externalMemoryFeatures
    stream->write(&data->exportFromImportedHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV exportFromImportedHandleTypes
    stream->write(&data->compatibleHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV compatibleHandleTypes
}

VkExternalImageFormatPropertiesNV* vkUtilsUnpack_VkExternalImageFormatPropertiesNV(android::base::InplaceStream* stream) {
    VkExternalImageFormatPropertiesNV* data = new VkExternalImageFormatPropertiesNV; memset(data, 0, sizeof(VkExternalImageFormatPropertiesNV)); 
    { VkImageFormatProperties* tmpUnpacked = vkUtilsUnpack_VkImageFormatProperties(stream); memcpy(&data->imageFormatProperties, tmpUnpacked, sizeof(VkImageFormatProperties)); delete tmpUnpacked; } // VkImageFormatProperties imageFormatProperties
    stream->read(&data->externalMemoryFeatures, 1 * 4); // VkExternalMemoryFeatureFlagsNV externalMemoryFeatures
    stream->read(&data->exportFromImportedHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV exportFromImportedHandleTypes
    stream->read(&data->compatibleHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV compatibleHandleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalMemoryImageCreateInfoNV(const VkExternalMemoryImageCreateInfoNV* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsNV handleTypes
    return res;
}

void vkUtilsPack_VkExternalMemoryImageCreateInfoNV(android::base::InplaceStream* stream, const VkExternalMemoryImageCreateInfoNV* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV handleTypes
}

VkExternalMemoryImageCreateInfoNV* vkUtilsUnpack_VkExternalMemoryImageCreateInfoNV(android::base::InplaceStream* stream) {
    VkExternalMemoryImageCreateInfoNV* data = new VkExternalMemoryImageCreateInfoNV; memset(data, 0, sizeof(VkExternalMemoryImageCreateInfoNV)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV handleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkExportMemoryAllocateInfoNV(const VkExportMemoryAllocateInfoNV* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsNV handleTypes
    return res;
}

void vkUtilsPack_VkExportMemoryAllocateInfoNV(android::base::InplaceStream* stream, const VkExportMemoryAllocateInfoNV* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV handleTypes
}

VkExportMemoryAllocateInfoNV* vkUtilsUnpack_VkExportMemoryAllocateInfoNV(android::base::InplaceStream* stream) {
    VkExportMemoryAllocateInfoNV* data = new VkExportMemoryAllocateInfoNV; memset(data, 0, sizeof(VkExportMemoryAllocateInfoNV)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsNV handleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceFeatures2KHR(const VkPhysicalDeviceFeatures2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkPhysicalDeviceFeatures(&data->features); // VkPhysicalDeviceFeatures features
    return res;
}

void vkUtilsPack_VkPhysicalDeviceFeatures2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceFeatures2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkPhysicalDeviceFeatures(stream, &data->features); // VkPhysicalDeviceFeatures features
}

VkPhysicalDeviceFeatures2KHR* vkUtilsUnpack_VkPhysicalDeviceFeatures2KHR(android::base::InplaceStream* stream) {
    VkPhysicalDeviceFeatures2KHR* data = new VkPhysicalDeviceFeatures2KHR; memset(data, 0, sizeof(VkPhysicalDeviceFeatures2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkPhysicalDeviceFeatures* tmpUnpacked = vkUtilsUnpack_VkPhysicalDeviceFeatures(stream); memcpy(&data->features, tmpUnpacked, sizeof(VkPhysicalDeviceFeatures)); delete tmpUnpacked; } // VkPhysicalDeviceFeatures features
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceProperties2KHR(const VkPhysicalDeviceProperties2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkPhysicalDeviceProperties(&data->properties); // VkPhysicalDeviceProperties properties
    return res;
}

void vkUtilsPack_VkPhysicalDeviceProperties2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceProperties2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkPhysicalDeviceProperties(stream, &data->properties); // VkPhysicalDeviceProperties properties
}

VkPhysicalDeviceProperties2KHR* vkUtilsUnpack_VkPhysicalDeviceProperties2KHR(android::base::InplaceStream* stream) {
    VkPhysicalDeviceProperties2KHR* data = new VkPhysicalDeviceProperties2KHR; memset(data, 0, sizeof(VkPhysicalDeviceProperties2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkPhysicalDeviceProperties* tmpUnpacked = vkUtilsUnpack_VkPhysicalDeviceProperties(stream); memcpy(&data->properties, tmpUnpacked, sizeof(VkPhysicalDeviceProperties)); delete tmpUnpacked; } // VkPhysicalDeviceProperties properties
    return data;
}

uint32_t vkUtilsEncodingSize_VkFormatProperties2KHR(const VkFormatProperties2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkFormatProperties(&data->formatProperties); // VkFormatProperties formatProperties
    return res;
}

void vkUtilsPack_VkFormatProperties2KHR(android::base::InplaceStream* stream, const VkFormatProperties2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkFormatProperties(stream, &data->formatProperties); // VkFormatProperties formatProperties
}

VkFormatProperties2KHR* vkUtilsUnpack_VkFormatProperties2KHR(android::base::InplaceStream* stream) {
    VkFormatProperties2KHR* data = new VkFormatProperties2KHR; memset(data, 0, sizeof(VkFormatProperties2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkFormatProperties* tmpUnpacked = vkUtilsUnpack_VkFormatProperties(stream); memcpy(&data->formatProperties, tmpUnpacked, sizeof(VkFormatProperties)); delete tmpUnpacked; } // VkFormatProperties formatProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageFormatProperties2KHR(const VkImageFormatProperties2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkImageFormatProperties(&data->imageFormatProperties); // VkImageFormatProperties imageFormatProperties
    return res;
}

void vkUtilsPack_VkImageFormatProperties2KHR(android::base::InplaceStream* stream, const VkImageFormatProperties2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkImageFormatProperties(stream, &data->imageFormatProperties); // VkImageFormatProperties imageFormatProperties
}

VkImageFormatProperties2KHR* vkUtilsUnpack_VkImageFormatProperties2KHR(android::base::InplaceStream* stream) {
    VkImageFormatProperties2KHR* data = new VkImageFormatProperties2KHR; memset(data, 0, sizeof(VkImageFormatProperties2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkImageFormatProperties* tmpUnpacked = vkUtilsUnpack_VkImageFormatProperties(stream); memcpy(&data->imageFormatProperties, tmpUnpacked, sizeof(VkImageFormatProperties)); delete tmpUnpacked; } // VkImageFormatProperties imageFormatProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceImageFormatInfo2KHR(const VkPhysicalDeviceImageFormatInfo2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkFormat format
    res += 1 * 4; // VkImageType type
    res += 1 * 4; // VkImageTiling tiling
    res += 1 * 4; // VkImageUsageFlags usage
    res += 1 * 4; // VkImageCreateFlags flags
    return res;
}

void vkUtilsPack_VkPhysicalDeviceImageFormatInfo2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceImageFormatInfo2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->format, 1 * 4); // VkFormat format
    stream->write(&data->type, 1 * 4); // VkImageType type
    stream->write(&data->tiling, 1 * 4); // VkImageTiling tiling
    stream->write(&data->usage, 1 * 4); // VkImageUsageFlags usage
    stream->write(&data->flags, 1 * 4); // VkImageCreateFlags flags
}

VkPhysicalDeviceImageFormatInfo2KHR* vkUtilsUnpack_VkPhysicalDeviceImageFormatInfo2KHR(android::base::InplaceStream* stream) {
    VkPhysicalDeviceImageFormatInfo2KHR* data = new VkPhysicalDeviceImageFormatInfo2KHR; memset(data, 0, sizeof(VkPhysicalDeviceImageFormatInfo2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->format, 1 * 4); // VkFormat format
    stream->read(&data->type, 1 * 4); // VkImageType type
    stream->read(&data->tiling, 1 * 4); // VkImageTiling tiling
    stream->read(&data->usage, 1 * 4); // VkImageUsageFlags usage
    stream->read(&data->flags, 1 * 4); // VkImageCreateFlags flags
    return data;
}

uint32_t vkUtilsEncodingSize_VkQueueFamilyProperties2KHR(const VkQueueFamilyProperties2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkQueueFamilyProperties(&data->queueFamilyProperties); // VkQueueFamilyProperties queueFamilyProperties
    return res;
}

void vkUtilsPack_VkQueueFamilyProperties2KHR(android::base::InplaceStream* stream, const VkQueueFamilyProperties2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkQueueFamilyProperties(stream, &data->queueFamilyProperties); // VkQueueFamilyProperties queueFamilyProperties
}

VkQueueFamilyProperties2KHR* vkUtilsUnpack_VkQueueFamilyProperties2KHR(android::base::InplaceStream* stream) {
    VkQueueFamilyProperties2KHR* data = new VkQueueFamilyProperties2KHR; memset(data, 0, sizeof(VkQueueFamilyProperties2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkQueueFamilyProperties* tmpUnpacked = vkUtilsUnpack_VkQueueFamilyProperties(stream); memcpy(&data->queueFamilyProperties, tmpUnpacked, sizeof(VkQueueFamilyProperties)); delete tmpUnpacked; } // VkQueueFamilyProperties queueFamilyProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMemoryProperties2KHR(const VkPhysicalDeviceMemoryProperties2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkPhysicalDeviceMemoryProperties(&data->memoryProperties); // VkPhysicalDeviceMemoryProperties memoryProperties
    return res;
}

void vkUtilsPack_VkPhysicalDeviceMemoryProperties2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceMemoryProperties2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkPhysicalDeviceMemoryProperties(stream, &data->memoryProperties); // VkPhysicalDeviceMemoryProperties memoryProperties
}

VkPhysicalDeviceMemoryProperties2KHR* vkUtilsUnpack_VkPhysicalDeviceMemoryProperties2KHR(android::base::InplaceStream* stream) {
    VkPhysicalDeviceMemoryProperties2KHR* data = new VkPhysicalDeviceMemoryProperties2KHR; memset(data, 0, sizeof(VkPhysicalDeviceMemoryProperties2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkPhysicalDeviceMemoryProperties* tmpUnpacked = vkUtilsUnpack_VkPhysicalDeviceMemoryProperties(stream); memcpy(&data->memoryProperties, tmpUnpacked, sizeof(VkPhysicalDeviceMemoryProperties)); delete tmpUnpacked; } // VkPhysicalDeviceMemoryProperties memoryProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkSparseImageFormatProperties2KHR(const VkSparseImageFormatProperties2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkSparseImageFormatProperties(&data->properties); // VkSparseImageFormatProperties properties
    return res;
}

void vkUtilsPack_VkSparseImageFormatProperties2KHR(android::base::InplaceStream* stream, const VkSparseImageFormatProperties2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkSparseImageFormatProperties(stream, &data->properties); // VkSparseImageFormatProperties properties
}

VkSparseImageFormatProperties2KHR* vkUtilsUnpack_VkSparseImageFormatProperties2KHR(android::base::InplaceStream* stream) {
    VkSparseImageFormatProperties2KHR* data = new VkSparseImageFormatProperties2KHR; memset(data, 0, sizeof(VkSparseImageFormatProperties2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkSparseImageFormatProperties* tmpUnpacked = vkUtilsUnpack_VkSparseImageFormatProperties(stream); memcpy(&data->properties, tmpUnpacked, sizeof(VkSparseImageFormatProperties)); delete tmpUnpacked; } // VkSparseImageFormatProperties properties
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceSparseImageFormatInfo2KHR(const VkPhysicalDeviceSparseImageFormatInfo2KHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkFormat format
    res += 1 * 4; // VkImageType type
    res += 1 * 4; // VkSampleCountFlagBits samples
    res += 1 * 4; // VkImageUsageFlags usage
    res += 1 * 4; // VkImageTiling tiling
    return res;
}

void vkUtilsPack_VkPhysicalDeviceSparseImageFormatInfo2KHR(android::base::InplaceStream* stream, const VkPhysicalDeviceSparseImageFormatInfo2KHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->format, 1 * 4); // VkFormat format
    stream->write(&data->type, 1 * 4); // VkImageType type
    stream->write(&data->samples, 1 * 4); // VkSampleCountFlagBits samples
    stream->write(&data->usage, 1 * 4); // VkImageUsageFlags usage
    stream->write(&data->tiling, 1 * 4); // VkImageTiling tiling
}

VkPhysicalDeviceSparseImageFormatInfo2KHR* vkUtilsUnpack_VkPhysicalDeviceSparseImageFormatInfo2KHR(android::base::InplaceStream* stream) {
    VkPhysicalDeviceSparseImageFormatInfo2KHR* data = new VkPhysicalDeviceSparseImageFormatInfo2KHR; memset(data, 0, sizeof(VkPhysicalDeviceSparseImageFormatInfo2KHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->format, 1 * 4); // VkFormat format
    stream->read(&data->type, 1 * 4); // VkImageType type
    stream->read(&data->samples, 1 * 4); // VkSampleCountFlagBits samples
    stream->read(&data->usage, 1 * 4); // VkImageUsageFlags usage
    stream->read(&data->tiling, 1 * 4); // VkImageTiling tiling
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryAllocateFlagsInfoKHX(const VkMemoryAllocateFlagsInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkMemoryAllocateFlagsKHX flags
    res += 1 * 4; // u32 deviceMask
    return res;
}

void vkUtilsPack_VkMemoryAllocateFlagsInfoKHX(android::base::InplaceStream* stream, const VkMemoryAllocateFlagsInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkMemoryAllocateFlagsKHX flags
    stream->write(&data->deviceMask, 1 * 4); // u32 deviceMask
}

VkMemoryAllocateFlagsInfoKHX* vkUtilsUnpack_VkMemoryAllocateFlagsInfoKHX(android::base::InplaceStream* stream) {
    VkMemoryAllocateFlagsInfoKHX* data = new VkMemoryAllocateFlagsInfoKHX; memset(data, 0, sizeof(VkMemoryAllocateFlagsInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkMemoryAllocateFlagsKHX flags
    stream->read(&data->deviceMask, 1 * 4); // u32 deviceMask
    return data;
}

uint32_t vkUtilsEncodingSize_VkBindBufferMemoryInfoKHX(const VkBindBufferMemoryInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkBuffer buffer
    res += 1 * 8; // VkDeviceMemory memory
    res += 1 * 8; // VkDeviceSize memoryOffset
    res += 1 * 4; // u32 deviceIndexCount
    res += data->deviceIndexCount * 4; // u32 pDeviceIndices
    return res;
}

void vkUtilsPack_VkBindBufferMemoryInfoKHX(android::base::InplaceStream* stream, const VkBindBufferMemoryInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->write(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->write(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->write(&data->deviceIndexCount, 1 * 4); // u32 deviceIndexCount
    stream->write(data->pDeviceIndices, data->deviceIndexCount * 4); // u32 pDeviceIndices
}

VkBindBufferMemoryInfoKHX* vkUtilsUnpack_VkBindBufferMemoryInfoKHX(android::base::InplaceStream* stream) {
    VkBindBufferMemoryInfoKHX* data = new VkBindBufferMemoryInfoKHX; memset(data, 0, sizeof(VkBindBufferMemoryInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->buffer, 1 * 8); // VkBuffer buffer
    stream->read(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->read(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->read(&data->deviceIndexCount, 1 * 4); // u32 deviceIndexCount
    data->pDeviceIndices = data->deviceIndexCount ? new u32[data->deviceIndexCount] : nullptr; // u32 pDeviceIndices
    stream->read((char*)data->pDeviceIndices, data->deviceIndexCount * 4); // u32 pDeviceIndices
    return data;
}

uint32_t vkUtilsEncodingSize_VkBindImageMemoryInfoKHX(const VkBindImageMemoryInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkImage image
    res += 1 * 8; // VkDeviceMemory memory
    res += 1 * 8; // VkDeviceSize memoryOffset
    res += 1 * 4; // u32 deviceIndexCount
    res += data->deviceIndexCount * 4; // u32 pDeviceIndices
    res += 1 * 4; // u32 SFRRectCount
    res += 8; if (data->pSFRRects) { res += data->SFRRectCount * vkUtilsEncodingSize_VkRect2D(data->pSFRRects); } // VkRect2D pSFRRects
    return res;
}

void vkUtilsPack_VkBindImageMemoryInfoKHX(android::base::InplaceStream* stream, const VkBindImageMemoryInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->image, 1 * 8); // VkImage image
    stream->write(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->write(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->write(&data->deviceIndexCount, 1 * 4); // u32 deviceIndexCount
    stream->write(data->pDeviceIndices, data->deviceIndexCount * 4); // u32 pDeviceIndices
    stream->write(&data->SFRRectCount, 1 * 4); // u32 SFRRectCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pSFRRects); stream->write(&ptrval, 8);
    if (data->pSFRRects) { for (uint32_t i = 0; i < data->SFRRectCount; i++) { vkUtilsPack_VkRect2D(stream, data->pSFRRects + i); } } } // VkRect2D pSFRRects
}

VkBindImageMemoryInfoKHX* vkUtilsUnpack_VkBindImageMemoryInfoKHX(android::base::InplaceStream* stream) {
    VkBindImageMemoryInfoKHX* data = new VkBindImageMemoryInfoKHX; memset(data, 0, sizeof(VkBindImageMemoryInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->image, 1 * 8); // VkImage image
    stream->read(&data->memory, 1 * 8); // VkDeviceMemory memory
    stream->read(&data->memoryOffset, 1 * 8); // VkDeviceSize memoryOffset
    stream->read(&data->deviceIndexCount, 1 * 4); // u32 deviceIndexCount
    data->pDeviceIndices = data->deviceIndexCount ? new u32[data->deviceIndexCount] : nullptr; // u32 pDeviceIndices
    stream->read((char*)data->pDeviceIndices, data->deviceIndexCount * 4); // u32 pDeviceIndices
    stream->read(&data->SFRRectCount, 1 * 4); // u32 SFRRectCount
    { // VkRect2D pSFRRects
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkRect2D* tmpArr = ptrval ? new VkRect2D[data->SFRRectCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->SFRRectCount; i++) {
        VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } }
        data->pSFRRects = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupRenderPassBeginInfoKHX(const VkDeviceGroupRenderPassBeginInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 deviceMask
    res += 1 * 4; // u32 deviceRenderAreaCount
    res += 8; if (data->pDeviceRenderAreas) { res += data->deviceRenderAreaCount * vkUtilsEncodingSize_VkRect2D(data->pDeviceRenderAreas); } // VkRect2D pDeviceRenderAreas
    return res;
}

void vkUtilsPack_VkDeviceGroupRenderPassBeginInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupRenderPassBeginInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->deviceMask, 1 * 4); // u32 deviceMask
    stream->write(&data->deviceRenderAreaCount, 1 * 4); // u32 deviceRenderAreaCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDeviceRenderAreas); stream->write(&ptrval, 8);
    if (data->pDeviceRenderAreas) { for (uint32_t i = 0; i < data->deviceRenderAreaCount; i++) { vkUtilsPack_VkRect2D(stream, data->pDeviceRenderAreas + i); } } } // VkRect2D pDeviceRenderAreas
}

VkDeviceGroupRenderPassBeginInfoKHX* vkUtilsUnpack_VkDeviceGroupRenderPassBeginInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupRenderPassBeginInfoKHX* data = new VkDeviceGroupRenderPassBeginInfoKHX; memset(data, 0, sizeof(VkDeviceGroupRenderPassBeginInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->deviceMask, 1 * 4); // u32 deviceMask
    stream->read(&data->deviceRenderAreaCount, 1 * 4); // u32 deviceRenderAreaCount
    { // VkRect2D pDeviceRenderAreas
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkRect2D* tmpArr = ptrval ? new VkRect2D[data->deviceRenderAreaCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->deviceRenderAreaCount; i++) {
        VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } }
        data->pDeviceRenderAreas = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupCommandBufferBeginInfoKHX(const VkDeviceGroupCommandBufferBeginInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 deviceMask
    return res;
}

void vkUtilsPack_VkDeviceGroupCommandBufferBeginInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupCommandBufferBeginInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->deviceMask, 1 * 4); // u32 deviceMask
}

VkDeviceGroupCommandBufferBeginInfoKHX* vkUtilsUnpack_VkDeviceGroupCommandBufferBeginInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupCommandBufferBeginInfoKHX* data = new VkDeviceGroupCommandBufferBeginInfoKHX; memset(data, 0, sizeof(VkDeviceGroupCommandBufferBeginInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->deviceMask, 1 * 4); // u32 deviceMask
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupSubmitInfoKHX(const VkDeviceGroupSubmitInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 waitSemaphoreCount
    res += data->waitSemaphoreCount * 4; // u32 pWaitSemaphoreDeviceIndices
    res += 1 * 4; // u32 commandBufferCount
    res += data->commandBufferCount * 4; // u32 pCommandBufferDeviceMasks
    res += 1 * 4; // u32 signalSemaphoreCount
    res += data->signalSemaphoreCount * 4; // u32 pSignalSemaphoreDeviceIndices
    return res;
}

void vkUtilsPack_VkDeviceGroupSubmitInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupSubmitInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    stream->write(data->pWaitSemaphoreDeviceIndices, data->waitSemaphoreCount * 4); // u32 pWaitSemaphoreDeviceIndices
    stream->write(&data->commandBufferCount, 1 * 4); // u32 commandBufferCount
    stream->write(data->pCommandBufferDeviceMasks, data->commandBufferCount * 4); // u32 pCommandBufferDeviceMasks
    stream->write(&data->signalSemaphoreCount, 1 * 4); // u32 signalSemaphoreCount
    stream->write(data->pSignalSemaphoreDeviceIndices, data->signalSemaphoreCount * 4); // u32 pSignalSemaphoreDeviceIndices
}

VkDeviceGroupSubmitInfoKHX* vkUtilsUnpack_VkDeviceGroupSubmitInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupSubmitInfoKHX* data = new VkDeviceGroupSubmitInfoKHX; memset(data, 0, sizeof(VkDeviceGroupSubmitInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->waitSemaphoreCount, 1 * 4); // u32 waitSemaphoreCount
    data->pWaitSemaphoreDeviceIndices = data->waitSemaphoreCount ? new u32[data->waitSemaphoreCount] : nullptr; // u32 pWaitSemaphoreDeviceIndices
    stream->read((char*)data->pWaitSemaphoreDeviceIndices, data->waitSemaphoreCount * 4); // u32 pWaitSemaphoreDeviceIndices
    stream->read(&data->commandBufferCount, 1 * 4); // u32 commandBufferCount
    data->pCommandBufferDeviceMasks = data->commandBufferCount ? new u32[data->commandBufferCount] : nullptr; // u32 pCommandBufferDeviceMasks
    stream->read((char*)data->pCommandBufferDeviceMasks, data->commandBufferCount * 4); // u32 pCommandBufferDeviceMasks
    stream->read(&data->signalSemaphoreCount, 1 * 4); // u32 signalSemaphoreCount
    data->pSignalSemaphoreDeviceIndices = data->signalSemaphoreCount ? new u32[data->signalSemaphoreCount] : nullptr; // u32 pSignalSemaphoreDeviceIndices
    stream->read((char*)data->pSignalSemaphoreDeviceIndices, data->signalSemaphoreCount * 4); // u32 pSignalSemaphoreDeviceIndices
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupBindSparseInfoKHX(const VkDeviceGroupBindSparseInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 resourceDeviceIndex
    res += 1 * 4; // u32 memoryDeviceIndex
    return res;
}

void vkUtilsPack_VkDeviceGroupBindSparseInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupBindSparseInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->resourceDeviceIndex, 1 * 4); // u32 resourceDeviceIndex
    stream->write(&data->memoryDeviceIndex, 1 * 4); // u32 memoryDeviceIndex
}

VkDeviceGroupBindSparseInfoKHX* vkUtilsUnpack_VkDeviceGroupBindSparseInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupBindSparseInfoKHX* data = new VkDeviceGroupBindSparseInfoKHX; memset(data, 0, sizeof(VkDeviceGroupBindSparseInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->resourceDeviceIndex, 1 * 4); // u32 resourceDeviceIndex
    stream->read(&data->memoryDeviceIndex, 1 * 4); // u32 memoryDeviceIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupPresentCapabilitiesKHX(const VkDeviceGroupPresentCapabilitiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += VK_MAX_DEVICE_GROUP_SIZE_KHX * 4; // u32 presentMask
    res += 1 * 4; // VkDeviceGroupPresentModeFlagsKHX modes
    return res;
}

void vkUtilsPack_VkDeviceGroupPresentCapabilitiesKHX(android::base::InplaceStream* stream, const VkDeviceGroupPresentCapabilitiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->presentMask, VK_MAX_DEVICE_GROUP_SIZE_KHX * 4); // u32 presentMask
    stream->write(&data->modes, 1 * 4); // VkDeviceGroupPresentModeFlagsKHX modes
}

VkDeviceGroupPresentCapabilitiesKHX* vkUtilsUnpack_VkDeviceGroupPresentCapabilitiesKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupPresentCapabilitiesKHX* data = new VkDeviceGroupPresentCapabilitiesKHX; memset(data, 0, sizeof(VkDeviceGroupPresentCapabilitiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->presentMask, VK_MAX_DEVICE_GROUP_SIZE_KHX * 4); // u32 presentMask
    stream->read(&data->modes, 1 * 4); // VkDeviceGroupPresentModeFlagsKHX modes
    return data;
}

uint32_t vkUtilsEncodingSize_VkImageSwapchainCreateInfoKHX(const VkImageSwapchainCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkSwapchainKHR swapchain
    return res;
}

void vkUtilsPack_VkImageSwapchainCreateInfoKHX(android::base::InplaceStream* stream, const VkImageSwapchainCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->swapchain, 1 * 8); // VkSwapchainKHR swapchain
}

VkImageSwapchainCreateInfoKHX* vkUtilsUnpack_VkImageSwapchainCreateInfoKHX(android::base::InplaceStream* stream) {
    VkImageSwapchainCreateInfoKHX* data = new VkImageSwapchainCreateInfoKHX; memset(data, 0, sizeof(VkImageSwapchainCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->swapchain, 1 * 8); // VkSwapchainKHR swapchain
    return data;
}

uint32_t vkUtilsEncodingSize_VkBindImageMemorySwapchainInfoKHX(const VkBindImageMemorySwapchainInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkSwapchainKHR swapchain
    res += 1 * 4; // u32 imageIndex
    return res;
}

void vkUtilsPack_VkBindImageMemorySwapchainInfoKHX(android::base::InplaceStream* stream, const VkBindImageMemorySwapchainInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->swapchain, 1 * 8); // VkSwapchainKHR swapchain
    stream->write(&data->imageIndex, 1 * 4); // u32 imageIndex
}

VkBindImageMemorySwapchainInfoKHX* vkUtilsUnpack_VkBindImageMemorySwapchainInfoKHX(android::base::InplaceStream* stream) {
    VkBindImageMemorySwapchainInfoKHX* data = new VkBindImageMemorySwapchainInfoKHX; memset(data, 0, sizeof(VkBindImageMemorySwapchainInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->swapchain, 1 * 8); // VkSwapchainKHR swapchain
    stream->read(&data->imageIndex, 1 * 4); // u32 imageIndex
    return data;
}

uint32_t vkUtilsEncodingSize_VkAcquireNextImageInfoKHX(const VkAcquireNextImageInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkSwapchainKHR swapchain
    res += 1 * 8; // u64 timeout
    res += 1 * 8; // VkSemaphore semaphore
    res += 1 * 8; // VkFence fence
    res += 1 * 4; // u32 deviceMask
    return res;
}

void vkUtilsPack_VkAcquireNextImageInfoKHX(android::base::InplaceStream* stream, const VkAcquireNextImageInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->swapchain, 1 * 8); // VkSwapchainKHR swapchain
    stream->write(&data->timeout, 1 * 8); // u64 timeout
    stream->write(&data->semaphore, 1 * 8); // VkSemaphore semaphore
    stream->write(&data->fence, 1 * 8); // VkFence fence
    stream->write(&data->deviceMask, 1 * 4); // u32 deviceMask
}

VkAcquireNextImageInfoKHX* vkUtilsUnpack_VkAcquireNextImageInfoKHX(android::base::InplaceStream* stream) {
    VkAcquireNextImageInfoKHX* data = new VkAcquireNextImageInfoKHX; memset(data, 0, sizeof(VkAcquireNextImageInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->swapchain, 1 * 8); // VkSwapchainKHR swapchain
    stream->read(&data->timeout, 1 * 8); // u64 timeout
    stream->read(&data->semaphore, 1 * 8); // VkSemaphore semaphore
    stream->read(&data->fence, 1 * 8); // VkFence fence
    stream->read(&data->deviceMask, 1 * 4); // u32 deviceMask
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupPresentInfoKHX(const VkDeviceGroupPresentInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 swapchainCount
    res += data->swapchainCount * 4; // u32 pDeviceMasks
    res += 1 * 4; // VkDeviceGroupPresentModeFlagBitsKHX mode
    return res;
}

void vkUtilsPack_VkDeviceGroupPresentInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupPresentInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    stream->write(data->pDeviceMasks, data->swapchainCount * 4); // u32 pDeviceMasks
    stream->write(&data->mode, 1 * 4); // VkDeviceGroupPresentModeFlagBitsKHX mode
}

VkDeviceGroupPresentInfoKHX* vkUtilsUnpack_VkDeviceGroupPresentInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupPresentInfoKHX* data = new VkDeviceGroupPresentInfoKHX; memset(data, 0, sizeof(VkDeviceGroupPresentInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    data->pDeviceMasks = data->swapchainCount ? new u32[data->swapchainCount] : nullptr; // u32 pDeviceMasks
    stream->read((char*)data->pDeviceMasks, data->swapchainCount * 4); // u32 pDeviceMasks
    stream->read(&data->mode, 1 * 4); // VkDeviceGroupPresentModeFlagBitsKHX mode
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupSwapchainCreateInfoKHX(const VkDeviceGroupSwapchainCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDeviceGroupPresentModeFlagsKHX modes
    return res;
}

void vkUtilsPack_VkDeviceGroupSwapchainCreateInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupSwapchainCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->modes, 1 * 4); // VkDeviceGroupPresentModeFlagsKHX modes
}

VkDeviceGroupSwapchainCreateInfoKHX* vkUtilsUnpack_VkDeviceGroupSwapchainCreateInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupSwapchainCreateInfoKHX* data = new VkDeviceGroupSwapchainCreateInfoKHX; memset(data, 0, sizeof(VkDeviceGroupSwapchainCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->modes, 1 * 4); // VkDeviceGroupPresentModeFlagsKHX modes
    return data;
}

uint32_t vkUtilsEncodingSize_VkValidationFlagsEXT(const VkValidationFlagsEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 disabledValidationCheckCount
    res += data->disabledValidationCheckCount * 4; // VkValidationCheckEXT pDisabledValidationChecks
    return res;
}

void vkUtilsPack_VkValidationFlagsEXT(android::base::InplaceStream* stream, const VkValidationFlagsEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->disabledValidationCheckCount, 1 * 4); // u32 disabledValidationCheckCount
    stream->write(data->pDisabledValidationChecks, data->disabledValidationCheckCount * 4); // VkValidationCheckEXT pDisabledValidationChecks
}

VkValidationFlagsEXT* vkUtilsUnpack_VkValidationFlagsEXT(android::base::InplaceStream* stream) {
    VkValidationFlagsEXT* data = new VkValidationFlagsEXT; memset(data, 0, sizeof(VkValidationFlagsEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->disabledValidationCheckCount, 1 * 4); // u32 disabledValidationCheckCount
    data->pDisabledValidationChecks = data->disabledValidationCheckCount ? new VkValidationCheckEXT[data->disabledValidationCheckCount] : nullptr; // VkValidationCheckEXT pDisabledValidationChecks
    stream->read((char*)data->pDisabledValidationChecks, data->disabledValidationCheckCount * 4); // VkValidationCheckEXT pDisabledValidationChecks
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceGroupPropertiesKHX(const VkPhysicalDeviceGroupPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 physicalDeviceCount
    res += VK_MAX_DEVICE_GROUP_SIZE_KHX * 8; // VkPhysicalDevice physicalDevices
    res += 1 * 4; // VkBool32 subsetAllocation
    return res;
}

void vkUtilsPack_VkPhysicalDeviceGroupPropertiesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceGroupPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->physicalDeviceCount, 1 * 4); // u32 physicalDeviceCount
    stream->write(&data->physicalDevices, VK_MAX_DEVICE_GROUP_SIZE_KHX * 8); // VkPhysicalDevice physicalDevices
    stream->write(&data->subsetAllocation, 1 * 4); // VkBool32 subsetAllocation
}

VkPhysicalDeviceGroupPropertiesKHX* vkUtilsUnpack_VkPhysicalDeviceGroupPropertiesKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceGroupPropertiesKHX* data = new VkPhysicalDeviceGroupPropertiesKHX; memset(data, 0, sizeof(VkPhysicalDeviceGroupPropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->physicalDeviceCount, 1 * 4); // u32 physicalDeviceCount
    stream->read(&data->physicalDevices, VK_MAX_DEVICE_GROUP_SIZE_KHX * 8); // VkPhysicalDevice physicalDevices
    stream->read(&data->subsetAllocation, 1 * 4); // VkBool32 subsetAllocation
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceGroupDeviceCreateInfoKHX(const VkDeviceGroupDeviceCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 physicalDeviceCount
    res += data->physicalDeviceCount * 8; // VkPhysicalDevice pPhysicalDevices
    return res;
}

void vkUtilsPack_VkDeviceGroupDeviceCreateInfoKHX(android::base::InplaceStream* stream, const VkDeviceGroupDeviceCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->physicalDeviceCount, 1 * 4); // u32 physicalDeviceCount
    stream->write(data->pPhysicalDevices, data->physicalDeviceCount * 8); // VkPhysicalDevice pPhysicalDevices
}

VkDeviceGroupDeviceCreateInfoKHX* vkUtilsUnpack_VkDeviceGroupDeviceCreateInfoKHX(android::base::InplaceStream* stream) {
    VkDeviceGroupDeviceCreateInfoKHX* data = new VkDeviceGroupDeviceCreateInfoKHX; memset(data, 0, sizeof(VkDeviceGroupDeviceCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->physicalDeviceCount, 1 * 4); // u32 physicalDeviceCount
    data->pPhysicalDevices = data->physicalDeviceCount ? new VkPhysicalDevice[data->physicalDeviceCount] : nullptr; // VkPhysicalDevice pPhysicalDevices
    stream->read((char*)data->pPhysicalDevices, data->physicalDeviceCount * 8); // VkPhysicalDevice pPhysicalDevices
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalMemoryPropertiesKHX(const VkExternalMemoryPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkExternalMemoryFeatureFlagsKHX externalMemoryFeatures
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsKHX exportFromImportedHandleTypes
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsKHX compatibleHandleTypes
    return res;
}

void vkUtilsPack_VkExternalMemoryPropertiesKHX(android::base::InplaceStream* stream, const VkExternalMemoryPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->externalMemoryFeatures, 1 * 4); // VkExternalMemoryFeatureFlagsKHX externalMemoryFeatures
    stream->write(&data->exportFromImportedHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX exportFromImportedHandleTypes
    stream->write(&data->compatibleHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX compatibleHandleTypes
}

VkExternalMemoryPropertiesKHX* vkUtilsUnpack_VkExternalMemoryPropertiesKHX(android::base::InplaceStream* stream) {
    VkExternalMemoryPropertiesKHX* data = new VkExternalMemoryPropertiesKHX; memset(data, 0, sizeof(VkExternalMemoryPropertiesKHX)); 
    stream->read(&data->externalMemoryFeatures, 1 * 4); // VkExternalMemoryFeatureFlagsKHX externalMemoryFeatures
    stream->read(&data->exportFromImportedHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX exportFromImportedHandleTypes
    stream->read(&data->compatibleHandleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX compatibleHandleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceExternalImageFormatInfoKHX(const VkPhysicalDeviceExternalImageFormatInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    return res;
}

void vkUtilsPack_VkPhysicalDeviceExternalImageFormatInfoKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceExternalImageFormatInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleType, 1 * 4); // VkExternalMemoryHandleTypeFlagBitsKHX handleType
}

VkPhysicalDeviceExternalImageFormatInfoKHX* vkUtilsUnpack_VkPhysicalDeviceExternalImageFormatInfoKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceExternalImageFormatInfoKHX* data = new VkPhysicalDeviceExternalImageFormatInfoKHX; memset(data, 0, sizeof(VkPhysicalDeviceExternalImageFormatInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleType, 1 * 4); // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalImageFormatPropertiesKHX(const VkExternalImageFormatPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkExternalMemoryPropertiesKHX(&data->externalMemoryProperties); // VkExternalMemoryPropertiesKHX externalMemoryProperties
    return res;
}

void vkUtilsPack_VkExternalImageFormatPropertiesKHX(android::base::InplaceStream* stream, const VkExternalImageFormatPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkExternalMemoryPropertiesKHX(stream, &data->externalMemoryProperties); // VkExternalMemoryPropertiesKHX externalMemoryProperties
}

VkExternalImageFormatPropertiesKHX* vkUtilsUnpack_VkExternalImageFormatPropertiesKHX(android::base::InplaceStream* stream) {
    VkExternalImageFormatPropertiesKHX* data = new VkExternalImageFormatPropertiesKHX; memset(data, 0, sizeof(VkExternalImageFormatPropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkExternalMemoryPropertiesKHX* tmpUnpacked = vkUtilsUnpack_VkExternalMemoryPropertiesKHX(stream); memcpy(&data->externalMemoryProperties, tmpUnpacked, sizeof(VkExternalMemoryPropertiesKHX)); delete tmpUnpacked; } // VkExternalMemoryPropertiesKHX externalMemoryProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceExternalBufferInfoKHX(const VkPhysicalDeviceExternalBufferInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBufferCreateFlags flags
    res += 1 * 4; // VkBufferUsageFlags usage
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    return res;
}

void vkUtilsPack_VkPhysicalDeviceExternalBufferInfoKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceExternalBufferInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkBufferCreateFlags flags
    stream->write(&data->usage, 1 * 4); // VkBufferUsageFlags usage
    stream->write(&data->handleType, 1 * 4); // VkExternalMemoryHandleTypeFlagBitsKHX handleType
}

VkPhysicalDeviceExternalBufferInfoKHX* vkUtilsUnpack_VkPhysicalDeviceExternalBufferInfoKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceExternalBufferInfoKHX* data = new VkPhysicalDeviceExternalBufferInfoKHX; memset(data, 0, sizeof(VkPhysicalDeviceExternalBufferInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkBufferCreateFlags flags
    stream->read(&data->usage, 1 * 4); // VkBufferUsageFlags usage
    stream->read(&data->handleType, 1 * 4); // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalBufferPropertiesKHX(const VkExternalBufferPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkExternalMemoryPropertiesKHX(&data->externalMemoryProperties); // VkExternalMemoryPropertiesKHX externalMemoryProperties
    return res;
}

void vkUtilsPack_VkExternalBufferPropertiesKHX(android::base::InplaceStream* stream, const VkExternalBufferPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkExternalMemoryPropertiesKHX(stream, &data->externalMemoryProperties); // VkExternalMemoryPropertiesKHX externalMemoryProperties
}

VkExternalBufferPropertiesKHX* vkUtilsUnpack_VkExternalBufferPropertiesKHX(android::base::InplaceStream* stream) {
    VkExternalBufferPropertiesKHX* data = new VkExternalBufferPropertiesKHX; memset(data, 0, sizeof(VkExternalBufferPropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkExternalMemoryPropertiesKHX* tmpUnpacked = vkUtilsUnpack_VkExternalMemoryPropertiesKHX(stream); memcpy(&data->externalMemoryProperties, tmpUnpacked, sizeof(VkExternalMemoryPropertiesKHX)); delete tmpUnpacked; } // VkExternalMemoryPropertiesKHX externalMemoryProperties
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceIDPropertiesKHX(const VkPhysicalDeviceIDPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += VK_UUID_SIZE * 1; // u8 deviceUUID
    res += VK_UUID_SIZE * 1; // u8 driverUUID
    res += VK_LUID_SIZE_KHX * 1; // u8 deviceLUID
    res += 1 * 4; // VkBool32 deviceLUIDValid
    return res;
}

void vkUtilsPack_VkPhysicalDeviceIDPropertiesKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceIDPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->deviceUUID, VK_UUID_SIZE * 1); // u8 deviceUUID
    stream->write(&data->driverUUID, VK_UUID_SIZE * 1); // u8 driverUUID
    stream->write(&data->deviceLUID, VK_LUID_SIZE_KHX * 1); // u8 deviceLUID
    stream->write(&data->deviceLUIDValid, 1 * 4); // VkBool32 deviceLUIDValid
}

VkPhysicalDeviceIDPropertiesKHX* vkUtilsUnpack_VkPhysicalDeviceIDPropertiesKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceIDPropertiesKHX* data = new VkPhysicalDeviceIDPropertiesKHX; memset(data, 0, sizeof(VkPhysicalDeviceIDPropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->deviceUUID, VK_UUID_SIZE * 1); // u8 deviceUUID
    stream->read(&data->driverUUID, VK_UUID_SIZE * 1); // u8 driverUUID
    stream->read(&data->deviceLUID, VK_LUID_SIZE_KHX * 1); // u8 deviceLUID
    stream->read(&data->deviceLUIDValid, 1 * 4); // VkBool32 deviceLUIDValid
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalMemoryImageCreateInfoKHX(const VkExternalMemoryImageCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsKHX handleTypes
    return res;
}

void vkUtilsPack_VkExternalMemoryImageCreateInfoKHX(android::base::InplaceStream* stream, const VkExternalMemoryImageCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX handleTypes
}

VkExternalMemoryImageCreateInfoKHX* vkUtilsUnpack_VkExternalMemoryImageCreateInfoKHX(android::base::InplaceStream* stream) {
    VkExternalMemoryImageCreateInfoKHX* data = new VkExternalMemoryImageCreateInfoKHX; memset(data, 0, sizeof(VkExternalMemoryImageCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX handleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalMemoryBufferCreateInfoKHX(const VkExternalMemoryBufferCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsKHX handleTypes
    return res;
}

void vkUtilsPack_VkExternalMemoryBufferCreateInfoKHX(android::base::InplaceStream* stream, const VkExternalMemoryBufferCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX handleTypes
}

VkExternalMemoryBufferCreateInfoKHX* vkUtilsUnpack_VkExternalMemoryBufferCreateInfoKHX(android::base::InplaceStream* stream) {
    VkExternalMemoryBufferCreateInfoKHX* data = new VkExternalMemoryBufferCreateInfoKHX; memset(data, 0, sizeof(VkExternalMemoryBufferCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX handleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkExportMemoryAllocateInfoKHX(const VkExportMemoryAllocateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagsKHX handleTypes
    return res;
}

void vkUtilsPack_VkExportMemoryAllocateInfoKHX(android::base::InplaceStream* stream, const VkExportMemoryAllocateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX handleTypes
}

VkExportMemoryAllocateInfoKHX* vkUtilsUnpack_VkExportMemoryAllocateInfoKHX(android::base::InplaceStream* stream) {
    VkExportMemoryAllocateInfoKHX* data = new VkExportMemoryAllocateInfoKHX; memset(data, 0, sizeof(VkExportMemoryAllocateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleTypes, 1 * 4); // VkExternalMemoryHandleTypeFlagsKHX handleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkImportMemoryFdInfoKHX(const VkImportMemoryFdInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    res += 1 * 4; // int fd
    return res;
}

void vkUtilsPack_VkImportMemoryFdInfoKHX(android::base::InplaceStream* stream, const VkImportMemoryFdInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleType, 1 * 4); // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    stream->write(&data->fd, 1 * 4); // int fd
}

VkImportMemoryFdInfoKHX* vkUtilsUnpack_VkImportMemoryFdInfoKHX(android::base::InplaceStream* stream) {
    VkImportMemoryFdInfoKHX* data = new VkImportMemoryFdInfoKHX; memset(data, 0, sizeof(VkImportMemoryFdInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleType, 1 * 4); // VkExternalMemoryHandleTypeFlagBitsKHX handleType
    stream->read(&data->fd, 1 * 4); // int fd
    return data;
}

uint32_t vkUtilsEncodingSize_VkMemoryFdPropertiesKHX(const VkMemoryFdPropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 memoryTypeBits
    return res;
}

void vkUtilsPack_VkMemoryFdPropertiesKHX(android::base::InplaceStream* stream, const VkMemoryFdPropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->memoryTypeBits, 1 * 4); // u32 memoryTypeBits
}

VkMemoryFdPropertiesKHX* vkUtilsUnpack_VkMemoryFdPropertiesKHX(android::base::InplaceStream* stream) {
    VkMemoryFdPropertiesKHX* data = new VkMemoryFdPropertiesKHX; memset(data, 0, sizeof(VkMemoryFdPropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->memoryTypeBits, 1 * 4); // u32 memoryTypeBits
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceExternalSemaphoreInfoKHX(const VkPhysicalDeviceExternalSemaphoreInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalSemaphoreHandleTypeFlagBitsKHX handleType
    return res;
}

void vkUtilsPack_VkPhysicalDeviceExternalSemaphoreInfoKHX(android::base::InplaceStream* stream, const VkPhysicalDeviceExternalSemaphoreInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleType, 1 * 4); // VkExternalSemaphoreHandleTypeFlagBitsKHX handleType
}

VkPhysicalDeviceExternalSemaphoreInfoKHX* vkUtilsUnpack_VkPhysicalDeviceExternalSemaphoreInfoKHX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceExternalSemaphoreInfoKHX* data = new VkPhysicalDeviceExternalSemaphoreInfoKHX; memset(data, 0, sizeof(VkPhysicalDeviceExternalSemaphoreInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleType, 1 * 4); // VkExternalSemaphoreHandleTypeFlagBitsKHX handleType
    return data;
}

uint32_t vkUtilsEncodingSize_VkExternalSemaphorePropertiesKHX(const VkExternalSemaphorePropertiesKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalSemaphoreHandleTypeFlagsKHX exportFromImportedHandleTypes
    res += 1 * 4; // VkExternalSemaphoreHandleTypeFlagsKHX compatibleHandleTypes
    res += 1 * 4; // VkExternalSemaphoreFeatureFlagsKHX externalSemaphoreFeatures
    return res;
}

void vkUtilsPack_VkExternalSemaphorePropertiesKHX(android::base::InplaceStream* stream, const VkExternalSemaphorePropertiesKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->exportFromImportedHandleTypes, 1 * 4); // VkExternalSemaphoreHandleTypeFlagsKHX exportFromImportedHandleTypes
    stream->write(&data->compatibleHandleTypes, 1 * 4); // VkExternalSemaphoreHandleTypeFlagsKHX compatibleHandleTypes
    stream->write(&data->externalSemaphoreFeatures, 1 * 4); // VkExternalSemaphoreFeatureFlagsKHX externalSemaphoreFeatures
}

VkExternalSemaphorePropertiesKHX* vkUtilsUnpack_VkExternalSemaphorePropertiesKHX(android::base::InplaceStream* stream) {
    VkExternalSemaphorePropertiesKHX* data = new VkExternalSemaphorePropertiesKHX; memset(data, 0, sizeof(VkExternalSemaphorePropertiesKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->exportFromImportedHandleTypes, 1 * 4); // VkExternalSemaphoreHandleTypeFlagsKHX exportFromImportedHandleTypes
    stream->read(&data->compatibleHandleTypes, 1 * 4); // VkExternalSemaphoreHandleTypeFlagsKHX compatibleHandleTypes
    stream->read(&data->externalSemaphoreFeatures, 1 * 4); // VkExternalSemaphoreFeatureFlagsKHX externalSemaphoreFeatures
    return data;
}

uint32_t vkUtilsEncodingSize_VkExportSemaphoreCreateInfoKHX(const VkExportSemaphoreCreateInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkExternalSemaphoreHandleTypeFlagsKHX handleTypes
    return res;
}

void vkUtilsPack_VkExportSemaphoreCreateInfoKHX(android::base::InplaceStream* stream, const VkExportSemaphoreCreateInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->handleTypes, 1 * 4); // VkExternalSemaphoreHandleTypeFlagsKHX handleTypes
}

VkExportSemaphoreCreateInfoKHX* vkUtilsUnpack_VkExportSemaphoreCreateInfoKHX(android::base::InplaceStream* stream) {
    VkExportSemaphoreCreateInfoKHX* data = new VkExportSemaphoreCreateInfoKHX; memset(data, 0, sizeof(VkExportSemaphoreCreateInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->handleTypes, 1 * 4); // VkExternalSemaphoreHandleTypeFlagsKHX handleTypes
    return data;
}

uint32_t vkUtilsEncodingSize_VkImportSemaphoreFdInfoKHX(const VkImportSemaphoreFdInfoKHX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 8; // VkSemaphore semaphore
    res += 1 * 4; // VkExternalSemaphoreHandleTypeFlagBitsKHX handleType
    res += 1 * 4; // s32 fd
    return res;
}

void vkUtilsPack_VkImportSemaphoreFdInfoKHX(android::base::InplaceStream* stream, const VkImportSemaphoreFdInfoKHX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->semaphore, 1 * 8); // VkSemaphore semaphore
    stream->write(&data->handleType, 1 * 4); // VkExternalSemaphoreHandleTypeFlagBitsKHX handleType
    stream->write(&data->fd, 1 * 4); // s32 fd
}

VkImportSemaphoreFdInfoKHX* vkUtilsUnpack_VkImportSemaphoreFdInfoKHX(android::base::InplaceStream* stream) {
    VkImportSemaphoreFdInfoKHX* data = new VkImportSemaphoreFdInfoKHX; memset(data, 0, sizeof(VkImportSemaphoreFdInfoKHX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->semaphore, 1 * 8); // VkSemaphore semaphore
    stream->read(&data->handleType, 1 * 4); // VkExternalSemaphoreHandleTypeFlagBitsKHX handleType
    stream->read(&data->fd, 1 * 4); // s32 fd
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDevicePushDescriptorPropertiesKHR(const VkPhysicalDevicePushDescriptorPropertiesKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 maxPushDescriptors
    return res;
}

void vkUtilsPack_VkPhysicalDevicePushDescriptorPropertiesKHR(android::base::InplaceStream* stream, const VkPhysicalDevicePushDescriptorPropertiesKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->maxPushDescriptors, 1 * 4); // u32 maxPushDescriptors
}

VkPhysicalDevicePushDescriptorPropertiesKHR* vkUtilsUnpack_VkPhysicalDevicePushDescriptorPropertiesKHR(android::base::InplaceStream* stream) {
    VkPhysicalDevicePushDescriptorPropertiesKHR* data = new VkPhysicalDevicePushDescriptorPropertiesKHR; memset(data, 0, sizeof(VkPhysicalDevicePushDescriptorPropertiesKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->maxPushDescriptors, 1 * 4); // u32 maxPushDescriptors
    return data;
}

uint32_t vkUtilsEncodingSize_VkRectLayerKHR(const VkRectLayerKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * vkUtilsEncodingSize_VkOffset2D(&data->offset); // VkOffset2D offset
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->extent); // VkExtent2D extent
    res += 1 * 4; // u32 layer
    return res;
}

void vkUtilsPack_VkRectLayerKHR(android::base::InplaceStream* stream, const VkRectLayerKHR* data) {
    if (!data) return;
    vkUtilsPack_VkOffset2D(stream, &data->offset); // VkOffset2D offset
    vkUtilsPack_VkExtent2D(stream, &data->extent); // VkExtent2D extent
    stream->write(&data->layer, 1 * 4); // u32 layer
}

VkRectLayerKHR* vkUtilsUnpack_VkRectLayerKHR(android::base::InplaceStream* stream) {
    VkRectLayerKHR* data = new VkRectLayerKHR; memset(data, 0, sizeof(VkRectLayerKHR)); 
    { VkOffset2D* tmpUnpacked = vkUtilsUnpack_VkOffset2D(stream); memcpy(&data->offset, tmpUnpacked, sizeof(VkOffset2D)); delete tmpUnpacked; } // VkOffset2D offset
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->extent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D extent
    stream->read(&data->layer, 1 * 4); // u32 layer
    return data;
}

uint32_t vkUtilsEncodingSize_VkPresentRegionKHR(const VkPresentRegionKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 rectangleCount
    res += 8; if (data->pRectangles) { res += data->rectangleCount * vkUtilsEncodingSize_VkRectLayerKHR(data->pRectangles); } // VkRectLayerKHR pRectangles
    return res;
}

void vkUtilsPack_VkPresentRegionKHR(android::base::InplaceStream* stream, const VkPresentRegionKHR* data) {
    if (!data) return;
    stream->write(&data->rectangleCount, 1 * 4); // u32 rectangleCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pRectangles); stream->write(&ptrval, 8);
    if (data->pRectangles) { for (uint32_t i = 0; i < data->rectangleCount; i++) { vkUtilsPack_VkRectLayerKHR(stream, data->pRectangles + i); } } } // VkRectLayerKHR pRectangles
}

VkPresentRegionKHR* vkUtilsUnpack_VkPresentRegionKHR(android::base::InplaceStream* stream) {
    VkPresentRegionKHR* data = new VkPresentRegionKHR; memset(data, 0, sizeof(VkPresentRegionKHR)); 
    stream->read(&data->rectangleCount, 1 * 4); // u32 rectangleCount
    { // VkRectLayerKHR pRectangles
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkRectLayerKHR* tmpArr = ptrval ? new VkRectLayerKHR[data->rectangleCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->rectangleCount; i++) {
        VkRectLayerKHR* tmpUnpacked = vkUtilsUnpack_VkRectLayerKHR(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkRectLayerKHR)); delete tmpUnpacked; } }
        data->pRectangles = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkPresentRegionsKHR(const VkPresentRegionsKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 swapchainCount
    res += 8; if (data->pRegions) { res += data->swapchainCount * vkUtilsEncodingSize_VkPresentRegionKHR(data->pRegions); } // VkPresentRegionKHR pRegions
    return res;
}

void vkUtilsPack_VkPresentRegionsKHR(android::base::InplaceStream* stream, const VkPresentRegionsKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pRegions); stream->write(&ptrval, 8);
    if (data->pRegions) { for (uint32_t i = 0; i < data->swapchainCount; i++) { vkUtilsPack_VkPresentRegionKHR(stream, data->pRegions + i); } } } // VkPresentRegionKHR pRegions
}

VkPresentRegionsKHR* vkUtilsUnpack_VkPresentRegionsKHR(android::base::InplaceStream* stream) {
    VkPresentRegionsKHR* data = new VkPresentRegionsKHR; memset(data, 0, sizeof(VkPresentRegionsKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    { // VkPresentRegionKHR pRegions
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPresentRegionKHR* tmpArr = ptrval ? new VkPresentRegionKHR[data->swapchainCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->swapchainCount; i++) {
        VkPresentRegionKHR* tmpUnpacked = vkUtilsUnpack_VkPresentRegionKHR(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPresentRegionKHR)); delete tmpUnpacked; } }
        data->pRegions = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorUpdateTemplateEntryKHR(const VkDescriptorUpdateTemplateEntryKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 dstBinding
    res += 1 * 4; // u32 dstArrayElement
    res += 1 * 4; // u32 descriptorCount
    res += 1 * 4; // VkDescriptorType descriptorType
    res += 1 * 4; // size_t offset
    res += 1 * 4; // size_t stride
    return res;
}

void vkUtilsPack_VkDescriptorUpdateTemplateEntryKHR(android::base::InplaceStream* stream, const VkDescriptorUpdateTemplateEntryKHR* data) {
    if (!data) return;
    stream->write(&data->dstBinding, 1 * 4); // u32 dstBinding
    stream->write(&data->dstArrayElement, 1 * 4); // u32 dstArrayElement
    stream->write(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    stream->write(&data->descriptorType, 1 * 4); // VkDescriptorType descriptorType
    stream->write(&data->offset, 1 * 4); // size_t offset
    stream->write(&data->stride, 1 * 4); // size_t stride
}

VkDescriptorUpdateTemplateEntryKHR* vkUtilsUnpack_VkDescriptorUpdateTemplateEntryKHR(android::base::InplaceStream* stream) {
    VkDescriptorUpdateTemplateEntryKHR* data = new VkDescriptorUpdateTemplateEntryKHR; memset(data, 0, sizeof(VkDescriptorUpdateTemplateEntryKHR)); 
    stream->read(&data->dstBinding, 1 * 4); // u32 dstBinding
    stream->read(&data->dstArrayElement, 1 * 4); // u32 dstArrayElement
    stream->read(&data->descriptorCount, 1 * 4); // u32 descriptorCount
    stream->read(&data->descriptorType, 1 * 4); // VkDescriptorType descriptorType
    stream->read(&data->offset, 1 * 4); // size_t offset
    stream->read(&data->stride, 1 * 4); // size_t stride
    return data;
}

uint32_t vkUtilsEncodingSize_VkDescriptorUpdateTemplateCreateInfoKHR(const VkDescriptorUpdateTemplateCreateInfoKHR* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDescriptorUpdateTemplateCreateFlagsKHR flags
    res += 1 * 4; // u32 descriptorUpdateEntryCount
    res += 8; if (data->pDescriptorUpdateEntries) { res += data->descriptorUpdateEntryCount * vkUtilsEncodingSize_VkDescriptorUpdateTemplateEntryKHR(data->pDescriptorUpdateEntries); } // VkDescriptorUpdateTemplateEntryKHR pDescriptorUpdateEntries
    res += 1 * 4; // VkDescriptorUpdateTemplateTypeKHR templateType
    res += 1 * 8; // VkDescriptorSetLayout descriptorSetLayout
    res += 1 * 4; // VkPipelineBindPoint pipelineBindPoint
    res += 1 * 8; // VkPipelineLayout pipelineLayout
    res += 1 * 4; // u32 set
    return res;
}

void vkUtilsPack_VkDescriptorUpdateTemplateCreateInfoKHR(android::base::InplaceStream* stream, const VkDescriptorUpdateTemplateCreateInfoKHR* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkDescriptorUpdateTemplateCreateFlagsKHR flags
    stream->write(&data->descriptorUpdateEntryCount, 1 * 4); // u32 descriptorUpdateEntryCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDescriptorUpdateEntries); stream->write(&ptrval, 8);
    if (data->pDescriptorUpdateEntries) { for (uint32_t i = 0; i < data->descriptorUpdateEntryCount; i++) { vkUtilsPack_VkDescriptorUpdateTemplateEntryKHR(stream, data->pDescriptorUpdateEntries + i); } } } // VkDescriptorUpdateTemplateEntryKHR pDescriptorUpdateEntries
    stream->write(&data->templateType, 1 * 4); // VkDescriptorUpdateTemplateTypeKHR templateType
    stream->write(&data->descriptorSetLayout, 1 * 8); // VkDescriptorSetLayout descriptorSetLayout
    stream->write(&data->pipelineBindPoint, 1 * 4); // VkPipelineBindPoint pipelineBindPoint
    stream->write(&data->pipelineLayout, 1 * 8); // VkPipelineLayout pipelineLayout
    stream->write(&data->set, 1 * 4); // u32 set
}

VkDescriptorUpdateTemplateCreateInfoKHR* vkUtilsUnpack_VkDescriptorUpdateTemplateCreateInfoKHR(android::base::InplaceStream* stream) {
    VkDescriptorUpdateTemplateCreateInfoKHR* data = new VkDescriptorUpdateTemplateCreateInfoKHR; memset(data, 0, sizeof(VkDescriptorUpdateTemplateCreateInfoKHR)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkDescriptorUpdateTemplateCreateFlagsKHR flags
    stream->read(&data->descriptorUpdateEntryCount, 1 * 4); // u32 descriptorUpdateEntryCount
    { // VkDescriptorUpdateTemplateEntryKHR pDescriptorUpdateEntries
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkDescriptorUpdateTemplateEntryKHR* tmpArr = ptrval ? new VkDescriptorUpdateTemplateEntryKHR[data->descriptorUpdateEntryCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->descriptorUpdateEntryCount; i++) {
        VkDescriptorUpdateTemplateEntryKHR* tmpUnpacked = vkUtilsUnpack_VkDescriptorUpdateTemplateEntryKHR(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkDescriptorUpdateTemplateEntryKHR)); delete tmpUnpacked; } }
        data->pDescriptorUpdateEntries = tmpArr;
    }
    stream->read(&data->templateType, 1 * 4); // VkDescriptorUpdateTemplateTypeKHR templateType
    stream->read(&data->descriptorSetLayout, 1 * 8); // VkDescriptorSetLayout descriptorSetLayout
    stream->read(&data->pipelineBindPoint, 1 * 4); // VkPipelineBindPoint pipelineBindPoint
    stream->read(&data->pipelineLayout, 1 * 8); // VkPipelineLayout pipelineLayout
    stream->read(&data->set, 1 * 4); // u32 set
    return data;
}

uint32_t vkUtilsEncodingSize_VkSurfaceCapabilities2EXT(const VkSurfaceCapabilities2EXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 minImageCount
    res += 1 * 4; // u32 maxImageCount
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->currentExtent); // VkExtent2D currentExtent
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->minImageExtent); // VkExtent2D minImageExtent
    res += 1 * vkUtilsEncodingSize_VkExtent2D(&data->maxImageExtent); // VkExtent2D maxImageExtent
    res += 1 * 4; // u32 maxImageArrayLayers
    res += 1 * 4; // VkSurfaceTransformFlagsKHR supportedTransforms
    res += 1 * 4; // VkSurfaceTransformFlagBitsKHR currentTransform
    res += 1 * 4; // VkCompositeAlphaFlagsKHR supportedCompositeAlpha
    res += 1 * 4; // VkImageUsageFlags supportedUsageFlags
    res += 1 * 4; // VkSurfaceCounterFlagsEXT supportedSurfaceCounters
    return res;
}

void vkUtilsPack_VkSurfaceCapabilities2EXT(android::base::InplaceStream* stream, const VkSurfaceCapabilities2EXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->minImageCount, 1 * 4); // u32 minImageCount
    stream->write(&data->maxImageCount, 1 * 4); // u32 maxImageCount
    vkUtilsPack_VkExtent2D(stream, &data->currentExtent); // VkExtent2D currentExtent
    vkUtilsPack_VkExtent2D(stream, &data->minImageExtent); // VkExtent2D minImageExtent
    vkUtilsPack_VkExtent2D(stream, &data->maxImageExtent); // VkExtent2D maxImageExtent
    stream->write(&data->maxImageArrayLayers, 1 * 4); // u32 maxImageArrayLayers
    stream->write(&data->supportedTransforms, 1 * 4); // VkSurfaceTransformFlagsKHR supportedTransforms
    stream->write(&data->currentTransform, 1 * 4); // VkSurfaceTransformFlagBitsKHR currentTransform
    stream->write(&data->supportedCompositeAlpha, 1 * 4); // VkCompositeAlphaFlagsKHR supportedCompositeAlpha
    stream->write(&data->supportedUsageFlags, 1 * 4); // VkImageUsageFlags supportedUsageFlags
    stream->write(&data->supportedSurfaceCounters, 1 * 4); // VkSurfaceCounterFlagsEXT supportedSurfaceCounters
}

VkSurfaceCapabilities2EXT* vkUtilsUnpack_VkSurfaceCapabilities2EXT(android::base::InplaceStream* stream) {
    VkSurfaceCapabilities2EXT* data = new VkSurfaceCapabilities2EXT; memset(data, 0, sizeof(VkSurfaceCapabilities2EXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->minImageCount, 1 * 4); // u32 minImageCount
    stream->read(&data->maxImageCount, 1 * 4); // u32 maxImageCount
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->currentExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D currentExtent
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->minImageExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D minImageExtent
    { VkExtent2D* tmpUnpacked = vkUtilsUnpack_VkExtent2D(stream); memcpy(&data->maxImageExtent, tmpUnpacked, sizeof(VkExtent2D)); delete tmpUnpacked; } // VkExtent2D maxImageExtent
    stream->read(&data->maxImageArrayLayers, 1 * 4); // u32 maxImageArrayLayers
    stream->read(&data->supportedTransforms, 1 * 4); // VkSurfaceTransformFlagsKHR supportedTransforms
    stream->read(&data->currentTransform, 1 * 4); // VkSurfaceTransformFlagBitsKHR currentTransform
    stream->read(&data->supportedCompositeAlpha, 1 * 4); // VkCompositeAlphaFlagsKHR supportedCompositeAlpha
    stream->read(&data->supportedUsageFlags, 1 * 4); // VkImageUsageFlags supportedUsageFlags
    stream->read(&data->supportedSurfaceCounters, 1 * 4); // VkSurfaceCounterFlagsEXT supportedSurfaceCounters
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayPowerInfoEXT(const VkDisplayPowerInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDisplayPowerStateEXT powerState
    return res;
}

void vkUtilsPack_VkDisplayPowerInfoEXT(android::base::InplaceStream* stream, const VkDisplayPowerInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->powerState, 1 * 4); // VkDisplayPowerStateEXT powerState
}

VkDisplayPowerInfoEXT* vkUtilsUnpack_VkDisplayPowerInfoEXT(android::base::InplaceStream* stream) {
    VkDisplayPowerInfoEXT* data = new VkDisplayPowerInfoEXT; memset(data, 0, sizeof(VkDisplayPowerInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->powerState, 1 * 4); // VkDisplayPowerStateEXT powerState
    return data;
}

uint32_t vkUtilsEncodingSize_VkDeviceEventInfoEXT(const VkDeviceEventInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDeviceEventTypeEXT deviceEvent
    return res;
}

void vkUtilsPack_VkDeviceEventInfoEXT(android::base::InplaceStream* stream, const VkDeviceEventInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->deviceEvent, 1 * 4); // VkDeviceEventTypeEXT deviceEvent
}

VkDeviceEventInfoEXT* vkUtilsUnpack_VkDeviceEventInfoEXT(android::base::InplaceStream* stream) {
    VkDeviceEventInfoEXT* data = new VkDeviceEventInfoEXT; memset(data, 0, sizeof(VkDeviceEventInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->deviceEvent, 1 * 4); // VkDeviceEventTypeEXT deviceEvent
    return data;
}

uint32_t vkUtilsEncodingSize_VkDisplayEventInfoEXT(const VkDisplayEventInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkDisplayEventTypeEXT displayEvent
    return res;
}

void vkUtilsPack_VkDisplayEventInfoEXT(android::base::InplaceStream* stream, const VkDisplayEventInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->displayEvent, 1 * 4); // VkDisplayEventTypeEXT displayEvent
}

VkDisplayEventInfoEXT* vkUtilsUnpack_VkDisplayEventInfoEXT(android::base::InplaceStream* stream) {
    VkDisplayEventInfoEXT* data = new VkDisplayEventInfoEXT; memset(data, 0, sizeof(VkDisplayEventInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->displayEvent, 1 * 4); // VkDisplayEventTypeEXT displayEvent
    return data;
}

uint32_t vkUtilsEncodingSize_VkSwapchainCounterCreateInfoEXT(const VkSwapchainCounterCreateInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkSurfaceCounterFlagsEXT surfaceCounters
    return res;
}

void vkUtilsPack_VkSwapchainCounterCreateInfoEXT(android::base::InplaceStream* stream, const VkSwapchainCounterCreateInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->surfaceCounters, 1 * 4); // VkSurfaceCounterFlagsEXT surfaceCounters
}

VkSwapchainCounterCreateInfoEXT* vkUtilsUnpack_VkSwapchainCounterCreateInfoEXT(android::base::InplaceStream* stream) {
    VkSwapchainCounterCreateInfoEXT* data = new VkSwapchainCounterCreateInfoEXT; memset(data, 0, sizeof(VkSwapchainCounterCreateInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->surfaceCounters, 1 * 4); // VkSurfaceCounterFlagsEXT surfaceCounters
    return data;
}

uint32_t vkUtilsEncodingSize_VkRefreshCycleDurationGOOGLE(const VkRefreshCycleDurationGOOGLE* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 8; // u64 refreshDuration
    return res;
}

void vkUtilsPack_VkRefreshCycleDurationGOOGLE(android::base::InplaceStream* stream, const VkRefreshCycleDurationGOOGLE* data) {
    if (!data) return;
    stream->write(&data->refreshDuration, 1 * 8); // u64 refreshDuration
}

VkRefreshCycleDurationGOOGLE* vkUtilsUnpack_VkRefreshCycleDurationGOOGLE(android::base::InplaceStream* stream) {
    VkRefreshCycleDurationGOOGLE* data = new VkRefreshCycleDurationGOOGLE; memset(data, 0, sizeof(VkRefreshCycleDurationGOOGLE)); 
    stream->read(&data->refreshDuration, 1 * 8); // u64 refreshDuration
    return data;
}

uint32_t vkUtilsEncodingSize_VkPastPresentationTimingGOOGLE(const VkPastPresentationTimingGOOGLE* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 presentID
    res += 1 * 8; // u64 desiredPresentTime
    res += 1 * 8; // u64 actualPresentTime
    res += 1 * 8; // u64 earliestPresentTime
    res += 1 * 8; // u64 presentMargin
    return res;
}

void vkUtilsPack_VkPastPresentationTimingGOOGLE(android::base::InplaceStream* stream, const VkPastPresentationTimingGOOGLE* data) {
    if (!data) return;
    stream->write(&data->presentID, 1 * 4); // u32 presentID
    stream->write(&data->desiredPresentTime, 1 * 8); // u64 desiredPresentTime
    stream->write(&data->actualPresentTime, 1 * 8); // u64 actualPresentTime
    stream->write(&data->earliestPresentTime, 1 * 8); // u64 earliestPresentTime
    stream->write(&data->presentMargin, 1 * 8); // u64 presentMargin
}

VkPastPresentationTimingGOOGLE* vkUtilsUnpack_VkPastPresentationTimingGOOGLE(android::base::InplaceStream* stream) {
    VkPastPresentationTimingGOOGLE* data = new VkPastPresentationTimingGOOGLE; memset(data, 0, sizeof(VkPastPresentationTimingGOOGLE)); 
    stream->read(&data->presentID, 1 * 4); // u32 presentID
    stream->read(&data->desiredPresentTime, 1 * 8); // u64 desiredPresentTime
    stream->read(&data->actualPresentTime, 1 * 8); // u64 actualPresentTime
    stream->read(&data->earliestPresentTime, 1 * 8); // u64 earliestPresentTime
    stream->read(&data->presentMargin, 1 * 8); // u64 presentMargin
    return data;
}

uint32_t vkUtilsEncodingSize_VkPresentTimeGOOGLE(const VkPresentTimeGOOGLE* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // u32 presentID
    res += 1 * 8; // u64 desiredPresentTime
    return res;
}

void vkUtilsPack_VkPresentTimeGOOGLE(android::base::InplaceStream* stream, const VkPresentTimeGOOGLE* data) {
    if (!data) return;
    stream->write(&data->presentID, 1 * 4); // u32 presentID
    stream->write(&data->desiredPresentTime, 1 * 8); // u64 desiredPresentTime
}

VkPresentTimeGOOGLE* vkUtilsUnpack_VkPresentTimeGOOGLE(android::base::InplaceStream* stream) {
    VkPresentTimeGOOGLE* data = new VkPresentTimeGOOGLE; memset(data, 0, sizeof(VkPresentTimeGOOGLE)); 
    stream->read(&data->presentID, 1 * 4); // u32 presentID
    stream->read(&data->desiredPresentTime, 1 * 8); // u64 desiredPresentTime
    return data;
}

uint32_t vkUtilsEncodingSize_VkPresentTimesInfoGOOGLE(const VkPresentTimesInfoGOOGLE* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 swapchainCount
    res += 8; if (data->pTimes) { res += data->swapchainCount * vkUtilsEncodingSize_VkPresentTimeGOOGLE(data->pTimes); } // VkPresentTimeGOOGLE pTimes
    return res;
}

void vkUtilsPack_VkPresentTimesInfoGOOGLE(android::base::InplaceStream* stream, const VkPresentTimesInfoGOOGLE* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pTimes); stream->write(&ptrval, 8);
    if (data->pTimes) { for (uint32_t i = 0; i < data->swapchainCount; i++) { vkUtilsPack_VkPresentTimeGOOGLE(stream, data->pTimes + i); } } } // VkPresentTimeGOOGLE pTimes
}

VkPresentTimesInfoGOOGLE* vkUtilsUnpack_VkPresentTimesInfoGOOGLE(android::base::InplaceStream* stream) {
    VkPresentTimesInfoGOOGLE* data = new VkPresentTimesInfoGOOGLE; memset(data, 0, sizeof(VkPresentTimesInfoGOOGLE)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->swapchainCount, 1 * 4); // u32 swapchainCount
    { // VkPresentTimeGOOGLE pTimes
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkPresentTimeGOOGLE* tmpArr = ptrval ? new VkPresentTimeGOOGLE[data->swapchainCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->swapchainCount; i++) {
        VkPresentTimeGOOGLE* tmpUnpacked = vkUtilsUnpack_VkPresentTimeGOOGLE(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkPresentTimeGOOGLE)); delete tmpUnpacked; } }
        data->pTimes = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX(const VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkBool32 perViewPositionAllComponents
    return res;
}

void vkUtilsPack_VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX(android::base::InplaceStream* stream, const VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->perViewPositionAllComponents, 1 * 4); // VkBool32 perViewPositionAllComponents
}

VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* vkUtilsUnpack_VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX(android::base::InplaceStream* stream) {
    VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX* data = new VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX; memset(data, 0, sizeof(VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->perViewPositionAllComponents, 1 * 4); // VkBool32 perViewPositionAllComponents
    return data;
}

uint32_t vkUtilsEncodingSize_VkPhysicalDeviceDiscardRectanglePropertiesEXT(const VkPhysicalDeviceDiscardRectanglePropertiesEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // u32 maxDiscardRectangles
    return res;
}

void vkUtilsPack_VkPhysicalDeviceDiscardRectanglePropertiesEXT(android::base::InplaceStream* stream, const VkPhysicalDeviceDiscardRectanglePropertiesEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->maxDiscardRectangles, 1 * 4); // u32 maxDiscardRectangles
}

VkPhysicalDeviceDiscardRectanglePropertiesEXT* vkUtilsUnpack_VkPhysicalDeviceDiscardRectanglePropertiesEXT(android::base::InplaceStream* stream) {
    VkPhysicalDeviceDiscardRectanglePropertiesEXT* data = new VkPhysicalDeviceDiscardRectanglePropertiesEXT; memset(data, 0, sizeof(VkPhysicalDeviceDiscardRectanglePropertiesEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->maxDiscardRectangles, 1 * 4); // u32 maxDiscardRectangles
    return data;
}

uint32_t vkUtilsEncodingSize_VkPipelineDiscardRectangleStateCreateInfoEXT(const VkPipelineDiscardRectangleStateCreateInfoEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * 4; // VkPipelineDiscardRectangleStateCreateFlagsEXT flags
    res += 1 * 4; // VkDiscardRectangleModeEXT discardRectangleMode
    res += 1 * 4; // u32 discardRectangleCount
    res += 8; if (data->pDiscardRectangles) { res += data->discardRectangleCount * vkUtilsEncodingSize_VkRect2D(data->pDiscardRectangles); } // VkRect2D pDiscardRectangles
    return res;
}

void vkUtilsPack_VkPipelineDiscardRectangleStateCreateInfoEXT(android::base::InplaceStream* stream, const VkPipelineDiscardRectangleStateCreateInfoEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    stream->write(&data->flags, 1 * 4); // VkPipelineDiscardRectangleStateCreateFlagsEXT flags
    stream->write(&data->discardRectangleMode, 1 * 4); // VkDiscardRectangleModeEXT discardRectangleMode
    stream->write(&data->discardRectangleCount, 1 * 4); // u32 discardRectangleCount
    {
    uint64_t ptrval = (uint64_t)(uintptr_t)(data->pDiscardRectangles); stream->write(&ptrval, 8);
    if (data->pDiscardRectangles) { for (uint32_t i = 0; i < data->discardRectangleCount; i++) { vkUtilsPack_VkRect2D(stream, data->pDiscardRectangles + i); } } } // VkRect2D pDiscardRectangles
}

VkPipelineDiscardRectangleStateCreateInfoEXT* vkUtilsUnpack_VkPipelineDiscardRectangleStateCreateInfoEXT(android::base::InplaceStream* stream) {
    VkPipelineDiscardRectangleStateCreateInfoEXT* data = new VkPipelineDiscardRectangleStateCreateInfoEXT; memset(data, 0, sizeof(VkPipelineDiscardRectangleStateCreateInfoEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    stream->read(&data->flags, 1 * 4); // VkPipelineDiscardRectangleStateCreateFlagsEXT flags
    stream->read(&data->discardRectangleMode, 1 * 4); // VkDiscardRectangleModeEXT discardRectangleMode
    stream->read(&data->discardRectangleCount, 1 * 4); // u32 discardRectangleCount
    { // VkRect2D pDiscardRectangles
        uint64_t ptrval = 0; stream->read(&ptrval, 8); VkRect2D* tmpArr = ptrval ? new VkRect2D[data->discardRectangleCount] : nullptr;
        if (ptrval) { for (uint32_t i = 0; i < data->discardRectangleCount; i++) {
        VkRect2D* tmpUnpacked = vkUtilsUnpack_VkRect2D(stream); memcpy(&tmpArr[i], tmpUnpacked, sizeof(VkRect2D)); delete tmpUnpacked; } }
        data->pDiscardRectangles = tmpArr;
    }
    return data;
}

uint32_t vkUtilsEncodingSize_VkXYColorEXT(const VkXYColorEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // f32 x
    res += 1 * 4; // f32 y
    return res;
}

void vkUtilsPack_VkXYColorEXT(android::base::InplaceStream* stream, const VkXYColorEXT* data) {
    if (!data) return;
    stream->write(&data->x, 1 * 4); // f32 x
    stream->write(&data->y, 1 * 4); // f32 y
}

VkXYColorEXT* vkUtilsUnpack_VkXYColorEXT(android::base::InplaceStream* stream) {
    VkXYColorEXT* data = new VkXYColorEXT; memset(data, 0, sizeof(VkXYColorEXT)); 
    stream->read(&data->x, 1 * 4); // f32 x
    stream->read(&data->y, 1 * 4); // f32 y
    return data;
}

uint32_t vkUtilsEncodingSize_VkHdrMetadataEXT(const VkHdrMetadataEXT* data) {
    uint32_t res = 0;
    if (!data) return res;
    res += 1 * 4; // VkStructureType sType
    res += 1 * 8; // void pNext
    res += 1 * vkUtilsEncodingSize_VkXYColorEXT(&data->displayPrimaryRed); // VkXYColorEXT displayPrimaryRed
    res += 1 * vkUtilsEncodingSize_VkXYColorEXT(&data->displayPrimaryGreen); // VkXYColorEXT displayPrimaryGreen
    res += 1 * vkUtilsEncodingSize_VkXYColorEXT(&data->displayPrimaryBlue); // VkXYColorEXT displayPrimaryBlue
    res += 1 * vkUtilsEncodingSize_VkXYColorEXT(&data->whitePoint); // VkXYColorEXT whitePoint
    res += 1 * 4; // f32 maxLuminance
    res += 1 * 4; // f32 minLuminance
    res += 1 * 4; // f32 maxContentLightLevel
    res += 1 * 4; // f32 maxFrameAverageLightLevel
    return res;
}

void vkUtilsPack_VkHdrMetadataEXT(android::base::InplaceStream* stream, const VkHdrMetadataEXT* data) {
    if (!data) return;
    stream->write(&data->sType, 1 * 4); // VkStructureType sType
    uint64_t forHost_pNext = 0; // TODO: take into acct pNext's
    stream->write(&forHost_pNext, 8); // void pNext
    vkUtilsPack_VkXYColorEXT(stream, &data->displayPrimaryRed); // VkXYColorEXT displayPrimaryRed
    vkUtilsPack_VkXYColorEXT(stream, &data->displayPrimaryGreen); // VkXYColorEXT displayPrimaryGreen
    vkUtilsPack_VkXYColorEXT(stream, &data->displayPrimaryBlue); // VkXYColorEXT displayPrimaryBlue
    vkUtilsPack_VkXYColorEXT(stream, &data->whitePoint); // VkXYColorEXT whitePoint
    stream->write(&data->maxLuminance, 1 * 4); // f32 maxLuminance
    stream->write(&data->minLuminance, 1 * 4); // f32 minLuminance
    stream->write(&data->maxContentLightLevel, 1 * 4); // f32 maxContentLightLevel
    stream->write(&data->maxFrameAverageLightLevel, 1 * 4); // f32 maxFrameAverageLightLevel
}

VkHdrMetadataEXT* vkUtilsUnpack_VkHdrMetadataEXT(android::base::InplaceStream* stream) {
    VkHdrMetadataEXT* data = new VkHdrMetadataEXT; memset(data, 0, sizeof(VkHdrMetadataEXT)); 
    stream->read(&data->sType, 1 * 4); // VkStructureType sType
    stream->read(&data->pNext, 8); // void pNext
    { VkXYColorEXT* tmpUnpacked = vkUtilsUnpack_VkXYColorEXT(stream); memcpy(&data->displayPrimaryRed, tmpUnpacked, sizeof(VkXYColorEXT)); delete tmpUnpacked; } // VkXYColorEXT displayPrimaryRed
    { VkXYColorEXT* tmpUnpacked = vkUtilsUnpack_VkXYColorEXT(stream); memcpy(&data->displayPrimaryGreen, tmpUnpacked, sizeof(VkXYColorEXT)); delete tmpUnpacked; } // VkXYColorEXT displayPrimaryGreen
    { VkXYColorEXT* tmpUnpacked = vkUtilsUnpack_VkXYColorEXT(stream); memcpy(&data->displayPrimaryBlue, tmpUnpacked, sizeof(VkXYColorEXT)); delete tmpUnpacked; } // VkXYColorEXT displayPrimaryBlue
    { VkXYColorEXT* tmpUnpacked = vkUtilsUnpack_VkXYColorEXT(stream); memcpy(&data->whitePoint, tmpUnpacked, sizeof(VkXYColorEXT)); delete tmpUnpacked; } // VkXYColorEXT whitePoint
    stream->read(&data->maxLuminance, 1 * 4); // f32 maxLuminance
    stream->read(&data->minLuminance, 1 * 4); // f32 minLuminance
    stream->read(&data->maxContentLightLevel, 1 * 4); // f32 maxContentLightLevel
    stream->read(&data->maxFrameAverageLightLevel, 1 * 4); // f32 maxFrameAverageLightLevel
    return data;
}

