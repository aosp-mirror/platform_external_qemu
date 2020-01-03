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

/*
 * Contains declarations of platform-independent the stuff that is used in
 * camera emulation.
 */

#include "android/utils/compiler.h"
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

#include <errno.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif

ANDROID_BEGIN_HEADER

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
#ifndef V4L2_PIX_FMT_SGBRG8
#define V4L2_PIX_FMT_SGBRG8  v4l2_fourcc('G', 'B', 'R', 'G')
#endif  /* V4L2_PIX_FMT_SGBRG8 */
#ifndef V4L2_PIX_FMT_SGRBG8
#define V4L2_PIX_FMT_SGRBG8  v4l2_fourcc('G', 'R', 'B', 'G')
#endif  /* V4L2_PIX_FMT_SGRBG8 */
#ifndef V4L2_PIX_FMT_SRGGB8
#define V4L2_PIX_FMT_SRGGB8  v4l2_fourcc('R', 'G', 'G', 'B')
#endif  /* V4L2_PIX_FMT_SRGGB8 */
#ifndef V4L2_PIX_FMT_SBGGR10
#define V4L2_PIX_FMT_SBGGR10 v4l2_fourcc('B', 'G', '1', '0')
#endif  /* V4L2_PIX_FMT_SBGGR10 */
#ifndef V4L2_PIX_FMT_SGBRG10
#define V4L2_PIX_FMT_SGBRG10 v4l2_fourcc('G', 'B', '1', '0')
#endif  /* V4L2_PIX_FMT_SGBRG10 */
#ifndef V4L2_PIX_FMT_SGRBG10
#define V4L2_PIX_FMT_SGRBG10 v4l2_fourcc('B', 'A', '1', '0')
#endif  /* V4L2_PIX_FMT_SGRBG10 */
#ifndef V4L2_PIX_FMT_SRGGB10
#define V4L2_PIX_FMT_SRGGB10 v4l2_fourcc('R', 'G', '1', '0')
#endif  /* V4L2_PIX_FMT_SRGGB10 */
#ifndef V4L2_PIX_FMT_SBGGR12
#define V4L2_PIX_FMT_SBGGR12 v4l2_fourcc('B', 'G', '1', '2')
#endif  /* V4L2_PIX_FMT_SBGGR12 */
#ifndef V4L2_PIX_FMT_SGBRG12
#define V4L2_PIX_FMT_SGBRG12 v4l2_fourcc('G', 'B', '1', '2')
#endif  /* V4L2_PIX_FMT_SGBRG12 */
#ifndef V4L2_PIX_FMT_SGRBG12
#define V4L2_PIX_FMT_SGRBG12 v4l2_fourcc('B', 'A', '1', '2')
#endif  /* V4L2_PIX_FMT_SGRBG12 */
#ifndef V4L2_PIX_FMT_SRGGB12
#define V4L2_PIX_FMT_SRGGB12 v4l2_fourcc('R', 'G', '1', '2')
#endif  /* V4L2_PIX_FMT_SRGGB12 */
#ifndef V4L2_PIX_FMT_ARGB32
#define V4L2_PIX_FMT_ARGB32 v4l2_fourcc('B', 'A', '2', '4')
#endif /* V4L2_PIX_FMT_ARGB32 */

/* Describes framebuffer, used by the client of camera capturing API.
 * This descriptor is used in camera_device_read_frame call.
 */
typedef struct ClientFrameBuffer {
    /* Pixel format used in the client framebuffer. */
    uint32_t    pixel_format;
    /* Address of the client framebuffer. */
    void*       framebuffer;
} ClientFrameBuffer;

/* Defines framebuffers and metadata associated with a specific camera frame. */
typedef struct ClientFrame {
    /* Number of entries in the framebuffers array. */
    uint32_t framebuffers_count;
    /* Array of framebuffers, the size of this array is defined by
     * framebuffers_count. Note that the caller must make sure that the buffers
     * are large enough to contain the entire frame captured from the device. */
    ClientFrameBuffer* framebuffers;

    /* Pointer to staging buffer, used internally by convert_frame. */
    uint8_t** staging_framebuffer;
    /* Pointer to size of the staging_framebuffer, in bytes. */
    size_t* staging_framebuffer_size;

    /* The time at which the frame was produced. */
    int64_t frame_time;
} ClientFrame;

/* Describes frame dimensions.
 */
typedef struct CameraFrameDim {
    /* Frame width. */
    int     width;
    /* Frame height. */
    int     height;
} CameraFrameDim;

/* Defines the camera source type, which determines what generates frames on
 * the host. */
typedef enum CameraSourceType {
    /* A webcam camera device enumerated on the host. */
    kWebcam,
    /* A virtual scene camera, renders a virtual environment on the host. */
    kVirtualScene,
    /* A video playback camera, feeds in frames from a video file. */
    kVideoPlayback
} CameraSourceType;

/* Camera information descriptor, containing properties of a camera connected
 * to the host.
 *
 * Instances of this structure are created during camera device enumerations,
 * and are considered to be constant everywhere else. The only exception to this
 * rule is changing the 'in_use' flag during creation / destruction of a service
 * representing that camera.
 */
typedef struct CameraInfo {
    /* User-friendly camera display name. */
    char*               display_name;
    /* Device name for the camera. */
    char*               device_name;
    /* The source of the frames for the camera. */
    CameraSourceType camera_source;
    /* Input channel for the camera. */
    int                 inp_channel;
    /* Pixel format chosen for the camera. */
    uint32_t            pixel_format;
    /* Direction the camera is facing: 'front', or 'back' */
    char*               direction;
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

/* Frees all resources allocated for CameraInfo instance (NOT including the
 * instance itself). */
void camera_info_done(CameraInfo* ci);

/* Copy a CameraInfo instance |*from| into |*ci|. */
void camera_info_copy(CameraInfo* ci, const CameraInfo* from);

/* Describes a connected camera device.
 * This is a pratform-independent camera device descriptor that is used in
 * the camera API.
 */
typedef struct CameraDevice {
    /* Opaque pointer used by the camera capturing API. */
    void*       opaque;
} CameraDevice;

enum ClientStartResult {
    CLIENT_START_RESULT_SUCCESS = 2,
    CLIENT_START_RESULT_ALREADY_STARTED = 1,
    CLIENT_START_RESULT_PARAMETER_MISMATCH = -1,
    CLIENT_START_RESULT_UNKNOWN_PIXEL_FORMAT = -2,
    CLIENT_START_RESULT_NO_PIXEL_CONVERSION = -3,
    CLIENT_START_RESULT_OUT_OF_MEMORY = -4,
    CLIENT_START_RESULT_FAILED = -5,
};

typedef enum ClientStartResult ClientStartResult;

ANDROID_END_HEADER
