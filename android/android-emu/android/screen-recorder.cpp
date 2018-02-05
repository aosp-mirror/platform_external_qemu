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
#include "android/base/Optional.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/ffmpeg-muxer.h"
#include "android/framebuffer.h"
#include "android/gpu_frame.h"
#include "android/opengles.h"
#include "android/screen-recorder-constants.h"
#include "android/utils/debug.h"

#include <atomic>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace {

using android::base::AutoLock;
using android::base::Optional;

// The maximum number of frames we will buffer
constexpr int kMaxFrames = 16;

// A small structure to model a single frame of the GPU display,
// as passed between the EmuGL and main loop thread.
struct Frame {
    uint16_t width = 0;
    uint16_t height = 0;
    RecordPixFmt pix_fmt = RecordPixFmt::INVALID_FMT;
    uint64_t pt_us = 0;  // presentation time in microseconds
    std::unique_ptr<char[]> pixels;

    Frame() {}
    Frame(int w, int h, uint64_t pt, RecordPixFmt p, const void* px)
        : width(w), height(h), pix_fmt(p), pt_us(pt) {
        if (px) {
            int sz = get_record_pixel_size(pix_fmt);
            pixels.reset(new char[w * sz * h]);
            ::memcpy(static_cast<void*>(pixels.get()), px, w * sz * h);
        }
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
    QAndroidDisplayAgent displayAgent;
    int fbWidth = 0;
    int fbHeight = 0;
    int fbBpp = 0;
    bool isGuestMode = false;
    std::string fileName = {};
    std::atomic<RecordState> recordState{RecordState::Ready};
    ::android::base::Lock guestFBLock;
    int frameCount = 0;
    std::unique_ptr<Frame> guestFrame;
    ::android::base::MessageChannel<Frame, kMaxFrames> channel;
    RecordingInfo recordingInfo;
    CVInfo encodingCVInfo;  // Used to signal encoding thread completion
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// Callback for guest fb update
static void screen_recorder_fb_update(void* opaque,
                                      int x,
                                      int y,
                                      int width,
                                      int height) {
    auto& globals = *sGlobals;
    D("[%d] %s(x=%d, y=%d, w=%d, h=%d)", ++globals.frameCount, __func__, x, y,
      width, height);

    if (width <= 0 || height <= 0)
        return;

    int w = 0, h = 0, bpp = 0;
    unsigned char* px = nullptr;
    globals.displayAgent.getFrameBuffer(&w, &h, nullptr, &bpp, &px);
    if (!px || w <= 0 || h <= 0) {
        return;
    }

    AutoLock lock(globals.guestFBLock);

    if (!globals.guestFrame) {
        globals.guestFrame.reset(new Frame(
                w, h, android::base::System::get()->getHighResTimeUs(),
                RecordPixFmt::RGB565, px));
    } else if (w == globals.guestFrame->width &&
               h == globals.guestFrame->height) {
        memcpy(static_cast<void*>(globals.guestFrame->pixels.get()), px,
               w * h * get_record_pixel_size(globals.guestFrame->pix_fmt));
    }
}

// Two threads: one will be used to get the frames at some fps, and forward them
// to the second thread for processing. The second thread will be responsible
// for encoding the received frames.

// Function to fetch the frames
static intptr_t sendFrames(int fps) {
    auto& globals = *sGlobals;
    int i = 0;

    // The assumption here when starting is that globals.frame contains a
    // valid frame when is_recording is true.
    long long startMs = android::base::System::get()->getHighResTimeUs() / 1000;
    while (globals.recordState == RecordState::Recording) {
        long long currTimeMs =
                android::base::System::get()->getHighResTimeUs() / 1000;

        Optional<Frame> f;
        if (!globals.isGuestMode) {
            auto px = (unsigned char*)gpu_frame_get_record_frame();
            if (px) {
                f.emplace(globals.fbWidth, globals.fbHeight,
                          android::base::System::get()->getHighResTimeUs(),
                          RecordPixFmt::RGBA8888, px);
            }
        } else {
            AutoLock lock(globals.guestFBLock);
            if (globals.guestFrame) {
                f.emplace(globals.guestFrame->width, globals.guestFrame->height,
                          android::base::System::get()->getHighResTimeUs(),
                          RecordPixFmt::RGB565,
                          globals.guestFrame->pixels.get());
            }
        }

        if (f) {
            D("sending frame %d", i++);
            if (!globals.channel.trySend(std::move(*f))) {
                derror("Frame queue full. Frame dropped");
            }
        }

        // Need to do some calculation here so we are calling
        // gpu_frame_get_record_frame() at mFPS.
        long long newTimeMs =
                android::base::System::get()->getHighResTimeUs() / 1000;

        // Stop recording if time limit is reached
        if (newTimeMs - startMs >= globals.recordingInfo.timeLimit * 1000) {
            D("Time limit reached (%lld ms). Stopping the recording",
              newTimeMs - startMs);
            screen_recorder_stop_async();
            break;
        }

        long long timeDeltaMs = 1000 / fps;
        if (newTimeMs - currTimeMs < timeDeltaMs) {
            android::base::System::get()->sleepMs(timeDeltaMs - newTimeMs +
                                                  currTimeMs);
        }
    }

    // Send frame with null data to stop encoding thread.
    Frame f;
    globals.channel.send(std::move(f));
    D("Finished sending frames");
    return 0;
}

// Function to encode the frames
intptr_t encodeFrames() {
    auto& globals = *sGlobals;
    Frame f;
    int i = 0;

    while (1) {
        globals.channel.receive(&f);
        if (!f.pixels) {
            D("Received null frame");
            // We are done recording.
            break;
        }
        D("Received frame %d", i++);
        int pxSize = get_record_pixel_size(f.pix_fmt);
        ffmpeg_encode_video_frame(
                globals.recorder, (const uint8_t*)f.pixels.get(),
                f.width * f.height * pxSize, f.pt_us, f.pix_fmt);
    }

    D("Finished encoding");
    auto cvInfo = &globals.encodingCVInfo;
    cvInfo->lock.lock();
    cvInfo->done = true;
    cvInfo->cv.signal();
    cvInfo->lock.unlock();

    return 0;
}

// Function to stop the recording
static intptr_t stopRecording() {
    auto& globals = *sGlobals;

    globals.recordState = RecordState::Stopping;
    // Need to wait for encoding thread to finish before deleting the
    // encoder.
    globals.encodingCVInfo.lock.lock();
    while (!globals.encodingCVInfo.done) {
        globals.encodingCVInfo.cv.wait(&globals.encodingCVInfo.lock);
    }
    globals.encodingCVInfo.done = false;
    globals.encodingCVInfo.lock.unlock();

    bool has_frames = ffmpeg_delete_recorder(globals.recorder);
    gpu_frame_set_record_mode(false);

    if (globals.isGuestMode) {
        globals.displayAgent.unregisterUpdateListener(
                &screen_recorder_fb_update);
        AutoLock lock(globals.guestFBLock);
        globals.guestFrame.reset(nullptr);
    }

    if (globals.recordingInfo.cb) {
        globals.recordingInfo.cb(globals.recordingInfo.opaque,
                                 has_frames ? RECORD_STOP_FINISHED : RECORD_STOP_FAILED);
    }
    globals.recordingInfo.fileName = nullptr;
    globals.recorder = nullptr;

    globals.recordState = RecordState::Ready;
    return 0;
}
}  // namespace

void screen_recorder_init(int w, int h, const QAndroidDisplayAgent* dpy_agent) {
    auto& globals = *sGlobals;

    globals.fbWidth = w;
    globals.fbHeight = h;
    if (dpy_agent) {
        globals.displayAgent = *dpy_agent;
        globals.isGuestMode = true;
    } else {
        // The host framebuffer is always in a 4 bytes-per-pixel format.
        // In guest mode, the bpp is set when calling the getFrameBuffer()
        // method, so just use whatever bpp it gives us.
    }
    D("%s(w=%d, h=%d, isGuestMode=%d)", __func__, w, h, globals.isGuestMode);
}

bool parseRecordingInfo(const RecordingInfo* info) {
    auto& globals = *sGlobals;
    RecordingInfo* sinfo = &globals.recordingInfo;

    if (info == nullptr) {
        derror("Recording info is null");
        return false;
    }

    if (info->fileName == nullptr || strlen(info->fileName) == 0) {
        derror("Recording filename cannot be empty");
        return false;
    }

    if (info->width <= 0 || info->height <= 0) {
        D("Defaulting width and height to %dx%d", globals.fbWidth,
          globals.fbHeight);
        sinfo->width = globals.fbWidth;
        sinfo->height = globals.fbHeight;
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

    globals.fileName = info->fileName;
    sinfo->fileName = globals.fileName.c_str();

    return true;
}

int screen_recorder_start(const RecordingInfo* info) {
    auto& globals = *sGlobals;

    if (globals.recorder) {
        derror("Recorder already started");
        return false;
    }

    if (!parseRecordingInfo(info)) {
        return false;
    }

    globals.frameCount = 0;
    globals.recorder = ffmpeg_create_recorder(
            &globals.recordingInfo, globals.fbWidth, globals.fbHeight);
    if (!globals.recorder) {
        derror("ffmpeg_create_recorder failed");
        return false;
    }
    D("created recorder");

    // Add the video and audio tracks
    ffmpeg_add_video_track(globals.recorder, globals.recordingInfo.width,
                           globals.recordingInfo.height,
                           globals.recordingInfo.videoBitrate, kFPS,
                           kIntraSpacing);
    ffmpeg_add_audio_track(globals.recorder, kAudioBitrate, kAudioSampleRate);
    D("Added AV tracks");

    globals.recordState = RecordState::Recording;
    gpu_frame_set_record_mode(true);

    // Force a repost to setup the first frame
    if (globals.isGuestMode) {
        globals.displayAgent.registerUpdateListener(&screen_recorder_fb_update,
                                                    nullptr);
        qframebuffer_invalidate_all();
    } else {
        android_redrawOpenglesWindow();
    }

    // Start the two threads that will fetch and encode the frames
    auto sendFramesFunc = std::bind(&sendFrames, kFPS);
    android::base::async(sendFramesFunc);
    android::base::async(&encodeFrames);

    return true;
}

void screen_recorder_stop_async() {
    auto& globals = *sGlobals;

    if (!globals.recorder) {
        derror("Screen recording was never started");
        return;
    }

    if (globals.recordState != RecordState::Recording) {
        derror("Already stopping the recording");
        return;
    }

    if (globals.recordingInfo.cb) {
        globals.recordingInfo.cb(globals.recordingInfo.opaque,
                                 RECORD_STOP_INITIATED);
    }
    android::base::async(&stopRecording);
}
