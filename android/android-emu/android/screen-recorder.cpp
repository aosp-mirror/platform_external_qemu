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
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/ffmpeg-muxer.h"
#include "android/gpu_frame.h"
#include "android/opengles.h"
#include "android/screen-recorder-constants.h"
#include "android/utils/debug.h"

#include <atomic>

extern "C" {
#include "android/framebuffer.h"
}

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

// forward declaration
void screen_recorder_stop(void);

namespace {

using android::base::AutoLock;
using android::base::Thread;

// The maximum number of frames we will buffer
constexpr int kMaxFrames = 16;

// A small structure to model a single frame of the GPU display,
// as passed between the EmuGL and main loop thread.
struct Frame {
    int width;
    int height;
    uint64_t pt_us;  // presentation time in microseconds
    int bpp;
    std::unique_ptr<char[]> pixels;

    Frame(int w, int h, uint64_t pt, int b, const void* px)
        : width(w), height(h), pt_us(pt), bpp(b) {
        if (px) {
            pixels.reset(new char[w * bpp * h]);
            ::memcpy(static_cast<void*>(pixels.get()), px, w * bpp * h);
        }
    }

    ~Frame() {
        pixels.reset();
    }
};

struct Globals {
    ffmpeg_recorder* recorder = nullptr;
    Thread* frameSenderThread = nullptr;
    Thread* encodingThread = nullptr;
    std::atomic<Thread*> stopRecordingThread{nullptr};
    QAndroidDisplayAgent displayAgent;
    int fbWidth = 0;
    int fbHeight = 0;
    int fbBpp = 0;
    bool isGuestMode = false;
    std::unique_ptr<char[]> filename;
    ::android::base::Lock guestFBLock;
    int frameCount = 0;
    Frame* guestFrame = nullptr;
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
    intptr_t main() override final {
        int i = 0;

        // The assumption here when starting is that sGlobals->frame contains a
        // valid frame when is_recording is true.
        long long startMs = android::base::System::get()->getHighResTimeUs() / 1000;
        while (sGlobals->is_recording) {
            long long currTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;

            Frame* f = nullptr;
            if (!sGlobals->isGuestMode) {
                auto px = (unsigned char*)gpu_frame_get_record_frame();
                if (px) {
                    f = new Frame(
                            sGlobals->fbWidth,
                            sGlobals->fbHeight,
                            android::base::System::get()->getHighResTimeUs(),
                            sGlobals->fbBpp,
                            px);
                }
            } else {
                AutoLock lock(sGlobals->guestFBLock);
                if (sGlobals->guestFrame) {
                    f = new Frame(
                            sGlobals->guestFrame->width,
                            sGlobals->guestFrame->height,
                            android::base::System::get()->getHighResTimeUs(),
                            sGlobals->guestFrame->bpp,
                            sGlobals->guestFrame->pixels.get());
                }
            }

            if (f) {
                D("sending frame %d\n", i++);
                if (!sGlobals->channel.trySend(f)) {
                    derror("Frame queue full. Frame dropped\n");
                    delete f;
                }
            }

            // Need to do some calculation here so we are calling
            // gpu_frame_get_record_frame() at mFPS.
            long long newTimeMs = android::base::System::get()->getHighResTimeUs() / 1000;

            // Stop recording if time limit is reached
            if (newTimeMs - startMs >=
                sGlobals->recordingInfo.time_limit * 1000) {
                D("Time limit reached. Stopping the recording\n");
                screen_recorder_stop_async();
                break;
            }

            long long timeDeltaMs = 1000 / mFPS;
            if (newTimeMs - currTimeMs < timeDeltaMs) {
                android::base::System::get()->sleepMs(
                        timeDeltaMs - newTimeMs + currTimeMs);
            }
        }

        // Send frame with null data to stop encoding thread.
        Frame* f = new Frame(0, 0, 0, 0, nullptr);
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
    intptr_t main() override final {
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
            ffmpeg_encode_video_frame(
                    sGlobals->recorder, (const uint8_t*)f->pixels.get(),
                    f->width * f->height * f->bpp, f->pt_us, f->bpp);
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
        screen_recorder_stop();

        return 0;
    }
};
}  // namespace

void _screen_recorder_fb_update(void* opaque, int x, int y, int width, int height) {
    D("[%d] %s(x=%d, y=%d, w=%d, h=%d)\n", ++sGlobals->frameCount, __func__, x, y, width, height);

    if (width <= 0 || height <= 0)
        return;

    int w = 0, h = 0, bpp = 0;
    unsigned char* px = nullptr;
    sGlobals->displayAgent.getFrameBuffer(&w, &h, nullptr, &bpp, &px);
    if (!px || w <= 0 || h <= 0) {
        return;
    }

    AutoLock lock(sGlobals->guestFBLock);

    if (!sGlobals->guestFrame) {
        sGlobals->guestFrame = new Frame(
                w, h, android::base::System::get()->getHighResTimeUs(),
                bpp, px);
    } else if (w == sGlobals->guestFrame->width &&
               h == sGlobals->guestFrame->height) {
        memcpy(static_cast<void*>(sGlobals->guestFrame->pixels.get()),
               px, w * h * bpp);
    }
}

void screen_recorder_init(int w, int h, const QAndroidDisplayAgent* dpy_agent) {
    sGlobals->fbWidth = w;
    sGlobals->fbHeight = h;
    if (dpy_agent) {
        sGlobals->displayAgent = *dpy_agent;
        sGlobals->isGuestMode = true;
    } else {
        // The host framebuffer is always in a 4 bytes-per-pixel format.
        // In guest mode, the bpp is set when calling the getFrameBuffer()
        // method, so just use whatever bpp it gives us.
        sGlobals->fbBpp = 4;
    }
    D("%s(w=%d, h=%d, isGuestMode=%d)\n", __func__, w, h, sGlobals->isGuestMode);
}

bool parse_recording_info(const RecordingInfo* info) {
    RecordingInfo* sinfo = &sGlobals->recordingInfo;

    if (info == nullptr) {
        derror("Recording info is null\n");
        return false;
    }

    if (info->filename == nullptr || strlen(info->filename) == 0) {
        derror("Recording filename cannot be empty\n");
        return false;
    }

    if (info->width <= 0 || info->height <= 0) {
        D("Defaulting width and height to %dx%d\n", sGlobals->fbWidth,
          sGlobals->fbHeight);
        sinfo->width = sGlobals->fbWidth;
        sinfo->height = sGlobals->fbHeight;
    } else {
        sinfo->width = info->width;
        sinfo->height = info->height;
    }

    if (info->video_bitrate < kMinVideoBitrate ||
        info->video_bitrate > kMaxVideoBitrate) {
        D("Defaulting video_bitrate to %d bps\n", kDefaultVideoBitrate);
        sinfo->video_bitrate = kDefaultVideoBitrate;
    } else {
        sinfo->video_bitrate = info->video_bitrate;
    }

    if (info->time_limit < 1 || info->time_limit > kMaxTimeLimit) {
        D("Defaulting time limit to %d seconds\n", kMaxTimeLimit);
        sinfo->time_limit = kMaxTimeLimit;
    } else {
        sinfo->time_limit = info->time_limit;
    }

    sinfo->cb = info->cb;
    sinfo->opaque = info->opaque;

    sGlobals->filename.reset(new char[strlen(info->filename) + 1]);
    strcpy(sGlobals->filename.get(), info->filename);
    sinfo->filename = sGlobals->filename.get();

    return true;
}

int screen_recorder_start(const RecordingInfo* info) {
    if (sGlobals->recorder) {
        derror("Recorder already started\n");
        return false;
    }

    if (!parse_recording_info(info)) {
        return false;
    }

    sGlobals->frameCount = 0;
    sGlobals->recorder = ffmpeg_create_recorder(&sGlobals->recordingInfo);
    if (!sGlobals->recorder) {
        derror("ffmpeg_create_recorder failed\n");
        return false;
    }
    D("created recorder\n");

    sGlobals->stopRecordingThread = nullptr;

    // Add the video and audio tracks
    ffmpeg_add_video_track(
            sGlobals->recorder, sGlobals->fbWidth, sGlobals->fbHeight,
            sGlobals->recordingInfo.video_bitrate, kFPS, kIntraSpacing);
    ffmpeg_add_audio_track(sGlobals->recorder, kAudioBitrate, kAudioSampleRate);
    D("Added AV tracks\n");

    // Start the two threads that will fetch and encode the frames
    sGlobals->frameSenderThread = new FrameSenderThread(kFPS);
    sGlobals->encodingThread = new EncodingThread();

    sGlobals->is_recording = true;
    gpu_frame_set_record_mode(true);

    // Force a repost to setup the first frame
    if (sGlobals->isGuestMode) {
        sGlobals->displayAgent.attachRecordUpdateListener(&_screen_recorder_fb_update, nullptr);
        qframebuffer_invalidate_all();
    } else {
        android_redrawOpenglesWindow();
    }

    sGlobals->encodingThread->start();
    sGlobals->frameSenderThread->start();

    return true;
}

void screen_recorder_stop_async(void) {
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

void screen_recorder_stop(void) {
    sGlobals->is_recording = false;
    // Need to wait for encoding thread to finish before deleting the
    // encoder.
    sGlobals->encodingThread->wait();
    ffmpeg_delete_recorder(sGlobals->recorder);
    gpu_frame_set_record_mode(false);

    if (sGlobals->isGuestMode) {
        sGlobals->displayAgent.attachRecordUpdateListener(nullptr, nullptr);
        AutoLock lock(sGlobals->guestFBLock);
        if (sGlobals->guestFrame) {
            delete sGlobals->guestFrame;
            sGlobals->guestFrame = nullptr;
        }
    }

    if (sGlobals->recordingInfo.cb) {
        sGlobals->recordingInfo.cb(sGlobals->recordingInfo.opaque, 1);
    }
    sGlobals->filename.reset(nullptr);
    sGlobals->recordingInfo.filename = nullptr;
    sGlobals->recorder = nullptr;
}
