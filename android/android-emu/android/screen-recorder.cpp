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
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/ffmpeg-muxer.h"
#include "android/gpu_frame.h"
#include "android/opengles.h"
#include "android/utils/debug.h"
#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace {

using android::base::Thread;

// Spacing between intra frames
constexpr int kIntraSpacing = 12;
// The maximum number of frames we will buffer
constexpr int kMaxFrames = 16;
// The FPS we are recording at.
constexpr int kFPS = 20;

// A small structure to model a single frame of the GPU display,
// as passed between the EmuGL and main loop thread.
struct Frame {
    int width;
    int height;
    void* pixels;

    Frame(int w, int h, const void* pixels) :
            width(w), height(h), pixels(NULL) {
        this->pixels = ::calloc(w * 4, h);
        if (this->pixels && pixels) {
            ::memcpy(this->pixels, pixels, w * 4 * h);
        }
    }

    ~Frame() {
        if (pixels)
            ::free(pixels);
    }
};

struct Globals {
    ffmpeg_recorder* recorder;
    Thread* frameSenderThread;
    Looper* mainLooper;
    int fbWidth;
    int fbHeight;
    std::atomic<bool> is_recording;
    void* frame;
    // serialize access to frame
    ::android::base::Lock frame_lock;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// A thread that will send for pixel data at whatever
// FPS we set it at.
//class EmptyThread : public Thread {
//public:
//    intptr_t main() { return 42; }
//};

// Two threads: one will be used to get the frames at some fps, and forward them
// to the second thread for processing. The second thread will be responsible
// for encoding the received frames.
//
// When the frame fetching thread receives a NULL, that means to stop the
// recording. So at this point, we need to send some type of signal through the
// MessageChannel so the encoding thread can finish.

// Thread to fetch the frames
class FrameSenderThread : public Thread {
public:
    FrameSenderThread(int fps) : Thread(), mFPS(fps) {}
    intptr_t main() {
        unsigned char* px;
        long long timeDeltaMs = 1000 / mFPS;
        long long currTimeMs, newTimeMs;
        int i = 0;

        // The assumption here when starting is that sGlobals->frame contains a
        // valid frame when is_recording is true.
        while (sGlobals->is_recording) {
            currTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
            {
                android::base::AutoLock lock(sGlobals->frame_lock);
                ffmpeg_encode_video_frame(sGlobals->recorder,
                                          (const uint8_t*)sGlobals->frame,
                                          sGlobals->fbWidth * sGlobals->fbHeight * 4);
            }
            // Need to do some calculation here so we are calling
            // android_getFrame() at mFPS.
            newTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
            if (newTimeMs - currTimeMs < timeDeltaMs) {
                android::base::System::get()->sleepMs(
                        timeDeltaMs - newTimeMs + currTimeMs);
            }
        }

        D("Finishing encoding\n");
        ffmpeg_delete_recorder(sGlobals->recorder);
        ::free(sGlobals->frame);
        sGlobals->frame = nullptr;
        sGlobals->recorder = nullptr;
        D("Finished encoding video\n");
        return 0;
    }

private:
    int mFPS;
};
}  // namespace

// Used as an emugl callback to receive each frame of GPU display.
static void screen_recorder_on_gpu_frame(void* context,
                                         int width,
                                         int height,
                                         const void* pixels) {
    if (!sGlobals->is_recording) {
        return;
    }

    {
        android::base::AutoLock lock(sGlobals->frame_lock);
        ::memcpy(sGlobals->frame, pixels, width * height * 4);
    }
    // Doesn't do anything if thread was already started or terminated.
    sGlobals->frameSenderThread->start();
}

void screen_recorder_init(Looper* looper, int w, int h) {
    sGlobals->recorder = nullptr;
    sGlobals->fbWidth = w;
    sGlobals->fbHeight = h;
    sGlobals->frameSenderThread = nullptr;
    sGlobals->frame = nullptr;
    // Run the post callback on the main loop thread, not Qt thread
    sGlobals->mainLooper = looper;
}

bool screen_recorder_start(const char* filename) {
    if (sGlobals->recorder) {
        derror("Recorder already started\n");
        return false;
    }

    sGlobals->recorder = ffmpeg_create_recorder(filename);
    if (!sGlobals->recorder) {
        derror("ffmpeg_create_recorder failed\n");
        return false;
    }
    D("created recorder\n");

    sGlobals->frame = calloc(sGlobals->fbWidth * 4,
                             sGlobals->fbHeight);
    if (!sGlobals->frame) {
        derror("Unable to allocate frame\n");
        ffmpeg_delete_recorder(sGlobals->recorder);
        sGlobals->recorder = nullptr;
        return false;
    }

    D("Adding av tracks\n");
    // Add the video and audio tracks
    ffmpeg_add_video_track(sGlobals->recorder,
                           sGlobals->fbWidth,
                           sGlobals->fbHeight,
                           4 * 1024 * 1024,
                           kFPS,
                           kIntraSpacing);
    ffmpeg_add_audio_track(sGlobals->recorder, 64 * 1024, 48000);
    // Start the two threads that will fetch and encode the frames
    sGlobals->frameSenderThread = new FrameSenderThread(kFPS);

    // The order of the below calls are very important. Since frameSenderThread
    // assumes that frame is valid as soon as it's started, we must make sure to
    // fill the data in it first. The callback and recording flag must also be
    // set prior to reposting so we are guaranteed to get a post call to fill
    // the frame buffer.
    gpu_frame_add_callback(sGlobals->mainLooper,
                           NULL,
                           screen_recorder_on_gpu_frame);
    sGlobals->is_recording = true;
    android_redrawOpenglesWindow();
    return true;
}

void screen_recorder_stop(void) {
    if (sGlobals->recorder) {
        sGlobals->is_recording = false;
        // Remove the callback
        gpu_frame_add_callback(nullptr, nullptr, nullptr);
        /*
        // Need to wait for encoding thread to finish before deleting the
        // encoder.
        sGlobals->encodingThread->wait();
        ffmpeg_delete_recorder(sGlobals->recorder);
        sGlobals->recorder = nullptr;
        */
        // TODO: Need some kind of signal to notify when the video is finished.
        // Maybe pass a callback here to call later?
    } else {
        derror("Screen recording was never started\n");
    }
}
