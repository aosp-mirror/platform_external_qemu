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
 * on Mac. This code uses QTKit API to work with camera devices, and requires
 * Mac OS at least 10.5
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

/* Converts internal QT pixel format to a FOURCC value. */
static uint32_t
FourCCToInternal(uint32_t cm_pix_format)
{
  switch (cm_pix_format) {
    case kCVPixelFormatType_24RGB:
      fprintf(stderr, "%s: is kCVPixelFormatType_24RGB\n", __func__);
      return V4L2_PIX_FMT_RGB24;

    case kCVPixelFormatType_24BGR:
      fprintf(stderr, "%s: is kCVPixelFormatType_24BGR\n", __func__);
      return V4L2_PIX_FMT_BGR32;

    case kCVPixelFormatType_32ARGB:
      fprintf(stderr, "%s: is kCVPixelFormatType_32ARGB/32RGBA\n", __func__);
      return V4L2_PIX_FMT_ARGB32;
    case kCVPixelFormatType_32RGBA:
      fprintf(stderr, "%s: is kCVPixelFormatType_32ARGB/32RGBA\n", __func__);
      return V4L2_PIX_FMT_RGB32;

    case kCVPixelFormatType_32BGRA:
    case kCVPixelFormatType_32ABGR:
      fprintf(stderr, "%s: is kCVPixelFormatType_32BGRA/32ABGR\n", __func__);
      return V4L2_PIX_FMT_BGR32;

    case kCVPixelFormatType_422YpCbCr8:
      fprintf(stderr, "%s: is kCVPixelFormatType_422YpCbCr8\n", __func__);
      return V4L2_PIX_FMT_UYVY;

    case kCVPixelFormatType_420YpCbCr8Planar:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
      fprintf(stderr, "%s: is kCVPixelFormatType_422YpCbCr8Planar/BiPlanarVideoRange\n", __func__);
      return V4L2_PIX_FMT_YVU420;

    case 'yuvs':  // kCVPixelFormatType_422YpCbCr8_yuvs - undeclared?
      fprintf(stderr, "%s: is yuvs?????????\n", __func__);
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
    AVCaptureConnection*             capture_connection;
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
    void*                         frame_desired_scale;
}

/* Initializes MacCamera instance.
 * Return:
 *  Pointer to initialized instance on success, or nil on failure.
 */
- (MacCamera*)init;

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
 *  framebuffers - Array of framebuffers where to read the frame. Size of this
 *      array is defined by the 'fbs_num' parameter. Note that the caller must
 *      make sure that buffers are large enough to contain entire frame captured
 *      from the device.
 *  fbs_num - Number of entries in the 'framebuffers' array.
 * Return:
 *  0 on success, or non-zero value on failure. There is a special vaule 1
 *  returned from this routine which indicates that frames are not yet available
 *  in the device. The client should respond to this value by repeating the
 *  read, rather than reporting an error.
 */
- (int)read_frame:(ClientFrameBuffer*)framebuffers:(int)fbs_num:(float)r_scale:(float)g_scale:(float)b_scale:(float)exp_comp;

@end

@implementation MacCamera

- (MacCamera*)init
{
    NSError *error;
    BOOL success;

    /* Obtain the capture device, make sure it's not used by another
     * application, and open it. */
    capture_device =
        [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
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

    /* Create capture session. */
    capture_session = [[AVCaptureSession alloc] init];
    if (capture_session == nil) {
        E("Unable to create capure session.");
        [self free];
        [self release];
        return nil;
    } else {
        fprintf(stderr, "%s: successfully created capture session\n", __func__);

    }

	capture_session.sessionPreset = AVCaptureSessionPreset640x480;

    /* Create an input device and register it with the capture session. */
    NSError *videoDeviceError = nil;
    input_device = [[AVCaptureDeviceInput alloc] initWithDevice:capture_device error:&videoDeviceError];
    if ([capture_session canAddInput:input_device]) {
        [capture_session addInput:input_device];
        fprintf(stderr, "%s: successfully added capture input dev\n", __func__);
    } else {
        fprintf(stderr, "%s: ERROR cannot added capture input dev\n", __func__);
    }

    /* Create an output device and register it with the capture session. */
    output_queue = dispatch_queue_create( "com.apple.sample.capturepipeline.video", DISPATCH_QUEUE_SERIAL );
    output_device = [[AVCaptureVideoDataOutput alloc] init];
    output_device.videoSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32ARGB),
    };
    [output_device setSampleBufferDelegate:self queue:output_queue];
    output_device.alwaysDiscardsLateVideoFrames = YES;

    if ([capture_session canAddOutput:output_device] ) {
        [capture_session addOutput:output_device];
        fprintf(stderr, "%s: added video out\n", __func__);
    } else {
        fprintf(stderr, "%s: ERROR: could not add video out\n", __func__);
    }

    capture_connection = [output_device connectionWithMediaType:AVMediaTypeVideo];

    return self;
}

