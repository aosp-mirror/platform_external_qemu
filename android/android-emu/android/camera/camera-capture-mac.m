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
  CGColorSpaceRef imageColorSpace_;
  vImage_CGImageFormat imageDesiredFormat_;

  int desiredWidth_;
  int desiredHeight_;
  AVCaptureSession *captureSession_;
  MacCameraDirection cameraDirection_;

  dispatch_queue_t outputQueue_;
  NSThread *imageProcessingThread_;
  os_unfair_lock inputBufferLock_;
  os_unfair_lock outputBufferLock_;
  BOOL inputFrameUpdated_;
  BOOL outputFrameUpdated_;
  AndroidCoarseOrientation inputFrameOrientation_;
  vImage_Buffer inputWriteBuffer_;    // this is where captureOutput writes frames into
  vImage_Buffer inputReadBuffer_;     // this is where imageProcessingImpl reads frames from
  vImage_Buffer readFrameBuffer_;     // this is where imageProcessingImpl writes frames into
  vImage_Buffer readFrameShadowBuffer_;  // readFrame own copy
}

/* Initializes MacCamera instance.
 * Return:
 *  Pointer to initialized instance on success, or nil on failure.
 */
- (instancetype)initWithName:(const char*)name NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
- (void)stopCapture;
- (void)dealloc;

/* Starts capturing video frames.
 * Param:
 *  width, height - Requested dimensions for the captured video frames.
 * Return:
 *  0 on success, or !=0 on failure.
 */
- (int)startCapturing:(const char*)device_name
                width:(int)width
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

static void swapImages(vImage_Buffer* lhs, vImage_Buffer* rhs) {
  vImage_Buffer tmp = *lhs;
  *lhs = *rhs;
  *rhs = tmp;
}

