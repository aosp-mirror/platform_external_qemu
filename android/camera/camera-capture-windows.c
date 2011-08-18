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
 * Contains code capturing video frames from a camera device on Windows.
 * This code uses capXxx API, available via capCreateCaptureWindow.
 */
/*
#include <stddef.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
*/
#include "qemu-common.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include <vfw.h>
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

/* Default name for the capture window. */
static const char* _default_window_name = "AndroidEmulatorVC";

typedef struct WndCameraDevice WndCameraDevice;
/* Windows-specific camera device descriptor. */
struct WndCameraDevice {
    /* Common camera device descriptor. */
    CameraDevice        header;
    /* Capture window name. (default is AndroidEmulatorVC) */
    char*               window_name;
    /* Input channel (video driver index). (default is 0) */
    int                 input_channel;
    /* Requested pixel format. */
    uint32_t            req_pixel_format;

    /*
     * Set when framework gets initialized.
     */

    /* Video capturing window. Null indicates that device is not connected. */
    HWND                cap_window;
    /* DC for frame bitmap manipulation. Null indicates that frames are not
     * being capturing. */
    HDC                 dc;
    /* Bitmap info for the frames obtained from the video capture driver.
     * This information will be used when we get bitmap bits via
     * GetDIBits API. */
    BITMAPINFO*         frame_bitmap;
    /* Framebuffer large enough to fit the frame. */
    uint8_t*            framebuffer;
    /* Converter used to convert camera frames to the format
     * expected by the client. */
    FormatConverter     converter;
};

/*******************************************************************************
 *                     CameraDevice routines
 ******************************************************************************/

/* Allocates an instance of WndCameraDevice structure.
 * Return:
 *  Allocated instance of WndCameraDevice structure. Note that this routine
 *  also sets 'opaque' field in the 'header' structure to point back to the
 *  containing WndCameraDevice instance.
 */
static WndCameraDevice*
_camera_device_alloc(void)
{
    WndCameraDevice* cd = (WndCameraDevice*)malloc(sizeof(WndCameraDevice));
    if (cd != NULL) {
        memset(cd, 0, sizeof(WndCameraDevice));
        cd->header.opaque = cd;
    } else {
        E("%s: Unable to allocate WndCameraDevice instance", __FUNCTION__);
    }
    return cd;
}

/* Uninitializes and frees WndCameraDevice descriptor.
 * Note that upon return from this routine memory allocated for the descriptor
 * will be freed.
 */
static void
_camera_device_free(WndCameraDevice* cd)
{
    if (cd != NULL) {
        if (cd->cap_window != NULL) {
            /* Since connecting to the driver is part of the successful
             * camera initialization, we're safe to assume that driver
             * is connected to the capture window. */
            capDriverDisconnect(cd->cap_window);

            if (cd->dc != NULL) {
                W("%s: Frames should not be capturing at this point",
                  __FUNCTION__);
                ReleaseDC(cd->cap_window, cd->dc);
                cd->dc = NULL;
            }
            /* Destroy the capturing window. */
            DestroyWindow(cd->cap_window);
            cd->cap_window = NULL;
        }
        if (cd->frame_bitmap != NULL) {
            free(cd->frame_bitmap);
        }
        if (cd->window_name != NULL) {
            free(cd->window_name);
        }
        if (cd->framebuffer != NULL) {
            free(cd->framebuffer);
        }
        AFREE(cd);
    } else {
        W("%s: No descriptor", __FUNCTION__);
    }
}