- (void)free
{
    // /* Uninitialize capture session. */
    // if (capture_session != nil) {
    //     /* Make sure that capturing is stopped. */
    //     if ([capture_session isRunning]) {
    //         [capture_session stopRunning];
    //     }
    //     /* Detach input and output devices from the session. */
    //     if (input_device != nil) {
    //         [capture_session removeInput:input_device];
    //         [input_device release];
    //         input_device = nil;
    //     }
    //     if (output_device != nil) {
    //         [capture_session removeOutput:output_device];
    //         [output_device release];
    //         output_device = nil;
    //     }
    //     /* Destroy capture session. */
    //     [capture_session release];
    //     capture_session = nil;
    // }

    // /* Uninitialize capture device. */
    // if (capture_device != nil) {
    //     /* Make sure device is not opened. */
    //     if ([capture_device isOpen]) {
    //         [capture_device close];
    //     }
    //     capture_device = nil;
    // }

    // @synchronized (self)
    // {
    //     /* Release current framebuffer. */
    //     if (current_frame != nil) {
    //         CVBufferRelease(current_frame);
    //         current_frame = nil;
    //     }
    // }
}

- (int)start_capturing:(int)width:(int)height
{
    fprintf(stderr, "%s: call. start capture session\n", __func__);
    current_frame = nil;

    desired_width = width;
    desired_height = height;
    frame_desired_scale = realloc(frame_desired_scale, 4 * desired_width * desired_height);
    [capture_session startRunning];

    return 0;
  // if (![capture_session isRunning]) {
  //       /* Set desired frame dimensions. */
  //       desired_width = width;
  //       desired_height = height;
  //       [output_device setPixelBufferAttributes:
  //         [NSDictionary dictionaryWithObjectsAndKeys:
  //             [NSNumber numberWithInt: width], kCVPixelBufferWidthKey,
  //             [NSNumber numberWithInt: height], kCVPixelBufferHeightKey,
  //             nil]];
  //       [capture_session startRunning];
  //       return 0;
  // } else if (width == desired_width && height == desired_height) {
  //     W("%s: Already capturing %dx%d frames",
  //       __FUNCTION__, desired_width, desired_height);
  //     return -1;
  // } else {
  //     E("%s: Already capturing %dx%d frames. Requested frame dimensions are %dx%d",
  //       __FUNCTION__, desired_width, desired_height, width, height);
  //     return -1;
  // }
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    fprintf(stderr, "%s: call\n", __func__);
    if ( connection == capture_connection ) {
        @synchronized (self) {

            CVPixelBufferRef to_release;
            CVBufferRetain(sampleBuffer);

            to_release = current_frame;
            current_frame = CMSampleBufferGetImageBuffer( sampleBuffer );

            CVBufferRelease(to_release);

            fprintf(stderr, "%s: is correct capture connection\n", __func__);
        }
    }
}

