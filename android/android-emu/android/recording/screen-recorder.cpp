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

#include "android/recording/screen-recorder.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulator-window.h"
#include "android/recording/FfmpegRecorder.h"
#include "android/recording/audio/AudioProducer.h"
#include "android/recording/codecs/audio/VorbisCodec.h"
#include "android/recording/codecs/video/VP9Codec.h"
#include "android/recording/screen-recorder-constants.h"
#include "android/recording/video/VideoProducer.h"
#include "android/recording/video/VideoFrameSharer.h"
#include "android/utils/debug.h"

#include <atomic>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace {

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::Lock;
using android::recording::AudioFormat;
using android::recording::CodecParams;
using android::recording::FfmpegRecorder;
using android::recording::VorbisCodec;
using android::recording::VP9Codec;
using android::recording::VideoFrameSharer;

// The incoming audio sample format.
constexpr AudioFormat kInSampleFormat = AudioFormat::AUD_FMT_S16;

// forward declaration
class ScreenRecorder;

struct Globals {
    // Need to lock when accessing this global struct because the console and ui
    // may be accessing this concurrently.
    Lock lock;
    std::unique_ptr<ScreenRecorder> recorder;
    std::unique_ptr<VideoFrameSharer> webrtc_module;
    const QAndroidDisplayAgent* displayAgent = nullptr;
    uint32_t fbWidth = 0;
    uint32_t fbHeight = 0;
    QFrameBuffer dummyQf = {};
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

class ScreenRecorder {
public:
    explicit ScreenRecorder(uint32_t fbWidth,
                            uint32_t fbHeight,
                            const RecordingInfo* info,
                            const QAndroidDisplayAgent* agent);

    virtual ~ScreenRecorder();

    bool startRecording(bool async);
    bool stopRecording(bool async);

    RecorderState getRecorderState() const;

private:
    bool startRecordingWorker();
    bool stopRecordingWorker();

    bool startFfmpegRecorder();
    void sendRecordingStatus(RecordingStatus status);
    bool parseRecordingInfo(RecordingInfo& info);

private:
    std::unique_ptr<FfmpegRecorder> ffmpegRecorder;
    uint32_t mFbWidth = 0;
    uint32_t mFbHeight = 0;

    std::string mFilename;
    RecordingInfo mInfo = {0};
    const QAndroidDisplayAgent* mAgent = nullptr;
    std::atomic<RecorderState> mRecorderState{RECORDER_STARTING};
    QFrameBuffer mDummyQf;

    FunctorThread mStartThread;
    FunctorThread mStopThread;
    FunctorThread mTimeoutThread;

