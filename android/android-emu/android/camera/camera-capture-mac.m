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
    /* Desired frame width */
    int                           desired_width;
    /* Desired frame height */
    int                           desired_height;
    /* Scratch buffer to hold imgs of desired width and height. */
    void*                         desired_scaled_frame;
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
- (int)read_frame:(ClientFrame*)
     result_frame:(int)
          fbs_num:(float)
          r_scale:(float)
          g_scale:(float)
          b_scale:(float)exp_comp;

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
    desired_scaled_frame = nil;

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

        if (desired_scaled_frame != nil) {
            free(desired_scaled_frame);
            desired_scaled_frame = nil;
        }
    }
}

- (int)start_capturing:(int)width:(int)height
{
    D("%s: call. start capture session\n", __func__);

    @synchronized (self) {
        current_frame = nil;
        desired_width = width;
        desired_height = height;
        desired_scaled_frame = realloc(desired_scaled_frame, 4 * desired_width * desired_height);
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

- (int)read_frame:(ClientFrame*)
     result_frame:(float)
          r_scale:(float)
          g_scale:(float)
          b_scale:(float)exp_comp {
    int res = -1;


    const void* pixels = nil;
    vImage_Buffer scale_output;
    uint32_t pixel_format;
    int frame_width, frame_height, srcBpr, srcBpp, frame_size;

    /* Frames are pushed by macOS in another thread.
     * So we need a protection here. */
    @synchronized (self)
    {
        if (current_frame != nil) {
            /* Collect frame info. */
            pixel_format =
                FourCCToInternal(CVPixelBufferGetPixelFormatType(current_frame));
            frame_width = CVPixelBufferGetWidth(current_frame);
            frame_height = CVPixelBufferGetHeight(current_frame);
            srcBpr = CVPixelBufferGetBytesPerRow(current_frame);
            srcBpp = 4;
            frame_size = srcBpr * frame_height;

            D("%s: w h %d %d bytes %zu\n", __func__, frame_width, frame_height, frame_size);

            /* Get framebuffer pointer. */
            D("%s: lock base addr\n", __func__);
            CVPixelBufferLockBaseAddress(current_frame, 0);

            D("%s: get pixels base addr\n", __func__);
            pixels = CVPixelBufferGetBaseAddress(current_frame);

            if (pixels != nil) {
                D("%s: convert frame\n", __func__);

                // AVFoundation doesn't provide pre-scaled output.
                // Scale it here, using the Accelerate framework.
                // Also needs to be the correct aspect ratio,
                // so do a centered crop so that aspect ratios match.
                float currAspect = ((float)frame_width) / ((float)frame_height);
                float desiredAspect = ((float)desired_width) / ((float)desired_height);
                bool shrinkHoriz = false;
                int cropX0, cropY0, cropW, cropH;
                uintptr_t start = 0;

                shrinkHoriz = desiredAspect < currAspect;

                if (shrinkHoriz) {
                    cropW = (desiredAspect / currAspect) * frame_width;
                    cropH = frame_height;

                    cropX0 = (frame_width - cropW) / 2;
                    cropY0 = 0;

                    start = srcBpp * cropX0;
                } else {
                    cropW = frame_width;
                    cropH = (currAspect / desiredAspect) * frame_height;

                    cropX0 = 0;
                    cropY0 = (frame_height - cropH) / 2;

                    start = srcBpr * cropY0;
                }

                vImage_Buffer scale_input;
                scale_input.width = cropW;
                scale_input.height = cropH;
                scale_input.rowBytes = srcBpr;
                scale_input.data = ((char*)pixels) + start;

                const int dstBpp = 4; // ARGB8888
                scale_output.width = desired_width;
                scale_output.height = desired_height;
                scale_output.rowBytes = dstBpp * desired_width;
                scale_output.data = desired_scaled_frame;

                D("%s:  image scale %d %d -> %d %d\n",
                        __func__, frame_width, frame_height, desired_width, desired_height);

                vImage_Error err = vImageScale_ARGB8888(&scale_input, &scale_output, NULL, 0);
                if (err != kvImageNoError) {
                    E("%s: error in scale: 0x%x\n", __func__, err);
                }
                CVPixelBufferUnlockBaseAddress(current_frame, 0);
                // unlock everything early here because we have already copied to
                // the rescaled buffer.
            }

        } else {
            /* First frame didn't come in just yet. Let the caller repeat. */
            res = 1;
        }

    }

    if (pixels != nil) {
        /* Convert framebuffer. */
        res = convert_frame(scale_output.data, pixel_format,
                            scale_output.rowBytes * scale_output.height,
                            desired_width, desired_height, result_frame,
                            r_scale, g_scale, b_scale, exp_comp);
    } else if (current_frame != nil) {
        E("%s: Unable to obtain framebuffer", __func__);
        res = -1;
    }

    return res;
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
                             float exp_comp) {
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
                           b_scale:exp_comp];
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