- (int)read_frame:(ClientFrameBuffer*)framebuffers:(int)fbs_num:(float)r_scale:(float)g_scale:(float)b_scale:(float)exp_comp
{
    int res = -1;

    /* Frames are pushed by QT in another thread.
     * So we need a protection here. */
    @synchronized (self)
    {
        if (current_frame != nil) {
            /* Collect frame info. */
            const uint32_t pixel_format =
                FourCCToInternal(CVPixelBufferGetPixelFormatType(current_frame));
            const int frame_width = CVPixelBufferGetWidth(current_frame);
            const int frame_height = CVPixelBufferGetHeight(current_frame);
            const int bpr = CVPixelBufferGetBytesPerRow(current_frame);
            const size_t frame_size = bpr * frame_height;


            fprintf(stderr, "%s: w h %d %d bytes %zu\n", __func__, frame_width, frame_height, frame_size);

            /* Get framebuffer pointer. */
            fprintf(stderr, "%s: lock base addr\n", __func__);
            CVPixelBufferLockBaseAddress(current_frame, 0);
                fprintf(stderr, "%s: get pixels base addr\n", __func__);
            const void* pixels = CVPixelBufferGetBaseAddress(current_frame);
            if (pixels != nil) {
            fprintf(stderr, "%s: convert frame\n", __func__);

            vImage_Buffer scale_input;
            scale_input.width = frame_width;
            scale_input.height = frame_height;
            scale_input.rowBytes = bpr;
            scale_input.data = pixels;

            vImage_Buffer scale_output;
            scale_output.width = desired_width;
            scale_output.height = desired_height;
            scale_output.rowBytes = 4 * desired_width;
            scale_output.data = frame_desired_scale;

            fprintf(stderr, "%s: calling image scale %d %d -> %d %d\n", __func__,
                    frame_width, frame_height,
                    desired_width, desired_height);
            vImage_Error err = vImageScale_ARGB8888(&scale_input, &scale_output, NULL, 0);
            if (err != kvImageNoError) fprintf(stderr, "%s: error in scale: 0x%x\n", __func__, err);

                /* Convert framebuffer. */
                res = convert_frame(scale_output.data, pixel_format, scale_output.rowBytes * scale_output.height,
                                    desired_width, desired_height,
                                    framebuffers, fbs_num,
                                    r_scale, g_scale, b_scale, exp_comp);
            } else {
                E("%s: Unable to obtain framebuffer", __FUNCTION__);
                res = -1;
            }
            CVPixelBufferUnlockBaseAddress(current_frame, 0);
        } else {
            /* First frame didn't come in just yet. Let the caller repeat. */
            res = 1;
        }
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
            [cd->device free];
            [cd->device release];
            cd->device = nil;
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
        cd->device = [cd->device init];
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
    mcd->device = [[MacCamera alloc] init];
    if (mcd->device == nil) {
        E("%s: Unable to initialize camera device.", __FUNCTION__);
        return NULL;
    } else {
        fprintf(stderr, "%s: successfuly initialized camera device\n", __func__);
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

    fprintf(stderr, "%s: w h %d %d\n", __func__, frame_width, frame_height);

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

int
camera_device_read_frame(CameraDevice* cd,
                         ClientFrameBuffer* framebuffers,
                         int fbs_num,
                         float r_scale,
                         float g_scale,
                         float b_scale,
                         float exp_comp)
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

    if (!mcd->started) {
        int result = [mcd->device start_capturing:mcd->frame_width:mcd->frame_height];
        if (result) return -1;
        mcd->started = 1;
    }
    return [mcd->device read_frame:framebuffers:fbs_num:r_scale:g_scale:b_scale:exp_comp];
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
    /* Array containing emulated webcam frame dimensions.
     * QT API provides device independent frame dimensions, by scaling frames
     * received from the device to whatever dimensions were requested for the
     * output device. So, we can just use a small set of frame dimensions to
     * emulate.
     */
    static const CameraFrameDim _emulate_dims[] = {
            /* Emulates 640x480 frame. */
            {640, 480},
            /* Emulates 352x288 frame (required by camera framework). */
            {352, 288},
            /* Emulates 320x240 frame (required by camera framework). */
            {320, 240},
            /* Emulates 176x144 frame (required by camera framework). */
            {176, 144}};

    // /* Obtain default video device. QT API doesn't really provide a reliable
    //  * way to identify camera devices. There is a AVCaptureDevice::uniqueId
    //  * method that supposedly does that, but in some cases it just doesn't
    //  * work. Until we figure out a reliable device identification, we will
    //  * stick to using only one (default) camera for emulation. */
    AVCaptureDevice* video_dev =
        [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if (video_dev == nil) {
        E("No web cameras are connected to the host.");
        return 0;
    }

    // /* Obtain pixel format for the device. */
    // NSArray* pix_formats = [video_dev formatDescriptions];
    // if (pix_formats == nil || [pix_formats count] == 0) {
    //     E("Unable to obtain pixel format for the default camera device.");
    //     [video_dev release];
    //     video_dev = nil;
    //     return 0;
    // }
    // const uint32_t qt_pix_format = [[pix_formats objectAtIndex:0] formatType];
    // [pix_formats release];

    // /* Obtain FOURCC pixel format for the device. */
    // cis[0].pixel_format = _QTtoFOURCC(qt_pix_format);
    // if (cis[0].pixel_format == 0) {
    AVCaptureDevice *videoDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    for(AVCaptureDeviceFormat *vFormat in [videoDevice formats]) {
        CMFormatDescriptionRef description= vFormat.formatDescription;
        cis[0].pixel_format = FourCCToInternal(CMFormatDescriptionGetMediaSubType(description));
        if (cis[0].pixel_format) break;
    }
    if (cis[0].pixel_format == 0) {
        /* Unsupported pixel format. */
        E("Pixel format reported by the camera device is unsupported");
        [video_dev release];
        video_dev = nil;
        return 0;
    }

    /* Initialize camera info structure. */
    cis[0].frame_sizes = (CameraFrameDim*)malloc(sizeof(_emulate_dims));
    if (cis[0].frame_sizes != NULL) {
        cis[0].frame_sizes_num = sizeof(_emulate_dims) / sizeof(*_emulate_dims);
        memcpy(cis[0].frame_sizes, _emulate_dims, sizeof(_emulate_dims));
        cis[0].device_name = ASTRDUP("webcam0");
        cis[0].inp_channel = 0;
        cis[0].display_name = ASTRDUP("webcam0");
        cis[0].in_use = 0;
        [video_dev release];
        video_dev = nil;
        return 1;
    } else {
        E("Unable to allocate memory for camera information.");
        [video_dev release];
        video_dev = nil;
        return 0;
    }
}
