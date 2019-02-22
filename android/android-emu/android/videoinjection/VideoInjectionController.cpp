// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/videoinjection/VideoInjectionController.h"

#include "android/base/synchronization/Lock.h"
#include "android/offworld/OffworldPipe.h"

#include <deque>
#include <sstream>

using namespace android::base;

namespace {

static offworld::Response createAsyncResponse(
    uint32_t asyncId,
    uint32_t sequenceId,
    android::videoinjection::VideoInjectionResult result) {
    offworld::Response response;
    if (result.ok()) {
        response.set_result(offworld::Response::RESULT_NO_ERROR);
    } else {
        response.set_result(offworld::Response::RESULT_ERROR_UNKNOWN);
        std::stringstream ss;
        ss << result.unwrapErr();
        std::string errorString = ss.str();

        if (!errorString.empty()) {
            response.set_error_string(errorString);
        }
    }

    offworld::AsyncResponse* asyncResponse = response.mutable_async();
    asyncResponse->set_async_id(asyncId);
    asyncResponse->set_complete(true);
    asyncResponse->mutable_video_injection()->set_sequence_id(sequenceId);
    return response;
}

}  // namespace

namespace android {
namespace videoinjection {

std::ostream& operator<<(std::ostream& os, const VideoInjectionError& value) {
    switch (value) {
        case VideoInjectionError::InvalidFilename:
            os << "Invalid filename";
            break;
        case VideoInjectionError::FileOpenError:
            os << "Could not open file";
            break;
        case VideoInjectionError::InvalidRequest:
            os << "Invalid request";
            break;
        case VideoInjectionError::FileIsNotLoaded:
            os << "Video file is not loaded";
            break;
        case VideoInjectionError::AnotherVideoIsPlaying:
            os << "Another video is currently playing";
            break;
        case VideoInjectionError::VideoIsNotStarted:
            os << "Video is not started";
            break;
        case VideoInjectionError::InternalError:
            os << "Internal error";
            break;
            // Default intentionally omitted so that missing statements generate
            // errors.
    }

    return os;
}

struct RequestContext {
    ::offworld::VideoInjectionRequest request;
    Optional<android::AsyncMessagePipeHandle> pipe;
    uint32_t asyncId = 0;
};

class VideoInjectionControllerImpl : public VideoInjectionController {
public:
    VideoInjectionControllerImpl(
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)>
                sendMessageCallback)
        : mSendMessageCallback(sendMessageCallback) {}

    ~VideoInjectionControllerImpl() {};

    void shutdown();

    void reset() override;

    Optional<::offworld::VideoInjectionRequest> getNextRequest(
        VideoInjectionResult previousResult) override;

    VideoInjectionResult handleRequest(
        android::AsyncMessagePipeHandle pipe,
        ::offworld::VideoInjectionRequest event,
        uint32_t asyncId) override;

    void pipeClosed(android::AsyncMessagePipeHandle pipe) override;

private:
    Lock mLock;
    bool mShutdown = false;
    std::deque<RequestContext> mRequestContexts;
    bool mRequestPending = false;
    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)>
            mSendMessageCallback;
};

static VideoInjectionControllerImpl* sInstance = nullptr;

void VideoInjectionController::initialize() {
    CHECK(!sInstance)
            << "VideoInjectionController::initialize() called more than once";
    sInstance = new VideoInjectionControllerImpl(
        android::offworld::sendResponse);
}

void VideoInjectionController::shutdown() {
    // Since other threads may be using the VideoInjectionController, it's not
    // safe to delete.  Instead, call shutdown which disables future calls.
    // Note that there will only be one instance of VideoInjectionController in
    // the emulator, which will be cleaned up on process exit so it's safe to
    // leak.
    if (sInstance) {
        VLOG(videoinjection) << "Shutting down VideoInjectionController";
        sInstance->shutdown();
    }
}

void VideoInjectionControllerImpl::shutdown() {
    {
        AutoLock lock(mLock);
        mShutdown = true;
    }
    reset();
}

void VideoInjectionControllerImpl::reset() {
    AutoLock lock(mLock);
    if (mRequestPending) {
        RequestContext requestContext = std::move(mRequestContexts.front());
        mRequestContexts.pop_front();
        ::offworld::Response response = createAsyncResponse(
            requestContext.asyncId,
            requestContext.request.sequence_id(),
            Err(VideoInjectionError::InternalError));
        if (requestContext.pipe) {
            mSendMessageCallback(*requestContext.pipe, response);
        }
        mRequestPending = false;
    }
    mRequestContexts.clear();
}

VideoInjectionController& VideoInjectionController::get() {
    CHECK(sInstance) << "VideoInjectionController instance not created";
    return *sInstance;
}

std::unique_ptr<VideoInjectionController>
VideoInjectionController::createForTest(
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)> sendMessageCallback) {
    return std::unique_ptr<VideoInjectionControllerImpl>(
            new VideoInjectionControllerImpl(sendMessageCallback));
}

Optional<::offworld::VideoInjectionRequest>
VideoInjectionController::tryGetNextRequest(
    VideoInjectionResult previousResult) {
    if (sInstance) {
        return sInstance->getNextRequest(
            std::move(previousResult));
    }
    return {};
}

VideoInjectionResult VideoInjectionControllerImpl::handleRequest(
    android::AsyncMessagePipeHandle pipe,
    ::offworld::VideoInjectionRequest request,
    uint32_t asyncId) {
    AutoLock lock(mLock);
    if (mShutdown) {
        return Err(VideoInjectionError::InternalError);
    }

    if (!request.function_case()) {
        VLOG(videoinjection) << "Empty Request";
        return Err(VideoInjectionError::InvalidRequest);
    }

    RequestContext requestContext;
    requestContext.request = request;
    requestContext.pipe = pipe;
    requestContext.asyncId = asyncId;
    mRequestContexts.push_back(requestContext);

    return Ok();
}

void VideoInjectionControllerImpl::pipeClosed(
    android::AsyncMessagePipeHandle pipe) {
    for (RequestContext &r : mRequestContexts) {
        if (*(r.pipe) == pipe) {
            r.pipe = {};
        }
    }
}

Optional<::offworld::VideoInjectionRequest>
VideoInjectionControllerImpl::getNextRequest(
    VideoInjectionResult previousResult) {
    AutoLock lock(mLock);
    if (mShutdown) {
        return {};
    }

    if (mRequestPending) {
        RequestContext requestContext = std::move(mRequestContexts.front());
        mRequestContexts.pop_front();
        ::offworld::Response response = createAsyncResponse(
            requestContext.asyncId,
            requestContext.request.sequence_id(),
            std::move(previousResult));
        if (requestContext.pipe) {
            mSendMessageCallback(*requestContext.pipe, response);
        }
    }

    if (mRequestContexts.empty()) {
        mRequestPending = false;
        return {};
    } else {
        mRequestPending = true;
        return makeOptional<::offworld::VideoInjectionRequest>(
            mRequestContexts.front().request);
    }
}

}  // namespace videoinjection
}  // namespace android
