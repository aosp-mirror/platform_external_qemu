// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "AndroidVulkanDispatch.h"

#include "AndroidHostCommon.h"
#include "Ashmem.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

extern "C" {

EXPORT android_vulkan_dispatch* load_android_vulkan_dispatch(const char* path) {
#ifdef _WIN32
    const char* libPath = path ? path : "vulkan_android.dll";
#elif defined(__APPLE__)
    const char* libPath = path ? path : "libvulkan_android.dylib";
#else
    const char* libPath = path ? path : "libvulkan_android.so";
#endif

    android_vulkan_dispatch* out = new android_vulkan_dispatch;
    memset(out, 0x0, sizeof(android_vulkan_dispatch));

    void* lib = dlopen(libPath, RTLD_NOW);
    if (!lib) {
        fprintf(stderr, "%s: libvulkan_android not found!\n", __func__);
        return out;
    } else {
        fprintf(stderr, "%s: libvulkan_android found\n", __func__);
    }

#ifdef VK_VERSION_1_0
    out->vkDestroyInstance = (PFN_vkDestroyInstance)dlsym(lib, "vkDestroyInstance");
    out->vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)dlsym(lib, "vkEnumeratePhysicalDevices");
    out->vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)dlsym(lib, "vkGetPhysicalDeviceFeatures");
    out->vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)dlsym(lib, "vkGetPhysicalDeviceFormatProperties");
    out->vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)dlsym(lib, "vkGetPhysicalDeviceImageFormatProperties");
    out->vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)dlsym(lib, "vkGetPhysicalDeviceProperties");
    out->vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)dlsym(lib, "vkGetPhysicalDeviceQueueFamilyProperties");
    out->vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)dlsym(lib, "vkGetPhysicalDeviceMemoryProperties");
    out->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
    out->vkCreateDevice = (PFN_vkCreateDevice)dlsym(lib, "vkCreateDevice");
    out->vkDestroyDevice = (PFN_vkDestroyDevice)dlsym(lib, "vkDestroyDevice");
    out->vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)dlsym(lib, "vkEnumerateDeviceExtensionProperties");
    out->vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)dlsym(lib, "vkEnumerateDeviceLayerProperties");
    out->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)dlsym(lib, "vkGetDeviceQueue");
    out->vkQueueSubmit = (PFN_vkQueueSubmit)dlsym(lib, "vkQueueSubmit");
    out->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)dlsym(lib, "vkQueueWaitIdle");
    out->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)dlsym(lib, "vkDeviceWaitIdle");
    out->vkCreateInstance = (PFN_vkCreateInstance)dlsym(lib, "vkCreateInstance");
    out->vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)dlsym(lib, "vkEnumerateInstanceExtensionProperties");
    out->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)dlsym(lib, "vkEnumerateInstanceLayerProperties");
    out->vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)dlsym(lib, "vkGetDeviceProcAddr");
#endif
#ifdef VK_VERSION_1_1
    out->vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups)dlsym(lib, "vkEnumeratePhysicalDeviceGroups");
#endif
#ifdef VK_KHR_surface
    out->vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)dlsym(lib, "vkDestroySurfaceKHR");
#endif
#ifdef VK_KHR_swapchain
    out->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)dlsym(lib, "vkCreateSwapchainKHR");
    out->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)dlsym(lib, "vkDestroySwapchainKHR");
    out->vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)dlsym(lib, "vkGetSwapchainImagesKHR");
    out->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)dlsym(lib, "vkAcquireNextImageKHR");
    out->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)dlsym(lib, "vkQueuePresentKHR");
#endif
#ifdef VK_KHR_display
    out->vkCreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR)dlsym(lib, "vkCreateDisplayPlaneSurfaceKHR");
#endif
#ifdef VK_KHR_xlib_surface
    out->vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)dlsym(lib, "vkCreateXlibSurfaceKHR");
    out->vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)dlsym(lib, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif
#ifdef VK_KHR_xcb_surface
    out->vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)dlsym(lib, "vkCreateXcbSurfaceKHR");
    out->vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)dlsym(lib, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif
#ifdef VK_KHR_wayland_surface
    out->vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)dlsym(lib, "vkCreateWaylandSurfaceKHR");
#endif
#ifdef VK_KHR_mir_surface
    out->vkCreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)dlsym(lib, "vkCreateMirSurfaceKHR");
#endif
#ifdef VK_KHR_android_surface
    out->vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)dlsym(lib, "vkCreateAndroidSurfaceKHR");
#endif
#ifdef VK_KHR_win32_surface
    out->vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)dlsym(lib, "vkCreateWin32SurfaceKHR");
    out->vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)dlsym(lib, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif
#ifdef VK_KHR_device_group_creation
    out->vkEnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR)dlsym(lib, "vkEnumeratePhysicalDeviceGroupsKHR");