static uint32_t
_camera_device_convertable_format(WndCameraDevice* cd)
{
    if (cd != NULL) {
        switch (cd->header.pixel_format) {
            case BI_RGB:
                switch (cd->frame_bitmap->bmiHeader.biBitCount) {
                    case 24:
                        return V4L2_PIX_FMT_RGB24;
                    default:
                        E("%s: Camera API uses unsupported RGB format RGB%d",
                          __FUNCTION__, cd->frame_bitmap->bmiHeader.biBitCount * 3);
                        return 0;
                }
                break;
            default:
                E("%s: Camera API uses unsupported format %d",
                  __FUNCTION__, cd->header.pixel_format);
                break;
        }
    } else {
        E("%s: No descriptor", __FUNCTION__);
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
    WndCameraDevice* wcd;
    size_t format_info_size;

    /* Allocate descriptor and initialize windows-specific fields. */
    wcd = _camera_device_alloc();
    if (wcd == NULL) {
        E("%s: Unable to allocate WndCameraDevice instance", __FUNCTION__);
        return NULL;
    }
    wcd->window_name = (name != NULL) ? ASTRDUP(name) :
                                        ASTRDUP(_default_window_name);
    if (wcd->window_name == NULL) {
        E("%s: Unable to save window name", __FUNCTION__);
        _camera_device_free(wcd);
        return NULL;
    }
    wcd->input_channel = inp_channel;
    wcd->req_pixel_format = pixel_format;

    /* Create capture window that is a child of HWND_MESSAGE window.
     * We make it invisible, so it doesn't mess with the UI. Also
     * note that we supply standard HWND_MESSAGE window handle as
     * the parent window, since we don't want video capturing
     * machinery to be dependent on the details of our UI. */
    wcd->cap_window = capCreateCaptureWindow(wcd->window_name, WS_CHILD, 0, 0,
                                             0, 0, HWND_MESSAGE, 1);
    if (wcd->cap_window == NULL) {
        E("%s: Unable to create video capturing window: %d",
          __FUNCTION__, GetLastError());
        _camera_device_free(wcd);
        return NULL;
    }

    /* Connect capture window to the video capture driver. */
    if (!capDriverConnect(wcd->cap_window, wcd->input_channel)) {
        /* Unable to connect to a driver. Need to cleanup everything
         * now since we're not going to receive camera_cleanup() call
         * after unsuccessful camera initialization. */
        E("%s: Unable to connect to the video capturing driver #%d: %d",
          __FUNCTION__, wcd->input_channel, GetLastError());
        DestroyWindow(wcd->cap_window);
        wcd->cap_window = NULL;
        _camera_device_free(wcd);
        return NULL;
    }

    /* Get frame information from the driver. */
    format_info_size = capGetVideoFormatSize(wcd->cap_window);
    if (format_info_size == 0) {
        E("%s: Unable to get video format size: %d",
          __FUNCTION__, GetLastError());
        return NULL;
    }
    wcd->frame_bitmap = (BITMAPINFO*)malloc(format_info_size);
    if (wcd->frame_bitmap == NULL) {
        E("%s: Unable to allocate frame bitmap info buffer", __FUNCTION__);
        _camera_device_free(wcd);
        return NULL;
    }
    if (!capGetVideoFormat(wcd->cap_window, wcd->frame_bitmap,
        format_info_size)) {
        E("%s: Unable to obtain video format: %d", __FUNCTION__, GetLastError());
        _camera_device_free(wcd);
        return NULL;
    }

    /* Initialize the common header. */
    wcd->header.width = wcd->frame_bitmap->bmiHeader.biWidth;
    wcd->header.height = wcd->frame_bitmap->bmiHeader.biHeight;
    wcd->header.bpp = wcd->frame_bitmap->bmiHeader.biBitCount;
    wcd->header.pixel_format = wcd->frame_bitmap->bmiHeader.biCompression;
    wcd->header.bpl = (wcd->header.width * wcd->header.bpp) / 8;
    if ((wcd->header.width * wcd->header.bpp) % 8) {
        // TODO: Is this correct to assume that new line in framebuffer is aligned
        // to a byte, or is it alogned to a multiple of bytes occupied by a pixel?
        wcd->header.bpl++;
    }
    wcd->header.framebuffer_size = wcd->header.bpl * wcd->header.height;

    /* Lets see if we have a convertor for the format. */
    wcd->converter = get_format_converted(_camera_device_convertable_format(wcd),
                                          wcd->req_pixel_format);
    if (wcd->converter == NULL) {
        E("%s: No converter available", __FUNCTION__);
        _camera_device_free(wcd);
        return NULL;
    }

    /* Allocate framebuffer. */
    wcd->framebuffer = (uint8_t*)malloc(wcd->header.framebuffer_size);
    if (wcd->framebuffer == NULL) {
        E("%s: Unable to allocate framebuffer", __FUNCTION__);
        _camera_device_free(wcd);
        return NULL;
    }

    return &wcd->header;
}

int
camera_device_start_capturing(CameraDevice* cd)
{
    WndCameraDevice* wcd;
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }
    wcd = (WndCameraDevice*)cd->opaque;

    /* wcd->dc is an indicator of capturin: !NULL - capturing, NULL - not */
    if (wcd->dc != NULL) {
        /* It's already capturing. */
        W("%s: Capturing is already on %s", __FUNCTION__, wcd->window_name);
        return 0;
    }

    /* Get DC for the capturing window that will be used when we deal with
     * bitmaps obtained from the camera device during frame capturing. */
    wcd->dc = GetDC(wcd->cap_window);
    if (wcd->dc == NULL) {
        E("%s: Unable to obtain DC for %s: %d",
          __FUNCTION__, wcd->window_name, GetLastError());
        return -1;
    }

    return 0;
}

