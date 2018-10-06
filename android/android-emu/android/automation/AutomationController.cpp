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
#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/FileShareOpen.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/hw-sensors.h"
#include "android/offworld/OffworldPipe.h"
#include "android/physics/PhysicalModel.h"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#include <deque>
#include <ostream>

using namespace android::base;

static constexpr uint32_t kFileVersion = 2;

namespace {

static offworld::Response createAsyncResponse(uint32_t asyncId) {
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);

    offworld::AsyncResponse* asyncResponse = response.mutable_async();
    asyncResponse->set_async_id(asyncId);
    return response;
}

}  // namespace.

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

std::ostream& operator<<(std::ostream& os, const ListenError& value) {
    switch (value) {
        case ListenError::AlreadyListening:
            os << "Already listening";
            break;
        case ListenError::StartFailed:
            os << "Start failed";
            break;
            // Default intentionally omitted so that missing statements generate
            // errors.
    }

    return os;
}

class ListenPipeStream : public android::base::Stream {
public:
    ListenPipeStream(android::AsyncMessagePipeHandle pipe,
                     uint32_t asyncId,
                     std::function<void(android::AsyncMessagePipeHandle,
                                        const ::offworld::Response&)>
                             sendMessageCallback)
        : mPipe(pipe),
          mAsyncId(asyncId),
          mSendMessageCallback(sendMessageCallback) {}
    virtual ~ListenPipeStream() { close(); }

    // Stream interface implementation.
    ssize_t read(void* buffer, size_t size) override { return -EPERM; }
    ssize_t write(const void* buffer, size_t size) override {
        namespace pb = android::offworld::pb;

        pb::Response response = createAsyncResponse(mAsyncId);
        auto event = response.mutable_async()
                             ->mutable_automation()
                             ->mutable_event_generated();
        event->set_event(static_cast<const char*>(buffer), size);

        mSendMessageCallback(mPipe, response);
        return static_cast<ssize_t>(size);
    }

    void close() {
        namespace pb = android::offworld::pb;

        pb::Response response = createAsyncResponse(mAsyncId);
        response.mutable_async()->set_complete(true);
        mSendMessageCallback(mPipe, response);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ListenPipeStream);

    AsyncMessagePipeHandle mPipe;
    const uint32_t mAsyncId;
    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)>
            mSendMessageCallback;
};

class EventSource {
public:
    virtual ~EventSource() = default;

    // Return the next command from the source.
    virtual pb::RecordedEvent consumeNextCommand() = 0;

    // Get the next command time from the event source.  Returns false if there
    // are no events remaining.
    virtual bool getNextCommandTime(DurationNs* outTime) = 0;
};

class BinaryStreamEventSource : public EventSource {
public:
    BinaryStreamEventSource(std::unique_ptr<android::base::Stream>&& stream)
        : mStream(std::move(stream)) {}

    pb::RecordedEvent consumeNextCommand() override {
        CHECK(mNextCommand);
        pb::RecordedEvent event = std::move(mNextCommand.value());
        mNextCommand.clear();
        return event;
    }

    bool getNextCommandTime(DurationNs* outTime) override {
        if (!mNextCommand) {
            const std::string nextCommand = mStream->getString();
            if (nextCommand.empty()) {
                VLOG(automation) << "Empty command";
                return false;
            }

            pb::RecordedEvent event;
            if (!event.ParseFromString(nextCommand)) {
                VLOG(automation) << "Event parse failed";
                return false;
            }

            mNextCommand = event;
        }

        if (!mNextCommand->has_event_time() ||
            !mNextCommand->event_time().has_timestamp()) {
            VLOG(automation) << "Event missing timestamp";
            return false;
        }

        *outTime = mNextCommand->event_time().timestamp();
        return true;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(BinaryStreamEventSource);

    Optional<pb::RecordedEvent> mNextCommand;
    std::unique_ptr<android::base::Stream> mStream;
};
class AutomationControllerImpl : public AutomationController {
public:
    AutomationControllerImpl(PhysicalModel* physicalModel,
                             base::Looper* looper,
                             std::function<void(android::AsyncMessagePipeHandle,
                                                const ::offworld::Response&)>
                                     sendMessageCallback);
    ~AutomationControllerImpl();

    void shutdown();

    AutomationEventSink& getEventSink() override;

    void reset() override;
    DurationNs advanceTime() override;

    StartResult startRecording(StringView filename) override;
    StopResult stopRecording() override;

    StartResult startPlayback(StringView filename) override;
    StopResult stopPlayback() override;

    ListenResult listen(android::AsyncMessagePipeHandle pipe,
                        uint32_t asyncId) override;

    void stopListening() override;

private:
    // Helper to replay the last event in the playback stream, must be called
    // under lock.
    //
    // If the playback stream reaches EOF, automatically ends playback.
    void replayNextEvent(const AutoLock& proofOfLock);

    AutomationEventSink mEventSink;
    PhysicalModel* const mPhysicalModel;
    base::Looper* const mLooper;

    Lock mLock;
    bool mShutdown = false;
    std::unique_ptr<android::base::Stream> mRecordingStream;

    // Playback state.
    bool mPlayingFromFile = false;
    std::unique_ptr<EventSource> mPlaybackEventSource;
    DurationNs mPlaybackStartTime = 0L;
    DurationNs mPlaybackTimeBase = 0L;
    DurationNs mNextPlaybackCommandTime = 0L;

    // Offworld state.
    std::unique_ptr<ListenPipeStream> mPipeListener;

    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)>
            mSendMessageCallback;
};

static AutomationControllerImpl* sInstance = nullptr;

void AutomationController::initialize() {
    CHECK(!sInstance)
            << "AutomationController::initialize() called more than once";
    sInstance = new AutomationControllerImpl(android_physical_model_instance(),
                                             ThreadLooper::get(),
                                             android::offworld::sendResponse);
}

