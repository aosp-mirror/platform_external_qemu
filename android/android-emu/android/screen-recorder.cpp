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
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/ffmpeg-muxer.h"
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
        if (pixels) {
            this->pixels = ::calloc(w * 4, h);
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
    Thread* frameThread;
    Thread* encodingThread;
    int fbWidth;
    int fbHeight;
    bool requestStop;
    ::android::base::MessageChannel<Frame*, kMaxFrames> channel;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// A thread that will request for pixel data at whatever
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
class FrameThread : public Thread {
public:
    FrameThread(int fps) : Thread(), mFPS(fps) {}
    intptr_t main() {
        unsigned char* px;
        long long timeDeltaMs = 1000 / mFPS;
        long long currTimeMs, newTimeMs;
        int i = 0;

        while (!sGlobals->requestStop || i % kIntraSpacing != 0) {
            currTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
            px = android_getFrame();
            Frame* frame = new Frame(sGlobals->fbWidth, sGlobals->fbHeight, px);
            D("sending frame %d\n", i++);
            if(!sGlobals->channel.send(frame)) {
                derror("Frame queue full. Frame dropped.\n");
            }
            if (!px) {
              break;
            }
            // Need to do some calculation here so we are calling
            // android_getFrame() at mFPS.
            newTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
            if (newTimeMs - currTimeMs < timeDeltaMs) {
                android::base::System::get()->sleepMs(
                        timeDeltaMs - newTimeMs + currTimeMs);
            }
        }

        // send null frame to stop encoding thread
        android_stopRecording();
        Frame* frame = new Frame(sGlobals->fbWidth, sGlobals->fbHeight, nullptr);
        sGlobals->channel.send(frame);
        D("finished sending frames\n");
        return 0;
    }

private:
    int mFPS;
};

// Thread to encode the frames
class EncodingThread : public Thread {
public:
    intptr_t main() {
        Frame* frame;
        int i = 0;

        while (1) {
            frame = nullptr;
            sGlobals->channel.receive(&frame);
            if (!frame || !frame->pixels) {
                if (frame)
                    delete frame;
                // We are done recording.
                break;
            }
            D("received frame %d\n", i++);
            ffmpeg_encode_video_frame(sGlobals->recorder,
                                      (const uint8_t*)frame->pixels,
                                      frame->width * frame->height * 4);
            delete frame;
        }

        D("finished encoding\n");
        return 0;
    }
};
}  // namespace

void screen_recorder_init(int w, int h) {
    sGlobals->recorder = nullptr;
    sGlobals->fbWidth = w;
    sGlobals->fbHeight = h;
    sGlobals->encodingThread = nullptr;
    sGlobals->frameThread = nullptr;
}

bool screen_recorder_start(const char* filename) {
    if (!sGlobals->recorder) {
        sGlobals->requestStop = false;
        sGlobals->recorder = ffmpeg_create_recorder(filename);
        D("created recorder\n");
        if (sGlobals->recorder) {
            if (!android_startRecording()) {
                D("Starting recording\n");
                // Add the video and audio tracks
                ffmpeg_add_video_track(sGlobals->recorder,
                                       sGlobals->fbWidth,
                                       sGlobals->fbHeight,
                                       4 * 1024 * 1024,
                                       kFPS,
                                       kIntraSpacing);
                ffmpeg_add_audio_track(sGlobals->recorder, 64 * 1024, 48000);
                // Start the two threads that will fetch and encode the frames
                sGlobals->encodingThread = new EncodingThread();
                sGlobals->frameThread = new FrameThread(kFPS);
                // Start the encoding thread first so we are consuming the
                // frames when we start the frame thread.
                sGlobals->encodingThread->start();
                sGlobals->frameThread->start();
                return true;
            } else {
                derror("android_startRecording failed\n");
            }
        } else {
            derror("ffmpeg_create_recorder failed\n");
        }
    } else {
        derror("Recorder already started\n");
    }

    return false;
}

void screen_recorder_stop(void) {
    if (sGlobals->recorder) {
        sGlobals->requestStop = true;
        // Need to wait for encoding thread to finish before deleting the
        // encoder.
        sGlobals->encodingThread->wait();
        ffmpeg_delete_recorder(sGlobals->recorder);
        sGlobals->recorder = nullptr;
    } else {
        derror("Screen recording was never started\n");
    }
}
