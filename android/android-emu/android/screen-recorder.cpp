// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/screen-recorder.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/ffmpeg-muxer.h"
#include "android/gpu_frame.h"
#include "android/opengles.h"
#include "android/screen-recorder-constants.h"
#include "android/utils/debug.h"

#include <atomic>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace {

// The maximum number of frames we will buffer
constexpr int kMaxFrames = 16;

// A small structure to model a single frame of the GPU display,
// as passed between the EmuGL and main loop thread.
struct Frame {
    int width;
    int height;
    std::unique_ptr<char[]> pixels;

    Frame(int w, int h, const void* px) :
            width(w), height(h) {
        if (px) {
            pixels.reset(new char[w * 4 *h]);
            ::memcpy(static_cast<void*>(pixels.get()), px, w * 4 * h);
        }
    }

    ~Frame() {
        pixels.reset();
    }
};

enum class RecordState { Ready, Recording, Stopping };

struct CVInfo {
    android::base::Lock lock;
    android::base::ConditionVariable cv;
    bool done = false;
};

struct Globals {
    ffmpeg_recorder* recorder = nullptr;
    int fbWidth = 0;
    int fbHeight = 0;
    bool isGuestMode = false;
    std::string fileName = {};
    std::atomic<RecordState> recordState{RecordState::Ready};
    ::android::base::MessageChannel<Frame*, kMaxFrames> channel;
    RecordingInfo recordingInfo;
    CVInfo encodingCVInfo;  // Used to signal encoding thread completion
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// Two threads: one will be used to get the frames at some fps, and forward them
// to the second thread for processing. The second thread will be responsible
// for encoding the received frames.

// Function to fetch the frames
static intptr_t sendFrames(int fps) {
    unsigned char* px;
    long long timeDeltaMs = 1000 / fps;
    long long currTimeMs, newTimeMs, startMs;
    long long i = 0;

    // The assumption here when starting is that sGlobals->frame contains a
    // valid frame when recordState == Recording.
    startMs = android::base::System::get()->getHighResTimeUs() / 1000;
    while (sGlobals->recordState == RecordState::Recording) {
        currTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
        px = (unsigned char*)gpu_frame_get_record_frame();
        if (px) {
            Frame* f = new Frame(sGlobals->fbWidth, sGlobals->fbHeight, px);
            D("sending frame %d", i++);
            if (!sGlobals->channel.send(f)) {
                derror("Frame queue full. Frame dropped");
                delete f;
            }
        }

        // Need to do some calculation here so we are calling
        // gpu_frame_get_record_frame() at |fps|.
        newTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;

        // Stop recording if time limit is reached
        if (newTimeMs - startMs >= sGlobals->recordingInfo.timeLimit * 1000) {
            D("Time limit reached (%lld ms). Stopping the recording",
              newTimeMs - startMs);
            screen_recorder_stop_async();
            break;
        }

        if (newTimeMs - currTimeMs < timeDeltaMs) {
            android::base::System::get()->sleepMs(timeDeltaMs - newTimeMs +
                                                  currTimeMs);
        }
    }

    // Send frame with null data to stop encoding thread.
    Frame* f = new Frame(0, 0, nullptr);
    sGlobals->channel.send(f);
    D("Finished sending frames");
    return 0;
}

// Function to encode the frames
static intptr_t encodeFrames() {
    Frame* f;
    int i = 0;

    while (1) {
        f = nullptr;
        sGlobals->channel.receive(&f);
        if (!f || !f->pixels) {
            D("Received null frame");
            if (f) {
                delete f;
            }
            // We are done recording.
            break;
        }
        D("Received frame %d", i++);
        ffmpeg_encode_video_frame(sGlobals->recorder,
                                  (const uint8_t*)f->pixels.get(),
                                  f->width * f->height * 4);
        delete f;
    }

    D("Finished encoding");
    sGlobals->encodingCVInfo.lock.lock();
    sGlobals->encodingCVInfo.done = true;
    sGlobals->encodingCVInfo.cv.signal();
    sGlobals->encodingCVInfo.lock.unlock();

    return 0;
}

// Function to stop the recording
static intptr_t stopRecording() {
    sGlobals->recordState = RecordState::Stopping;
    // Need to wait for encoding thread to finish before deleting the
    // encoder.
    sGlobals->encodingCVInfo.lock.lock();
    while (!sGlobals->encodingCVInfo.done) {
        sGlobals->encodingCVInfo.cv.wait(&sGlobals->encodingCVInfo.lock);
    }
    sGlobals->encodingCVInfo.done = false;
    sGlobals->encodingCVInfo.lock.unlock();

    ffmpeg_delete_recorder(sGlobals->recorder);
    gpu_frame_set_record_mode(false);

    if (sGlobals->recordingInfo.cb) {
        sGlobals->recordingInfo.cb(sGlobals->recordingInfo.opaque,
                                   RECORD_STOP_FINISHED);
    }
    sGlobals->recordingInfo.fileName = nullptr;
    sGlobals->recorder = nullptr;

    sGlobals->recordState = RecordState::Ready;
    return 0;
}
}  // namespace

