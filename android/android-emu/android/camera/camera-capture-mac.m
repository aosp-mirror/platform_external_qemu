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
 * on Mac. This code uses AVFoundation and Accelerate framework.
 */

#include "android/camera/camera-capture.h"
#include "android/camera/camera-format-converters.h"
#include "android/hw-sensors.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)

#import <Accelerate/Accelerate.h>
#import <AVFoundation/AVFoundation.h>
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>

#include <dispatch/queue.h>

/* Converts CoreVideo pixel format to our internal FOURCC value for
 * conversion to the right guest format. */
static uint32_t
FourCCToInternal(uint32_t cm_pix_format)
{
  switch (cm_pix_format) {
    case kCVPixelFormatType_24RGB:
      D("%s: is kCVPixelFormatType_24RGB\n", __func__);
      return V4L2_PIX_FMT_RGB24;

    case kCVPixelFormatType_24BGR:
      D("%s: is kCVPixelFormatType_24BGR\n", __func__);
      return V4L2_PIX_FMT_BGR32;

    case kCVPixelFormatType_32ARGB:
      D("%s: is kCVPixelFormatType_32ARGB\n", __func__);
      return V4L2_PIX_FMT_ARGB32;
    case kCVPixelFormatType_32RGBA:
      D("%s: is kCVPixelFormatType_32RGBA\n", __func__);
      return V4L2_PIX_FMT_RGB32;

    // TODO: Account for leading A vs trailing A in this as well
    case kCVPixelFormatType_32BGRA:
    case kCVPixelFormatType_32ABGR:
      D("%s: is kCVPixelFormatType_32BGRA/32ABGR\n", __func__);
      return V4L2_PIX_FMT_BGR32;

    case kCVPixelFormatType_422YpCbCr8:
      D("%s: is kCVPixelFormatType_422YpCbCr8\n", __func__);
      return V4L2_PIX_FMT_UYVY;

    case kCVPixelFormatType_420YpCbCr8Planar:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
      D("%s: is kCVPixelFormatType_422YpCbCr8Planar/BiPlanarVideoRange\n", __func__);
      return V4L2_PIX_FMT_YVU420;

    case 'yuvs':  // kCVPixelFormatType_422YpCbCr8_yuvs - undeclared?
      D("%s: is yuvs?????????\n", __func__);
      return V4L2_PIX_FMT_YUYV;

    default:
      E("Unrecognized pixel format '%.4s'", (const char*)&cm_pix_format);
      return 0;
  }
}

/*******************************************************************************
 *                     MacCamera implementation
 ******************************************************************************/

/* Encapsulates a camera device on MacOS */
@interface MacCamera : NSObject<AVCaptureAudioDataOutputSampleBufferDelegate, AVCaptureVideoDataOutputSampleBufferDelegate> {
    /* Capture session. */
    AVCaptureSession*             capture_session;
    /* Capture connection. */
    AVCaptureConnection*          capture_connection;
    /* Camera capture device. */
    /* NOTE: do NOT release capture_device */
    /* Releasing t causes a crash on exit. */
    AVCaptureDevice*              capture_device;
    /* Input device registered with the capture session. */
    AVCaptureDeviceInput*         input_device;
    /* Output device registered with the capture session. */
    AVCaptureVideoDataOutput*  output_device;
    dispatch_queue_t output_queue;
    /* Current framebuffer. */
    CVPixelBufferRef              current_frame;

    /* output dimensions */
    uint32_t                      desired_width;
    uint32_t                      desired_height;

    /* scratch for cropping */
    void*                         cropped_frame_scratch;
    uint32_t                      cropped_frame_width;
    uint32_t                      cropped_frame_height;
    uint32_t                      cropped_frame_bpr;
}

/* Initializes MacCamera instance.
 * Return:
 *  Pointer to initialized instance on success, or nil on failure.
 */
- (MacCamera*)init:(const char*)name;

/* Undoes 'init' */
- (void)free;

/* Starts capturing video frames.
 * Param:
 *  width, height - Requested dimensions for the captured video frames.
 * Return:
 *  0 on success, or !=0 on failure.
 */
