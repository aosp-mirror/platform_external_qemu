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

#ifndef ANDROID_CAMERA_CAMERA_COMMON_H_
#define ANDROID_CAMERA_CAMERA_COMMON_H_

/*
 * Contains declarations of platform-independent the stuff that is used in
 * camera emulation.
 */

#include "qemu-common.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#ifdef _WIN32
/* Include declarations that are missing in non-Linux headers. */
#include "android/camera/camera-win.h"
#elif _DARWIN_C_SOURCE
/* Include declarations that are missing in non-Linux headers. */
#include "android/camera/camera-win.h"
#else
#include <linux/videodev2.h>
#endif  /* _WIN32 */

/*
 * These are missing in the current linux/videodev2.h
 */

#ifndef V4L2_PIX_FMT_YVYU
#define V4L2_PIX_FMT_YVYU    v4l2_fourcc('Y', 'V', 'Y', 'U')
#endif /* V4L2_PIX_FMT_YVYU */
#ifndef V4L2_PIX_FMT_VYUY
#define V4L2_PIX_FMT_VYUY    v4l2_fourcc('V', 'Y', 'U', 'Y')
#endif /* V4L2_PIX_FMT_VYUY */
#ifndef V4L2_PIX_FMT_YUY2
#define V4L2_PIX_FMT_YUY2    v4l2_fourcc('Y', 'U', 'Y', '2')
#endif /* V4L2_PIX_FMT_YUY2 */
#ifndef V4L2_PIX_FMT_YUNV
#define V4L2_PIX_FMT_YUNV    v4l2_fourcc('Y', 'U', 'N', 'V')
#endif /* V4L2_PIX_FMT_YUNV */
#ifndef V4L2_PIX_FMT_V422
#define V4L2_PIX_FMT_V422    v4l2_fourcc('V', '4', '2', '2')
#endif /* V4L2_PIX_FMT_V422 */
#ifndef V4L2_PIX_FMT_YYVU
#define V4L2_PIX_FMT_YYVU    v4l2_fourcc('Y', 'Y', 'V', 'U')
#endif /* V4L2_PIX_FMT_YYVU */


/* Describes framebuffer, used by the client of camera capturing API.
 * This descritptor is used in camera_device_read_frame call.
 */
typedef struct ClientFrameBuffer {
    /* Pixel format used in the client framebuffer. */
    uint32_t    pixel_format;
    /* Address of the client framebuffer. */
    void*       framebuffer;
} ClientFrameBuffer;

/* Describes frame dimensions.
 */
typedef struct CameraFrameDim {
    /* Frame width. */
    int     width;
    /* Frame height. */
    int     height;
} CameraFrameDim;

/* Camera information descriptor, containing properties of a camera connected
 * to the host.
 *
 * Instances of this structure are created during camera device enumerations,
 * and are considered to be constant everywhere else. The only exception to this
 * rule is changing the 'in_use' flag during creation / destruction of a service
 * representing that camera.
 */
typedef struct CameraInfo {
    /* Device name for the camera. */
    char*               device_name;
    /* Input channel for the camera. */
    int                 inp_channel;
    /* Pixel format chosen for the camera. */
    uint32_t            pixel_format;
    /* Array of frame sizes supported for the pixel format chosen for the camera.
     * The size of the array is defined by the frame_sizes_num field of this
     * structure. */
    CameraFrameDim*     frame_sizes;
    /* Number of frame sizes supported for the pixel format chosen
     * for the camera. */
    int                 frame_sizes_num;
    /* In use status. When there is a camera service created for this camera,
     * "in use" is set to one. Otherwise this flag is zet to 0. */
    int                 in_use;
} CameraInfo;

/* Allocates CameraInfo instance. */
static __inline__ CameraInfo* _camera_info_alloc(void)
{
    CameraInfo* ci;
    ANEW0(ci);
    return ci;
}

/* Frees all resources allocated for CameraInfo instance (including the
 * instance itself).
 */
static __inline__ void _camera_info_free(CameraInfo* ci)
{
    if (ci != NULL) {
        if (ci->device_name != NULL)
            free(ci->device_name);
        if (ci->frame_sizes != NULL)
            free(ci->frame_sizes);
        AFREE(ci);
    }
}

/* Describes a connected camera device.
 * This is a pratform-independent camera device descriptor that is used in
 * the camera API.
 */
typedef struct CameraDevice {
    /* Opaque pointer used by the camera capturing API. */
    void*       opaque;
} CameraDevice;

#endif  /* ANDROID_CAMERA_CAMERA_COMMON_H_ */