#endif
#ifdef VK_EXT_debug_report
    out->vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)dlsym(lib, "vkCreateDebugReportCallbackEXT");
    out->vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)dlsym(lib, "vkDestroyDebugReportCallbackEXT");
    out->vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)dlsym(lib, "vkDebugReportMessageEXT");
#endif
#ifdef VK_NN_vi_surface
    out->vkCreateViSurfaceNN = (PFN_vkCreateViSurfaceNN)dlsym(lib, "vkCreateViSurfaceNN");
#endif
#ifdef VK_MVK_ios_surface
    out->vkCreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK)dlsym(lib, "vkCreateIOSSurfaceMVK");
#endif
#ifdef VK_MVK_macos_surface
    out->vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)dlsym(lib, "vkCreateMacOSSurfaceMVK");
#endif
#ifdef VK_EXT_debug_utils
    out->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)dlsym(lib, "vkCreateDebugUtilsMessengerEXT");
    out->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)dlsym(lib, "vkDestroyDebugUtilsMessengerEXT");
    out->vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)dlsym(lib, "vkSubmitDebugUtilsMessageEXT");
#endif
#ifdef VK_VERSION_1_0
    out->vkAllocateMemory = (PFN_vkAllocateMemory)dlsym(lib, "vkAllocateMemory");
    out->vkFreeMemory = (PFN_vkFreeMemory)dlsym(lib, "vkFreeMemory");
    out->vkMapMemory = (PFN_vkMapMemory)dlsym(lib, "vkMapMemory");
    out->vkUnmapMemory = (PFN_vkUnmapMemory)dlsym(lib, "vkUnmapMemory");
    out->vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)dlsym(lib, "vkFlushMappedMemoryRanges");
    out->vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)dlsym(lib, "vkInvalidateMappedMemoryRanges");
    out->vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)dlsym(lib, "vkGetDeviceMemoryCommitment");
    out->vkBindBufferMemory = (PFN_vkBindBufferMemory)dlsym(lib, "vkBindBufferMemory");
    out->vkBindImageMemory = (PFN_vkBindImageMemory)dlsym(lib, "vkBindImageMemory");
    out->vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)dlsym(lib, "vkGetBufferMemoryRequirements");
    out->vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)dlsym(lib, "vkGetImageMemoryRequirements");
    out->vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)dlsym(lib, "vkGetImageSparseMemoryRequirements");
    out->vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)dlsym(lib, "vkGetPhysicalDeviceSparseImageFormatProperties");
    out->vkQueueBindSparse = (PFN_vkQueueBindSparse)dlsym(lib, "vkQueueBindSparse");
    out->vkCreateFence = (PFN_vkCreateFence)dlsym(lib, "vkCreateFence");
    out->vkDestroyFence = (PFN_vkDestroyFence)dlsym(lib, "vkDestroyFence");
    out->vkResetFences = (PFN_vkResetFences)dlsym(lib, "vkResetFences");
    out->vkGetFenceStatus = (PFN_vkGetFenceStatus)dlsym(lib, "vkGetFenceStatus");
    out->vkWaitForFences = (PFN_vkWaitForFences)dlsym(lib, "vkWaitForFences");
    out->vkCreateSemaphore = (PFN_vkCreateSemaphore)dlsym(lib, "vkCreateSemaphore");
    out->vkDestroySemaphore = (PFN_vkDestroySemaphore)dlsym(lib, "vkDestroySemaphore");
    out->vkCreateEvent = (PFN_vkCreateEvent)dlsym(lib, "vkCreateEvent");
    out->vkDestroyEvent = (PFN_vkDestroyEvent)dlsym(lib, "vkDestroyEvent");
    out->vkGetEventStatus = (PFN_vkGetEventStatus)dlsym(lib, "vkGetEventStatus");
    out->vkSetEvent = (PFN_vkSetEvent)dlsym(lib, "vkSetEvent");
    out->vkResetEvent = (PFN_vkResetEvent)dlsym(lib, "vkResetEvent");
    out->vkCreateQueryPool = (PFN_vkCreateQueryPool)dlsym(lib, "vkCreateQueryPool");
    out->vkDestroyQueryPool = (PFN_vkDestroyQueryPool)dlsym(lib, "vkDestroyQueryPool");
    out->vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)dlsym(lib, "vkGetQueryPoolResults");
    out->vkCreateBuffer = (PFN_vkCreateBuffer)dlsym(lib, "vkCreateBuffer");
    out->vkDestroyBuffer = (PFN_vkDestroyBuffer)dlsym(lib, "vkDestroyBuffer");
    out->vkCreateBufferView = (PFN_vkCreateBufferView)dlsym(lib, "vkCreateBufferView");
    out->vkDestroyBufferView = (PFN_vkDestroyBufferView)dlsym(lib, "vkDestroyBufferView");
    out->vkCreateImage = (PFN_vkCreateImage)dlsym(lib, "vkCreateImage");
    out->vkDestroyImage = (PFN_vkDestroyImage)dlsym(lib, "vkDestroyImage");
    out->vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)dlsym(lib, "vkGetImageSubresourceLayout");
    out->vkCreateImageView = (PFN_vkCreateImageView)dlsym(lib, "vkCreateImageView");
    out->vkDestroyImageView = (PFN_vkDestroyImageView)dlsym(lib, "vkDestroyImageView");
    out->vkCreateShaderModule = (PFN_vkCreateShaderModule)dlsym(lib, "vkCreateShaderModule");
    out->vkDestroyShaderModule = (PFN_vkDestroyShaderModule)dlsym(lib, "vkDestroyShaderModule");
    out->vkCreatePipelineCache = (PFN_vkCreatePipelineCache)dlsym(lib, "vkCreatePipelineCache");
    out->vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)dlsym(lib, "vkDestroyPipelineCache");
    out->vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)dlsym(lib, "vkGetPipelineCacheData");
    out->vkMergePipelineCaches = (PFN_vkMergePipelineCaches)dlsym(lib, "vkMergePipelineCaches");
    out->vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)dlsym(lib, "vkCreateGraphicsPipelines");
    out->vkCreateComputePipelines = (PFN_vkCreateComputePipelines)dlsym(lib, "vkCreateComputePipelines");
    out->vkDestroyPipeline = (PFN_vkDestroyPipeline)dlsym(lib, "vkDestroyPipeline");
    out->vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)dlsym(lib, "vkCreatePipelineLayout");
    out->vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)dlsym(lib, "vkDestroyPipelineLayout");
    out->vkCreateSampler = (PFN_vkCreateSampler)dlsym(lib, "vkCreateSampler");
    out->vkDestroySampler = (PFN_vkDestroySampler)dlsym(lib, "vkDestroySampler");
    out->vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)dlsym(lib, "vkCreateDescriptorSetLayout");
    out->vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)dlsym(lib, "vkDestroyDescriptorSetLayout");
    out->vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)dlsym(lib, "vkCreateDescriptorPool");
    out->vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)dlsym(lib, "vkDestroyDescriptorPool");
    out->vkResetDescriptorPool = (PFN_vkResetDescriptorPool)dlsym(lib, "vkResetDescriptorPool");
    out->vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)dlsym(lib, "vkAllocateDescriptorSets");
    out->vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)dlsym(lib, "vkFreeDescriptorSets");
    out->vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)dlsym(lib, "vkUpdateDescriptorSets");
    out->vkCreateFramebuffer = (PFN_vkCreateFramebuffer)dlsym(lib, "vkCreateFramebuffer");
    out->vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)dlsym(lib, "vkDestroyFramebuffer");
    out->vkCreateRenderPass = (PFN_vkCreateRenderPass)dlsym(lib, "vkCreateRenderPass");
    out->vkDestroyRenderPass = (PFN_vkDestroyRenderPass)dlsym(lib, "vkDestroyRenderPass");
    out->vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)dlsym(lib, "vkGetRenderAreaGranularity");
    out->vkCreateCommandPool = (PFN_vkCreateCommandPool)dlsym(lib, "vkCreateCommandPool");
    out->vkDestroyCommandPool = (PFN_vkDestroyCommandPool)dlsym(lib, "vkDestroyCommandPool");
    out->vkResetCommandPool = (PFN_vkResetCommandPool)dlsym(lib, "vkResetCommandPool");
    out->vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)dlsym(lib, "vkAllocateCommandBuffers");
    out->vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)dlsym(lib, "vkFreeCommandBuffers");
    out->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)dlsym(lib, "vkBeginCommandBuffer");
    out->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)dlsym(lib, "vkEndCommandBuffer");
    out->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)dlsym(lib, "vkResetCommandBuffer");
    out->vkCmdBindPipeline = (PFN_vkCmdBindPipeline)dlsym(lib, "vkCmdBindPipeline");
    out->vkCmdSetViewport = (PFN_vkCmdSetViewport)dlsym(lib, "vkCmdSetViewport");
    out->vkCmdSetScissor = (PFN_vkCmdSetScissor)dlsym(lib, "vkCmdSetScissor");
    out->vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)dlsym(lib, "vkCmdSetLineWidth");
    out->vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)dlsym(lib, "vkCmdSetDepthBias");
    out->vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)dlsym(lib, "vkCmdSetBlendConstants");
    out->vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)dlsym(lib, "vkCmdSetDepthBounds");
    out->vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)dlsym(lib, "vkCmdSetStencilCompareMask");
    out->vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)dlsym(lib, "vkCmdSetStencilWriteMask");
    out->vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)dlsym(lib, "vkCmdSetStencilReference");
    out->vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)dlsym(lib, "vkCmdBindDescriptorSets");
    out->vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)dlsym(lib, "vkCmdBindIndexBuffer");
    out->vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)dlsym(lib, "vkCmdBindVertexBuffers");
    out->vkCmdDraw = (PFN_vkCmdDraw)dlsym(lib, "vkCmdDraw");
    out->vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)dlsym(lib, "vkCmdDrawIndexed");
    out->vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)dlsym(lib, "vkCmdDrawIndirect");
    out->vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)dlsym(lib, "vkCmdDrawIndexedIndirect");
    out->vkCmdDispatch = (PFN_vkCmdDispatch)dlsym(lib, "vkCmdDispatch");
    out->vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)dlsym(lib, "vkCmdDispatchIndirect");
    out->vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)dlsym(lib, "vkCmdCopyBuffer");
    out->vkCmdCopyImage = (PFN_vkCmdCopyImage)dlsym(lib, "vkCmdCopyImage");
    out->vkCmdBlitImage = (PFN_vkCmdBlitImage)dlsym(lib, "vkCmdBlitImage");
    out->vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)dlsym(lib, "vkCmdCopyBufferToImage");
    out->vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)dlsym(lib, "vkCmdCopyImageToBuffer");
    out->vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)dlsym(lib, "vkCmdUpdateBuffer");
    out->vkCmdFillBuffer = (PFN_vkCmdFillBuffer)dlsym(lib, "vkCmdFillBuffer");
    out->vkCmdClearColorImage = (PFN_vkCmdClearColorImage)dlsym(lib, "vkCmdClearColorImage");
    out->vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)dlsym(lib, "vkCmdClearDepthStencilImage");
    out->vkCmdClearAttachments = (PFN_vkCmdClearAttachments)dlsym(lib, "vkCmdClearAttachments");
    out->vkCmdResolveImage = (PFN_vkCmdResolveImage)dlsym(lib, "vkCmdResolveImage");
    out->vkCmdSetEvent = (PFN_vkCmdSetEvent)dlsym(lib, "vkCmdSetEvent");
    out->vkCmdResetEvent = (PFN_vkCmdResetEvent)dlsym(lib, "vkCmdResetEvent");
    out->vkCmdWaitEvents = (PFN_vkCmdWaitEvents)dlsym(lib, "vkCmdWaitEvents");
    out->vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)dlsym(lib, "vkCmdPipelineBarrier");
    out->vkCmdBeginQuery = (PFN_vkCmdBeginQuery)dlsym(lib, "vkCmdBeginQuery");
    out->vkCmdEndQuery = (PFN_vkCmdEndQuery)dlsym(lib, "vkCmdEndQuery");
    out->vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)dlsym(lib, "vkCmdResetQueryPool");
    out->vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)dlsym(lib, "vkCmdWriteTimestamp");
    out->vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)dlsym(lib, "vkCmdCopyQueryPoolResults");
    out->vkCmdPushConstants = (PFN_vkCmdPushConstants)dlsym(lib, "vkCmdPushConstants");
    out->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)dlsym(lib, "vkCmdBeginRenderPass");
    out->vkCmdNextSubpass = (PFN_vkCmdNextSubpass)dlsym(lib, "vkCmdNextSubpass");
    out->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)dlsym(lib, "vkCmdEndRenderPass");
    out->vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)dlsym(lib, "vkCmdExecuteCommands");
