#pragma once

#include "vk_types.h"
#include "vkUtils.h"
#include "vkUtils_custom.h"

template <typename T> T* AllocForHost(uint32_t count, const T* in);
