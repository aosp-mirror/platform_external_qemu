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
#include "offworld.pb.h"
#include "android/recording/video/player/VideoPlayerNotifier.h"

namespace android {
namespace videoplayback {

using ::android::base::Looper;
using ::android::base::ThreadLooper;
using videoinjection::VideoInjectionController;

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

    // Before start next execution, check if there is error of last execution.
    if (mOngoingAsyncId && mPlayer && mPlayer->getErrorStatus()) {
        std::string errorDetails = mPlayer->getErrorMessage();
        videoinjection::VideoInjectionController::trySendAsyncResponse(
                mOngoingAsyncId.value(),
                base::Err(videoinjection::VideoInjectionError::InternalError),
                true, errorDetails);
        return -1;
    }

    base::Optional<::android::videoinjection::RequestContext>
            maybe_next_request_context = videoinjection::
                    VideoInjectionController::tryGetNextRequestContext();

    // If there is pending RequestContext, invoke execution accordingly.
    if (maybe_next_request_context) {
        if (mOngoingAsyncId) {
            videoinjection::VideoInjectionController::trySendAsyncResponse(
                mOngoingAsyncId.value(), base::Ok(), true, "");
            mOngoingAsyncId.clear();
        }

        base::Optional<::offworld::VideoInjectionRequest> maybe_next_request =
                maybe_next_request_context->request;
        uint32_t async_id = maybe_next_request_context->asyncId;

        switch (maybe_next_request->function_case()) {
            case ::offworld::VideoInjectionRequest::kDisplayDefaultFrame:
                switchRenderer(mDefaultRenderer.get());
                videoinjection::VideoInjectionController::trySendAsyncResponse(
                        async_id, base::Ok(), true, "");
                break;
            case ::offworld::VideoInjectionRequest::kPlay: {
                if (!videoIsLoaded(async_id)) {
                    return -1;
                }
                mOngoingAsyncId = async_id;
                switchRenderer(mVideoRenderer.get());
                if (maybe_next_request->play().has_offset_in_seconds() &&
                    mPlaysData) {
                  LOG(ERROR) << "Seeking is not implemented for data playback.";
                  return -1;
                }

                if (maybe_next_request->play().looping() && mPlaysData) {
                  LOG(ERROR) << "Looping is not implemented for data playback.";
                  return -1;
                }

                if (maybe_next_request->play().has_offset_in_seconds() &&
                    maybe_next_request->play().offset_in_seconds() < 0) {
                    // TODO: send error response through VideoInjectionController
                    return -1;
                }

                videoplayer::PlayConfig playConfig(
                    maybe_next_request->play().offset_in_seconds(),
                    maybe_next_request->play().looping());
                mPlayer->start(playConfig);
                // Successfully started playing the video.
                videoinjection::VideoInjectionController::trySendAsyncResponse(
                    async_id, base::Ok(), false, "");
                break;
            }
            case ::offworld::VideoInjectionRequest::kStop:
                if (mPlayer != nullptr) {
                    switchRenderer(mVideoRenderer.get());
                    mPlayer->stop();
                }
                videoinjection::VideoInjectionController::trySendAsyncResponse(
                            async_id, base::Ok(), true, "");
                break;
            case ::offworld::VideoInjectionRequest::kPause:
                if (!videoIsLoaded(async_id)) {
                    return -1;
                }
                if (mPlaysData) {
                  LOG(ERROR) << "Pause not implemented for data playback.";
                  return -1;
                }
                mOngoingAsyncId = async_id;
                switchRenderer(mVideoRenderer.get());
                if (!maybe_next_request->pause().has_offset_in_seconds()) {
                    // pause request
                    mPlayer->pause();
                } else {
                    // pauseAt request
                    if (maybe_next_request->pause().offset_in_seconds() < 0) {
                        // TODO: send error response through VideoInjectionController
                        return -1;
                    }
                    mPlayer->pauseAt(maybe_next_request->pause().offset_in_seconds());
                }
                videoinjection::VideoInjectionController::trySendAsyncResponse(
                        async_id, base::Ok(), false, "");
                break;
            case ::offworld::VideoInjectionRequest::kLoad:
                switchRenderer(mVideoRenderer.get());
                mPlaysData = maybe_next_request->load().has_dataset_info();
                if (mPlaysData) {
                    LOG(VERBOSE) << "Load video with dataset info";
                    loadVideoWithData(maybe_next_request->load().video_data(),
                                      maybe_next_request->load().dataset_info(),
                                      async_id);
                } else {
                    LOG(VERBOSE) << "Load video without dataset info";
                    loadVideo(maybe_next_request->load().video_data(),
                              async_id);
                }
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

void RenderMultiplexer::loadVideo(const std::string& video_data,
                                  uint32_t async_id) {
    if (mTempfile != nullptr) {
        tempfile_close(mTempfile);
    }
    mTempfile = tempfile_create();
    const char* path = tempfile_path(mTempfile);
    FILE* videofile = fopen(path, "wb");
    android::base::ScopedFd fd(fileno(videofile));

    //If there is any Error in Loading.
    if (!writeStringToFile(fd.get(), video_data)) {
        LOG(ERROR) << "Failed to write video to tempfile.";
        videoinjection::VideoInjectionController::trySendAsyncResponse(
                async_id,
                base::Err(videoinjection::VideoInjectionError::FileIsNotLoaded),
                true, "Failed to load the video.");
        return;
    }
    mPlayer = videoplayer::VideoPlayer::create(
            path, mVideoRenderer->renderTarget(),
            std::unique_ptr<videoplayer::VideoPlayerNotifier>(
                    new VideoplaybackNotifier()));

    //File Loaded, sends async response indicating execution completed.
    videoinjection::VideoInjectionController::trySendAsyncResponse(
            async_id, base::Ok(), true, "");
}

void RenderMultiplexer::loadVideoWithData(const std::string& video_data,
                                          const DatasetInfo& datasetInfo,
                                          uint32_t async_id) {
    loadVideo(video_data, async_id);
    mPlayer->loadVideoFileWithData(datasetInfo);
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

// Helper function to check whether video file is loaded before executing
// video player requests. Sends async response through VideoInjectionController
// if video is not loaded.
//
// Returns: true if video is loaded, false if video is not loaded
bool RenderMultiplexer::videoIsLoaded(uint32_t async_id) {
    if (mPlayer == nullptr) {
        LOG(ERROR) << "No video loaded.";
        videoinjection::VideoInjectionController::
                trySendAsyncResponse(
                        async_id,
                        base::Err(videoinjection::
                                          VideoInjectionError::
                                                  FileIsNotLoaded),
                        true, "Video is not loaded");
        return false;
    }
    return true;
}

}  // namespace videoplayback
}  // namespace android