void AutomationController::shutdown() {
    // Since other threads may be using the AutomationController, it's not safe
    // to delete.  Instead, call shutdown which disables future calls.
    // Note that there will only be one instance of AutomationController in
    // the emulator, which will be cleaned up on process exit so it's safe to
    // leak.
    if (sInstance) {
        VLOG(automation) << "Shutting down AutomationController";
        sInstance->shutdown();
    }
}

AutomationController& AutomationController::get() {
    CHECK(sInstance) << "AutomationController instance not created";
    return *sInstance;
}

std::unique_ptr<AutomationController> AutomationController::createForTest(
        PhysicalModel* physicalModel,
        base::Looper* looper,
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)> sendMessageCallback) {
    return std::unique_ptr<AutomationControllerImpl>(
            new AutomationControllerImpl(
                    physicalModel, looper, sendMessageCallback));
}

void AutomationController::tryAdvanceTime() {
    if (sInstance) {
        sInstance->advanceTime();
    }
}

AutomationControllerImpl::AutomationControllerImpl(
        PhysicalModel* physicalModel,
        base::Looper* looper,
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)> sendMessageCallback)
    : mPhysicalModel(physicalModel),
      mLooper(looper),
      mSendMessageCallback(sendMessageCallback) {
    physicalModel_setAutomationController(physicalModel, this);
}

AutomationControllerImpl::~AutomationControllerImpl() {
    physicalModel_setAutomationController(mPhysicalModel, nullptr);
}

void AutomationControllerImpl::shutdown() {
    {
        AutoLock lock(mLock);
        mShutdown = true;
    }
    stopRecording();
    stopPlayback();
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
    if (mShutdown) {
        return 0;
    }

    const DurationNs nowNs = mLooper->nowNs(Looper::ClockType::kVirtual);
    while (mPlaybackEventSource) {
        const DurationNs nextEventTimeNs = mPlaybackStartTime +
                                           mNextPlaybackCommandTime -
                                           mPlaybackTimeBase;

        if (nowNs >= nextEventTimeNs) {
            physicalModel_setCurrentTime(mPhysicalModel, nextEventTimeNs);
            replayNextEvent(lock);
        } else {
            break;
        }
    }

    physicalModel_setCurrentTime(mPhysicalModel, nowNs);
    return nowNs;
}

StartResult AutomationControllerImpl::startRecording(StringView filename) {
    AutoLock lock(mLock);
    if (mShutdown) {
        return Err(StartError::InternalError);
    }

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

StartResult AutomationControllerImpl::startPlayback(StringView filename) {
    AutoLock lock(mLock);
    if (mShutdown) {
        return Err(StartError::InternalError);
    }

    if (filename.empty()) {
        return Err(StartError::InvalidFilename);
    }

    // Block playback if either the playing from file flag is explicitly set,
    // which may be set by itself if the file is has ended, or if there is an
    // event source.
    if (mPlayingFromFile || mPlaybackEventSource) {
        return Err(StartError::AlreadyStarted);
    }

    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
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
        LOG(ERROR) << "physicalModel_loadState failed with " << loadStateResult;
        return Err(StartError::InternalError);
    }

    std::unique_ptr<EventSource> source(
            new BinaryStreamEventSource(std::move(playbackStream)));

    DurationNs nextCommandTime;
    if (source->getNextCommandTime(&nextCommandTime)) {
        // Header parsed, initialize class state.
        mPlaybackEventSource = std::move(source);
        mPlaybackStartTime = mLooper->nowNs(Looper::ClockType::kVirtual);
        mPlaybackTimeBase = initialTime;

        mNextPlaybackCommandTime = nextCommandTime;
    } else {
        LOG(INFO) << "Playback did not contain any commands";
    }

    mPlayingFromFile = true;
    return Ok();
}

StopResult AutomationControllerImpl::stopPlayback() {
    AutoLock lock(mLock);
    if (!mPlayingFromFile) {
        return Err(StopError::NotStarted);
    }

    mPlayingFromFile = false;
    mPlaybackEventSource.reset();
    return Ok();
}

ListenResult AutomationControllerImpl::listen(
        android::AsyncMessagePipeHandle pipe,
        uint32_t asyncId) {
    if (mPipeListener) {
        return Err(ListenError::AlreadyListening);
    }

    pb::InitialState initialState;
    const int saveStateResult =
            physicalModel_saveState(mPhysicalModel, &initialState);
    if (saveStateResult != 0) {
        return Err(ListenError::StartFailed);
    }

    google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.SetUseShortRepeatedPrimitives(true);

    std::string textInitialState;
    if (!printer.PrintToString(initialState, &textInitialState)) {
        LOG(WARNING) << "Could not serialize initialstate to string.";
        return Err(ListenError::StartFailed);
    }

    mPipeListener.reset(
            new ListenPipeStream(pipe, asyncId, mSendMessageCallback));
    mEventSink.registerStream(mPipeListener.get(), StreamEncoding::TextPb);

    return Ok(textInitialState);
}

void AutomationControllerImpl::stopListening() {
    if (!mPipeListener) {
        return;
    }

    mEventSink.unregisterStream(mPipeListener.get());
    mPipeListener.reset();
}

void AutomationControllerImpl::replayNextEvent(const AutoLock& proofOfLock) {
    pb::RecordedEvent event = mPlaybackEventSource->consumeNextCommand();
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

    if (!mPlaybackEventSource->getNextCommandTime(&mNextPlaybackCommandTime)) {
        VLOG(automation) << "Playback hit EOF";
        mPlaybackEventSource.reset();
    }
}

}  // namespace automation
}  // namespace android
