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
 * Contains code that is used to capture video frames from a camera device
 * on Linux. This code uses V4L2 API to work with camera devices, and requires
 * Linux kernel version at least 2.5
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "qemu-common.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include "android/camera/camera-capture.h"
#include "android/camera/camera-format-converters.h"

#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  W(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  E(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define  T_ACTIVE   0

#if T_ACTIVE
#define  T(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#else
#define  T(...)    ((void)0)
#endif

#define CLEAR(x) memset (&(x), 0, sizeof(x))

/* Describes a framebuffer. */
typedef struct CameraFrameBuffer {
    /* Framebuffer data. */
    uint8_t*    data;
    /* Framebuffer data size. */
    size_t      size;
} CameraFrameBuffer;

/* Defines type of the I/O used to obtain frames from the device. */
typedef enum CameraIoType {
    /* Framebuffers are shared via memory mapping. */
    CAMERA_IO_MEMMAP,
    /* Framebuffers are available via user pointers. */
    CAMERA_IO_USERPTR,
    /* Framebuffers are to be read from the device. */
    CAMERA_IO_DIRECT
} CameraIoType;

typedef struct LinuxCameraDevice LinuxCameraDevice;
/*
 * Describes a connection to an actual camera device.
 */
struct LinuxCameraDevice {
    /* Common header. */
    CameraDevice                header;

    /* Camera device name. (default is /dev/video0) */
    char*                       device_name;
    /* Input channel. (default is 0) */
    int                         input_channel;
    /* Requested pixel format. */
    uint32_t                    req_pixel_format;

    /*
     * Set by the framework after initializing camera connection.
     */

    /* Handle to the opened camera device. */
    int                         handle;
    /* Device capabilities. */
    struct v4l2_capability      caps;
    /* Actual pixel format reported by the device. */
    struct v4l2_pix_format      actual_pixel_format;
    /* Defines type of the I/O to use to retrieve frames from the device. */
    CameraIoType                io_type;
    /* Allocated framebuffers. */
    struct CameraFrameBuffer*   framebuffers;
    /* Actual number of allocated framebuffers. */
    int                         framebuffer_num;
};

/*******************************************************************************
 *                     Helper routines
 ******************************************************************************/

/* IOCTL wrapper. */
static int
_xioctl(int fd, int request, void *arg) {
  int r;
  do {
      r = ioctl(fd, request, arg);
  } while (-1 == r && EINTR == errno);
  return r;
}

/*******************************************************************************
 *                     CameraFrameBuffer routines
 ******************************************************************************/

/* Frees array of framebuffers, depending on the I/O method the array has been
 * initialized for.
 * Note that this routine doesn't frees the array itself.
 * Param:
 *  fb, num - Array data, and its size.
 *  io_type - Type of the I/O the array has been initialized for.
 */
static void
_free_framebuffers(CameraFrameBuffer* fb, int num, CameraIoType io_type)
{
    if (fb != NULL) {
        int n;

        switch (io_type) {
            case CAMERA_IO_MEMMAP:
                /* Unmap framebuffers. */
                for (n = 0; n < num; n++) {
                    if (fb[n].data != NULL) {
                        munmap(fb[n].data, fb[n].size);
                        fb[n].data = NULL;
                        fb[n].size = 0;
                    }
                }
                break;

            case CAMERA_IO_USERPTR:
            case CAMERA_IO_DIRECT:
                /* Free framebuffers. */
                for (n = 0; n < num; n++) {
                    if (fb[n].data != NULL) {
                        free(fb[n].data);
                        fb[n].data = NULL;
                        fb[n].size = 0;
                    }
                }
                break;

            default:
                E("Invalid I/O type %d", io_type);
                break;
        }
    }
}

/*******************************************************************************
 *                     CameraDevice routines
 ******************************************************************************/

