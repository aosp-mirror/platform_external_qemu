// Copyright (C) 2018 The Android Open Source Project
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

#include "android/automation/AutomationController.h"
#include "android/automation/AutomationEventSink.h"
#include "android/base/StringView.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/physics/PhysicalModel.h"

#include <ostream>

#include <memory>

using namespace android::base;

static constexpr uint32_t kFileVersion = 2;

namespace android {
namespace automation {

std::ostream& operator<<(std::ostream& os, const StartError& value) {
    switch (value) {
        case StartError::InvalidFilename:
            os << "Invalid filename";
        case StartError::FileOpenError:
            os << "Could not open file";
            break;
        case StartError::AlreadyStarted:
            os << "Already started";
            break;
        case StartError::InternalError:
            os << "Internal error";
            break;
        case StartError::PlaybackFileCorrupt:
            os << "File corrupt";
            break;
            // Default intentionally omitted so that missing statements generate
            // errors.
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const StopError& value) {
    switch (value) {
        case StopError::NotStarted:
            os << "Not started";
            break;
            // Default intentionally omitted so that missing statements generate
            // errors.
    }

    return os;
}

class AutomationControllerImpl : public AutomationController {
public:
    AutomationControllerImpl();

    // Get the event sink for recording automation events.
    AutomationEventSink& getEventSink() override;

    // Advance the state and process any playback events.
    // Returns the current time.
    DurationNs advanceTime() override;

    // Start a recording to a file.
    StartResult startRecording(StringView filename) override;

    // Stops a recording to a file.
    StopResult stopRecording() override;

    // Start a playback from a file.
    StartResult startPlayback(StringView filename) override;

    // Stop playback from a file.
    StopResult stopPlayback() override;

private:
    // Helper to replay the last event in the playback stream, must be called
    // under lock.
    //
    // If the playback stream reaches EOF, automatically ends playback.
    void replayEvent(const AutoLock& proofOfLock,
                     PhysicalModel* const physicalModel);

    std::unique_ptr<AutomationEventSink> mEventSink;
    android::base::Looper* const mLooper;

    Lock mLock;
    std::unique_ptr<android::base::Stream> mRecordingStream;

    // Playback state.
    std::unique_ptr<android::base::Stream> mPlaybackStream;
    DurationNs mPlaybackStartTime = 0L;
    DurationNs mPlaybackTimeBase = 0L;
    DurationNs mNextPlaybackCommandTime = 0L;
    pb::RecordedEvent mNextPlaybackCommand;
};


AutomationController& AutomationController::get() {
    static LazyInstance<AutomationControllerImpl> sInstance;
    return sInstance.get();
}

AutomationControllerImpl::AutomationControllerImpl()
    : mEventSink(new AutomationEventSink()), mLooper(ThreadLooper::get()) {}

AutomationEventSink& AutomationControllerImpl::getEventSink() {
    return *mEventSink;
}

DurationNs AutomationControllerImpl::advanceTime() {
    AutoLock lock(mLock);

    PhysicalModel* const physicalModel = android_physical_model_instance();

    const DurationNs nowNs =
            mLooper->nowNs(android::base::Looper::ClockType::kVirtual);
    DurationNs lastLoopTick = 0;  // 0 indicates no loop executed.

    while (mPlaybackStream) {
        const DurationNs nextEventTimeNs = mPlaybackStartTime +
                                           mNextPlaybackCommandTime -
                                           mPlaybackTimeBase;

        if (nowNs >= nextEventTimeNs) {
            if (lastLoopTick != nextEventTimeNs) {
                lastLoopTick = nextEventTimeNs;
                physicalModel_setCurrentTime(physicalModel, nextEventTimeNs);
            }

            replayEvent(lock, physicalModel);
        } else {
            break;
        }
    }

    if (lastLoopTick != nowNs) {
        physicalModel_setCurrentTime(physicalModel, nowNs);
    }

    return nowNs;
}

StartResult AutomationControllerImpl::startRecording(StringView filename) {
    AutoLock lock(mLock);

    if (filename.empty()) {
        return Err(StartError::InvalidFilename);
    }

    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
    }

    if (mRecordingStream) {
        return Err(StartError::AlreadyStarted);
    }

    pb::InitialState initialState;
    const int saveStateResult = physicalModel_saveState(
            android_physical_model_instance(), &initialState);
    if (saveStateResult != 0) {
        LOG(ERROR) << "physicalModel_saveState failed with " << saveStateResult;
        return Err(StartError::InternalError);
    }

    std::string binaryProto;
    if (!initialState.SerializeToString(&binaryProto)) {
        LOG(ERROR) << "SerializeToString failed";
        return Err(StartError::InternalError);
    }

    mRecordingStream.reset(
            new StdioStream(fopen(path.c_str(), "wb"), StdioStream::kOwner));
    if (!mRecordingStream.get()) {
        return Err(StartError::InvalidFilename);
    }

    mRecordingStream->putBe32(kFileVersion);
    mRecordingStream->putString(binaryProto);
    mEventSink->registerStream(mRecordingStream.get(),
                               StreamEncoding::BinaryPbChunks);

    return Ok();
}

StopResult AutomationControllerImpl::stopRecording() {
    AutoLock lock(mLock);
    if (!mRecordingStream) {
        return Err(StopError::NotStarted);
    }

    mEventSink->unregisterStream(mRecordingStream.get());
    mRecordingStream.reset();
    return Ok();
}

static bool parseNextCommand(
        const std::unique_ptr<android::base::Stream>& stream,
        DurationNs* outTime,
        pb::RecordedEvent* outCommand) {
    const std::string nextCommand = stream->getString();
    if (nextCommand.empty()) {
        VLOG(automation) << "Empty command";
        return false;
    }

    pb::RecordedEvent event;
    if (!event.ParseFromString(nextCommand)) {
        VLOG(automation) << "Event parse failed";
        return false;
    }

    if (!event.has_event_time() || !event.event_time().has_timestamp()) {
        VLOG(automation) << "Event missing timestamp";
        return false;
    }

    *outTime = event.event_time().timestamp();
    *outCommand = std::move(event);
    return true;
}

StartResult AutomationControllerImpl::startPlayback(StringView filename) {
    AutoLock lock(mLock);

    if (filename.empty()) {
        return Err(StartError::InvalidFilename);
    }

    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
    }

