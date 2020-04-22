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

#include <assert.h>                                          // for assert
#include <string.h>                                          // for strlen
#include <atomic>                                            // for atomic
#include <functional>                                        // for __base
#include <memory>                                            // for unique_ptr
#include <string>                                            // for basic_st...
#include <thread>
#include <utility>                                           // for move

#include "android/android.h"                                 // for android_...
#include "android/base/Log.h"                                // for LOG, Log...
#include "android/base/memory/LazyInstance.h"                // for LazyInst...
#include "android/base/misc/StringUtils.h"                   // for split
#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/base/system/System.h"                      // for System
#include "android/base/threads/FunctorThread.h"              // for FunctorT...
#include "android/emulator-window.h"                         // for emulator...
#include "android/framebuffer.h"                             // for qframebu...
#include "android/gpu_frame.h"                               // for gpu_fram...
#include "android/recording/FfmpegRecorder.h"                // for FfmpegRe...
#include "android/recording/Frame.h"                         // for toAVPixe...
#include "android/recording/Producer.h"                      // for Producer
#include "android/recording/audio/AudioProducer.h"           // for createAu...
#include "android/recording/codecs/Codec.h"                  // for CodecParams
#include "android/recording/codecs/audio/VorbisCodec.h"      // for VorbisCodec
#include "android/recording/codecs/video/VP9Codec.h"         // for VP9Codec
#include "android/recording/screen-recorder-constants.h"     // for kMaxTime...
#include "android/recording/video/VideoFrameSharer.h"        // for VideoFra...
#include "android/recording/video/VideoProducer.h"           // for createVi...
#include "android/utils/debug.h"                             // for VERBOSE_...

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__)

namespace {

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::Lock;
using android::recording::AudioFormat;
using android::recording::CodecParams;
using android::recording::FfmpegRecorder;
using android::recording::VideoFrameSharer;
using android::recording::VorbisCodec;
using android::recording::VP9Codec;

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
    std::string sharedMemoryHandle;
    const QAndroidDisplayAgent* displayAgent = nullptr;
    const QAndroidMultiDisplayAgent* multiDisplayAgent = nullptr;
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

    RecorderStates getRecorderState() const;

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
      "\n\ttimeLimit=[%u],\n\tdisplay=[%u],\n\tcb=[%p],\n\topaque=[%p]\n}\n",
      info->fileName, info->width, info->height, info->videoBitrate,
      info->timeLimit, info->displayId, info->cb, info->opaque);
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
            mFbWidth, mFbHeight, mInfo.fps, mInfo.displayId, mAgent);
    // Fill in the codec params for the audio and video
    CodecParams videoParams;
    videoParams.width = mInfo.width;
    videoParams.height = mInfo.height;
    videoParams.bitrate = mInfo.videoBitrate;
    videoParams.fps = mInfo.fps;
    videoParams.intra_spacing = kIntraSpacing;
    VP9Codec videoCodec(
            std::move(videoParams), mFbWidth, mFbHeight,
            toAVPixelFormat(videoProducer->getFormat().videoFormat));
    if (!ffmpegRecorder->addVideoTrack(std::move(videoProducer), &videoCodec)) {
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
    if (!ffmpegRecorder->addAudioTrack(std::move(audioProducer), &audioCodec)) {
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

    if (info.timeLimit < 1) {
        D("Defaulting time limit to %d seconds", kMaxTimeLimit);
        info.timeLimit = kMaxTimeLimit;
    } else if (info.timeLimit > kMaxTimeLimit &&
               !(android_cmdLineOptions && android_cmdLineOptions->record_session)) {
        // Allow indefinite recording for emulator recording sessions.
        D("Defaulting time limit to %d seconds", kMaxTimeLimit);
        info.timeLimit = kMaxTimeLimit;
    }

    if (info.fps == 0) {
        fprintf(stderr, "Defaulting fps to %u fps\n", kFPS);
        info.fps = kFPS;
    }
    return true;
}

RecorderStates ScreenRecorder::getRecorderState() const {
    RecorderStates ret = {mRecorderState, mInfo.displayId};
    return ret;
}
}  // namespace

// C compatibility functions
RecorderStates screen_recorder_state_get(void) {
    auto& globals = *sGlobals;
    RecorderStates ret = {RECORDER_STOPPED, 0};

    AutoLock lock(globals.lock);
    if (globals.recorder) {
        ret = globals.recorder->getRecorderState();
    }
    return ret;
}

