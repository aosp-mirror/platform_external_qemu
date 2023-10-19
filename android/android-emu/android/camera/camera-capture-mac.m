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
#include <dispatch/dispatch.h>
#include <os/lock.h>

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

    case kCVPixelFormatType_422YpCbCr8_yuvs:
      D("%s: is kCVPixelFormatType_422YpCbCr8_yuvs\n", __func__);
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

typedef enum {
  kMacCameraForward,
  kMacCameraBackward
} MacCameraDirection;

/* Encapsulates a camera device on MacOS */
@interface MacCamera : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate> {
  AVCaptureSession *captureSession_;
  int desiredWidth_;
  int desiredHeight_;
  MacCameraDirection cameraDirection_;

  // rotateInputBuffer_, scaleInputBuffer_ and scaleOutputTempBuffer_ are
  // guarded by outputQueue_.
  dispatch_queue_t outputQueue_;
  vImage_Buffer rotateInputBuffer_;
  vImage_Buffer scaleInputBuffer_;
  void *scaleOutputTempBuffer_;

  // scaleOutputBuffer_ is guarded byscaleOutputBufferLock_ and is used both
  // on the outputQueue_ and accessed from the hypervisor thread.
  os_unfair_lock scaleOutputBufferLock_;
  vImage_Buffer scaleOutputBuffer_;
}

/* Initializes MacCamera instance.
 * Return:
 *  Pointer to initialized instance on success, or nil on failure.
 */
- (instancetype)initWithName:(const char*)name NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
- (void)dealloc;

/* Starts capturing video frames.
 * Param:
 *  width, height - Requested dimensions for the captured video frames.
 * Return:
 *  0 on success, or !=0 on failure.
 */
- (int)startCapturingWidth:(int)width
                    height:(int)height
                 direction:(const char *)direction;

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
- (int)readFrame:(ClientFrame*)resultFrame
          rScale:(float)rScale
          gScale:(float)gScale
          bScale:(float)bScale
         expComp:(float)expComp;

@end

@implementation MacCamera

