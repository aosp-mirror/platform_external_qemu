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

#include "android/automation/VideoInjectionManager.h"
#include "android/base/synchronization/Lock.h"
#include "android/offworld/OffworldPipe.h"

#include <deque>
#include <ostream>
#include <sstream>

using namespace android::base;

namespace {

static offworld::Response createAsyncResponse(
    uint32_t asyncId,
    android::automation::VideoInjectionResult result) {
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
    asyncResponse->mutable_automation()->mutable_video_injection_complete();
    return response;
}

}  // namespace

namespace android {
namespace automation {

std::ostream& operator<<(std::ostream& os, const VideoInjectionError& value) {
    switch (value) {
        case VideoInjectionError::InvalidFilename:
            os << "Invalid filename";
            break;
        case VideoInjectionError::FileOpenError:
            os << "Could not open file";
            break;
        case VideoInjectionError::InvalidCommand:
            os << "Invalid command for video playback";
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

struct CommandContext {
    pb::VideoInjectionCommand command;
    android::AsyncMessagePipeHandle pipe;
    uint32_t asyncId = 0;
};


class VideoInjectionManagerImpl : public VideoInjectionManager {
public:
    VideoInjectionManagerImpl(
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)>
                sendMessageCallback)
        : mSendMessageCallback(sendMessageCallback) {}

    virtual ~VideoInjectionManagerImpl();

    virtual VideoInjectionResult addCommand(
        android::AsyncMessagePipeHandle pipe,
        android::base::StringView command,
        uint32_t asyncId);

    virtual std::unique_ptr<pb::VideoInjectionCommand> getNextCommand(
        VideoInjectionResult previousResult);
private:
    Lock mLock;
    std::deque<CommandContext> mCommandContexts;
    bool mCommandPending = false;
    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)>
            mSendMessageCallback;
};

VideoInjectionResult VideoInjectionManagerImpl::addCommand(
    android::AsyncMessagePipeHandle pipe,
    android::base::StringView command,
    uint32_t asyncId) {
      AutoLock lock(mLock);
      pb::VideoInjectionCommand newCommand;
      if (!newCommand.ParseFromString(command)) {
        VLOG(automation) << "Command parse failed";
        return Err(VideoInjectionError::InvalidCommand);
      }

      if (!newCommand.command_case()) {
        VLOG(automation) << "Empty command";
        return Err(VideoInjectionError::InvalidCommand);
      }

      CommandContext commandContext;
      commandContext.command = std::move(newCommand);
      commandContext.pipe = pipe;
      commandContext.asyncId = asyncId;
      mCommandContexts.push_back(commandContext);

      return Ok();
}

std::unique_ptr<pb::VideoInjectionCommand>
VideoInjectionManagerImpl::getNextCommand(VideoInjectionResult previousResult) {
    AutoLock lock(mLock);
    if (mCommandPending) {
        CommandContext commandContext = std::move(mCommandContexts.front());
        mCommandContexts.pop_front();
        android::offworld::pb::Response response = createAsyncResponse(
            commandContext.asyncId, std::move(previousResult));
        mSendMessageCallback(commandContext.pipe, response);
    }

    if (mCommandContexts.empty()) {
        mCommandPending = false;
        return nullptr;
    } else {
        mCommandPending = true;
        return std::unique_ptr<pb::VideoInjectionCommand>(
            new pb::VideoInjectionCommand(mCommandContexts.front().command));
    }
}

VideoInjectionManagerImpl::~VideoInjectionManagerImpl() {}

// static
std::unique_ptr<VideoInjectionManager> VideoInjectionManager::create(
    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)> sendMessageCallback) {
  return std::unique_ptr<VideoInjectionManager>(
    new VideoInjectionManagerImpl(sendMessageCallback));
}

}  // namespace automation
}  // namespace android