#endif
#ifdef VK_VERSION_1_1
    out->vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)dlsym(lib, "vkEnumerateInstanceVersion");
    out->vkBindBufferMemory2 = (PFN_vkBindBufferMemory2)dlsym(lib, "vkBindBufferMemory2");
    out->vkBindImageMemory2 = (PFN_vkBindImageMemory2)dlsym(lib, "vkBindImageMemory2");
    out->vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures)dlsym(lib, "vkGetDeviceGroupPeerMemoryFeatures");
    out->vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask)dlsym(lib, "vkCmdSetDeviceMask");
    out->vkCmdDispatchBase = (PFN_vkCmdDispatchBase)dlsym(lib, "vkCmdDispatchBase");
    out->vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)dlsym(lib, "vkGetImageMemoryRequirements2");
    out->vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2)dlsym(lib, "vkGetBufferMemoryRequirements2");
    out->vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)dlsym(lib, "vkGetImageSparseMemoryRequirements2");
    out->vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)dlsym(lib, "vkGetPhysicalDeviceFeatures2");
    out->vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)dlsym(lib, "vkGetPhysicalDeviceProperties2");
    out->vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2)dlsym(lib, "vkGetPhysicalDeviceFormatProperties2");
    out->vkGetPhysicalDeviceImageFormatProperties2 = (PFN_vkGetPhysicalDeviceImageFormatProperties2)dlsym(lib, "vkGetPhysicalDeviceImageFormatProperties2");
    out->vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)dlsym(lib, "vkGetPhysicalDeviceQueueFamilyProperties2");
    out->vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)dlsym(lib, "vkGetPhysicalDeviceMemoryProperties2");
    out->vkGetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2)dlsym(lib, "vkGetPhysicalDeviceSparseImageFormatProperties2");
    out->vkTrimCommandPool = (PFN_vkTrimCommandPool)dlsym(lib, "vkTrimCommandPool");
    out->vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2)dlsym(lib, "vkGetDeviceQueue2");
    out->vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion)dlsym(lib, "vkCreateSamplerYcbcrConversion");
    out->vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion)dlsym(lib, "vkDestroySamplerYcbcrConversion");
    out->vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)dlsym(lib, "vkCreateDescriptorUpdateTemplate");
    out->vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)dlsym(lib, "vkDestroyDescriptorUpdateTemplate");
    out->vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)dlsym(lib, "vkUpdateDescriptorSetWithTemplate");
    out->vkGetPhysicalDeviceExternalBufferProperties = (PFN_vkGetPhysicalDeviceExternalBufferProperties)dlsym(lib, "vkGetPhysicalDeviceExternalBufferProperties");
    out->vkGetPhysicalDeviceExternalFenceProperties = (PFN_vkGetPhysicalDeviceExternalFenceProperties)dlsym(lib, "vkGetPhysicalDeviceExternalFenceProperties");
    out->vkGetPhysicalDeviceExternalSemaphoreProperties = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)dlsym(lib, "vkGetPhysicalDeviceExternalSemaphoreProperties");
    out->vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport)dlsym(lib, "vkGetDescriptorSetLayoutSupport");