/* Allocates an instance of LinuxCameraDevice structure.
 * Return:
 *  Allocated instance of LinuxCameraDevice structure. Note that this routine
 *  also sets 'opaque' field in the 'header' structure to point back to the
 *  containing LinuxCameraDevice instance.
 */
static LinuxCameraDevice*
_camera_device_alloc(void)
{
    LinuxCameraDevice* cd;

    ANEW0(cd);
    memset(cd, 0, sizeof(*cd));
    cd->header.opaque = cd;
    cd->handle = -1;

    return cd;
}

/* Uninitializes and frees CameraDevice structure.
 */
static void
_camera_device_free(LinuxCameraDevice* lcd)
{
    if (lcd != NULL) {
        /* Closing handle will also disconnect from the driver. */
        if (lcd->handle >= 0) {
            close(lcd->handle);
        }
        if (lcd->device_name != NULL) {
            free(lcd->device_name);
        }
        if (lcd->framebuffers != NULL) {
            _free_framebuffers(lcd->framebuffers, lcd->framebuffer_num,
                               lcd->io_type);
            free(lcd->framebuffers);
        }
        AFREE(lcd);
    } else {
        W("%s: No descriptor", __FUNCTION__);
    }
}

/* Memory maps buffers and shares mapped memory with the device.
 * Return:
 *  0 Framebuffers have been mapped.
 *  -1 A critical error has ocurred.
 *  1 Memory mapping is not available.
 */
static int
_camera_device_mmap_framebuffer(LinuxCameraDevice* cd)
{
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count   = 4;
    req.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory  = V4L2_MEMORY_MMAP;

    /* Request memory mapped buffers. Note that device can return less buffers
     * than requested. */
    if(_xioctl(cd->handle, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            D("%s: %s does not support memory mapping",
              __FUNCTION__, cd->device_name);
            return 1;
        } else {
            E("%s: VIDIOC_REQBUFS has failed: %s",
              __FUNCTION__, strerror(errno));
            return -1;
        }
    }

    /* Allocate framebuffer array. */
    cd->framebuffers = calloc(req.count, sizeof(CameraFrameBuffer));
    if (cd->framebuffers == NULL) {
        E("%s: Not enough memory to allocate framebuffer array", __FUNCTION__);
        return -1;
    }

    /* Map every framebuffer to the shared memory, and queue it
     * with the device. */
    for(cd->framebuffer_num = 0; cd->framebuffer_num < req.count;
        cd->framebuffer_num++) {
        /* Map framebuffer. */
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = cd->framebuffer_num;
        if(_xioctl(cd->handle, VIDIOC_QUERYBUF, &buf) < 0) {
            E("%s: VIDIOC_QUERYBUF has failed: %s",
              __FUNCTION__, strerror(errno));
            return -1;
        }
        cd->framebuffers[cd->framebuffer_num].size = buf.length;
        cd->framebuffers[cd->framebuffer_num].data =
            mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                 cd->handle, buf.m.offset);
        if (MAP_FAILED == cd->framebuffers[cd->framebuffer_num].data) {
            E("%s: Memory mapping has failed: %s",
              __FUNCTION__, strerror(errno));
            return -1;
        }

        /* Queue the mapped buffer. */
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = cd->framebuffer_num;
        if (_xioctl(cd->handle, VIDIOC_QBUF, &buf) < 0) {
            E("%s: VIDIOC_QBUF has failed: %s", __FUNCTION__, strerror(errno));
            return -1;
        }
    }

    cd->io_type = CAMERA_IO_MEMMAP;

    return 0;
}

/* Allocates frame buffers and registers them with the device.
 * Return:
 *  0 Framebuffers have been mapped.
 *  -1 A critical error has ocurred.
 *  1 Device doesn't support user pointers.
 */
