#pragma once

#include "vk_dispatch_gen.h"

void initVkDispatch();
VkDllDispatch* vkDllDispatcher();
VkGlobalDispatch* vkGlobalDispatcher();

VkInstanceDispatch initVkInstanceDispatch(VkInstance instance);
VkDeviceDispatch initVkDeviceDispatch(const VkInstanceDispatch& vki, VkDevice device);
