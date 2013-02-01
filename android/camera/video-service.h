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

#ifndef ANDROID_CAMERA_VIDEO_SERVICE_H_
#define ANDROID_CAMERA_VIDEO_SERVICE_H_

#include "qemu-common.h"
#include "android/utils/system.h"

/*
 * Contains the public video service API.
 */

/* Frame dimensions. */
typedef struct VideoFrameDim {
    int width;
    int height;
} VideoFrameDim;

typedef struct VideoDevice {
  FILE* fp;
} VideoDevice;

// TODO: Reorder to match CameraInfo
typedef struct VideoInfo {
  char* display_name;
  uint32_t pixel_format;
  int length;   // number of frames.
  VideoDevice* vd;  // Check if this is needed.
  VideoFrameDim* dims;
  int in_use;
  int inp_channel;
  char* direction;
  char* fb;
} VideoInfo;


/* Initializes video emulation service over qemu pipe. */
extern void android_video_service_init(void);

#endif /* ANDROID_CAMERA_VIDEO_SERVICE_H_ */