    // Time-limit stop
    ConditionVariable mCond;
    bool mFinished = false;
    Lock mLock;
};

ScreenRecorder::ScreenRecorder(uint32_t fbWidth,
                               uint32_t fbHeight,
                               const RecordingInfo* info,
                               const QAndroidDisplayAgent* agent)
    : mFbWidth(fbWidth),
      mFbHeight(fbHeight),
      mInfo(*info),
      mAgent(agent),
      mStartThread([this]() { startRecordingWorker(); }),
      mStopThread([this]() { stopRecordingWorker(); }),
      mTimeoutThread([this]() {
          auto timeoutTimeUs = android::base::System::get()->getUnixTimeUs() +
                               mInfo.timeLimit * 1000000;
          {
              AutoLock lock(mLock);
              while (!mFinished && mCond.timedWait(&mLock, timeoutTimeUs))
                  ;
          }

          if (!mFinished) {
              stopRecording(false);
          }
      }) {
    // Copy over the RecordingInfo struct. Need to copy the filename
    // string because info->fileName is pointing to data that we don't own.
    D("RecordingInfo "
      "{\n\tfileName=[%s],\n\twidth=[%u],\n\theight=[%u],\n\tvideoBitrate=[%u],"
      "\n\ttimeLimit=[%u],\n\tcb=[%p],\n\topaque=[%p]\n}\n",
      info->fileName, info->width, info->height, info->videoBitrate,
      info->timeLimit, info->cb, info->opaque);
    mFilename = info->fileName;
    mInfo.fileName = mFilename.c_str();
}

ScreenRecorder::~ScreenRecorder() {
    AutoLock lock(mLock);
    mFinished = true;
    mCond.signalAndUnlock(&lock);
    mStartThread.wait();
    mStopThread.wait();
    mTimeoutThread.wait();
}

// Simple function to check callback for null before using
void ScreenRecorder::sendRecordingStatus(RecordingStatus status) {
    if (mInfo.cb) {
        mInfo.cb(mInfo.opaque, status);
    }
}

// Start the recording
bool ScreenRecorder::startRecording(bool async) {
    assert(mRecorderState == RECORDER_STARTING);
    if (async) {
        mStartThread.start();
        return true;
    } else {
        return startRecordingWorker();
    }
}
bool ScreenRecorder::startRecordingWorker() {
    sendRecordingStatus(RECORD_START_INITIATED);

    if (!parseRecordingInfo(mInfo)) {
        LOG(ERROR) << "Recording info is invalid";
        mRecorderState = RECORDER_STOPPED;
        sendRecordingStatus(RECORD_START_FAILED);
        return false;
    }

    ffmpegRecorder = FfmpegRecorder::create(mFbWidth, mFbHeight, mFilename,
                                            kContainerFormat);
    if (!ffmpegRecorder->isValid()) {
        LOG(ERROR) << "Unable to create recorder";
        mRecorderState = RECORDER_STOPPED;
        sendRecordingStatus(RECORD_START_FAILED);
        return false;
    }
    D("Created recorder");

    // Add the video track
    auto videoProducer = android::recording::createVideoProducer(
            mFbWidth, mFbHeight, kFPS, mAgent);
    // Fill in the codec params for the audio and video
    CodecParams videoParams;
    videoParams.width = mInfo.width;
    videoParams.height = mInfo.height;
    videoParams.bitrate = mInfo.videoBitrate;
    videoParams.fps = kFPS;
    videoParams.intra_spacing = kIntraSpacing;
    VP9Codec videoCodec(
            std::move(videoParams), mFbWidth, mFbHeight,
            toAVPixelFormat(videoProducer->getFormat().videoFormat));
    if (!ffmpegRecorder->addVideoTrack(std::move(videoProducer),
                                       &videoCodec)) {
        LOG(ERROR) << "Failed to add video track";
        mRecorderState = RECORDER_STOPPED;
        sendRecordingStatus(RECORD_START_FAILED);
        return false;
    }

    // Add the audio track
    auto audioProducer = android::recording::createAudioProducer(
            kAudioSampleRate, kSrcNumSamples, kInSampleFormat,
            kNumAudioChannels);
    CodecParams audioParams;
    audioParams.sample_rate = kAudioSampleRate;
    audioParams.bitrate = kAudioBitrate;
    VorbisCodec audioCodec(
            std::move(audioParams),
            toAVSampleFormat(audioProducer->getFormat().audioFormat));
    if (!ffmpegRecorder->addAudioTrack(std::move(audioProducer),
                                       &audioCodec)) {
        LOG(ERROR) << "Failed to add audio track";
        mRecorderState = RECORDER_STOPPED;
        sendRecordingStatus(RECORD_START_FAILED);
        return false;
    }

    D("Added AV tracks");

    ffmpegRecorder->start();

    // Start the timer that will stop the recording when the time-limit is
    // reached.
    mTimeoutThread.start();

    mRecorderState = RECORDER_RECORDING;
    sendRecordingStatus(RECORD_STARTED);

    return true;
}

// Function to stop the recording
bool ScreenRecorder::stopRecording(bool async) {
    auto current = RECORDER_RECORDING;
    if (!mRecorderState.compare_exchange_strong(current, RECORDER_STOPPING)) {
        D("Recording already stopping or already stopped\n");
        return false;
    }

    if (async) {
        mStopThread.start();
        return true;
    } else {
        return stopRecordingWorker();
    }
}

bool ScreenRecorder::stopRecordingWorker() {
    sendRecordingStatus(RECORD_STOP_INITIATED);

    {
        // Stop the timer thread
        AutoLock lock(mLock);
        mFinished = true;
        mCond.signalAndUnlock(&lock);
    }

    bool has_frames = ffmpegRecorder->stop();
    ffmpegRecorder.reset();

    mRecorderState = RECORDER_STOPPED;
    sendRecordingStatus(has_frames ? RECORD_STOPPED : RECORD_STOP_FAILED);

    return has_frames;
}

bool ScreenRecorder::parseRecordingInfo(RecordingInfo& info) {
    if (info.fileName == nullptr || strlen(info.fileName) == 0) {
        LOG(ERROR) << "Recording filename cannot be empty";
        return false;
    }

    if (info.width <= 0 || info.height <= 0) {
        D("Defaulting width and height to %dx%d", mFbWidth, mFbHeight);
        info.width = mFbWidth;
        info.height = mFbHeight;
    }

    if (info.videoBitrate < kMinVideoBitrate ||
        info.videoBitrate > kMaxVideoBitrate) {
        D("Defaulting videoBitrate to %d bps", kDefaultVideoBitrate);
        info.videoBitrate = kDefaultVideoBitrate;
    }

    if (info.timeLimit < 1 || info.timeLimit > kMaxTimeLimit) {
        D("Defaulting time limit to %d seconds", kMaxTimeLimit);
        info.timeLimit = kMaxTimeLimit;
    }

    return true;
}

RecorderState ScreenRecorder::getRecorderState() const {
    return mRecorderState;
}
}  // namespace