- (int)start_capturing:(int)width:(int)height;

/* Captures a frame from the camera device.
 * Param:
 *  result_frame - ClientFrame struct containing an array of framebuffers where
 *                 to convert the frame.
 * Return:
 *  0 on success, or non-zero value on failure. There is a special vaule 1
 *  returned from this routine which indicates that frames are not yet available
 *  in the device. The client should respond to this value by repeating the
 *  read, rather than reporting an error.
 */
- (int)read_frame:
    (ClientFrame*)result_frame:
    (int)fbs_num:
    (float)r_scale:
    (float)g_scale:
    (float)b_scale:
    (float)exp_comp:
    (const char*)direction;

@end

@implementation MacCamera

- (MacCamera*)init:(const char*)name
{
    NSError *error;
    BOOL success;

    /* Obtain the capture device, make sure it's not used by another
     * application, and open it. */
    if (name == nil || *name  == '\0') {
        capture_device =
            [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    }
    else {
        NSString *deviceName = [NSString stringWithFormat:@"%s", name];
        capture_device = [AVCaptureDevice deviceWithUniqueID:deviceName];
    }
    if (capture_device == nil) {
        E("There are no available video devices found.");
        [self release];
        return nil;
    }
    if ([capture_device isInUseByAnotherApplication]) {
        E("Default camera device is in use by another application.");
        capture_device = nil;
        [self release];
        return nil;
    }
    success = false;
    desired_width = 0;
    desired_height = 0;
    cropped_frame_scratch = nil;
    cropped_frame_width = 0;
    cropped_frame_height = 0;
    cropped_frame_bpr = 0;

    /* Create capture session. */
    capture_session = [[AVCaptureSession alloc] init];
    if (capture_session == nil) {
        E("Unable to create capure session.");
        [self free];
        [self release];
        return nil;
    } else {
        D("%s: successfully created capture session\n", __func__);
    }

    /* Create an input device and register it with the capture session. */
    NSError *videoDeviceError = nil;
    input_device = [[AVCaptureDeviceInput alloc] initWithDevice:capture_device error:&videoDeviceError];
    if ([capture_session canAddInput:input_device]) {
        [capture_session addInput:input_device];
        D("%s: successfully added capture input device\n", __func__);
    } else {
        E("%s: cannot add camera capture input device\n", __func__);
        [self release];
        return nil;
    }

    /* Create an output device and register it with the capture session. */
    output_queue = dispatch_queue_create("com.google.goldfish.mac.camera.capture",
                                         DISPATCH_QUEUE_SERIAL );
    output_device = [[AVCaptureVideoDataOutput alloc] init];
    output_device.videoSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32ARGB),
    };
    [output_device setSampleBufferDelegate:self queue:output_queue];
    output_device.alwaysDiscardsLateVideoFrames = YES;

    if ([capture_session canAddOutput:output_device] ) {
        [capture_session addOutput:output_device];
        D("%s: added video out\n", __func__);
    } else {
        D("%s: could not add video out\n", __func__);
        [self release];
        return nil;
    }

    capture_connection = [output_device connectionWithMediaType:AVMediaTypeVideo];

    return self;
}

- (void)free
{
    /* Uninitialize capture session. */
    if (capture_session != nil) {
        /* Make sure that capturing is stopped. */
        if ([capture_session isRunning]) {
            [capture_session stopRunning];
        }
        /* Detach input and output devices from the session. */
        if (input_device != nil) {
            [capture_session removeInput:input_device];
            [input_device release];
            input_device = nil;
        }
        if (output_device != nil) {
            [capture_session removeOutput:output_device];
            [output_device release];
            output_device = nil;
        }
        /* Destroy capture session. */
        [capture_session release];
        capture_session = nil;
    }

    /* Uninitialize capture device. */
    if (capture_device != nil) {
        /* Make sure device is not opened. */
        if ([capture_device isOpen]) {
            [capture_device close];
        }
        capture_device = nil;
    }

    @synchronized (self)
    {
        /* Release current framebuffer. */
        if (current_frame != nil) {
            CVBufferRelease(current_frame);
            current_frame = nil;
        }

        if (cropped_frame_scratch) {
           free(cropped_frame_scratch);
        }

        cropped_frame_scratch = nil;
        cropped_frame_width = 0;
        cropped_frame_height = 0;
    }
}