- (instancetype)initWithName:(const char*)name {
  if (!(self = [super init])) {
    return nil;
  }
  scaleOutputBufferLock_ = OS_UNFAIR_LOCK_INIT;
  AVCaptureDevice *captureDevice;

  /* Obtain the capture device, make sure it's not used by another
   * application, and open it. */
  if (name == NULL || *name  == '\0') {
    captureDevice =
        [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  } else {
    NSString *deviceName = [NSString stringWithFormat:@"%s", name];
    captureDevice = [AVCaptureDevice deviceWithUniqueID:deviceName];
  }
  if (!captureDevice) {
    E("There are no available video devices found.");
    [self release];
    return nil;
  }
  if ([captureDevice isInUseByAnotherApplication]) {
    E("Default camera device is in use by another application.");
    [self release];
    return nil;
  }

  /* Create capture session. */
  captureSession_ = [[AVCaptureSession alloc] init];
  if (!captureSession_) {
    E("Unable to create capure session.");
    [self release];
    return nil;
  } else {
    D("%s: successfully created capture session\n", __func__);
  }

  /* Create an input device and register it with the capture session. */
  NSError *videoDeviceError;
  AVCaptureDeviceInput *input_device =
      [[[AVCaptureDeviceInput alloc] initWithDevice:captureDevice
                                              error:&videoDeviceError]
       autorelease];
  if ([captureSession_ canAddInput:input_device]) {
    [captureSession_ addInput:input_device];
    D("%s: successfully added capture input device\n", __func__);
  } else {
    E("%s: cannot add camera capture input device (%s)\n", __func__,
      [[videoDeviceError localizedDescription] UTF8String]);
    [self release];
    return nil;
  }

  /* Create an output device and register it with the capture session. */
  outputQueue_ = dispatch_queue_create("com.google.goldfish.mac.camera.capture",
                                       DISPATCH_QUEUE_SERIAL);
  AVCaptureVideoDataOutput *outputDevice =
      [[[AVCaptureVideoDataOutput alloc] init] autorelease];
  outputDevice.videoSettings = @{
    (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32ARGB),
  };
  [outputDevice setSampleBufferDelegate:self queue:outputQueue_];
  outputDevice.alwaysDiscardsLateVideoFrames = YES;

  if ([captureSession_ canAddOutput:outputDevice] ) {
    [captureSession_ addOutput:outputDevice];
    D("%s: added video out\n", __func__);
  } else {
    D("%s: could not add video out\n", __func__);
    [self release];
    return nil;
  }
  return self;
}

- (void)dealloc {
  [captureSession_ stopRunning];
  [captureSession_ release];
  dispatch_release(outputQueue_);
  free(rotateInputBuffer_.data);
  free(scaleInputBuffer_.data);
  free(scaleOutputBuffer_.data);
  free(scaleOutputTempBuffer_);
  [super dealloc];
}

- (int)startCapturingWidth:(int)width
                    height:(int)height
                 direction:(const char*)direction {
  D("%s: call. start capture session\n", __func__);
  desiredWidth_ = width;
  desiredHeight_ = height;

  cameraDirection_ = (strcmp("back", direction) == 0
                      ? kMacCameraBackward : kMacCameraForward);

  if (![captureSession_ isRunning]) {
    [captureSession_ startRunning];
  }
  return 0;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection {
  D("%s: call\n", __func__);
  // We do all of this work in this method instead of in
  // readFrame:rScale:gScale:bScale:expComp: because there is an issue in
  // macOS 14.0 (23A344) where vImage functions can't be run on a hypervisor
  // thread.
  // FB13254074.
  CVImageBufferRef workingFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
  static vImage_CGImageFormat desiredFormat = {
    .bitsPerComponent = 8,
    .bitsPerPixel = 32,
    .colorSpace = nil,
    .bitmapInfo =
    (CGBitmapInfo)(kCGImageByteOrder32Big | kCGImageAlphaNoneSkipFirst),
    .version = 0,
    .decode = nil,
    .renderingIntent = kCGRenderingIntentDefault
  };

  AndroidCoarseOrientation orientation = get_coarse_orientation();
  vImage_Error error;
  if (orientation == ANDROID_COARSE_PORTRAIT ||
      orientation == ANDROID_COARSE_REVERSE_PORTRAIT) {
    vImage_Flags flags =
        rotateInputBuffer_.data != NULL ? kvImageNoAllocate : kvImageNoFlags;
    error = vImageBuffer_InitWithCVPixelBuffer(&rotateInputBuffer_,
                                               &desiredFormat, workingFrame,
                                               nil, nil, flags);
    if (error != kvImageNoError) {
      E("%s: error in allocate rotateInputBuffer_: %ld\n", __func__, error);
      return;
    }
    if (scaleInputBuffer_.data &&
        (scaleInputBuffer_.width != rotateInputBuffer_.height ||
         scaleInputBuffer_.height != rotateInputBuffer_.width)) {
      // Sizes have changed. Rebuild our buffers.
      free(scaleInputBuffer_.data);
      scaleInputBuffer_.data = NULL;
    }
    if (!scaleInputBuffer_.data) {
      error = vImageBuffer_Init(&scaleInputBuffer_, rotateInputBuffer_.width,
                                rotateInputBuffer_.height, 32, kvImageNoFlags);
      if (error != kvImageNoError) {
        E("%s: error in allocate scaleInputBuffer_: %ld\n", __func__, error);
        return;
      }
    }
    const Pixel_8888 backColor = {0, 0, 0, 0};
    int rotation = (cameraDirection_ ==  kMacCameraBackward ?
                    (1 + orientation) % 4 :
                    (5 - orientation) % 4);
    error = vImageRotate90_ARGB8888(&rotateInputBuffer_, &scaleInputBuffer_,
                                    rotation, backColor, kvImageNoFlags);
    if (error != kvImageNoError) {
      E("%s: error in rotate: %ld\n", __func__, error);
      return;
    }
  } else {
    if (scaleInputBuffer_.data &&
        (scaleInputBuffer_.width != desiredWidth_ ||
         scaleInputBuffer_.height != desiredHeight_)) {
      // Sizes have changed. Rebuild our buffers.
      free(scaleInputBuffer_.data);
      scaleInputBuffer_.data = NULL;
    }
    vImage_Flags flags =
        scaleInputBuffer_.data != NULL ? kvImageNoAllocate : kvImageNoFlags;
    error = vImageBuffer_InitWithCVPixelBuffer(&scaleInputBuffer_,
                                               &desiredFormat, workingFrame,
                                               nil, nil, flags);
    if (error != kvImageNoError) {
      E("%s: error in allocate scaleInputBuffer_: %ld\n", __func__, error);
      return;
    }
  }
  // AVFoundation doesn't provide pre-scaled output.
  // Scale it here, using the Accelerate framework.
  // Also needs to be the correct aspect ratio,
  // so do a centered crop so that aspect ratios match.
  CGSize size = vImageBuffer_GetSize(&scaleInputBuffer_);
  float frameWidth = size.width;
  float frameHeight = size.height;
  float currAspect = frameWidth / frameHeight;
  float desiredAspect = ((float)desiredWidth_) / ((float)desiredHeight_);
  int cropX0, cropY0, cropW, cropH;

  BOOL shrinkHoriz = desiredAspect < currAspect;

  if (shrinkHoriz) {
    cropW = (desiredAspect / currAspect) * frameWidth;
    cropH = frameHeight;

    cropX0 = (frameWidth - cropW) / 2;
    cropY0 = 0;
  } else {
    cropW = frameWidth;
    cropH = (currAspect / desiredAspect) * frameHeight;

    cropX0 = 0;
    cropY0 = (frameHeight - cropH) / 2;
  }

  vImage_Buffer croppedBuffer = scaleInputBuffer_;
  croppedBuffer.data += (cropY0 * croppedBuffer.rowBytes) + cropX0 * 4;
  croppedBuffer.width = cropW;
  croppedBuffer.height = cropH;

  os_unfair_lock_lock(&scaleOutputBufferLock_);
  if (scaleOutputBuffer_.height != desiredHeight_ ||
      scaleOutputBuffer_.width != desiredWidth_) {
    // Sizes have changed. Rebuild our buffers.
    free(scaleOutputBuffer_.data);
    scaleOutputBuffer_.data = NULL;
    free(scaleOutputTempBuffer_);
    scaleOutputTempBuffer_ = NULL;
  }
  if (!scaleOutputBuffer_.data) {
    vImage_Error error = vImageBuffer_Init(&scaleOutputBuffer_,
                                           desiredHeight_,
                                           desiredWidth_, 32,
                                           kvImageNoFlags);
    if (error != kvImageNoError) {
      E("%s: error in allocate scaleOutputBuffer_: %ld\n", __func__, error);
      os_unfair_lock_unlock(&scaleOutputBufferLock_);
      return;
    }
    ssize_t bufferSize = vImageScale_ARGB8888(&croppedBuffer,
                                              &scaleOutputBuffer_,
                                              NULL,
                                              kvImageGetTempBufferSize);
    // We are okay with scaleOutputTempBuffer_ being NULL.
    // If that happens then it is likely that the vImageScale_ARGB8888 below
    // will fail.
    scaleOutputTempBuffer_ = malloc(bufferSize);
  }
  error = vImageScale_ARGB8888(&croppedBuffer,
                               &scaleOutputBuffer_,
                               scaleOutputTempBuffer_,
                               kvImageNoFlags);
  os_unfair_lock_unlock(&scaleOutputBufferLock_);
  if (error != kvImageNoError) {
    E("%s: error in scale: %ld\n", __func__, error);
    return;
  }
}

- (int)readFrame:(ClientFrame*)resultFrame
          rScale:(float)rScale
          gScale:(float)gScale
          bScale:(float)bScale
         expComp:(float)expComp {
  D("%s: call\n", __func__);

  int res = 1;
  // Grab the buffer out of scaleOutputBuffer_ and assign scaleOutputBuffer_
  // height to 0, which will cause the other thread to allocate new memory
  // if it needs to. Note that we use "height" as our flag instead of just
  // using scaleOutputBuffer_.data because we need scaleOutputTempBuffer_ to
  // be freed as well on the other thread if it executes.
  os_unfair_lock_lock(&scaleOutputBufferLock_);
  vImage_Buffer localBuffer = scaleOutputBuffer_;
  scaleOutputBuffer_.height = 0;
  scaleOutputBuffer_.data = NULL;
  os_unfair_lock_unlock(&scaleOutputBufferLock_);

  if (localBuffer.data != NULL) {
    // TODO(dmaclach): This call probably does way more work than it needs
    // to, and we could probably do a lot of it above in a more Apple Silicon
    // optimized manner.
    res = convert_frame(localBuffer.data, V4L2_PIX_FMT_ARGB32,
                        localBuffer.rowBytes * localBuffer.height,
                        localBuffer.width, localBuffer.height, resultFrame,
                        rScale, gScale, bScale, expComp, "front", 1);

    // Check and see if scaleOutputBuffer_.height has been updated on the
    // other thread. If not, we will just put the buffer back now that we are
    // done with it and save an extra allocation on the other thread.
    BOOL freeLocalBuffer = YES;
    os_unfair_lock_lock(&scaleOutputBufferLock_);
    if (scaleOutputBuffer_.height == 0) {
      scaleOutputBuffer_.height = localBuffer.height;
      scaleOutputBuffer_.data = localBuffer.data;
      freeLocalBuffer = NO;
    }
    os_unfair_lock_unlock(&scaleOutputBufferLock_);
    if (freeLocalBuffer) {
      // The other thread allocated more data, so we now own this pointer and
      // have to dispose it.
      free(localBuffer.data);
    }
  }

  return res;
}

@end

/*******************************************************************************
 *                     CameraDevice routines
 ******************************************************************************/

/* MacOS-specific camera device descriptor. */
typedef struct MacCameraDevice {
  /* Common camera device descriptor. */
  CameraDevice  header;
  /* Actual camera device object. */
  MacCamera*    device;
  char* device_name;
  int input_channel;
  int started;
  int frame_width;
  int frame_height;
} MacCameraDevice;

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
    [cd->device release];
    free(cd->device_name);
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
    [cd->device release];
    cd->device = [[MacCamera alloc] initWithName:cd->device_name];
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
  mcd->device = [[MacCamera alloc] initWithName:name];
  if (!mcd->device) {
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
  if (!mcd->device) {
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
  if (!mcd->device) {
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
  if (!mcd->device) {
    E("%s: Camera device is not opened", __FUNCTION__);
    return -1;
  }

  if (!mcd->started) {
    int result = [mcd->device startCapturingWidth:mcd->frame_width
                                           height:mcd->frame_height
                                        direction:direction];
    if (result) return -1;
    mcd->started = 1;
  }
  return [mcd->device readFrame:result_frame
                         rScale:r_scale
                         gScale:g_scale
                         bScale:b_scale
                        expComp:exp_comp];
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

  NSArray *videoDevices =
      [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
  if (!videoDevices) {
    E("No web cameras are connected to the host.");
    return 0;
  }
  int found = 0;
  for (AVCaptureDevice *videoDevice in videoDevices) {
    for(AVCaptureDeviceFormat *vFormat in [videoDevice formats]) {
      CMFormatDescriptionRef description = vFormat.formatDescription;
      cis[found].pixel_format =
          FourCCToInternal(CMFormatDescriptionGetMediaSubType(description));
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
      char *user_name;
      char *device_name = [[videoDevice uniqueID] UTF8String];
      asprintf(&user_name, "webcam%d", found);
      cis[found].frame_sizes_num = sizeof(_emulate_dims) / sizeof(*_emulate_dims);
      memcpy(cis[found].frame_sizes, _emulate_dims, sizeof(_emulate_dims));
      cis[found].device_name = ASTRDUP(device_name);
      cis[found].inp_channel = 0;
      cis[found].display_name = user_name;
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
