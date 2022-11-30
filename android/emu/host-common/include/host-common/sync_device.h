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

EMUGL_COMMON_API extern emugl_sync_create_timeline_t emugl_sync_create_timeline;
EMUGL_COMMON_API extern emugl_sync_create_fence_t emugl_sync_create_fence;
EMUGL_COMMON_API extern emugl_sync_timeline_inc_t emugl_sync_timeline_inc;
EMUGL_COMMON_API extern emugl_sync_destroy_timeline_t emugl_sync_destroy_timeline;
EMUGL_COMMON_API extern emugl_sync_register_trigger_wait_t emugl_sync_register_trigger_wait;
EMUGL_COMMON_API extern emugl_sync_device_exists_t emugl_sync_device_exists;

EMUGL_COMMON_API void set_emugl_sync_create_timeline(emugl_sync_create_timeline_t);
EMUGL_COMMON_API void set_emugl_sync_create_fence(emugl_sync_create_fence_t);
EMUGL_COMMON_API void set_emugl_sync_timeline_inc(emugl_sync_timeline_inc_t);
EMUGL_COMMON_API void set_emugl_sync_destroy_timeline(emugl_sync_destroy_timeline_t);
EMUGL_COMMON_API void set_emugl_sync_register_trigger_wait(emugl_sync_register_trigger_wait_t trigger_fn);
EMUGL_COMMON_API void set_emugl_sync_device_exists(emugl_sync_device_exists_t);

}  // namespace emugl
