// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "VkHandleDispatch.h"

#include "android/base/memory/LazyInstance.h"

#include <unordered_map>

using android::base::LazyInstance;

struct HandleDispatches {
    std::unordered_map<VkInstance, VkInstanceDispatch*> sInstanceDispatches;
    std::unordered_map<VkPhysicalDevice, VkInstanceDispatch*> sInstanceDispatchesPhysicalDevice;
    std::unordered_map<VkDevice, VkDeviceDispatch*> sDeviceDispatches;
    std::unordered_map<VkQueue, VkDeviceDispatch*> sDeviceDispatchesQueue;
    std::unordered_map<VkCommandBuffer, VkDeviceDispatch*> sDeviceDispatchesCmdBuf;
};

static LazyInstance<HandleDispatches> sHandleDispatches;

VkInstanceDispatch* s_ivk(VkInstance instance) {
    std::unordered_map<VkInstance, VkInstanceDispatch*>& dispatch =
        sHandleDispatches.ptr()->sInstanceDispatches;

    if (dispatch.find(instance) == dispatch.end()) {
        dispatch[instance] = new VkInstanceDispatch;
        *dispatch[instance] = initVkInstanceDispatch(instance);
    }
    return dispatch[instance];
}

VkInstanceDispatch* s_ivkPhysdev_new(VkInstance instance, VkPhysicalDevice physdev) {
    std::unordered_map<VkInstance, VkInstanceDispatch*>& dispatch =
        sHandleDispatches.ptr()->sInstanceDispatches;
    std::unordered_map<VkPhysicalDevice, VkInstanceDispatch*>& dispatchPhysdev =
        sHandleDispatches.ptr()->sInstanceDispatchesPhysicalDevice;

    dispatchPhysdev[physdev] = dispatch[instance];
    return dispatchPhysdev[physdev];
}

VkInstanceDispatch* s_ivkPhysdev(VkPhysicalDevice physdev) {
    return sHandleDispatches.ptr()->sInstanceDispatchesPhysicalDevice[physdev];
}

void s_ivkPhysdev_new_multi(VkInstance instance, uint32_t count, VkPhysicalDevice* devices) {
    if (!devices) return;
    for (uint32_t i = 0; i < count; i++) {
        s_ivkPhysdev_new(instance, devices[i]);
    }
}

VkDeviceDispatch* s_dvk_new(VkPhysicalDevice physdev, VkDevice device) {

    std::unordered_map<VkDevice, VkDeviceDispatch*>& dispatch =
        sHandleDispatches.ptr()->sDeviceDispatches;

    if (dispatch.find(device) == dispatch.end()) {
        dispatch[device] = new VkDeviceDispatch;
        *dispatch[device] =
            initVkDeviceDispatch(
                    *sHandleDispatches.ptr()->sInstanceDispatchesPhysicalDevice[physdev], device);
    }
    return dispatch[device];
}

VkDeviceDispatch* s_dvk(VkDevice device) {
    std::unordered_map<VkDevice, VkDeviceDispatch*>& dispatch =
        sHandleDispatches.ptr()->sDeviceDispatches;
    return dispatch[device];
}

VkDeviceDispatch* s_dvkQueue_new(VkDevice device, VkQueue queue) {
    std::unordered_map<VkDevice, VkDeviceDispatch*>& dispatch =
        sHandleDispatches.ptr()->sDeviceDispatches;
    sHandleDispatches.ptr()->sDeviceDispatchesQueue[queue] = dispatch[device];
    return sHandleDispatches.ptr()->sDeviceDispatchesQueue[queue];
}

VkDeviceDispatch* s_dvkQueue(VkQueue queue) {
    return sHandleDispatches.ptr()->sDeviceDispatchesQueue[queue];
}

VkDeviceDispatch* s_dvkCmdBuf_new(VkDevice device, VkCommandBuffer cmdbuf) {
    std::unordered_map<VkDevice, VkDeviceDispatch*>& dispatch =
        sHandleDispatches.ptr()->sDeviceDispatches;
    sHandleDispatches.ptr()->sDeviceDispatchesCmdBuf[cmdbuf] = dispatch[device];
    return sHandleDispatches.ptr()->sDeviceDispatchesCmdBuf[cmdbuf];
}

void s_dvkCmdBuf_new_multi(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) {
    if (!pCommandBuffers) return;
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        s_dvkCmdBuf_new(device, pCommandBuffers[i]);
    }
}

VkDeviceDispatch* s_dvkCmdBuf(VkCommandBuffer cmdbuf) {
    return sHandleDispatches.ptr()->sDeviceDispatchesCmdBuf[cmdbuf];
}

void sMapInstanceDispatch(VkInstance instance) {
    if (sHandleDispatches.ptr()->sInstanceDispatches.find(instance) ==
        sHandleDispatches.ptr()->sInstanceDispatches.end()) {
        sHandleDispatches.ptr()->sInstanceDispatches[instance] = new VkInstanceDispatch;
        *sHandleDispatches.ptr()->sInstanceDispatches[instance] = initVkInstanceDispatch(instance);
    }
}

