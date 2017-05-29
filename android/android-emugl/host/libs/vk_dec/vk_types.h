/*
* Copyright 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "vkUtils.h"

#include <memory>

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t s32;
typedef int64_t s64;
typedef int16_t s16;
typedef int8_t  s8;

typedef float f32;
struct float4x {
    float r;
    float g;
    float b;
    float a;
};
typedef float4x f32_4;
typedef float4x uint128_t;

using android::base::InplaceStream;
