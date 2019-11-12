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

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#include <chrono>
#include <deque>
#include <ostream>

#include "android/automation/AutomationEventSink.h"
#include "android/automation/EventSource.h"
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

extern "C" {
#include "android/gps.h"
}

using namespace android::base;

static constexpr uint32_t kFileVersion = 2;

namespace pb = emulator_automation;

namespace {

static offworld::Response createAsyncResponse(uint32_t asyncId) {
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);

    offworld::AsyncResponse* asyncResponse = response.mutable_async();
    asyncResponse->set_async_id(asyncId);
    return response;
}

void sendLocationOverride(const pb::LocationOverrideEvent& location_override) {
    const int kNumMicrosInSecond = 1000000;
    timeval timeVal = {};
    timeVal.tv_sec = location_override.timestamp_in_us() / kNumMicrosInSecond;
    timeVal.tv_usec = location_override.timestamp_in_us() % kNumMicrosInSecond;

    android_gps_send_location(
        location_override.latitude(),
        location_override.longitude(),
        location_override.meters_elevation(),
        location_override.speed_knots(),
        location_override.heading_degrees(),
        location_override.num_satellites(),
        &timeVal);
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
        case StopError::ParseError:
            os << "Parse error";
            break;
            // Default intentionally omitted so that missing statements generate
            // errors.
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const ReplayError& value) {
    switch (value) {
        case ReplayError::PlaybackInProgress:
            os << "Playback in progress";
            break;
        case ReplayError::ParseError:
            os << "Parse error";
            break;
        case ReplayError::InternalError:
            os << "Internal error";
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

    AsyncMessagePipeHandle getPipe() { return mPipe; }

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

    NextCommandStatus getNextCommandDelay(DurationNs* outDelay) override {
        if (!mNextCommand) {
            const std::string nextCommand = mStream->getString();
            if (nextCommand.empty()) {
                VLOG(automation) << "Empty command";
                return NextCommandStatus::NONE;
            }

            pb::RecordedEvent event;
            if (!event.ParseFromString(nextCommand)) {
                VLOG(automation) << "Event parse failed";
                return NextCommandStatus::NONE;
            }

            mNextCommand = event;
        }

        if (!mNextCommand->has_delay()) {
            VLOG(automation) << "Event missing delay";
            return NextCommandStatus::NONE;
        }

        *outDelay = mNextCommand->delay();
        return NextCommandStatus::OK;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(BinaryStreamEventSource);

    Optional<pb::RecordedEvent> mNextCommand;
    std::unique_ptr<android::base::Stream> mStream;
};

class OffworldEventSource : public EventSource {
public:
    OffworldEventSource(android::AsyncMessagePipeHandle pipe,
                        StringView event,
                        uint32_t asyncId,
                        std::function<void(android::AsyncMessagePipeHandle,
                                           const ::offworld::Response&)>
                                sendMessageCallback)
        : mSendMessageCallback(sendMessageCallback) {
        addEvents(pipe, event, asyncId);
    }

    ~OffworldEventSource() {
        // Make sure we consume all events to trigger all replay complete
        // notifications.
        while (!mEvents.empty()) {
            consumeNextCommand();
        }
    }

    pb::RecordedEvent consumeNextCommand() override {
        CHECK(!mEvents.empty());

        Event event = std::move(mEvents.front());
        mEvents.pop_front();

        if (event.lastEvent) {
            ::offworld::Response response = createAsyncResponse(event.asyncId);
            response.mutable_async()->set_complete(true);
            response.mutable_async()
                    ->mutable_automation()
                    ->mutable_replay_complete();
            mSendMessageCallback(event.pipe, response);
        }

        return event.event;
    }

    NextCommandStatus getNextCommandDelay(DurationNs* outDelay) override {
        if (mEvents.empty()) {
            VLOG(automation) << "No more events";
            return NextCommandStatus::NONE;
        }

        const Event& event = mEvents.front();
        *outDelay = event.event.delay();
        return NextCommandStatus::OK;
    }

    bool addEvents(android::AsyncMessagePipeHandle pipe,
                   StringView event,
                   uint32_t asyncId) {
        using namespace google::protobuf;

        std::vector<StringView> splitEventStrings;
        split(event, "\n", [&splitEventStrings](StringView str) {
            if (!str.empty()) {
                splitEventStrings.push_back(str);
            }
        });

        std::vector<pb::RecordedEvent> newEvents;
        for (auto subEventStr : splitEventStrings) {
            io::ArrayInputStream stream(subEventStr.data(), subEventStr.size());
            pb::RecordedEvent event;
            if (!TextFormat::Parse(&stream, &event) ||
                stream.ByteCount() != subEventStr.size()) {
                VLOG(automation) << "Did not reach string EOF, parse error?";
                return false;
            }

            if (!event.has_delay()) {
                VLOG(automation) << "Event missing delay: " << subEventStr;
                return false;
            }

            newEvents.push_back(event);
        }

        if (newEvents.empty()) {
            VLOG(automation) << "No events";
            return false;
        }

        for (size_t i = 0; i < newEvents.size(); ++i) {
            const bool last = i + 1 == newEvents.size();

            Event event;
            event.event = std::move(newEvents[i]);
            event.lastEvent = last;
            event.pipe = pipe;
            event.asyncId = asyncId;

            mEvents.push_back(std::move(event));
        }

        return true;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(OffworldEventSource);

    struct Event {
        pb::RecordedEvent event;

        bool lastEvent = false;
        android::AsyncMessagePipeHandle pipe;
        uint32_t asyncId = 0;
    };

    std::deque<Event> mEvents;
    std::function<void(android::AsyncMessagePipeHandle,
                       const ::offworld::Response&)>
            mSendMessageCallback;
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
    void setNextPlaybackCommandTime(DurationNs nextTimestamp) override;

    uint64_t getLooperNowTimestamp() override;

    StartResult startRecording(StringView filename) override;
    StopResult stopRecording() override;

    StartResult startPlaybackFromSource(
            std::shared_ptr<EventSource> source) override;

    StartResult startPlayback(StringView filename) override;
    StartResult startPlaybackWithCallback(StringView filename,
                                          void (*onStopCallback)()) override;
    StopResult stopPlayback() override;

    // MacroUI helper functions.
    void setMacroName(StringView macroName, StringView filename) override;
    std::string getMacroName(StringView filename) override;
    std::pair<uint64_t, uint64_t> getMetadata(StringView filename) override;

    ReplayResult replayInitialState(StringView state) override;

    ReplayResult replayEvent(android::AsyncMessagePipeHandle pipe,
                             StringView event,
                             uint32_t asyncId) override;

    void sendEvent(const pb::RecordedEvent& event) override;

    ListenResult listen(android::AsyncMessagePipeHandle pipe,
                        uint32_t asyncId) override;

    void stopListening() override;

    void pipeClosed(android::AsyncMessagePipeHandle pipe) override;

private:
    // Helper to replay the last event in the playback stream, must be called
    // under lock.
    //
    // If the playback stream reaches EOF, automatically ends playback.
    void replayNextEvent(const AutoLock& proofOfLock);

    // Reset velocity, used during snapshot load and canceling playback.
    void resetVelocity();

    // Adds macro duration and datetime to header by copying and replacing the
    // file.
    void addMetadataToHeader(DurationNs durationNs);

    // Copies remaining events from originalStream to modifiedStream.
    void copyStreamToStream(std::unique_ptr<StdioStream>&& originalStream,
                            std::unique_ptr<StdioStream>&& modifiedStream);

    AutomationEventSink mEventSink;
    PhysicalModel* const mPhysicalModel;
    base::Looper* const mLooper;

    Lock mLock;
    bool mShutdown = false;
    std::unique_ptr<android::base::Stream> mRecordingStream;

    // Playback state.
    bool mPlayingFromFile = false;
    std::shared_ptr<EventSource> mPlaybackEventSource;
    DurationNs mNextPlaybackCommandTime = 0L;

    // After replay callback.
    std::function<void()> mOnStopCallback;

    // After recording time placing.
    DurationNs mStartTimeNs;
    std::string mFilePath;

    // Offworld state.
    OffworldEventSource* mOffworldEventSource = nullptr;
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
    : mEventSink(looper),
      mPhysicalModel(physicalModel),
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
    mEventSink.shutdown();
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

    resetVelocity();
}

DurationNs AutomationControllerImpl::advanceTime() {
    AutoLock lock(mLock);
    if (mShutdown) {
        return 0;
    }

    const DurationNs nowNs = mLooper->nowNs(Looper::ClockType::kVirtual);
    DurationNs outDelay;
    if (mPlaybackEventSource && mPlaybackEventSource->getNextCommandDelay(
                                        &outDelay) == NextCommandStatus::OK) {
        mPlayingFromFile = true;
        // If mNextPlaybackCommandTime was not initialized, initialize it here
        if (mNextPlaybackCommandTime == 0) {
            mNextPlaybackCommandTime = nowNs + outDelay;
        }
    }
    while (mPlayingFromFile && nowNs >= mNextPlaybackCommandTime) {
        physicalModel_setCurrentTime(mPhysicalModel, mNextPlaybackCommandTime);
        replayNextEvent(lock);
    }

    physicalModel_setCurrentTime(mPhysicalModel, nowNs);
    return nowNs;
}

uint64_t AutomationControllerImpl::getLooperNowTimestamp() {
    return mLooper->nowNs(Looper::ClockType::kVirtual);
}

void AutomationControllerImpl::setNextPlaybackCommandTime(
        DurationNs nextTimestamp) {
    AutoLock lock(mLock);
    mNextPlaybackCommandTime = nextTimestamp;
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
    mFilePath = path;

    pb::InitialState initialState;
    const int saveStateResult =
            physicalModel_saveState(mPhysicalModel, &initialState);
    if (saveStateResult != 0) {
        LOG(ERROR) << "physicalModel_saveState failed with " << saveStateResult;
        return Err(StartError::InternalError);
    }

    pb::FileHeader header;
    header.set_version(kFileVersion);
    *header.mutable_initial_state() = initialState;

    std::string serializedHeader;
    if (!header.SerializeToString(&serializedHeader)) {
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
    mRecordingStream->putString(serializedHeader);
    mEventSink.registerStream(mRecordingStream.get(),
                              StreamEncoding::BinaryPbChunks);

    mStartTimeNs = mLooper->nowNs(Looper::ClockType::kVirtual);

    return Ok();
}

StopResult AutomationControllerImpl::stopRecording() {
    AutoLock lock(mLock);
    if (!mRecordingStream) {
        return Err(StopError::NotStarted);
    }

    DurationNs currentTime = mLooper->nowNs(Looper::ClockType::kVirtual);
    DurationNs durationNs = currentTime - mStartTimeNs;

    DurationNs lastEventTime =
            mEventSink.getLastEventTimeForStream(mRecordingStream.get());
    mEventSink.unregisterStream(mRecordingStream.get());

    pb::RecordedEvent event;
    event.set_delay(currentTime - lastEventTime);
    std::string binaryProto;
    if (!event.SerializeToString(&binaryProto)) {
        LOG(WARNING) << "Could not serialize last event.";
        return Err(StopError::ParseError);
    }
    mRecordingStream->putString(binaryProto);
    mRecordingStream.reset();

    addMetadataToHeader(durationNs);

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

    const std::string headerStr = playbackStream->getString();
    pb::FileHeader header;
    if (headerStr.empty() || !header.ParseFromString(headerStr)) {
        LOG(ERROR) << "Could not load header";
        return Err(StartError::PlaybackFileCorrupt);
    }

    if (!header.has_version() || header.version() != kFileVersion) {
        LOG(ERROR) << "Unsupported file version " << header.version()
                   << " expected version " << kFileVersion;
        return Err(StartError::PlaybackFileCorrupt);
    }

    if (!header.has_initial_state()) {
        LOG(ERROR) << "Initial state missing";
        return Err(StartError::PlaybackFileCorrupt);
    }

    const int loadStateResult =
            physicalModel_loadState(mPhysicalModel, header.initial_state());
    if (loadStateResult != 0) {
        LOG(ERROR) << "physicalModel_loadState failed with " << loadStateResult;
        return Err(StartError::InternalError);
    }

    std::shared_ptr<EventSource> source(
            new BinaryStreamEventSource(std::move(playbackStream)));

    DurationNs nextCommandDelay;
    if (source->getNextCommandDelay(&nextCommandDelay) ==
        NextCommandStatus::OK) {
        // Header parsed, initialize class state.
        mPlaybackEventSource = std::move(source);
        mNextPlaybackCommandTime =
                mLooper->nowNs(Looper::ClockType::kVirtual) + nextCommandDelay;

        mPlayingFromFile = true;
    } else {
        LOG(INFO) << "Playback did not contain any commands";

        if (mOnStopCallback) {
            mOnStopCallback();
            mOnStopCallback = nullptr;
        }
    }

    return Ok();
}

StartResult AutomationControllerImpl::startPlaybackFromSource(
        std::shared_ptr<EventSource> source) {
    // Block playback if either the playing from file flag is explicitly set,
    // which may be set by itself if the file is has ended, or if there is an
    // event source.
    if (mPlayingFromFile || mPlaybackEventSource) {
        return Err(StartError::AlreadyStarted);
    }
    mPlaybackEventSource = std::move(source);

    DurationNs nextCommandDelay;
    if (mPlaybackEventSource->getNextCommandDelay(&nextCommandDelay) ==
        NextCommandStatus::OK) {
        // Header parsed, initialize class state.
        mNextPlaybackCommandTime =
                mLooper->nowNs(Looper::ClockType::kVirtual) + nextCommandDelay;

        mPlayingFromFile = true;
    } else {
        LOG(INFO) << "Playback did not contain any commands";
    }

    return Ok();
}

StartResult AutomationControllerImpl::startPlaybackWithCallback(
        StringView filename,
        void (*onStopCallback)()) {
    mOnStopCallback = onStopCallback;
    return startPlayback(filename);
}

StopResult AutomationControllerImpl::stopPlayback() {
    AutoLock lock(mLock);
    if (!mPlayingFromFile) {
        return Err(StopError::NotStarted);
    }

    resetVelocity();

    mPlayingFromFile = false;
    mPlaybackEventSource.reset();
    mNextPlaybackCommandTime = 0;
    return Ok();
}

std::string AutomationControllerImpl::getMacroName(StringView filename) {
    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
    }

    std::unique_ptr<StdioStream> playbackStream(new StdioStream(
            fsopen(path.c_str(), "rb", FileShare::Read), StdioStream::kOwner));

    const std::string headerStr = playbackStream->getString();
    pb::FileHeader header;
    if (headerStr.empty() || !header.ParseFromString(headerStr) ||
        !header.has_duration_ns()) {
        LOG(ERROR) << "Could not load header name";
        return "";
    }

    return header.name();
}

std::pair<uint64_t, uint64_t> AutomationControllerImpl::getMetadata(
        StringView filename) {
    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
    }

    std::unique_ptr<StdioStream> playbackStream(new StdioStream(
            fsopen(path.c_str(), "rb", FileShare::Read), StdioStream::kOwner));

    const std::string headerStr = playbackStream->getString();
    pb::FileHeader header;
    if (headerStr.empty() || !header.ParseFromString(headerStr) ||
        !header.has_duration_ns() || !header.has_record_datetime()) {
        LOG(ERROR) << "Could not load header";
        return std::make_pair(0, 0);
    }
    return std::make_pair(header.duration_ns(), header.record_datetime());
}

ReplayResult AutomationControllerImpl::replayInitialState(
        android::base::StringView state) {
    if (mPlayingFromFile) {
        return Err(ReplayError::PlaybackInProgress);
    }

    pb::InitialState initialState;
    if (!google::protobuf::TextFormat::ParseFromString(state, &initialState)) {
        LOG(ERROR) << "Could not load initial data";
        return Err(ReplayError::ParseError);
    }

    const int loadStateResult =
            physicalModel_loadState(mPhysicalModel, initialState);
    if (loadStateResult != 0) {
        LOG(ERROR) << "physicalModel_loadState failed with " << loadStateResult;
        return Err(ReplayError::InternalError);
    }

    return Ok();
}

ReplayResult AutomationControllerImpl::replayEvent(
        android::AsyncMessagePipeHandle pipe,
        android::base::StringView event,
        uint32_t asyncId) {
    if (mPlayingFromFile) {
        return Err(ReplayError::PlaybackInProgress);
    }

    if (!mOffworldEventSource) {
        std::shared_ptr<OffworldEventSource> source(new OffworldEventSource(
                pipe, event, asyncId, mSendMessageCallback));

        DurationNs nextCommandDelay;
        if (source->getNextCommandDelay(&nextCommandDelay) ==
            NextCommandStatus::OK) {
            // Header parsed, initialize class state.
            mOffworldEventSource = source.get();
            mPlaybackEventSource = std::move(source);

            mNextPlaybackCommandTime =
                    mLooper->nowNs(Looper::ClockType::kVirtual) +
                    nextCommandDelay;
        } else {
            LOG(INFO) << "Replay did not contain any commands";
            return Err(ReplayError::ParseError);
        }
    } else {
        if (!mOffworldEventSource->addEvents(pipe, event, asyncId)) {
            LOG(INFO) << "Replay did not contain any commands";
            return Err(ReplayError::ParseError);
        }
    }

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

void AutomationControllerImpl::pipeClosed(android::AsyncMessagePipeHandle pipe) {
    if (mPipeListener && mPipeListener->getPipe() == pipe) {
        stopListening();
    }
}

void AutomationControllerImpl::sendEvent(const pb::RecordedEvent& event) {
    switch (event.stream_case()) {
        case pb::RecordedEvent::StreamCase::kPhysicalModel:
            physicalModel_replayEvent(mPhysicalModel, event.physical_model());
            break;
        case pb::RecordedEvent::StreamCase::kSensorOverride: {
            physicalModel_replayOverrideEvent(mPhysicalModel,
                                              event.sensor_override());
            break;
        }
        case pb::RecordedEvent::StreamCase::kLocationOverride:
            sendLocationOverride(event.location_override());
            break;
        case pb::RecordedEvent::StreamCase::STREAM_NOT_SET:
            // Last event for files to simulate recordings.
            break;
        default:
            VLOG(automation) << "Unhandled stream type: "
                             << static_cast<uint32_t>(event.stream_case());
            break;
    }
}

void AutomationControllerImpl::replayNextEvent(const AutoLock& proofOfLock) {
    pb::RecordedEvent event = mPlaybackEventSource->consumeNextCommand();
    sendEvent(event);

    DurationNs nextCommandDelay;
    NextCommandStatus nextCommandStatus =
            mPlaybackEventSource->getNextCommandDelay(&nextCommandDelay);
    if (nextCommandStatus == NextCommandStatus::OK) {
        mNextPlaybackCommandTime += nextCommandDelay;
    } else {
        mPlayingFromFile = false;
        if (nextCommandStatus == NextCommandStatus::NONE) {
            VLOG(automation) << "Playback hit EOF";
            mPlaybackEventSource.reset();
            mOffworldEventSource = nullptr;
            mNextPlaybackCommandTime = 0;

            if (mOnStopCallback) {
                mOnStopCallback();
                mOnStopCallback = nullptr;
            }
        }
    }
}

void AutomationControllerImpl::resetVelocity() {
    const vec3 vecZero = {0.0f, 0.0f, 0.0f};
    physicalModel_setTargetVelocity(mPhysicalModel, vecZero,
                                    PHYSICAL_INTERPOLATION_SMOOTH);
    physicalModel_setTargetAmbientMotion(mPhysicalModel, 0.0f,
                                         PHYSICAL_INTERPOLATION_SMOOTH);
}

void AutomationControllerImpl::addMetadataToHeader(DurationNs durationNs) {
    std::unique_ptr<StdioStream> originalStream(
            new StdioStream(fsopen(mFilePath.c_str(), "rb", FileShare::Read),
                            StdioStream::kOwner));
    const std::string tmpPath = mFilePath + "_tmp";

    LOG(VERBOSE) << "Copy to " << tmpPath;
    std::unique_ptr<StdioStream> modifiedStream(
            new StdioStream(fsopen(tmpPath.c_str(), "wb", FileShare::Write),
                            StdioStream::kOwner));

    const std::string headerStr = originalStream->getString();
    pb::FileHeader header;
    if (headerStr.empty() || !header.ParseFromString(headerStr)) {
        LOG(ERROR) << "Could not find header.";
        return;
    }
    header.set_duration_ns(durationNs);

    // Add timestamp in ms since 1970.
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
    header.set_record_datetime(ms);

    std::string modifiedHeader;
    header.SerializeToString(&modifiedHeader);
    modifiedStream->putString(modifiedHeader);

    copyStreamToStream(std::move(originalStream), std::move(modifiedStream));
}

void AutomationControllerImpl::setMacroName(StringView macroName,
                                            StringView filename) {
    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
    }
    mFilePath = path;

    std::unique_ptr<StdioStream> originalStream(
            new StdioStream(fsopen(mFilePath.c_str(), "rb", FileShare::Read),
                            StdioStream::kOwner));
    const std::string tmpPath = mFilePath + "_tmp";

    LOG(VERBOSE) << "Copy to " << tmpPath;
    std::unique_ptr<StdioStream> modifiedStream(
            new StdioStream(fsopen(tmpPath.c_str(), "wb", FileShare::Write),
                            StdioStream::kOwner));

    const std::string headerStr = originalStream->getString();
    pb::FileHeader header;
    if (headerStr.empty() || !header.ParseFromString(headerStr)) {
        LOG(ERROR) << "Could not find header.";
        return;
    }
    header.set_name(macroName);

    std::string modifiedHeader;
    header.SerializeToString(&modifiedHeader);
    modifiedStream->putString(modifiedHeader);

    copyStreamToStream(std::move(originalStream), std::move(modifiedStream));
}

void AutomationControllerImpl::copyStreamToStream(
        std::unique_ptr<StdioStream>&& firstStream,
        std::unique_ptr<StdioStream>&& secondStream) {
    assert(firstStream != nullptr && secondStream != nullptr);

    std::unique_ptr<StdioStream> originalStream(std::move(firstStream));
    std::unique_ptr<StdioStream> modifiedStream(std::move(secondStream));

    std::unique_ptr<EventSource> source(
            new BinaryStreamEventSource(std::move(originalStream)));

    DurationNs nextCommandDelay;
    while (source->getNextCommandDelay(&nextCommandDelay) ==
           NextCommandStatus::OK) {
        pb::RecordedEvent modifiedEvent = source->consumeNextCommand();
        std::string binaryProto;
        if (!modifiedEvent.SerializeToString(&binaryProto)) {
            LOG(WARNING) << "Could not serialize event.";
            return;
        }
        modifiedStream->putString(binaryProto);
    }

    if (std::remove(mFilePath.c_str()) != 0) {
        LOG(WARNING) << "Could not remove original macro-file.";
    }

    const std::string tmpPath = mFilePath + "_tmp";
    if (std::rename(tmpPath.c_str(), mFilePath.c_str()) != 0) {
        LOG(WARNING) << "Could not rename modified macro-file.";
    }
}

}  // namespace automation
}  // namespace android