    if (mPlaybackStream) {
        return Err(StartError::AlreadyStarted);
    }

    // NOTE: The class state is intentionally not modified until after the file
    // header has been parsed.
    std::unique_ptr<android::base::Stream> playbackStream(
            new StdioStream(fopen(path.c_str(), "rb"), StdioStream::kOwner));
    if (!playbackStream.get()) {
        return Err(StartError::InvalidFilename);
    }

    const uint32_t version = playbackStream->getBe32();
    if (kFileVersion != version) {
        LOG(ERROR) << "Unsupported file version " << version
                   << " expected version " << kFileVersion;
        return Err(StartError::PlaybackFileCorrupt);
    }

    const std::string initialStateStr = playbackStream->getString();
    pb::InitialState initialState;
    if (initialStateStr.empty() ||
        !initialState.ParseFromString(initialStateStr)) {
        LOG(ERROR) << "Could not load initial data";
        return Err(StartError::PlaybackFileCorrupt);
    }

    if (!initialState.has_initial_time() ||
        !initialState.initial_time().has_timestamp()) {
        LOG(ERROR) << "Initial state missing timestamp";
        return Err(StartError::PlaybackFileCorrupt);
    }

    const DurationNs initialTime = initialState.initial_time().timestamp();

    const int loadStateResult = physicalModel_loadState(
            android_physical_model_instance(), initialState);
    if (loadStateResult != 0) {
        LOG(ERROR) << "physicalModel_saveState failed with " << loadStateResult;
        return Err(StartError::InternalError);
    }

    DurationNs nextCommandTime;
    pb::RecordedEvent nextCommand;
    if (!parseNextCommand(playbackStream, &nextCommandTime, &nextCommand)) {
        LOG(ERROR) << "Failed to parse first command";
        return Err(StartError::PlaybackFileCorrupt);
    }

    // Header parsed, initialize class state.
    mPlaybackStream = std::move(playbackStream);
    mPlaybackStartTime =
            mLooper->nowNs(android::base::Looper::ClockType::kVirtual);
    mPlaybackTimeBase = initialTime;

    mNextPlaybackCommand = std::move(nextCommand);
    mNextPlaybackCommandTime = nextCommandTime;

    return Ok();
}

StopResult AutomationControllerImpl::stopPlayback() {
    AutoLock lock(mLock);
    if (!mPlaybackStream) {
        return Err(StopError::NotStarted);
    }

    mPlaybackStream.reset();
    return Ok();
}

void AutomationControllerImpl::replayEvent(const AutoLock&,
                                           PhysicalModel* const physicalModel) {
    const auto& event = mNextPlaybackCommand;
    if (event.has_stream_type()) {
        switch (event.stream_type()) {
            case pb::RecordedEvent_StreamType_PHYSICAL_MODEL:
                physicalModel_replayEvent(physicalModel,
                                          event.physical_model());
                break;
            default:
                VLOG(automation) << "Unhandled stream type: "
                                 << static_cast<uint32_t>(event.stream_type());
                break;
        }
    }

    if (!parseNextCommand(mPlaybackStream, &mNextPlaybackCommandTime,
                          &mNextPlaybackCommand)) {
        VLOG(automation) << "Playback hit EOF";
        mPlaybackStream.reset();
    }
}

// TODO(jwmcglynn): Handle snapshots

}  // namespace automation
}  // namespace android
