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

/*
 * Contains code capturing video frames from a camera device on Mac.
 * This code uses capXxx API, available via capCreateCaptureWindow.
 */

#include "android/camera/camera-capture.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define  T_ACTIVE   0

#if T_ACTIVE
#define  T(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#else
#define  T(...)    ((void)0)
#endif

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

CameraDevice*
camera_device_open(const char* name, int inp_channel)
{
    /* TODO: Implement */
    return NULL;
}

int
camera_device_start_capturing(CameraDevice* cd,
                              uint32_t pixel_format,
                              int frame_width,
                              int frame_height)
{
    /* TODO: Implement */
    return -1;
}

int
camera_device_stop_capturing(CameraDevice* cd)
{
    /* TODO: Implement */
    return -1;
}

int
camera_device_read_frame(CameraDevice* cd,
                         ClientFrameBuffer* framebuffers,
                         int fbs_num)
{
    /* TODO: Implement */
    return -1;
}

void
camera_device_close(CameraDevice* cd)
{
    /* TODO: Implement */
}

int
enumerate_camera_devices(CameraInfo* cis, int max)
{
    /* TODO: Implement */
    return 0;
}
