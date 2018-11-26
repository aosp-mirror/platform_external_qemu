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
namespace emugl {

extern emugl_sync_create_timeline_t emugl_sync_create_timeline;
extern emugl_sync_create_fence_t emugl_sync_create_fence;
extern emugl_sync_timeline_inc_t emugl_sync_timeline_inc;
extern emugl_sync_destroy_timeline_t emugl_sync_destroy_timeline;
extern emugl_sync_register_trigger_wait_t emugl_sync_register_trigger_wait;
extern emugl_sync_device_exists_t emugl_sync_device_exists;

void set_emugl_sync_create_timeline(emugl_sync_create_timeline_t);
void set_emugl_sync_create_fence(emugl_sync_create_fence_t);
void set_emugl_sync_timeline_inc(emugl_sync_timeline_inc_t);
void set_emugl_sync_destroy_timeline(emugl_sync_destroy_timeline_t);
void set_emugl_sync_register_trigger_wait(emugl_sync_register_trigger_wait_t trigger_fn);
void set_emugl_sync_device_exists(emugl_sync_device_exists_t);

}  // namespace emugl
