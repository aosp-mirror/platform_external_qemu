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

#pragma once

#include "OpenGLESDispatch/VkDispatch.h"

VkInstanceDispatch* s_ivk(VkInstance instance);
VkInstanceDispatch* s_ivkPhysdev_new(VkInstance instance, VkPhysicalDevice physdev);
VkInstanceDispatch* s_ivkPhysdev(VkPhysicalDevice physdev);

void s_ivkPhysdev_new_multi(VkInstance instance, uint32_t count, VkPhysicalDevice* devices);

VkDeviceDispatch* s_dvk_new(VkPhysicalDevice physdev, VkDevice device);
VkDeviceDispatch* s_dvk(VkDevice device);
VkDeviceDispatch* s_dvkQueue_new(VkDevice device, VkQueue queue);
VkDeviceDispatch* s_dvkQueue(VkQueue queue);
VkDeviceDispatch* s_dvkCmdBuf_new(VkDevice device, VkCommandBuffer cmdbuf);
void s_dvkCmdBuf_new_multi(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
VkDeviceDispatch* s_dvkCmdBuf(VkCommandBuffer cmdbuf);

void sMapInstanceDispatch(VkInstance instance);
