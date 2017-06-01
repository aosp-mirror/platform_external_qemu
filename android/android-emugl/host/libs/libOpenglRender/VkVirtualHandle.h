#pragma once

#include <inttypes.h>

template <typename T>
uint32_t VkVirtualHandleCreate(T hostHandle);

template <typename T>
uint32_t VkVirtualHandleCreateOrGet(T hostHandle);

template <typename T>
void VkVirtualHandleDelete(T hostHandle);

template <typename T>
uint32_t VkVirtualHandleGetGuest(T hostHandle);

template <typename T>
T VkVirtualHandleGetHost(uint32_t hostHandle);
