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
#include "android/base/files/FileShareOpen.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/hw-sensors.h"
#include "android/physics/PhysicalModel.h"

#include <ostream>

using namespace android::base;

static constexpr uint32_t kFileVersion = 2;

namespace android {
namespace automation {

std::ostream& operator<<(std::ostream& os, const StartError& value) {
    switch (value) {
        case StartError::InvalidFilename:
            os << "Invalid filename";
            break;
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
    AutomationControllerImpl(PhysicalModel* physicalModel,
                             base::Looper* looper);
    ~AutomationControllerImpl();

    AutomationEventSink& getEventSink() override;

    void reset() override;
    DurationNs advanceTime() override;

    StartResult startRecording(StringView filename) override;
    StopResult stopRecording() override;

    StartResult startPlayback(StringView filename) override;
    StopResult stopPlayback() override;

private:
    // Helper to replay the last event in the playback stream, must be called
    // under lock.
    //
    // If the playback stream reaches EOF, automatically ends playback.
    void replayEvent(const AutoLock& proofOfLock);

    AutomationEventSink mEventSink;
    PhysicalModel* const mPhysicalModel;
    base::Looper* const mLooper;

    Lock mLock;
    std::unique_ptr<android::base::Stream> mRecordingStream;

    // Playback state.
    bool mPlaying = false;
    std::unique_ptr<android::base::Stream> mPlaybackStream;
    DurationNs mPlaybackStartTime = 0L;
    DurationNs mPlaybackTimeBase = 0L;
    DurationNs mNextPlaybackCommandTime = 0L;
    pb::RecordedEvent mNextPlaybackCommand;
};

static AutomationControllerImpl* sInstance = nullptr;

void AutomationController::initialize() {
    CHECK(!sInstance)
            << "AutomationController::initialize() called more than once";
    sInstance = new AutomationControllerImpl(android_physical_model_instance(),
                                             ThreadLooper::get());
}

AutomationController& AutomationController::get() {
    CHECK(sInstance) << "AutomationController instance not created";
    return *sInstance;
}

std::unique_ptr<AutomationController> AutomationController::createForTest(
        PhysicalModel* physicalModel,
        base::Looper* looper) {
    return std::unique_ptr<AutomationControllerImpl>(
            new AutomationControllerImpl(physicalModel, looper));
}

void AutomationController::tryAdvanceTime() {
    if (sInstance) {
        sInstance->advanceTime();
    }
}

AutomationControllerImpl::AutomationControllerImpl(PhysicalModel* physicalModel,
                                                   base::Looper* looper)
    : mPhysicalModel(physicalModel), mLooper(looper) {
    physicalModel_setAutomationController(physicalModel, this);
}

AutomationControllerImpl::~AutomationControllerImpl() {
    physicalModel_setAutomationController(mPhysicalModel, nullptr);
}

AutomationEventSink& AutomationControllerImpl::getEventSink() {
    return mEventSink;
}

void AutomationControllerImpl::reset() {
    // These may return and error, but it's safe to ignore them here.
    stopRecording();
    stopPlayback();
}

DurationNs AutomationControllerImpl::advanceTime() {
    AutoLock lock(mLock);

    const DurationNs nowNs = mLooper->nowNs(Looper::ClockType::kVirtual);
    while (mPlaybackStream) {
        const DurationNs nextEventTimeNs = mPlaybackStartTime +
                                           mNextPlaybackCommandTime -
                                           mPlaybackTimeBase;

        if (nowNs >= nextEventTimeNs) {
            physicalModel_setCurrentTime(mPhysicalModel, nextEventTimeNs);
            replayEvent(lock);
        } else {
            break;
        }
    }

    physicalModel_setCurrentTime(mPhysicalModel, nowNs);
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
    const int saveStateResult =
            physicalModel_saveState(mPhysicalModel, &initialState);
    if (saveStateResult != 0) {
        LOG(ERROR) << "physicalModel_saveState failed with " << saveStateResult;
        return Err(StartError::InternalError);
    }

    std::string binaryProto;
    if (!initialState.SerializeToString(&binaryProto)) {
        LOG(ERROR) << "SerializeToString failed";
        return Err(StartError::InternalError);
    }

    LOG(VERBOSE) << "Starting record to " << path;
    std::unique_ptr<StdioStream> recordingStream(new StdioStream(
            fsopen(path.c_str(), "wb", FileShare::Write), StdioStream::kOwner));

    if (!recordingStream->get()) {
        return Err(StartError::FileOpenError);
    }

    mRecordingStream = std::move(recordingStream);
    mRecordingStream->putBe32(kFileVersion);
    mRecordingStream->putString(binaryProto);
    mEventSink.registerStream(mRecordingStream.get(),
                              StreamEncoding::BinaryPbChunks);

    return Ok();
}

StopResult AutomationControllerImpl::stopRecording() {
    AutoLock lock(mLock);
    if (!mRecordingStream) {
        return Err(StopError::NotStarted);
    }

    mEventSink.unregisterStream(mRecordingStream.get());
    mRecordingStream.reset();
    return Ok();
}

static bool parseNextCommand(android::base::Stream* stream,
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

    if (mPlaying) {
        return Err(StartError::AlreadyStarted);
    }

    // NOTE: The class state is intentionally not modified until after the file
    // header has been parsed.
    LOG(VERBOSE) << "Starting playback of " << path;
    std::unique_ptr<StdioStream> playbackStream(new StdioStream(
            fsopen(path.c_str(), "rb", FileShare::Read), StdioStream::kOwner));
    if (!playbackStream->get()) {
        return Err(StartError::FileOpenError);
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

    const int loadStateResult =
            physicalModel_loadState(mPhysicalModel, initialState);
    if (loadStateResult != 0) {
        LOG(ERROR) << "physicalModel_saveState failed with " << loadStateResult;
        return Err(StartError::InternalError);
    }

    DurationNs nextCommandTime;
    pb::RecordedEvent nextCommand;
    if (parseNextCommand(playbackStream.get(), &nextCommandTime,
                         &nextCommand)) {
        // Header parsed, initialize class state.
        mPlaybackStream = std::move(playbackStream);
        mPlaybackStartTime = mLooper->nowNs(Looper::ClockType::kVirtual);
        mPlaybackTimeBase = initialTime;

        mNextPlaybackCommand = std::move(nextCommand);
        mNextPlaybackCommandTime = nextCommandTime;
    } else {
        LOG(INFO) << "Playback did not contain any commands";
    }

    mPlaying = true;
    return Ok();
}

StopResult AutomationControllerImpl::stopPlayback() {
    AutoLock lock(mLock);
    if (!mPlaying) {
        return Err(StopError::NotStarted);
    }

    mPlaying = false;
    mPlaybackStream.reset();
    return Ok();
}

void AutomationControllerImpl::replayEvent(const AutoLock& proofOfLock) {
    const auto& event = mNextPlaybackCommand;
    if (event.has_stream_type()) {
        switch (event.stream_type()) {
            case pb::RecordedEvent_StreamType_PHYSICAL_MODEL:
                physicalModel_replayEvent(mPhysicalModel,
                                          event.physical_model());
                break;
            default:
                VLOG(automation) << "Unhandled stream type: "
                                 << static_cast<uint32_t>(event.stream_type());
                break;
        }
    }

    if (!parseNextCommand(mPlaybackStream.get(), &mNextPlaybackCommandTime,
                          &mNextPlaybackCommand)) {
        VLOG(automation) << "Playback hit EOF";
        mPlaybackStream.reset();
    }
}

}  // namespace automation
}  // namespace android