static int
_camera_device_user_framebuffer(LinuxCameraDevice* cd)
{
    struct v4l2_requestbuffers req;
    CLEAR (req);
    req.count   = 4;
    req.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory  = V4L2_MEMORY_USERPTR;

    /* Request user buffers. Note that device can return less buffers
     * than requested. */
    if(_xioctl(cd->handle, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            D("%s: %s does not support user pointers",
              __FUNCTION__, cd->device_name);
            return 1;
        } else {
            E("%s: VIDIOC_REQBUFS has failed: %s",
              __FUNCTION__, strerror(errno));
            return -1;
        }
    }

    /* Allocate framebuffer array. */
    cd->framebuffers = calloc(req.count, sizeof(CameraFrameBuffer));
    if (cd->framebuffers == NULL) {
        E("%s: Not enough memory to allocate framebuffer array", __FUNCTION__);
        return -1;
    }

    /* Allocate buffers, queueing them wit the device at the same time */
    for(cd->framebuffer_num = 0; cd->framebuffer_num < req.count;
        cd->framebuffer_num++) {
        cd->framebuffers[cd->framebuffer_num].size =
            cd->actual_pixel_format.sizeimage;
        cd->framebuffers[cd->framebuffer_num].data =
            malloc(cd->framebuffers[cd->framebuffer_num].size);
        if (cd->framebuffers[cd->framebuffer_num].data == NULL) {
            E("%s: Not enough memory to allocate framebuffer", __FUNCTION__);
            return -1;
        }

        /* Queue the user buffer. */
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.m.userptr = cd->framebuffers[cd->framebuffer_num].data;
        buf.length = cd->framebuffers[cd->framebuffer_num].size;
        if (_xioctl(cd->handle, VIDIOC_QBUF, &buf) < 0) {
            E("%s: VIDIOC_QBUF has failed: %s", __FUNCTION__, strerror(errno));
            return -1;
        }
    }

    cd->io_type = CAMERA_IO_USERPTR;

    return 0;
}

/* Allocate frame buffer for direct read from the device.
 * Return:
 *  0 Framebuffers have been mapped.
 *  -1 A critical error has ocurred.
 *  1 Memory mapping is not available.
 */
static int
_camera_device_direct_framebuffer(LinuxCameraDevice* cd)
{
    /* Allocate framebuffer array. */
    cd->framebuffer_num = 1;
    cd->framebuffers = malloc(sizeof(CameraFrameBuffer));
    if (cd->framebuffers == NULL) {
        E("%s: Not enough memory to allocate framebuffer array", __FUNCTION__);
        return -1;
    }

    cd->framebuffers[0].size = cd->actual_pixel_format.sizeimage;
    cd->framebuffers[0].data = malloc(cd->framebuffers[0].size);
    if (cd->framebuffers[0].data == NULL) {
        E("%s: Not enough memory to allocate framebuffer", __FUNCTION__);
        return -1;
    }

    cd->io_type = CAMERA_IO_DIRECT;

    return 0;
}