void screen_recorder_init(bool isGuestMode, int w, int h) {
    sGlobals->fbWidth = w;
    sGlobals->fbHeight = h;
    sGlobals->isGuestMode = isGuestMode;
}

bool parseRecordingInfo(const RecordingInfo* info) {
    RecordingInfo* sinfo = &sGlobals->recordingInfo;

    if (info == nullptr) {
        derror("Recording info is null");
        return false;
    }

    if (info->fileName == nullptr || strlen(info->fileName) == 0) {
        derror("Recording filename cannot be empty");
        return false;
    }

    if (info->width <= 0 || info->height <= 0) {
        D("Defaulting width and height to %dx%d", sGlobals->fbWidth,
          sGlobals->fbHeight);
        sinfo->width = sGlobals->fbWidth;
        sinfo->height = sGlobals->fbHeight;
    } else {
        sinfo->width = info->width;
        sinfo->height = info->height;
    }

    if (info->videoBitrate < kMinVideoBitrate ||
        info->videoBitrate > kMaxVideoBitrate) {
        D("Defaulting videoBitrate to %d bps", kDefaultVideoBitrate);
        sinfo->videoBitrate = kDefaultVideoBitrate;
    } else {
        sinfo->videoBitrate = info->videoBitrate;
    }

    if (info->timeLimit < 1 || info->timeLimit > kMaxTimeLimit) {
        D("Defaulting time limit to %d seconds", kMaxTimeLimit);
        sinfo->timeLimit = kMaxTimeLimit;
    } else {
        sinfo->timeLimit = info->timeLimit;
    }

    sinfo->cb = info->cb;
    sinfo->opaque = info->opaque;

    sGlobals->fileName = info->fileName;
    sinfo->fileName = sGlobals->fileName.c_str();

    return true;
}

int screen_recorder_start(const RecordingInfo* info) {
    if (sGlobals->isGuestMode) {
        derror("Recording is only supported in host gpu configuration");
        return false;
    }

    if (sGlobals->recorder) {
        derror("Recorder already started");
        return false;
    }

    if (!parseRecordingInfo(info)) {
        return false;
    }

    sGlobals->recorder = ffmpeg_create_recorder(
            &sGlobals->recordingInfo, sGlobals->fbWidth, sGlobals->fbHeight);
    if (!sGlobals->recorder) {
        derror("ffmpeg_create_recorder failed");
        return false;
    }
    D("created recorder");

    // Add the video and audio tracks
    ffmpeg_add_video_track(sGlobals->recorder, sGlobals->recordingInfo.width,
                           sGlobals->recordingInfo.height,
                           sGlobals->recordingInfo.videoBitrate, kFPS,
                           kIntraSpacing);
    ffmpeg_add_audio_track(sGlobals->recorder, kAudioBitrate, kAudioSampleRate);
    D("Added AV tracks");

    sGlobals->recordState = RecordState::Recording;
    gpu_frame_set_record_mode(true);
    android_redrawOpenglesWindow();

    auto sendFramesFunc = std::bind(&sendFrames, kFPS);
    android::base::async(sendFramesFunc);
    android::base::async(&encodeFrames);

    return true;
}

void screen_recorder_stop_async(void) {
    if (!sGlobals->recorder) {
        derror("Screen recording was never started");
        return;
    }

    if (sGlobals->recordState != RecordState::Recording) {
        derror("Already stopping the recording");
        return;
    }

    if (sGlobals->recordingInfo.cb) {
        sGlobals->recordingInfo.cb(sGlobals->recordingInfo.opaque,
                                   RECORD_STOP_INITIATED);
    }
    android::base::async(&stopRecording);
}
