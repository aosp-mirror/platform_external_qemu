/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "OpenglRender/render_api_types.h"

#ifdef _MSC_VER
# ifdef BUILDING_EMUGL_COMMON_SHARED
#  define EMUGL_COMMON_API __declspec(dllexport)
# else
#  define EMUGL_COMMON_API __declspec(dllimport)
#endif
#else
# define EMUGL_COMMON_API
#endif

namespace emugl {

EMUGL_COMMON_API extern emugl_dma_get_host_addr_t g_emugl_dma_get_host_addr;
EMUGL_COMMON_API extern emugl_dma_unlock_t g_emugl_dma_unlock;

EMUGL_COMMON_API void set_emugl_dma_get_host_addr(emugl_dma_get_host_addr_t);
EMUGL_COMMON_API void set_emugl_dma_unlock(emugl_dma_unlock_t);

}  // namespace emugl