#endif
#ifdef VK_KHR_surface
    out->vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)dlsym(lib, "vkGetPhysicalDeviceSurfaceSupportKHR");
    out->vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)dlsym(lib, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    out->vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)dlsym(lib, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    out->vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)dlsym(lib, "vkGetPhysicalDeviceSurfacePresentModesKHR");
#endif
#ifdef VK_KHR_swapchain
    out->vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)dlsym(lib, "vkGetDeviceGroupPresentCapabilitiesKHR");
    out->vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)dlsym(lib, "vkGetDeviceGroupSurfacePresentModesKHR");
    out->vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)dlsym(lib, "vkGetPhysicalDevicePresentRectanglesKHR");
    out->vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)dlsym(lib, "vkAcquireNextImage2KHR");
#endif
#ifdef VK_KHR_display
    out->vkGetPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)dlsym(lib, "vkGetPhysicalDeviceDisplayPropertiesKHR");
    out->vkGetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)dlsym(lib, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");
    out->vkGetDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR)dlsym(lib, "vkGetDisplayPlaneSupportedDisplaysKHR");
    out->vkGetDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR)dlsym(lib, "vkGetDisplayModePropertiesKHR");
    out->vkCreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR)dlsym(lib, "vkCreateDisplayModeKHR");
    out->vkGetDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR)dlsym(lib, "vkGetDisplayPlaneCapabilitiesKHR");
