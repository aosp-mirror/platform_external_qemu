/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/camera/camera-common.h"
#include "android/utils/compiler.h"
ANDROID_BEGIN_HEADER
/*
 * Contains public camera service API.
 */

/* Initializes camera emulation service over qemu pipe. */
extern void android_camera_service_init(void);

typedef void (*camera_callback_t)(void* context, bool connected);

extern void register_camera_status_change_callback(camera_callback_t cb,
                                                   void* ctx,
                                                   CameraSourceType src);
ANDROID_END_HEADER
