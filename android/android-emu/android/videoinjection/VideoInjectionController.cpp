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

#include <deque>
#include <sstream>

#include "android/base/Log.h"
#include "android/base/synchronization/Lock.h"
#include "android/offworld/OffworldPipe.h"

using namespace android::base;

namespace {

static offworld::Response createAsyncResponse(
    uint32_t asyncId,
    uint32_t sequenceId,
    android::videoinjection::VideoInjectionResult result,
    bool isCompleted,
    std::string errorDetails) {
    offworld::Response response;
    if (result.ok()) {
        response.set_result(offworld::Response::RESULT_NO_ERROR);
    } else {
        response.set_result(offworld::Response::RESULT_ERROR_UNKNOWN);
        std::stringstream ss;
        ss << result.unwrapErr();
        if (!errorDetails.empty()) {
            ss << ": " << errorDetails;
        }
        std::string errorString = ss.str();

        if (!errorString.empty()) {
            response.set_error_string(errorString);
        }
    }

    offworld::AsyncResponse* asyncResponse = response.mutable_async();
    asyncResponse->set_async_id(asyncId);
    asyncResponse->mutable_video_injection()->set_sequence_id(sequenceId);
    if(isCompleted){
        asyncResponse->set_complete(true);
    }
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

    Optional<RequestContext> getNextRequestContext() override;

    VideoInjectionResult handleRequest(
        android::AsyncMessagePipeHandle pipe,
        ::offworld::VideoInjectionRequest event,
        uint32_t asyncId) override;

    void pipeClosed(android::AsyncMessagePipeHandle pipe) override;

    void sendFollowUpAsyncResponse(uint32_t async_id,
                                android::videoinjection::VideoInjectionResult result,
                                bool isCompleted, const std::string& errorDetails) override;

private:
    Lock mLock;
    bool mShutdown = false;
    std::deque<RequestContext> mRequestContexts;
    bool mRequestPending = false;
    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)>
            mSendMessageCallback;
    std::unordered_map<uint32_t, RequestContext> mAsyncRequestContextMap;
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
                requestContext.asyncId, requestContext.request.sequence_id(),
                Err(VideoInjectionError::InternalError), false,
                "Try to reset Video Injection Controller while there are pending requests.");
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

Optional<RequestContext> VideoInjectionController::tryGetNextRequestContext() {
    if (sInstance) {
        return sInstance->getNextRequestContext();
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

    mAsyncRequestContextMap.insert(std::make_pair(asyncId, requestContext));

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

Optional<RequestContext> VideoInjectionControllerImpl::getNextRequestContext() {
    AutoLock lock(mLock);
    if (mShutdown) {
        return {};
    }

    // Check if there is any request pending for execution.
    if (mRequestContexts.empty()) {
        mRequestPending = false;
        return {};
    } else {
        mRequestPending = true;
        RequestContext requestContext = std::move(mRequestContexts.front());
        mRequestContexts.pop_front();

        return makeOptional<RequestContext>(
                requestContext);
    }
}

void VideoInjectionController::trySendAsyncResponse(
        uint32_t async_id,
        android::videoinjection::VideoInjectionResult result,
        bool isCompleted,
        const std::string& errorDetails) {
    if (sInstance) {
        sInstance->sendFollowUpAsyncResponse(async_id, std::move(result),
                                             isCompleted, errorDetails);
    } else {
        LOG(ERROR) << "No controller instance to send async response.";
    }
}

void VideoInjectionControllerImpl::sendFollowUpAsyncResponse(
        uint32_t async_id,
        android::videoinjection::VideoInjectionResult result,
        bool isCompleted,
        const std::string& errorDetails) {
    AutoLock lock(mLock);
    // Retrives the original RequestContext.
    auto itr = mAsyncRequestContextMap.find(async_id);

    // Creates the async reponse and propagates it.
    if (itr == mAsyncRequestContextMap.end()) {
        LOG(ERROR) << "Unrecognizable async id: " << async_id;
    } else {
        //Remove all elements, so following request won't be executed.
        if(result.err()){
            mRequestContexts.clear();
        }

        ::offworld::Response asyncResponse = createAsyncResponse(
                itr->second.asyncId, itr->second.request.sequence_id(),
                std::move(result), isCompleted, errorDetails);

        mSendMessageCallback(*(itr->second.pipe), asyncResponse);
        if (isCompleted) {
            mAsyncRequestContextMap.erase(itr);
        }
    }
}

}  // namespace videoinjection
}  // namespace android