#endif
#ifdef VK_KHR_display_swapchain
    out->vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)dlsym(lib, "vkCreateSharedSwapchainsKHR");
#endif
#ifdef VK_KHR_wayland_surface
    out->vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)dlsym(lib, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif
#ifdef VK_KHR_mir_surface
    out->vkGetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)dlsym(lib, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif
#ifdef VK_KHR_get_physical_device_properties2
    out->vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR)dlsym(lib, "vkGetPhysicalDeviceFeatures2KHR");
    out->vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceProperties2KHR");
    out->vkGetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceFormatProperties2KHR");
    out->vkGetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceImageFormatProperties2KHR");
    out->vkGetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
    out->vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceMemoryProperties2KHR");
    out->vkGetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
#endif
#ifdef VK_KHR_device_group
    out->vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)dlsym(lib, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
    out->vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)dlsym(lib, "vkCmdSetDeviceMaskKHR");
    out->vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)dlsym(lib, "vkCmdDispatchBaseKHR");
#endif
#ifdef VK_KHR_maintenance1
    out->vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)dlsym(lib, "vkTrimCommandPoolKHR");
#endif
#ifdef VK_KHR_external_memory_capabilities
    out->vkGetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)dlsym(lib, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
#endif
#ifdef VK_KHR_external_memory_win32
    out->vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)dlsym(lib, "vkGetMemoryWin32HandleKHR");
    out->vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)dlsym(lib, "vkGetMemoryWin32HandlePropertiesKHR");
#endif
#ifdef VK_KHR_external_memory_fd
    out->vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)dlsym(lib, "vkGetMemoryFdKHR");
    out->vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)dlsym(lib, "vkGetMemoryFdPropertiesKHR");
#endif
#ifdef VK_KHR_external_semaphore_capabilities
    out->vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)dlsym(lib, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
#endif
#ifdef VK_KHR_external_semaphore_win32
    out->vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)dlsym(lib, "vkImportSemaphoreWin32HandleKHR");
    out->vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)dlsym(lib, "vkGetSemaphoreWin32HandleKHR");