// C compatibility functions
RecorderState screen_recorder_state_get(void) {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);
    if (globals.recorder) {
        return globals.recorder->getRecorderState();
    } else {
        return RECORDER_STOPPED;
    }
}


void screen_recorder_init(uint32_t w,
                          uint32_t h,
                          const QAndroidDisplayAgent* dpy_agent) {
    assert(w > 0 && h > 0);
    auto& globals = *sGlobals;
    globals.fbWidth = w;
    globals.fbHeight = h;
    globals.displayAgent = dpy_agent;

    if (dpy_agent) {
        if (emulator_window_get()->opts->no_window) {
            // no-window mode in gpu guest
            // Need to make a dummy producer to use the invalidate and fb check
            // callbacks, so we can request for a repost when we start
            // recording.
            qframebuffer_init_no_window(&globals.dummyQf);
            qframebuffer_fifo_add(&globals.dummyQf);
            dpy_agent->initFrameBufferNoWindow(&globals.dummyQf);
        }
    }

    D("%s(w=%d, h=%d, isGuestMode=%d)", __func__, w, h, dpy_agent != nullptr);
}

bool screen_recorder_start(const RecordingInfo* info, bool async) {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);

    globals.recorder.reset(new ScreenRecorder(globals.fbWidth, globals.fbHeight,
                                              info, globals.displayAgent));
    return globals.recorder->startRecording(async);
}

bool screen_recorder_stop(bool async) {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);

    if (globals.recorder) {
        return globals.recorder->stopRecording(async);
    }

    return false;
}

bool start_webrtc_module(const char* handle, int fps) {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);
    if (globals.webrtc_module) {
        globals.webrtc_module->stop();
    }

    auto producer = android::recording::createVideoProducer(
            globals.fbWidth, globals.fbHeight, fps, globals.displayAgent);
    globals.webrtc_module.reset(new VideoFrameSharer(
            globals.fbWidth, globals.fbHeight, fps, handle));

    // We can fail due to shared memory allocation issues.
    if (!globals.webrtc_module->attachProducer(std::move(producer)))
        return false;

    globals.webrtc_module->start();
    return true;
}

bool stop_webrtc_module() {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);

    if (globals.webrtc_module) {
        globals.webrtc_module->stop();
    }

    return true;
}

extern "C" {
bool start_webrtc(const char* handle, int fps) {
    return start_webrtc_module(handle, fps);
}

bool stop_webrtc() {
    return stop_webrtc_module();
}
}