static int
_camera_device_open(LinuxCameraDevice* cd)
{
    struct stat st;

    if (stat(cd->device_name, &st)) {
        E("%s: Cannot identify camera device '%s': %s",
          __FUNCTION__, cd->device_name, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        E("%s: %s is not a device", __FUNCTION__, cd->device_name);
        return -1;
    }

    /* Open handle to the device, and query device capabilities. */
    cd->handle = open(cd->device_name, O_RDWR | O_NONBLOCK, 0);
    if (cd->handle < 0) {
        E("%s: Cannot open camera device '%s': %s\n",
          __FUNCTION__, cd->device_name, strerror(errno));
        return -1;
    }
    if (_xioctl(cd->handle, VIDIOC_QUERYCAP, &cd->caps) < 0) {
        if (EINVAL == errno) {
            E("%s: Camera '%s' is not a V4L2 device",
              __FUNCTION__, cd->device_name);
            close(cd->handle);
            cd->handle = -1;
            return -1;
        } else {
            E("%s: Unable to query camera '%s' capabilities",
              __FUNCTION__, cd->device_name);
            close(cd->handle);
            cd->handle = -1;
            return -1;
        }
    }

    /* Make sure that camera supports minimal requirements. */
    if (!(cd->caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        E("%s: Camera '%s' is not a video capture device",
          __FUNCTION__, cd->device_name);
        close(cd->handle);
        cd->handle = -1;
        return -1;
    }

    return 0;
}

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

CameraDevice*
camera_device_open(const char* name,
                   int inp_channel,
                   uint32_t pixel_format)
{
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    LinuxCameraDevice* cd;

    /* Allocate and initialize the descriptor. */
    cd = _camera_device_alloc();
    cd->device_name = name != NULL ? ASTRDUP(name) : ASTRDUP("/dev/video0");
    cd->input_channel = inp_channel;
    cd->req_pixel_format = pixel_format;

    /* Open the device. */
    if (_camera_device_open(cd)) {
        _camera_device_free(cd);
        return NULL;
    }

    /* Select video input, video standard and tune here. */
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    _xioctl(cd->handle, VIDIOC_CROPCAP, &cropcap);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */
    _xioctl (cd->handle, VIDIOC_S_CROP, &crop);

    /* Image settings. */
    CLEAR(fmt);
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 0;
    fmt.fmt.pix.height      = 0;
    fmt.fmt.pix.pixelformat = 0;
    if (_xioctl(cd->handle, VIDIOC_G_FMT, &fmt) < 0) {
        E("%s: Unable to obtain pixel format", __FUNCTION__);
        _camera_device_free(cd);
        return NULL;
    }
    if (_xioctl(cd->handle, VIDIOC_S_FMT, &fmt) < 0) {
        char fmt_str[5];
        memcpy(fmt_str, &cd->req_pixel_format, 4);
        fmt_str[4] = '\0';
        E("%s: Camera '%s' does not support requested pixel format '%s'",
          __FUNCTION__, cd->device_name, fmt_str);
        _camera_device_free(cd);
        return NULL;
    }
    /* VIDIOC_S_FMT has changed some properties of the structure, adjusting them
     * to the actual values, supported by the device. */
    memcpy(&cd->actual_pixel_format, &fmt.fmt.pix,
           sizeof(cd->actual_pixel_format));
    {
        char fmt_str[5];
        memcpy(fmt_str, &cd->req_pixel_format, 4);
        fmt_str[4] = '\0';
        D("%s: Camera '%s' uses pixel format '%s'",
          __FUNCTION__, cd->device_name, fmt_str);
    }

    return &cd->header;
}

int
camera_device_start_capturing(CameraDevice* ccd)
{
    LinuxCameraDevice* cd;

    /* Sanity checks. */
    if (ccd == NULL || ccd->opaque == NULL) {
      E("%s: Invalid camera device descriptor", __FUNCTION__);
      return -1;
    }
    cd = (LinuxCameraDevice*)ccd->opaque;

    /*
     * Lets initialize frame buffers, and see what kind of I/O we're going to
     * use to retrieve frames.
     */

    /* First, lets see if we can do mapped I/O (as most performant one). */
    int r = _camera_device_mmap_framebuffer(cd);
    if (r < 0) {
        /* Some critical error has ocurred. Bail out. */
        return -1;
    } else if (r > 0) {
        /* Device doesn't support memory mapping. Retrieve to the next performant
         * one: preallocated user buffers. */
        r = _camera_device_user_framebuffer(cd);
        if (r < 0) {
            /* Some critical error has ocurred. Bail out. */
            return -1;
        } else if (r > 0) {
            /* The only thing left for us is direct reading from the device. */
            if (!(cd->caps.capabilities & V4L2_CAP_READWRITE)) {
                E("%s: Device '%s' doesn't support direct read",
                  __FUNCTION__, cd->device_name);
                return -1;
            }
            r = _camera_device_direct_framebuffer(cd);
            if (r != 0) {
                /* Any error at this point is a critical one. */
                return -1;
            }
        }
    }

    /* Start capturing from the device. */
    if (cd->io_type != CAMERA_IO_DIRECT) {
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (_xioctl (cd->handle, VIDIOC_STREAMON, &type) < 0) {
            E("%s: VIDIOC_STREAMON has failed: %s",
              __FUNCTION__, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int
camera_device_stop_capturing(CameraDevice* ccd)
{
    enum v4l2_buf_type type;
    LinuxCameraDevice* cd;

    /* Sanity checks. */
    if (ccd == NULL || ccd->opaque == NULL) {
      E("%s: Invalid camera device descriptor", __FUNCTION__);
      return -1;
    }
    cd = (LinuxCameraDevice*)ccd->opaque;

    switch (cd->io_type) {
        case CAMERA_IO_DIRECT:
            /* Nothing to do. */
            break;

        case CAMERA_IO_MEMMAP:
        case CAMERA_IO_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (_xioctl(cd->handle, VIDIOC_STREAMOFF, &type) < 0) {
	            E("%s: VIDIOC_STREAMOFF has failed: %s",
                  __FUNCTION__, strerror(errno));
                return -1;
            }
            break;
        default:
            E("%s: Unknown I/O method: %d", __FUNCTION__, cd->io_type);
            return -1;
    }

    if (cd->framebuffers != NULL) {
        _free_framebuffers(cd->framebuffers, cd->framebuffer_num, cd->io_type);
        free(cd->framebuffers);
        cd->framebuffers = NULL;
        cd->framebuffer_num = 0;
    }
    return 0;
}

int
camera_device_read_frame(CameraDevice* ccd, uint8_t* buff)
{
    LinuxCameraDevice* cd;

    /* Sanity checks. */
    if (ccd == NULL || ccd->opaque == NULL) {
      E("%s: Invalid camera device descriptor", __FUNCTION__);
      return -1;
    }
    cd = (LinuxCameraDevice*)ccd->opaque;

    if (cd->io_type == CAMERA_IO_DIRECT) {
        /* Read directly from the device. */
        size_t total_read_bytes = 0;
        do {
            int read_bytes =
                read(cd->handle, buff + total_read_bytes,
                     cd->actual_pixel_format.sizeimage - total_read_bytes);
            if (read_bytes < 0) {
                switch (errno) {
                    case EIO:
                    case EAGAIN:
                        continue;
                    default:
                        E("%s: Unable to read from the device: %s",
                          __FUNCTION__, strerror(errno));
                        return -1;
                }
            }
            total_read_bytes += read_bytes;
        } while (total_read_bytes < cd->actual_pixel_format.sizeimage);
        return 0;
    } else {
        /* Dequeue next buffer from the device. */
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = cd->io_type == CAMERA_IO_MEMMAP ? V4L2_MEMORY_MMAP :
                                                       V4L2_MEMORY_USERPTR;
        if (_xioctl(cd->handle, VIDIOC_DQBUF, &buf) < 0) {
            switch (errno) {
                case EAGAIN:
                  return 1;

                case EIO:
                  /* Could ignore EIO, see spec. */
                  /* fall through */
                default:
                  E("%s: VIDIOC_DQBUF has failed: %s",
                    __FUNCTION__, strerror(errno));
                  return 1;
            }
        }
        /* Copy frame to the buffer. */
        memcpy(buff, cd->framebuffers[buf.index].data,
               cd->framebuffers[buf.index].size);
        /* Requeue the buffer with the device. */
        if (_xioctl(cd->handle, VIDIOC_QBUF, &buf) < 0) {
            D("%s: VIDIOC_QBUF has failed: %s",
              __FUNCTION__, strerror(errno));
        }
        return 0;
    }
}

void
camera_device_close(CameraDevice* ccd)
{
    LinuxCameraDevice* cd;

    /* Sanity checks. */
    if (ccd != NULL && ccd->opaque != NULL) {
        cd = (LinuxCameraDevice*)ccd->opaque;
        _camera_device_free(cd);
    } else {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
    }
}