#endif
#ifdef VK_KHR_external_semaphore_fd
    out->vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)dlsym(lib, "vkImportSemaphoreFdKHR");
    out->vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)dlsym(lib, "vkGetSemaphoreFdKHR");
#endif
#ifdef VK_KHR_push_descriptor
    out->vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)dlsym(lib, "vkCmdPushDescriptorSetKHR");
    out->vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)dlsym(lib, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif
#ifdef VK_KHR_descriptor_update_template
    out->vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)dlsym(lib, "vkCreateDescriptorUpdateTemplateKHR");
    out->vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)dlsym(lib, "vkDestroyDescriptorUpdateTemplateKHR");
    out->vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)dlsym(lib, "vkUpdateDescriptorSetWithTemplateKHR");
#endif
#ifdef VK_KHR_create_renderpass2
    out->vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)dlsym(lib, "vkCreateRenderPass2KHR");
    out->vkCmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR)dlsym(lib, "vkCmdBeginRenderPass2KHR");
    out->vkCmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR)dlsym(lib, "vkCmdNextSubpass2KHR");
    out->vkCmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR)dlsym(lib, "vkCmdEndRenderPass2KHR");
#endif
#ifdef VK_KHR_shared_presentable_image
    out->vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)dlsym(lib, "vkGetSwapchainStatusKHR");
#endif
#ifdef VK_KHR_external_fence_capabilities
    out->vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)dlsym(lib, "vkGetPhysicalDeviceExternalFencePropertiesKHR");
#endif
#ifdef VK_KHR_external_fence_win32
    out->vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)dlsym(lib, "vkImportFenceWin32HandleKHR");
    out->vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)dlsym(lib, "vkGetFenceWin32HandleKHR");
#endif
#ifdef VK_KHR_external_fence_fd
    out->vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)dlsym(lib, "vkImportFenceFdKHR");
    out->vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)dlsym(lib, "vkGetFenceFdKHR");
#endif
#ifdef VK_KHR_get_surface_capabilities2
    out->vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)dlsym(lib, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
    out->vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)dlsym(lib, "vkGetPhysicalDeviceSurfaceFormats2KHR");
#endif
#ifdef VK_KHR_get_display_properties2
    out->vkGetPhysicalDeviceDisplayProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceDisplayProperties2KHR");
    out->vkGetPhysicalDeviceDisplayPlaneProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR)dlsym(lib, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR");
    out->vkGetDisplayModeProperties2KHR = (PFN_vkGetDisplayModeProperties2KHR)dlsym(lib, "vkGetDisplayModeProperties2KHR");
    out->vkGetDisplayPlaneCapabilities2KHR = (PFN_vkGetDisplayPlaneCapabilities2KHR)dlsym(lib, "vkGetDisplayPlaneCapabilities2KHR");
#endif
#ifdef VK_KHR_get_memory_requirements2
    out->vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)dlsym(lib, "vkGetImageMemoryRequirements2KHR");
    out->vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)dlsym(lib, "vkGetBufferMemoryRequirements2KHR");
    out->vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)dlsym(lib, "vkGetImageSparseMemoryRequirements2KHR");
#endif
#ifdef VK_KHR_sampler_ycbcr_conversion
    out->vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)dlsym(lib, "vkCreateSamplerYcbcrConversionKHR");
    out->vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)dlsym(lib, "vkDestroySamplerYcbcrConversionKHR");
#endif
#ifdef VK_KHR_bind_memory2
    out->vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)dlsym(lib, "vkBindBufferMemory2KHR");
    out->vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)dlsym(lib, "vkBindImageMemory2KHR");
#endif
#ifdef VK_KHR_maintenance3
    out->vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)dlsym(lib, "vkGetDescriptorSetLayoutSupportKHR");
#endif
#ifdef VK_KHR_draw_indirect_count
    out->vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)dlsym(lib, "vkCmdDrawIndirectCountKHR");
    out->vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)dlsym(lib, "vkCmdDrawIndexedIndirectCountKHR");
#endif
#ifdef VK_ANDROID_native_buffer
    out->vkGetSwapchainGrallocUsageANDROID = (PFN_vkGetSwapchainGrallocUsageANDROID)dlsym(lib, "vkGetSwapchainGrallocUsageANDROID");
    out->vkAcquireImageANDROID = (PFN_vkAcquireImageANDROID)dlsym(lib, "vkAcquireImageANDROID");
    out->vkQueueSignalReleaseImageANDROID = (PFN_vkQueueSignalReleaseImageANDROID)dlsym(lib, "vkQueueSignalReleaseImageANDROID");
#endif
#ifdef VK_EXT_debug_marker
    out->vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)dlsym(lib, "vkDebugMarkerSetObjectTagEXT");
    out->vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)dlsym(lib, "vkDebugMarkerSetObjectNameEXT");
    out->vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)dlsym(lib, "vkCmdDebugMarkerBeginEXT");
    out->vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)dlsym(lib, "vkCmdDebugMarkerEndEXT");
    out->vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)dlsym(lib, "vkCmdDebugMarkerInsertEXT");
