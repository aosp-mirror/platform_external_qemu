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

#include "android/camera/camera-metrics.h"

#include "android/base/memory/LazyInstance.h"
#include "android/metrics/MetricsReporter.h"
#include "android/utils/debug.h"

using android::base::AutoLock;
using android::base::System;
using android::camera::CameraMetrics;
using android::metrics::MetricsReporter;
using android_studio::EmulatorCameraSession;
using EmulatorCameraType =
        android_studio::EmulatorCameraSession::EmulatorCameraType;
using EmulatorCameraStartResult =
        android_studio::EmulatorCameraSession::EmulatorCameraStartResult;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

// Allow at most 5 reports every 60 seconds.
static constexpr uint64_t kReportWindowDurationUs = 1000 * 1000 * 60;
static constexpr uint32_t kMaxReportsPerWindow = 5;

namespace android {
namespace camera {

static android::base::LazyInstance<CameraMetrics> sCameraMetrics =
        LAZY_INSTANCE_INIT;

static uint64_t durationUsToMs(uint64_t startUs, uint64_t endUs) {
    assert(startUs < endUs);
    const uint64_t durationUs = (endUs - startUs);
    return durationUs / 1000;
}

CameraMetrics& CameraMetrics::instance() {
    return sCameraMetrics.get();
}

void CameraMetrics::startSession(EmulatorCameraType type,
                                 const char* direction,
                                 uint32_t frameWidth,
                                 uint32_t frameHeight,
                                 uint32_t pixelFormat) {
    AutoLock lock(mLock);
    mSessionActive = true;
    mStartTimeUs = System::get()->getHighResTimeUs();

    mCameraSession.Clear();
    mCameraSession.set_type(type);
    if (!strcmp(direction, "back")) {
        mCameraSession.set_direction(
                EmulatorCameraSession::EMULATOR_CAMERA_DIRECTION_BACK);
    } else if (!strcmp(direction, "front")) {
        mCameraSession.set_direction(
                EmulatorCameraSession::EMULATOR_CAMERA_DIRECTION_FRONT);
    }
    mCameraSession.set_width(frameWidth);
    mCameraSession.set_height(frameHeight);
    mCameraSession.set_pixel_format(pixelFormat);
}

void CameraMetrics::setStartResult(EmulatorCameraStartResult startResult) {
    AutoLock lock(mLock);
    if (!mSessionActive) {
        W("%s: Session not active, call ignored.", __FUNCTION__);
        return;
    }

    mCameraSession.set_start_result(startResult);
    mCameraSession.set_startup_time_ms(
            durationUsToMs(mStartTimeUs, System::get()->getHighResTimeUs()));
}

void CameraMetrics::stopSession(uint64_t frameCount) {
    AutoLock lock(mLock);
    if (!mSessionActive) {
        W("%s: Session not active, call ignored.", __FUNCTION__);
        return;
    }

    mSessionActive = false;

    android_studio::EmulatorCameraSession cameraSession = mCameraSession;
    mCameraSession.Clear();

    const uint64_t now = System::get()->getHighResTimeUs();

    // Reset the metrics reporting limiter if enough time has passed.
    if (mReportWindowStartUs + kReportWindowDurationUs < now) {
        mReportWindowStartUs = now;
        mReportWindowCount = 0;
    }

    if (mReportWindowCount > kMaxReportsPerWindow) {
        W("%s: Dropping metrics, too many recent reports.", __FUNCTION__);
        return;
    }

    ++mReportWindowCount;

    const uint64_t durationMs = durationUsToMs(mStartTimeUs, now);
    cameraSession.set_duration_ms(durationMs);

    if (durationMs > 0) {
        const double fps =
                static_cast<double>(frameCount) / (durationMs / 1000.0f);
        cameraSession.set_average_framerate(fps);
    }

    MetricsReporter::get().report(
            [cameraSession](android_studio::AndroidStudioEvent* event) {
                event->mutable_emulator_details()->mutable_camera()->CopyFrom(
                        cameraSession);
            });
}

void CameraMetrics::setVirtualSceneName(const char* name) {
    AutoLock lock(mLock);
    if (!mSessionActive) {
        W("%s: Session not active, call ignored.", __FUNCTION__);
        return;
    }

    mCameraSession.set_virtual_scene_name(name);
}

}  // namespace camera
}  // namespace android

void camera_metrics_report_start_session(CameraSourceType source_type,
                                         const char* direction,
                                         int frame_width,
                                         int frame_height,
                                         int pixel_format) {
    EmulatorCameraType type =
            EmulatorCameraSession::EMULATOR_CAMERA_TYPE_WEBCAM;
    switch (source_type) {
        case kVirtualScene:
            type = EmulatorCameraSession::EMULATOR_CAMERA_TYPE_VIRTUAL_SCENE;
            break;
        case kWebcam:
            type = EmulatorCameraSession::EMULATOR_CAMERA_TYPE_WEBCAM;
            break;
        case kVideoPlayback:
            type = EmulatorCameraSession::EMULATOR_CAMERA_TYPE_VIDEO_PLAYBACK;
            break;
        default:
            assert(false);
            break;
    }

    CameraMetrics::instance().startSession(type, direction,
                                           static_cast<uint32_t>(frame_width),
                                           static_cast<uint32_t>(frame_height),
                                           static_cast<uint32_t>(pixel_format));
}

static EmulatorCameraStartResult clientStartResultToMetricsStartResult(
        ClientStartResult result) {
    switch (result) {
        case CLIENT_START_RESULT_SUCCESS:
            return EmulatorCameraSession::EMULATOR_CAMERA_START_SUCCESS;
        case CLIENT_START_RESULT_ALREADY_STARTED:
            return EmulatorCameraSession::EMULATOR_CAMERA_START_ALREADY_STARTED;
        case CLIENT_START_RESULT_PARAMETER_MISMATCH:
            return EmulatorCameraSession::
                    EMULATOR_CAMERA_START_PARAMETER_MISMATCH;
        case CLIENT_START_RESULT_UNKNOWN_PIXEL_FORMAT:
            return EmulatorCameraSession::
                    EMULATOR_CAMERA_START_UNKNOWN_PIXEL_FORMAT;
        case CLIENT_START_RESULT_NO_PIXEL_CONVERSION:
            return EmulatorCameraSession::
                    EMULATOR_CAMERA_START_NO_PIXEL_CONVERSION;
        case CLIENT_START_RESULT_OUT_OF_MEMORY:
            return EmulatorCameraSession::EMULATOR_CAMERA_START_OUT_OF_MEMORY;
        case CLIENT_START_RESULT_FAILED:
        default:
            return EmulatorCameraSession::EMULATOR_CAMERA_START_FAILED;
    }

    // Unknown mapping, return a generic failure.
    assert(false);
    return EmulatorCameraSession::EMULATOR_CAMERA_START_FAILED;
}

void camera_metrics_report_start_result(ClientStartResult result) {
    CameraMetrics::instance().setStartResult(
            clientStartResultToMetricsStartResult(result));
}

void camera_metrics_report_stop_session(uint64_t frame_count) {
    CameraMetrics::instance().stopSession(frame_count);
}
