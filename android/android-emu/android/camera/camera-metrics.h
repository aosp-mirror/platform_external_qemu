/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * Provides metrics reporting for the camera API.
 */

#ifdef __cplusplus
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "studio_stats.pb.h"
#endif  // __cplusplus
#include "android/camera/camera-common.h"
#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

#ifdef __cplusplus

namespace android {
namespace camera {

typedef void (*CameraMetricsCallback)(
        android_studio::EmulatorCameraSession& metrics);

class CameraMetrics {
public:
    static CameraMetrics& instance();

    // Report the start of a camera session. Start aggregating metrics, which
    // are reported when stopSession is called.
    //
    // |type| - Camera type.
    // |direction| - Camera direction, either "front" or "back".
    // |frameWidth|,|frameHeight| - Frame dimensions.
    // |pixelFormat| - Camera pixel format.
    void startSession(
            android_studio::EmulatorCameraSession::EmulatorCameraType type,
            const char* direction,
            uint32_t frameWidth,
            uint32_t frameHeight,
            uint32_t pixelFormat);

    // Report the stop of a camera session, which sends metrics with aggregate
    // information from the session.
    //
    // |frameCount| - Total number of frames rendered during the session.
    void stopSession(uint64_t frameCount);

    // Set the start result for the session, and record the startup time in the
    // metrics. If the start result is an error, stopSession should be called.
    //
    // |startResult| - Start result.
    void setStartResult(
            android_studio::EmulatorCameraSession::EmulatorCameraStartResult
                    startResult);

    // Set the virtual scene name loaded for the current camera session.
    //
    // |name| - Virtual scene name.
    void setVirtualSceneName(const char* name);

private:
    android::base::Lock mLock;

    uint64_t mReportWindowStartUs = 0;
    uint32_t mReportWindowCount = 0;

    bool mSessionActive = false;
    uint64_t mStartTimeUs = 0;
    uint64_t mCameraInitializedUs = 0;

    android_studio::EmulatorCameraSession mCameraSession;
};

}  // namespace camera
}  // namespace android

#endif  // __cplusplus

// Wrapper C API that exposes the above functionality for camera-service.c

// Reports metrics for the current camera session.
//
// |source_type| - Camera type.
// |direction| - Camera direction, either "front" or "back".
// |frame_width|,|frame_height| - Frame dimensions.
// |pixel_format| - Fourcc of the pixel format.
void camera_metrics_report_start_session(CameraSourceType source_type,
                                         const char* direction,
                                         int frame_width,
                                         int frame_height,
                                         int pixel_format);

// Report the start result and startup duration for camera metrics.
//
// |succeeded| - Did the camera start succeed?
void camera_metrics_report_start_result(ClientStartResult result);

// Report metrics after a camera session has ended.
//
// |frame_count| - Number of frames returned during the session.
void camera_metrics_report_stop_session(uint64_t frame_count);

ANDROID_END_HEADER