- (int)start_capturing:(int)width:(int)height
{
    D("%s: call. start capture session, WxH=%dx%d\n", __func__, width, height);

    @synchronized (self) {
        desired_width = width;
        desired_height = height;
    }

    if (![capture_session isRunning]) {
        [capture_session startRunning];
    }
    return 0;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    D("%s: call\n", __func__);

    if ( connection == capture_connection ) {
        @synchronized (self) {

            CVPixelBufferRef to_release;
            CVBufferRetain(sampleBuffer);

            to_release = current_frame;
            current_frame = CMSampleBufferGetImageBuffer( sampleBuffer );

            CVBufferRelease(to_release);

            D("%s: is correct capture connection\n", __func__);
        }
    }
}

- (int)read_frame:
        (ClientFrame*)result_frame:
        (float)r_scale:
        (float)g_scale:
        (float)b_scale:
        (float)exp_comp:
        (const char*)direction {
    /* Frames are pushed by macOS in another thread.
     * So we need a protection here. */
    @synchronized (self)
    {
        if (current_frame != nil) {
            int res;
            uint32_t src_pixel_format;
            uint32_t src_width;
            uint32_t src_height;
            uint32_t src_bpr;
            AndroidCoarseOrientation orientation;

            /* Collect frame info. */
            CVPixelBufferLockBaseAddress(current_frame, 0);
            const void* const src_pixels = CVPixelBufferGetBaseAddress(current_frame);
            if (src_pixels != nil) {
                src_pixel_format = FourCCToInternal(CVPixelBufferGetPixelFormatType(current_frame));
                src_width = CVPixelBufferGetWidth(current_frame);
                src_height = CVPixelBufferGetHeight(current_frame);
                src_bpr = CVPixelBufferGetBytesPerRow(current_frame);
                orientation = get_coarse_orientation();

                {
                    uint32_t desired_width1;
                    uint32_t desired_height1;

                    switch (orientation) {
                    case ANDROID_COARSE_PORTRAIT:
                    case ANDROID_COARSE_REVERSE_PORTRAIT:
                        desired_width1 = desired_height;
                        desired_height1 = desired_width;
                        break;
                    default:
                        desired_width1 = desired_width;
                        desired_height1 = desired_height;
                        break;
                    }

                    if (cropped_frame_height != desired_height1 || cropped_frame_width != desired_width1 || !cropped_frame_scratch) {
                        cropped_frame_width = desired_width1;
                        cropped_frame_height = desired_height1;
                        cropped_frame_bpr = cropped_frame_width * sizeof(uint32_t);
                        cropped_frame_scratch = realloc(cropped_frame_scratch, cropped_frame_bpr * cropped_frame_height);
                    }
                }

                const uint32_t effective_crop_width = MIN(cropped_frame_width, src_width);
                const uint32_t effective_crop_bpr = effective_crop_width * sizeof(uint32_t);
                const uint32_t effective_crop_height = MIN(cropped_frame_height, src_height);
                const uint32_t half_blank_rows = (cropped_frame_height - effective_crop_height) / 2;
                const uint32_t half_blank_col_pixels = (cropped_frame_width - effective_crop_width) / 2 * sizeof(uint32_t);

                const uint8_t* src_pixels8 = (const uint8_t*)src_pixels +
                        (src_height - effective_crop_height) * src_bpr +
                        (src_width - effective_crop_width) / 2 * sizeof(uint32_t);
                uint8_t* dst_pixels8 = (const uint8_t*)cropped_frame_scratch +
                        half_blank_rows * cropped_frame_bpr;

                memset(cropped_frame_scratch, 0, half_blank_rows * cropped_frame_bpr);
                for (unsigned n = effective_crop_height; n > 0; --n, src_pixels8 += src_bpr) {
                    memset(dst_pixels8, 0, half_blank_col_pixels);
                    dst_pixels8 += half_blank_col_pixels;
                    memcpy(dst_pixels8, src_pixels8, effective_crop_bpr);
                    dst_pixels8 += effective_crop_bpr;
                    memset(dst_pixels8, 0, half_blank_col_pixels);
                    dst_pixels8 += half_blank_col_pixels;
                }
                memset(dst_pixels8, 0, half_blank_rows * cropped_frame_bpr);

                res = 0;
            } else {
                res = -1;
            }

            CVPixelBufferUnlockBaseAddress(current_frame, 0);

            if (!res) {
                res = convert_frame(cropped_frame_scratch, src_pixel_format,
                           cropped_frame_bpr * cropped_frame_height,
                           cropped_frame_width, cropped_frame_height, result_frame,
                           r_scale, g_scale, b_scale, exp_comp, direction, orientation);
            }

            return res;
        } else {
            /* First frame didn't come in just yet. Let the caller repeat. */
            return 1;
        }
    }
}