static void screen_recorder_record_session(const char* cmdLineArgs) {
    // Format is <filename>,<delay[,<duration>].
    // If no duration, then record until the emulator shuts down.
    std::vector<std::string> tokens;
    android::base::split(cmdLineArgs, ",", [&tokens](android::base::StringView s) {
        if (!s.empty()) {
            tokens.push_back(s);
        }
    });

    if (tokens.size() < 2) {
        fprintf(stderr, "Not enough arguments for record-session\n");
        return;
    }

    // Validate filename
    if (!android::base::endsWith(tokens[0], ".webm")) {
        fprintf(stderr, "Filename must end with .webm extension\n");
        return;
    }

    // Get delay
    int delay = atoi(tokens[1].c_str());

    // Get duration
    int duration = INT32_MAX;
    if (tokens.size() > 2) {
        duration = atoi(tokens[2].c_str());
    }

    std::thread(std::bind([](std::string filename, int delay, int duration) {
        RecordingInfo info = {};
        if (delay > 0) {
            android::base::Thread::sleepMs(delay * 1000);
        }
        info.fileName = filename.c_str();
        info.timeLimit = duration;
        // Make the quality and fps low, so we don't have so much cpu usage.
        info.fps = 3;
        info.videoBitrate = 500000;
        info.displayId = 0;
        screen_recorder_start(&info, true);
    }, tokens[0], delay, duration)).detach();
}
void screen_recorder_init(uint32_t w,
                          uint32_t h,
                          const QAndroidDisplayAgent* dpy_agent,
                          const QAndroidMultiDisplayAgent* mdpy_agent) {
    assert(w > 0 && h > 0);
    auto& globals = *sGlobals;
    globals.fbWidth = w;
    globals.fbHeight = h;
    globals.displayAgent = dpy_agent;
    globals.multiDisplayAgent = mdpy_agent;

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

    if (android_cmdLineOptions && android_cmdLineOptions->record_session) {
        screen_recorder_record_session(android_cmdLineOptions->record_session);
    }

    D("%s(w=%d, h=%d, isGuestMode=%d)", __func__, w, h, dpy_agent != nullptr);
}

bool screen_recorder_start(const RecordingInfo* info, bool async) {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);

    // Check if the display is created. For guest mode, display 0 return true.
    uint32_t w, h, cb = 0;
    if (globals.multiDisplayAgent->getMultiDisplay(info->displayId, nullptr, nullptr,
                                                   &w, &h, nullptr,
                                                   nullptr, nullptr) == false) {
        return false;
    }
    if (info->displayId != 0) {
        globals.multiDisplayAgent->getDisplayColorBuffer(info->displayId, &cb);
        if (cb == 0) {
            return false;
        }
    }

    globals.recorder.reset(new ScreenRecorder(w, h, info, globals.displayAgent));
    return globals.recorder->startRecording(async);
}

bool screen_recorder_stop(bool async) {
    auto& globals = *sGlobals;

    AutoLock lock(globals.lock);

    if (globals.recorder) {
        globals.recorder->stopRecording(async);
        globals.recorder.reset();
        return true;
    }
    return false;
}

const char* start_shared_memory_module(int fps) {
    auto& globals = *sGlobals;

    // emulator name --> shared memory handle.
    // 1. Discovery is easier.
    // 2. If we accidentally leak a region on posix, we can reclaim an clean up
    // on 2nd run.
    globals.sharedMemoryHandle = std::string("videmulator") +
                                 std::to_string(android_serial_number_port);
    if (!globals.webrtc_module) {
        globals.webrtc_module.reset(
                new VideoFrameSharer(globals.fbWidth, globals.fbHeight,
                                     globals.sharedMemoryHandle.c_str()));

        if (!globals.webrtc_module->initialize()) {
            LOG(ERROR) << "Unable to initialize frame sharing.. disabling "
                          "frame sharing.";
            globals.webrtc_module.reset(nullptr);
            return nullptr;
        }
    }

    globals.webrtc_module->start();
    gpu_frame_set_record_mode(true);
    const char* handle = globals.sharedMemoryHandle.c_str();
    D("%s(handle=%s, fps=%d)", __func__, handle, fps);
    return handle;
}

bool stop_shared_memory_module() {
    gpu_frame_set_record_mode(false);
    return true;
}

extern "C" {
const char* start_webrtc(int fps) {
    return start_shared_memory_module(fps);
}

bool stop_webrtc() {
    return stop_shared_memory_module();
}
}