#endif
#ifdef VK_AMD_draw_indirect_count
    out->vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)dlsym(lib, "vkCmdDrawIndirectCountAMD");
    out->vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)dlsym(lib, "vkCmdDrawIndexedIndirectCountAMD");
#endif
#ifdef VK_AMD_shader_info
    out->vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)dlsym(lib, "vkGetShaderInfoAMD");
#endif
#ifdef VK_NV_external_memory_capabilities
    out->vkGetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)dlsym(lib, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
#endif
#ifdef VK_NV_external_memory_win32
    out->vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)dlsym(lib, "vkGetMemoryWin32HandleNV");
#endif
#ifdef VK_EXT_conditional_rendering
    out->vkCmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT)dlsym(lib, "vkCmdBeginConditionalRenderingEXT");
    out->vkCmdEndConditionalRenderingEXT = (PFN_vkCmdEndConditionalRenderingEXT)dlsym(lib, "vkCmdEndConditionalRenderingEXT");
#endif
#ifdef VK_NVX_device_generated_commands
    out->vkCmdProcessCommandsNVX = (PFN_vkCmdProcessCommandsNVX)dlsym(lib, "vkCmdProcessCommandsNVX");
    out->vkCmdReserveSpaceForCommandsNVX = (PFN_vkCmdReserveSpaceForCommandsNVX)dlsym(lib, "vkCmdReserveSpaceForCommandsNVX");
    out->vkCreateIndirectCommandsLayoutNVX = (PFN_vkCreateIndirectCommandsLayoutNVX)dlsym(lib, "vkCreateIndirectCommandsLayoutNVX");
    out->vkDestroyIndirectCommandsLayoutNVX = (PFN_vkDestroyIndirectCommandsLayoutNVX)dlsym(lib, "vkDestroyIndirectCommandsLayoutNVX");
    out->vkCreateObjectTableNVX = (PFN_vkCreateObjectTableNVX)dlsym(lib, "vkCreateObjectTableNVX");
    out->vkDestroyObjectTableNVX = (PFN_vkDestroyObjectTableNVX)dlsym(lib, "vkDestroyObjectTableNVX");
    out->vkRegisterObjectsNVX = (PFN_vkRegisterObjectsNVX)dlsym(lib, "vkRegisterObjectsNVX");
    out->vkUnregisterObjectsNVX = (PFN_vkUnregisterObjectsNVX)dlsym(lib, "vkUnregisterObjectsNVX");
    out->vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = (PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX)dlsym(lib, "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX");
#endif
#ifdef VK_NV_clip_space_w_scaling
    out->vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)dlsym(lib, "vkCmdSetViewportWScalingNV");
#endif
#ifdef VK_EXT_direct_mode_display
    out->vkReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT)dlsym(lib, "vkReleaseDisplayEXT");
#endif
#ifdef VK_EXT_acquire_xlib_display
    out->vkAcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT)dlsym(lib, "vkAcquireXlibDisplayEXT");
    out->vkGetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT)dlsym(lib, "vkGetRandROutputDisplayEXT");
#endif
#ifdef VK_EXT_display_surface_counter
    out->vkGetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)dlsym(lib, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
#endif
#ifdef VK_EXT_display_control
    out->vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)dlsym(lib, "vkDisplayPowerControlEXT");
    out->vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)dlsym(lib, "vkRegisterDeviceEventEXT");
    out->vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)dlsym(lib, "vkRegisterDisplayEventEXT");
    out->vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)dlsym(lib, "vkGetSwapchainCounterEXT");
#endif
#ifdef VK_GOOGLE_display_timing
    out->vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)dlsym(lib, "vkGetRefreshCycleDurationGOOGLE");
    out->vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)dlsym(lib, "vkGetPastPresentationTimingGOOGLE");
#endif
#ifdef VK_EXT_discard_rectangles
    out->vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)dlsym(lib, "vkCmdSetDiscardRectangleEXT");
#endif
#ifdef VK_EXT_hdr_metadata
    out->vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)dlsym(lib, "vkSetHdrMetadataEXT");
#endif
#ifdef VK_EXT_debug_utils
    out->vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)dlsym(lib, "vkSetDebugUtilsObjectNameEXT");
    out->vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)dlsym(lib, "vkSetDebugUtilsObjectTagEXT");
    out->vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)dlsym(lib, "vkQueueBeginDebugUtilsLabelEXT");
    out->vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)dlsym(lib, "vkQueueEndDebugUtilsLabelEXT");
    out->vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)dlsym(lib, "vkQueueInsertDebugUtilsLabelEXT");
    out->vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)dlsym(lib, "vkCmdBeginDebugUtilsLabelEXT");
    out->vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)dlsym(lib, "vkCmdEndDebugUtilsLabelEXT");
    out->vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)dlsym(lib, "vkCmdInsertDebugUtilsLabelEXT");