@end
/*******************************************************************************
 *                     CameraDevice routines
 ******************************************************************************/

typedef struct MacCameraDevice MacCameraDevice;
/* MacOS-specific camera device descriptor. */
struct MacCameraDevice {
    /* Common camera device descriptor. */
    CameraDevice  header;
    /* Actual camera device object. */
    MacCamera*    device;
    char* device_name;
    int input_channel;
    int started;
    int frame_width;
    int frame_height;
};

/* Allocates an instance of MacCameraDevice structure.
 * Return:
 *  Allocated instance of MacCameraDevice structure. Note that this routine
 *  also sets 'opaque' field in the 'header' structure to point back to the
 *  containing MacCameraDevice instance.
 */
static MacCameraDevice*
_camera_device_alloc(void)
{
    MacCameraDevice* cd = (MacCameraDevice*)malloc(sizeof(MacCameraDevice));
    if (cd != NULL) {
        memset(cd, 0, sizeof(MacCameraDevice));
        cd->header.opaque = cd;
    } else {
        E("%s: Unable to allocate MacCameraDevice instance", __FUNCTION__);
    }
    return cd;
}

/* Uninitializes and frees MacCameraDevice descriptor.
 * Note that upon return from this routine memory allocated for the descriptor
 * will be freed.
 */
static void
_camera_device_free(MacCameraDevice* cd)
{
    if (cd != NULL) {
        if (cd->device != NULL) {
            [cd->device release];
            cd->device = nil;
        }
        if (cd->device_name != nil) {
            free(cd->device_name);
        }
        AFREE(cd);
    } else {
        W("%s: No descriptor", __FUNCTION__);
    }
}

/* Resets camera device after capturing.
 * Since new capture request may require different frame dimensions we must
 * reset frame info cached in the capture window. The only way to do that would
 * be closing, and reopening it again. */
static void
_camera_device_reset(MacCameraDevice* cd)
{
    if (cd != NULL && cd->device) {
        [cd->device free];
        cd->device = [cd->device init:cd->device_name];
        cd->started = 0;
    }
}

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

CameraDevice*
camera_device_open(const char* name, int inp_channel)
{
    MacCameraDevice* mcd;

    mcd = _camera_device_alloc();
    if (mcd == NULL) {
        E("%s: Unable to allocate MacCameraDevice instance", __FUNCTION__);
        return NULL;
    }
    mcd->device_name = ASTRDUP(name);
    mcd->device = [[MacCamera alloc] init:name];
    if (mcd->device == nil) {
        E("%s: Unable to initialize camera device.", __FUNCTION__);
        _camera_device_free(mcd);
        return NULL;
    } else {
        D("%s: successfuly initialized camera device\n", __func__);
    }
    return &mcd->header;
}

int
camera_device_start_capturing(CameraDevice* cd,
                              uint32_t pixel_format,
                              int frame_width,
                              int frame_height)
{
    MacCameraDevice* mcd;

    /* Sanity checks. */
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }
    mcd = (MacCameraDevice*)cd->opaque;
    if (mcd->device == nil) {
        E("%s: Camera device is not opened", __FUNCTION__);
        return -1;
    }

    D("%s: w h %d %d\n", __func__, frame_width, frame_height);

    mcd->frame_width = frame_width;
    mcd->frame_height = frame_height;
    return 0;
}

