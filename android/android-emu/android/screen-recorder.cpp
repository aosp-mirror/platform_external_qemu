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
#include "android/screen-recorder-constants.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/ffmpeg-muxer.h"
#include "android/gpu_frame.h"
#include "android/opengles.h"
#include "android/utils/debug.h"

#include <atomic>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

// forward declaration
void _screen_recorder_stop(void);

namespace {

using android::base::Thread;

// Spacing between intra frames
constexpr int kIntraSpacing = 12;
// The maximum number of frames we will buffer
constexpr int kMaxFrames = 16;
// The FPS we are recording at.
constexpr int kFPS = 24;
// The video bitrate
constexpr int kVideoBitrate = 1 * 1024 * 1024;
// The audio bitrate
constexpr int kAudioBitrate = 64 * 1024;
// The audio sample rate
constexpr int kAudioSampleRate = 48000;

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

struct Globals {
    ffmpeg_recorder* recorder{nullptr};
    Thread* frameSenderThread = nullptr;
    Thread* encodingThread = nullptr;
    std::atomic<Thread*> stopRecordingThread{nullptr};
    int fbWidth = 0;
    int fbHeight = 0;
    bool isGuestMode = false;
    std::unique_ptr<char[]> filename;
    std::atomic<bool> is_recording{false};
    ::android::base::MessageChannel<Frame*, kMaxFrames> channel;
    RecordingInfo recordingInfo;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// Two threads: one will be used to get the frames at some fps, and forward them
// to the second thread for processing. The second thread will be responsible
// for encoding the received frames.

// Thread to fetch the frames
class FrameSenderThread : public Thread {
public:
    FrameSenderThread(int fps) : Thread(), mFPS(fps) {}
    intptr_t main() {
        unsigned char* px;
        long long timeDeltaMs = 1000 / mFPS;
        long long currTimeMs, newTimeMs;
        long long maxFrames = mFPS * kMaxTimeLimit; // time limit
        long long i = 0;

        // The assumption here when starting is that sGlobals->frame contains a
        // valid frame when is_recording is true.
        while (sGlobals->is_recording) {
            currTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
            px = (unsigned char*)gpu_frame_get_record_frame();
            if (px) {
                Frame* f = new Frame(sGlobals->fbWidth, sGlobals->fbHeight, px);
                D("sending frame %d\n", i);
                if (!sGlobals->channel.send(f)) {
                    derror("Frame queue full. Frame dropped\n");
                    delete f;
                } else {
                    if (++i == maxFrames) {
                        // time limit reached
                        D("Time limit reached. Stopping the recording\n");
                        screen_recorder_stop();
                        break;
                    }
                }

            }

            // Need to do some calculation here so we are calling
            // gpu_frame_get_record_frame() at mFPS.
            newTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;
            if (newTimeMs - currTimeMs < timeDeltaMs) {
                android::base::System::get()->sleepMs(
                        timeDeltaMs - newTimeMs + currTimeMs);
            }
        }

        // Send frame with null data to stop encoding thread.
        Frame* f = new Frame(0, 0, nullptr);
        sGlobals->channel.send(f);
        D("Finished sending frames\n");
        return 0;
    }

private:
    int mFPS;
};

// Thread to encode the frames
class EncodingThread: public Thread {
public:
    intptr_t main() {
        Frame* f;
        int i = 0;

        while (1) {
            f = nullptr;
            sGlobals->channel.receive(&f);
            if (!f || !f->pixels) {
                D("Received null frame\n");
                if (f) {
                    delete f;
                }
                // We are done recording.
                break;
            }
            D("Received frame %d\n", i++);
            ffmpeg_encode_video_frame(sGlobals->recorder,
                                      (const uint8_t*)f->pixels.get(),
                                      f->width * f->height * 4);
            delete f;
        }

        D("Finished encoding\n");
        return 0;
    }
};

// Thread to stop the recording
class StopRecordingThread : public Thread {
public:
    intptr_t main() {
        _screen_recorder_stop();

        return 0;
    }
};
}  // namespace

void screen_recorder_init(int isGuestMode, int w, int h) {
    sGlobals->fbWidth = w;
    sGlobals->fbHeight = h;
    sGlobals->isGuestMode = isGuestMode;
}

int screen_recorder_start(const RecordingInfo* info) {
    if (sGlobals->isGuestMode) {
        derror("Recording is only supported in host gpu configuration\n");
        return false;
    }

    if (sGlobals->recorder) {
        derror("Recorder already started\n");
        return false;
    }

    sGlobals->recorder = ffmpeg_create_recorder(info);
    if (!sGlobals->recorder) {
        derror("ffmpeg_create_recorder failed\n");
        return false;
    }
    D("created recorder\n");

    sGlobals->filename.reset(new char[strlen(info->filename) + 1]);
    strcpy(sGlobals->filename.get(), info->filename);
    sGlobals->recordingInfo.filename = sGlobals->filename.get();
    sGlobals->recordingInfo.width = info->width;
    sGlobals->recordingInfo.height = info->height;
    sGlobals->recordingInfo.bitrate = info->bitrate;
    sGlobals->recordingInfo.time_limit = info->time_limit;
    sGlobals->recordingInfo.cb = info->cb;
    sGlobals->recordingInfo.opaque = info->opaque;
    sGlobals->stopRecordingThread = nullptr;

    // Add the video and audio tracks
    ffmpeg_add_video_track(sGlobals->recorder,
                           sGlobals->fbWidth,
                           sGlobals->fbHeight,
                           kVideoBitrate,
                           kFPS,
                           kIntraSpacing);
    ffmpeg_add_audio_track(sGlobals->recorder, kAudioBitrate, kAudioSampleRate);
    D("Added AV tracks\n");
    // Start the two threads that will fetch and encode the frames
    sGlobals->frameSenderThread = new FrameSenderThread(kFPS);
    sGlobals->encodingThread = new EncodingThread();

    sGlobals->is_recording = true;
    gpu_frame_set_record_mode(true);
    android_redrawOpenglesWindow();

    sGlobals->encodingThread->start();
    sGlobals->frameSenderThread->start();

    return true;
}

void screen_recorder_stop(void) {
    if (!sGlobals->recorder) {
        derror("Screen recording was never started\n");
        return;
    }

    auto tmp = new StopRecordingThread();
    Thread* t = nullptr;
    if (sGlobals->stopRecordingThread.compare_exchange_strong(t, tmp)) {
        if (sGlobals->recordingInfo.cb) {
            sGlobals->recordingInfo.cb(sGlobals->recordingInfo.opaque, 0);
        }
        sGlobals->stopRecordingThread.load()->start();
    } else {
        derror("Recording already being stopped\n");
        delete tmp;
    }
}

void _screen_recorder_stop(void) {
    sGlobals->is_recording = false;
    // Need to wait for encoding thread to finish before deleting the
    // encoder.
    sGlobals->encodingThread->wait();
    ffmpeg_delete_recorder(sGlobals->recorder);
    gpu_frame_set_record_mode(false);
    // TODO: Need some kind of signal to notify when the video is finished.
    // Maybe pass a callback here to call later?
    if (sGlobals->recordingInfo.cb) {
        sGlobals->recordingInfo.cb(sGlobals->recordingInfo.opaque, 1);
    }
    sGlobals->filename.reset(nullptr);
    sGlobals->recordingInfo.filename = nullptr;
    sGlobals->recorder = nullptr;
}