#endif
#ifdef VK_ANDROID_external_memory_android_hardware_buffer
    out->vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)dlsym(lib, "vkGetAndroidHardwareBufferPropertiesANDROID");
    out->vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)dlsym(lib, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif
#ifdef VK_EXT_sample_locations
    out->vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)dlsym(lib, "vkCmdSetSampleLocationsEXT");
    out->vkGetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)dlsym(lib, "vkGetPhysicalDeviceMultisamplePropertiesEXT");
#endif
#ifdef VK_EXT_validation_cache
    out->vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)dlsym(lib, "vkCreateValidationCacheEXT");
    out->vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)dlsym(lib, "vkDestroyValidationCacheEXT");
    out->vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)dlsym(lib, "vkMergeValidationCachesEXT");
    out->vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)dlsym(lib, "vkGetValidationCacheDataEXT");
#endif
#ifdef VK_EXT_external_memory_host
    out->vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)dlsym(lib, "vkGetMemoryHostPointerPropertiesEXT");
#endif
#ifdef VK_AMD_buffer_marker
    out->vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)dlsym(lib, "vkCmdWriteBufferMarkerAMD");
#endif
#ifdef VK_NV_device_diagnostic_checkpoints
    out->vkCmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV)dlsym(lib, "vkCmdSetCheckpointNV");
    out->vkGetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)dlsym(lib, "vkGetQueueCheckpointDataNV");
#endif
#ifdef VK_GOOGLE_address_space
    out->vkMapMemoryIntoAddressSpaceGOOGLE = (PFN_vkMapMemoryIntoAddressSpaceGOOGLE)dlsym(lib, "vkMapMemoryIntoAddressSpaceGOOGLE");
#endif
#ifdef VK_VERSION_1_0
    out->vkDestroyInstance = (PFN_vkDestroyInstance)dlsym(lib, "vkDestroyInstance");
    out->vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)dlsym(lib, "vkEnumeratePhysicalDevices");
    out->vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)dlsym(lib, "vkGetPhysicalDeviceFeatures");
    out->vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)dlsym(lib, "vkGetPhysicalDeviceFormatProperties");
    out->vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)dlsym(lib, "vkGetPhysicalDeviceImageFormatProperties");
    out->vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)dlsym(lib, "vkGetPhysicalDeviceProperties");
    out->vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)dlsym(lib, "vkGetPhysicalDeviceQueueFamilyProperties");
    out->vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)dlsym(lib, "vkGetPhysicalDeviceMemoryProperties");
    out->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
    out->vkCreateDevice = (PFN_vkCreateDevice)dlsym(lib, "vkCreateDevice");
    out->vkDestroyDevice = (PFN_vkDestroyDevice)dlsym(lib, "vkDestroyDevice");
    out->vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)dlsym(lib, "vkEnumerateDeviceExtensionProperties");
    out->vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)dlsym(lib, "vkEnumerateDeviceLayerProperties");
    out->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)dlsym(lib, "vkGetDeviceQueue");
    out->vkQueueSubmit = (PFN_vkQueueSubmit)dlsym(lib, "vkQueueSubmit");
    out->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)dlsym(lib, "vkQueueWaitIdle");
    out->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)dlsym(lib, "vkDeviceWaitIdle");
    out->vkDestroyInstance = (PFN_vkDestroyInstance)dlsym(lib, "vkDestroyInstance");
    out->vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)dlsym(lib, "vkEnumeratePhysicalDevices");
    out->vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)dlsym(lib, "vkGetPhysicalDeviceFeatures");
    out->vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)dlsym(lib, "vkGetPhysicalDeviceFormatProperties");
    out->vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)dlsym(lib, "vkGetPhysicalDeviceImageFormatProperties");
    out->vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)dlsym(lib, "vkGetPhysicalDeviceProperties");
    out->vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)dlsym(lib, "vkGetPhysicalDeviceQueueFamilyProperties");
    out->vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)dlsym(lib, "vkGetPhysicalDeviceMemoryProperties");
    out->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
    out->vkCreateDevice = (PFN_vkCreateDevice)dlsym(lib, "vkCreateDevice");
    out->vkDestroyDevice = (PFN_vkDestroyDevice)dlsym(lib, "vkDestroyDevice");
    out->vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)dlsym(lib, "vkEnumerateDeviceExtensionProperties");
    out->vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)dlsym(lib, "vkEnumerateDeviceLayerProperties");
    out->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)dlsym(lib, "vkGetDeviceQueue");
    out->vkQueueSubmit = (PFN_vkQueueSubmit)dlsym(lib, "vkQueueSubmit");
    out->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)dlsym(lib, "vkQueueWaitIdle");
    out->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)dlsym(lib, "vkDeviceWaitIdle");
#endif
    return out;
}

} // extern "C"