int
camera_device_stop_capturing(CameraDevice* cd)
{
    WndCameraDevice* wcd;
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }
    wcd = (WndCameraDevice*)cd->opaque;
    if (wcd->dc == NULL) {
        W("%s: Windows %s is not captuing video", __FUNCTION__, wcd->window_name);
        return 0;
    }
    ReleaseDC(wcd->cap_window, wcd->dc);
    wcd->dc = NULL;

    return 0;
}

int
camera_device_read_frame(CameraDevice* cd, uint8_t* buffer)
{
    WndCameraDevice* wcd;
    /* Bitmap handle taken from the clipboard. */
    HBITMAP bm_handle;
    /* Pitmap placed to the clipboard. */
    BITMAP  bitmap;

    /* Sanity checks. */
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }
    wcd = (WndCameraDevice*)cd->opaque;
    if (wcd->dc == NULL) {
        W("%s: Windows %s is not captuing video",
          __FUNCTION__, wcd->window_name);
        return -1;
    }

    /* Grab a frame, and post it to the clipboard. Not very effective, but this
     * is how capXxx API is operating. */
    if (!capGrabFrameNoStop(wcd->cap_window) ||
        !capEditCopy(wcd->cap_window) ||
        !OpenClipboard(wcd->cap_window)) {
        E("%s: %s: Unable to save frame to the clipboard: %d",
          __FUNCTION__, wcd->window_name, GetLastError());
        return -1;
    }

    /* Get bitmap handle saved into clipboard. Note that bitmap is still
     * owned by the clipboard here! */
    bm_handle = (HBITMAP)GetClipboardData(CF_BITMAP);
    CloseClipboard();
    if (bm_handle == NULL) {
        E("%s: %s: Unable to obtain frame from the clipboard: %d",
          __FUNCTION__, wcd->window_name, GetLastError());
        return -1;
    }

    /* Get bitmap information */
    if (!GetObject(bm_handle, sizeof(BITMAP), &bitmap)) {
        E("%s: %s Unable to obtain frame's bitmap: %d",
          __FUNCTION__, wcd->window_name, GetLastError());
        return -1;
    }

    /* Save bitmap bits to the framebuffer. */
    if (!GetDIBits(wcd->dc, bm_handle, 0, wcd->frame_bitmap->bmiHeader.biHeight,
                   wcd->framebuffer, wcd->frame_bitmap, DIB_RGB_COLORS)) {
        E("%s: %s: Unable to transfer frame to the framebuffer: %d",
          __FUNCTION__, wcd->window_name, GetLastError());
        return -1;
    }

    /* Lets see if conversion is required. */
    if (_camera_device_convertable_format(wcd) == wcd->req_pixel_format) {
        /* Formats match. Just copy framebuffer over. */
        memcpy(buffer, wcd->framebuffer, wcd->header.framebuffer_size);
    } else {
        /* Formats do not match. Use the converter. */
        wcd->converter(wcd->framebuffer, wcd->header.width, wcd->header.height,
                       buffer);
    }

    return 0;
}

void
camera_device_close(CameraDevice* cd)
{
    /* Sanity checks. */
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
    } else {
        WndCameraDevice* wcd = (WndCameraDevice*)cd->opaque;
        _camera_device_free(wcd);
    }
}
