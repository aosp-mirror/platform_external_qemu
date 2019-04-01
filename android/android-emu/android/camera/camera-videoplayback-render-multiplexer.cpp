/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "android/camera/camera-videoplayback-render-multiplexer.h"

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/recording/video/player/VideoPlayerNotifier.h"
#include "android/utils/looper.h"

namespace android {
namespace videoplayback {

class VideoplaybackNotifier : public android::videoplayer::VideoPlayerNotifier {
public:
    VideoplaybackNotifier();
    virtual ~VideoplaybackNotifier() override;

    void initTimer() override;
    void startTimer(int delayMs) override;
    void stopTimer() override;

    void videoRefreshTimer();
    void emitUpdateWidget() override {}
    void emitVideoFinished() override {}
    void emitVideoStopped() override {}

private:
    Looper* mLooper = nullptr;
    LoopTimer* mLoopTimer = nullptr;
};

VideoplaybackNotifier::VideoplaybackNotifier() {
    mLooper = looper_getForThread();
}

VideoplaybackNotifier::~VideoplaybackNotifier() {
    if (mLoopTimer != nullptr) {
        loopTimer_free(mLoopTimer);
    }
}

static void _on_videoplayback_time_out(void* opaque, LoopTimer* unused) {
    VideoplaybackNotifier* const notifier = (VideoplaybackNotifier*)opaque;
    notifier->videoRefreshTimer();
}

void VideoplaybackNotifier::initTimer() {
    mLoopTimer = loopTimer_new(mLooper, _on_videoplayback_time_out, this);
}

void VideoplaybackNotifier::startTimer(int delayMs) {
    loopTimer_startRelative(mLoopTimer, delayMs);
}

void VideoplaybackNotifier::stopTimer() {
    loopTimer_stop(mLoopTimer);
}

void VideoplaybackNotifier::videoRefreshTimer() {
    if (mPlayer) {
        mPlayer->videoRefresh();
    }
}

bool RenderMultiplexer::initialize(const GLESv2Dispatch* gles2,
                                   int width,
                                   int height) {
    mDefaultRenderer =
            std::unique_ptr<DefaultFrameRenderer>(new DefaultFrameRenderer());
    if (!mDefaultRenderer->initialize(gles2, width, height)) {
        uninitialize();
        return false;
    }
    mVideoRenderer = std::unique_ptr<VideoplaybackVideoRenderer>(
            new VideoplaybackVideoRenderer());
    if (!mVideoRenderer->initialize(gles2, width, height)) {
        uninitialize();
        return false;
    }

    loadVideo(nullptr);
    mCurrentRenderer = mVideoRenderer.get();
    mInitialized = true;
    return true;
}

void RenderMultiplexer::uninitialize() {
    mCurrentRenderer = nullptr;
    if (mDefaultRenderer) {
        mDefaultRenderer->uninitialize();
    }
    mDefaultRenderer.reset();

    if (mVideoRenderer) {
        mVideoRenderer->uninitialize();
    }
    mVideoRenderer.reset();
    mInitialized = false;
}

int64_t RenderMultiplexer::render() {
    if (!mInitialized) {
        LOG(ERROR) << "Attempting to render using an uninitialized render "
                      "multiplexer";
        return -1;
    }

    base::Optional<::offworld::VideoInjectionRequest> maybe_next_request =
            videoinjection::VideoInjectionController::tryGetNextRequest(
                    base::Ok());
    if (maybe_next_request) {
        switch (maybe_next_request->function_case()) {
            case ::offworld::VideoInjectionRequest::kDisplayDefaultFrame:
                mCurrentRenderer = mDefaultRenderer.get();
                break;
            case ::offworld::VideoInjectionRequest::kPlay:
                if (mPlayer == nullptr) {
                    LOG(ERROR) << "No video loaded.";
                    return -1;
                }
                mCurrentRenderer = mVideoRenderer.get();
                mPlayer->start();
                break;
            case ::offworld::VideoInjectionRequest::kStop:
                if (mPlayer == nullptr) {
                    LOG(ERROR) << "No video loaded.";
                    return -1;
                }
                mCurrentRenderer = mVideoRenderer.get();
                mPlayer->stop();
                break;
            case ::offworld::VideoInjectionRequest::kLoad:
                mCurrentRenderer = mVideoRenderer.get();
                loadVideo(maybe_next_request->load().video_data().c_str());
                break;
            default:
                mCurrentRenderer = mDefaultRenderer.get();
                break;
        }
    } else {
        if (!mPlayer->isRunning()) {
            mPlayer->start();
        }
        if (mCurrentRenderer == mVideoRenderer.get() && mPlayer->isRunning()) {
            mPlayer->videoRefresh();
        }
    }
    return mCurrentRenderer->render();
}

void RenderMultiplexer::loadVideo(const char* video_data) {
    mPlayer = videoplayer::VideoPlayer::create(
            "/tmp/coke_barcode.mp4", mVideoRenderer->renderTarget(),
            std::unique_ptr<videoplayer::VideoPlayerNotifier>(
                    new VideoplaybackNotifier()));
    mPlayer->stop();
}

}  // namespace videoplayback
}  // namespace android
