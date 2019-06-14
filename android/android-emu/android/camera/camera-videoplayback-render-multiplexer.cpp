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

#include <string>

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/misc/FileUtils.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/recording/video/player/VideoPlayerNotifier.h"
#include "android/utils/tempfile.h"

namespace android {
namespace videoplayback {

using ::android::base::Looper;
using ::android::base::ThreadLooper;

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
    Looper* mLooper = android::base::ThreadLooper::get();
    std::unique_ptr<Looper::Timer> mTimer;
};

VideoplaybackNotifier::VideoplaybackNotifier() {}

VideoplaybackNotifier::~VideoplaybackNotifier() {}

static void _on_videoplayback_time_out(void* opaque, Looper::Timer* unused) {
    VideoplaybackNotifier* const notifier =
            static_cast<VideoplaybackNotifier*>(opaque);
    notifier->videoRefreshTimer();
}

void VideoplaybackNotifier::initTimer() {
    mTimer = std::unique_ptr<Looper::Timer>(
            mLooper->createTimer(_on_videoplayback_time_out, this));
}

void VideoplaybackNotifier::startTimer(int delayMs) {
    mTimer->startRelative(delayMs);
}

void VideoplaybackNotifier::stopTimer() {
    mTimer->stop();
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

    mCurrentRenderer = mDefaultRenderer.get();
    mInitialized = true;
    return true;
}

void RenderMultiplexer::uninitialize() {
    if (mPlayer) {
        if (mPlayer->isRunning()) {
            mPlayer->stop();
        }
        mPlayer.reset();
    }
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
                switchRenderer(mDefaultRenderer.get());
                break;
            case ::offworld::VideoInjectionRequest::kPlay:
                if (mPlayer == nullptr) {
                    LOG(ERROR) << "No video loaded.";
                    return -1;
                }
                switchRenderer(mVideoRenderer.get());
                mPlayer->start();
                break;
            case ::offworld::VideoInjectionRequest::kStop:
                if (mPlayer == nullptr) {
                    LOG(ERROR) << "No video loaded.";
                    return -1;
                }
                switchRenderer(mVideoRenderer.get());
                mPlayer->stop();
                break;
            case ::offworld::VideoInjectionRequest::kLoad:
                switchRenderer(mVideoRenderer.get());
                loadVideo(maybe_next_request->load().video_data());
                break;
            default:
                switchRenderer(mDefaultRenderer.get());
                break;
        }
    }
    if (mCurrentRenderer == mVideoRenderer.get() && mPlayer) {
        mPlayer->videoRefresh();
    }
    return mCurrentRenderer->render();
}

void RenderMultiplexer::loadVideo(const std::string& video_data) {
    TempFile* tempfile = tempfile_create();
    const char* path = tempfile_path(tempfile);
    FILE* videofile = fopen(path, "wb");
    android::base::ScopedFd fd(fileno(videofile));
    if (!writeStringToFile(fd.get(), video_data)) {
        LOG(ERROR) << "Failed to write video to tempfile.";
        return;
    }

    mPlayer = videoplayer::VideoPlayer::create(
            path, mVideoRenderer->renderTarget(),
            std::unique_ptr<videoplayer::VideoPlayerNotifier>(
                    new VideoplaybackNotifier()));

    // Force the video player to be in a known stable state.
    mPlayer->stop();
}

// switchRenderer cleans up any previous renderer state or sets up any new
// renderer state that is necessary to make the switch work.
void RenderMultiplexer::switchRenderer(
        virtualscene::CameraRenderer* nextRenderer) {
    if (mCurrentRenderer == nextRenderer) {
        return;
    }

    if (mCurrentRenderer != mVideoRenderer.get() && mPlayer) {
        CHECK(!mPlayer->isRunning())
                << "video playing when not using the video renderer.";
    }

    // If we are changing out of the video renderer, we need to cleanup all
    // the openGL work that we had allocated for video playing.
    if (mCurrentRenderer == mVideoRenderer.get()) {
        if (mPlayer && mPlayer->isRunning()) {
            mPlayer->stop();
        }
    }
    mVideoRenderer->renderTarget()->uninitialize();

    // We need to make sure that we properly set up the openGL environment
    // for video playing if we are switching to the video renderer.
    if (nextRenderer == mVideoRenderer.get()) {
        mVideoRenderer->renderTarget()->initialize();
    }

    mCurrentRenderer = nextRenderer;
}

}  // namespace videoplayback
}  // namespace android