int
camera_device_stop_capturing(CameraDevice* cd)
{
    MacCameraDevice* mcd;

    /* Sanity checks. */
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }
    mcd = (MacCameraDevice*)cd->opaque;
    if (mcd->device == nil) {
        E("%s: Camera device is not opened", __FUNCTION__);
        return -1;
    }

    /* Reset capture settings, so next call to capture can set its own. */
    _camera_device_reset(mcd);

    return 0;
}

int camera_device_read_frame(CameraDevice* cd,
                             ClientFrame* result_frame,
                             float r_scale,
                             float g_scale,
                             float b_scale,
                             float exp_comp,
                             const char* direction) {
    MacCameraDevice* mcd;

    /* Sanity checks. */
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }
    mcd = (MacCameraDevice*)cd->opaque;
    if (mcd->device == nil) {
        E("%s: Camera device is not opened", __FUNCTION__);
        return -1;
    }

    if (!mcd->started) {
        int result = [mcd->device start_capturing:mcd->frame_width:mcd->frame_height];
        if (result) return -1;
        mcd->started = 1;
    }
    return [mcd->device read_frame:
                      result_frame:
                           r_scale:
                           g_scale:
                           b_scale:
                           exp_comp:
                           direction];
}

void
camera_device_close(CameraDevice* cd)
{
    /* Sanity checks. */
    if (cd == NULL || cd->opaque == NULL) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
    } else {
        _camera_device_free((MacCameraDevice*)cd->opaque);
    }
}

int camera_enumerate_devices(CameraInfo* cis, int max) {
    /* Array containing emulated webcam frame dimensions
     * expected by framework. */
    static const CameraFrameDim _emulate_dims[] = {
            /* Emulates 640x480 frame. */
            {640, 480},
            /* Emulates 352x288 frame (required by camera framework). */
            {352, 288},
            /* Emulates 320x240 frame (required by camera framework). */
            {320, 240},
            /* Emulates 176x144 frame (required by camera framework). */
            {176, 144},
            /* Emulates 1280x720 frame (required by camera framework). */
            {1280, 720},
            /* Emulates 1280x960 frame. */
            {1280, 960}};

    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    if (videoDevices == nil) {
        E("No web cameras are connected to the host.");
        return 0;
    }
    int found = 0;
    for (AVCaptureDevice *videoDevice in videoDevices) {
        for(AVCaptureDeviceFormat *vFormat in [videoDevice formats]) {
            CMFormatDescriptionRef description= vFormat.formatDescription;
            cis[found].pixel_format = FourCCToInternal(CMFormatDescriptionGetMediaSubType(description));
            if (cis[found].pixel_format) break;
        }
        if (cis[found].pixel_format == 0) {
            /* Unsupported pixel format. */
            E("Pixel format reported by the camera device is unsupported");
            continue;
        }

        /* Initialize camera info structure. */
        cis[found].frame_sizes = (CameraFrameDim*)malloc(sizeof(_emulate_dims));
        if (cis[found].frame_sizes != NULL) {
            char user_name[24];
            char *device_name = [[videoDevice uniqueID] UTF8String];
            snprintf(user_name, sizeof(user_name), "webcam%d", found);
            cis[found].frame_sizes_num = sizeof(_emulate_dims) / sizeof(*_emulate_dims);
            memcpy(cis[found].frame_sizes, _emulate_dims, sizeof(_emulate_dims));
            cis[found].device_name = ASTRDUP(device_name);
            cis[found].inp_channel = 0;
            cis[found].display_name = ASTRDUP(user_name);
            cis[found].in_use = 0;
            found++;
            if (found == max) {
                W("Number of Cameras exceeds max limit %d", max);
                return found;
            }
            D("Found Video Device %s id %s for %s", [[videoDevice manufacturer] UTF8String], device_name, user_name);
        } else {
            E("Unable to allocate memory for camera information.");
            return 0;
        }
    }
    return found;
}