static vImage_Buffer shallowCropToAspectRatio(const vImage_Buffer* src,
                                              const int desiredWidth,
                                              const int desiredHeight) {
  const float frameWidth = src->width;
  const float frameHeight = src->height;
  const float currAspect = frameWidth / frameHeight;
  const float desiredAspect = ((float)desiredWidth) / ((float)desiredHeight);
  int cropX0, cropY0, cropW, cropH;

  if (desiredAspect < currAspect) {
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

  vImage_Buffer cropped = *src;
  cropped.data += (cropY0 * cropped.rowBytes) + cropX0 * 4;
  cropped.width = cropW;
  cropped.height = cropH;

  return cropped;
}

- (void)imageProcessingImpl:(vImage_Buffer*)inputFrame
                orientation:(AndroidCoarseOrientation)orientation
                rotateBuffer:(vImage_Buffer*)rotateBuffer
                outputFrame:(vImage_Buffer*)outputFrame
                pScaleTempBuffer:(void**)pScaleTempBuffer {
  vImage_Error error;
  vImage_Buffer* rotatedFrame;

  if (orientation == ANDROID_COARSE_PORTRAIT ||
      orientation == ANDROID_COARSE_REVERSE_PORTRAIT) {
    if (rotateBuffer->width != inputFrame->height ||
        rotateBuffer->height != inputFrame->width) {
      free(rotateBuffer->data);
      rotateBuffer->data = NULL;
      rotateBuffer->height = 0;
      free(*pScaleTempBuffer);
      *pScaleTempBuffer = NULL;
    }

    if (!rotateBuffer->data) {
      error = vImageBuffer_Init(rotateBuffer, inputFrame->width, inputFrame->height,
                                32, kvImageNoFlags);
      if (error != kvImageNoError) {
        E("%s: error in vImageBuffer_Init rotateBuffer=%p, height=%d, width=%d: %ld\n",
          __func__, rotateBuffer, inputFrame->width, inputFrame->height, error);
        return;
      }
    }

    const Pixel_8888 backColor = {0, 0, 0, 0};
    const int rotation = (cameraDirection_ ==  kMacCameraBackward ?
                    (1 + orientation) % 4 :
                    (5 - orientation) % 4);
    error = vImageRotate90_ARGB8888(inputFrame, rotateBuffer,
                                    rotation, backColor, kvImageNoFlags);
    if (error != kvImageNoError) {
      E("%s: error in vImageRotate90_ARGB8888: %ld\n", __func__, error);
      return;
    }

    rotatedFrame = rotateBuffer;
  } else {
    rotatedFrame = inputFrame;
  }

  const vImage_Buffer cropped =
    shallowCropToAspectRatio(rotatedFrame, desiredWidth_, desiredHeight_);

  {
    if (desiredWidth_ != outputFrame->width ||
        desiredHeight_ != outputFrame->height) {
      free(outputFrame->data);
      outputFrame->data = NULL;
      outputFrame->height = 0;
      free(*pScaleTempBuffer);
      *pScaleTempBuffer = NULL;
    }

    if (!outputFrame->data) {
      error = vImageBuffer_Init(outputFrame,
                                desiredHeight_, desiredWidth_, 32, kvImageNoFlags);
      if (error != kvImageNoError) {
        E("%s: error in vImageBuffer_Init outputFrame=%p, height=%d, width=%d: %ld\n",
          __func__, outputFrame, desiredHeight_, desiredWidth_, error);
        return;
      }
    }

    if (!*pScaleTempBuffer) {
      *pScaleTempBuffer = malloc(
        vImageScale_ARGB8888(&cropped, outputFrame,
                             NULL, kvImageGetTempBufferSize));
    }
    error = vImageScale_ARGB8888(&cropped, outputFrame,
                                 *pScaleTempBuffer, kvImageNoFlags);
    if (error != kvImageNoError) {
      E("%s: error in vImageScale_ARGB8888: "
        "src(%p)={data=%p, rowBytes=%d, width=%d, height=%d}, "
        "dst(%p)={data=%p, rowBytes=%d, width=%d, height=%d}: %ld\n",
        __func__, &cropped, cropped.data, cropped.rowBytes,
        cropped.width, cropped.height,
        outputFrame, outputFrame->rowBytes, outputFrame->data,
        outputFrame->width, outputFrame->height,
        error);
      return;
    }
  }

  os_unfair_lock_lock(&outputBufferLock_);
  swapImages(&readFrameBuffer_, outputFrame);
  outputFrameUpdated_ = YES;
  os_unfair_lock_unlock(&outputBufferLock_);
}

- (void)imageProcessingLoop {
  vImage_Buffer inputFrame = {};
  vImage_Buffer rotateBuffer = {};
  vImage_Buffer outputFrame = {};
  void* scaleTempBuffer = NULL;
  AndroidCoarseOrientation orientation;

  while (![imageProcessingThread_ isCancelled]) {
    BOOL hasNewFrame;

    os_unfair_lock_lock(&inputBufferLock_);
    if (inputFrameUpdated_) {
      swapImages(&inputFrame, &inputReadBuffer_);
      orientation = inputFrameOrientation_;
      inputFrameUpdated_ = NO;
      hasNewFrame = YES;
    } else {
      hasNewFrame = NO;
    }
    os_unfair_lock_unlock(&inputBufferLock_);

    if (hasNewFrame) {
      [self imageProcessingImpl:&inputFrame
            orientation:orientation
            rotateBuffer:&rotateBuffer
            outputFrame:&outputFrame
            pScaleTempBuffer:&scaleTempBuffer];
    } else {
      [NSThread sleepForTimeInterval:0.025];
    }
  }

  free(scaleTempBuffer);
  free(outputFrame.data);
  free(rotateBuffer.data);
  free(inputFrame.data);
}

- (instancetype)initWithName:(const char*)name {
  if (!(self = [super init])) {
    return nil;
  }

  imageColorSpace_ = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  imageDesiredFormat_ = (vImage_CGImageFormat){
    .bitsPerComponent = 8,
    .bitsPerPixel = 32,
    .colorSpace = imageColorSpace_,
    .bitmapInfo =
    (CGBitmapInfo)(kCGImageByteOrder32Big | kCGImageAlphaNoneSkipFirst),
    .version = 0,
    .decode = nil,
    .renderingIntent = kCGRenderingIntentDefault
  };

  desiredWidth_ = 0;
  desiredHeight_ = 0;
  captureSession_ = NULL;

  inputBufferLock_ = OS_UNFAIR_LOCK_INIT;
  outputBufferLock_ = OS_UNFAIR_LOCK_INIT;
  inputFrameUpdated_ = NO;
  outputFrameUpdated_ = NO;

  return self;
}

- (int)startCapturing:(const char*)device_name
                width:(int)width
               height:(int)height
            direction:(const char*)direction {
  D("%s: call. start capture session, device_name=%s, "
    "width=%d, height=%d, direction=%s\n",
    __func__, device_name, width, height, direction);
  desiredWidth_ = width;
  desiredHeight_ = height;
  cameraDirection_ = (strcmp("back", direction) == 0
                      ? kMacCameraBackward : kMacCameraForward);

  AVCaptureDevice *captureDevice;

  /* Obtain the capture device, make sure it's not used by another
   * application, and open it. */
  if (device_name == NULL || *device_name  == '\0') {
    captureDevice =
        [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  } else {
    NSString *deviceName = [NSString stringWithFormat:@"%s", device_name];
    captureDevice = [AVCaptureDevice deviceWithUniqueID:deviceName];
  }
  if (!captureDevice) {
    E("There are no available video devices found.");
    return -1;
  }
  if ([captureDevice isInUseByAnotherApplication]) {
    E("Default camera device is in use by another application.");
    return -1;
  }

  /* Create capture session. */
  captureSession_ = [[AVCaptureSession alloc] init];
  if (!captureSession_) {
    E("Unable to create capure session.");
    return -1;
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
    return -1;
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
    return -1;
  }

  imageProcessingThread_ = [[NSThread alloc] initWithTarget:self selector:@selector(imageProcessingLoop) object:nil];

  [captureSession_ startRunning];
  [imageProcessingThread_ start];
  return 0;
}

- (void)stopCapture {
  if (imageProcessingThread_) {
    [imageProcessingThread_ cancel];
  }

  if (captureSession_) {
    [captureSession_ stopRunning];
    [captureSession_ release];
  }

  if (imageProcessingThread_) {
    while ([imageProcessingThread_ isFinished] == NO) {
      [NSThread sleepForTimeInterval:0.05];
    }
  }

  CGColorSpaceRelease(imageColorSpace_);
  dispatch_release(outputQueue_);
}

- (void)dealloc {
  free(inputWriteBuffer_.data);
  free(inputReadBuffer_.data);
  free(readFrameBuffer_.data);
  free(readFrameShadowBuffer_.data);

  [super dealloc];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection {
  CVImageBufferRef workingFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
  vImageCVImageFormatRef imageFormat =
      vImageCVImageFormat_CreateWithCVPixelBuffer(workingFrame);
  if (!imageFormat) {
    E("%s: NULL return from vImageCVImageFormat_CreateWithCVPixelBuffer\n", __func__);
    return;
  }

  vImage_Error error;
  error = vImageCVImageFormat_SetColorSpace(imageFormat, imageColorSpace_);
  if (error != kvImageNoError) {
    E("%s: error in vImageCVImageFormat_SetColorSpace: %ld\n", __func__, error);
    vImageCVImageFormat_Release(imageFormat);
    return;
  }

  vImage_Flags flags =
      inputWriteBuffer_.data ? kvImageNoAllocate : kvImageNoFlags;
  error = vImageBuffer_InitWithCVPixelBuffer(&inputWriteBuffer_,
                                             &imageDesiredFormat_, workingFrame,
                                             imageFormat, nil, flags);
  vImageCVImageFormat_Release(imageFormat);

  if (error != kvImageNoError) {
    E("%s: error in vImageBuffer_InitWithCVPixelBuffer inputWriteBuffer_: %ld\n",
      __func__, error);
    return;
  }

  os_unfair_lock_lock(&inputBufferLock_);
  inputFrameOrientation_ = get_coarse_orientation();
  swapImages(&inputReadBuffer_, &inputWriteBuffer_);
  inputFrameUpdated_ = YES;
  os_unfair_lock_unlock(&inputBufferLock_);
}

- (int)readFrame:(ClientFrame*)resultFrame
          rScale:(float)rScale
          gScale:(float)gScale
          bScale:(float)bScale
         expComp:(float)expComp {
  os_unfair_lock_lock(&outputBufferLock_);
  if (outputFrameUpdated_) {
    swapImages(&readFrameBuffer_, &readFrameShadowBuffer_);
    outputFrameUpdated_ = NO;
  }
  os_unfair_lock_unlock(&outputBufferLock_);

  if (readFrameShadowBuffer_.data) {
    return convert_frame(readFrameShadowBuffer_.data, V4L2_PIX_FMT_ARGB32,
                         readFrameShadowBuffer_.rowBytes * readFrameShadowBuffer_.height,
                         readFrameShadowBuffer_.width, readFrameShadowBuffer_.height, resultFrame,
                         rScale, gScale, bScale, expComp, "front", 1);
  } else {
    return 1;
  }
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

  [mcd->device stopCapture];
  mcd->started = 0;

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
    int result = [mcd->device startCapturing:mcd->device_name
                                       width:mcd->frame_width
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
