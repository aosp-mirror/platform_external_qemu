// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "perfetto-tracing-only.h"

#include "trace_packet.pbzero.h"
#include "counter_descriptor.pbzero.h"
#include "track_descriptor.pbzero.h"
#include "track_event.pbzero.h"
#include "interned_data.pbzero.h"

#include "perfetto/base/time.h"

#include "perfetto/protozero/message_handle.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_writer.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace virtualdeviceperfetto {

static FILE* sDefaultFileHandle = nullptr;

static VirtualDeviceTraceConfig sTraceConfig = {
    .initialized = false,
    .tracingDisabled = true,
    .packetsWritten = 0,
    .sequenceIdWritten = 0,
    .currentInterningId = 1,
    .currentThreadId = 1,
    .hostFilename = "vmm.trace",
    .guestFilename = nullptr,
    .combinedFilename = nullptr,
    .hostStartTime = 0,
    .guestStartTime = 0,
    .guestTimeDiff = 0,
    .perThreadStorageMb = 1,
};

#define TRACE_STACK_DEPTH_MAX 16

class TraceContext;

struct SavedTraceInfo {
    uint8_t* data;
    size_t allocSize;
    size_t written;
    bool first;
};

class TraceStorage {
public:
    void add(TraceContext* context) {
        std::lock_guard<std::mutex> lock(mContextsLock);
        mContexts.insert(context);
    }

    void remove(TraceContext* context) {
        std::lock_guard<std::mutex> lock(mContextsLock);
        mContexts.erase(context);
    }

    void onTracingEnabled() {
        // do stuff
    }

    void onTracingDisabled() {
        saveTracesToDisk();
    }

    // When a thread exited early before tracing was disabled
    void saveTrace(const SavedTraceInfo& trace) {
        std::lock_guard<std::mutex> lock(mContextsLock);
        saveTraceLocked(trace);
    }

private:
    void saveTraceLocked(const SavedTraceInfo& trace) {
        if (trace.data)
            mSavedTraces.push_back(trace);
    }

    void saveTracesToDisk();

    std::mutex mContextsLock; // protects |mContexts|
    std::unordered_set<TraceContext*> mContexts;
    std::vector<SavedTraceInfo> mSavedTraces;
};

static TraceStorage sTraceStorage;

class TraceContext : public protozero::ScatteredStreamWriter::Delegate {
public:
    TraceContext() :
        mWriter(this) {
            sTraceStorage.add(this);
        }

    SavedTraceInfo save(bool partialReset = false) {
        ScopedTracingLock lock(&mTracingLock);
        return saveLocked(partialReset);
    }

    virtual ~TraceContext() {
        sTraceStorage.remove(this);
        if (mTraceBuffer) {
            sTraceStorage.saveTrace(save());
        }
    }

    virtual protozero::ContiguousMemoryRange GetNewBuffer() {
        if (mWritingPacket) {
            mPacket.Finalize();
        }

        finishAndRefresh();

        if (mWritingPacket) {
            beginPacket();
            size_t writtenThisTime = size_t(((uintptr_t)mWriter.write_ptr()) - (uintptr_t)mTraceBuffer);
            return protozero::ContiguousMemoryRange{mWriter.write_ptr(), mWriter.write_ptr() + (mTraceBufferSize - writtenThisTime) };
        } else {
            return protozero::ContiguousMemoryRange{mTraceBuffer, mTraceBuffer + mTraceBufferSize};
        }

    }

#ifdef __cplusplus
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif
    static const uint32_t kSequenceId = 1;
    static constexpr char kTrackNamePrefix[] = "emu-";
    static constexpr char kCounterNamePrefix[] = "-count-";
    void beginTrace(const char* name) {
        if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
        if (CC_UNLIKELY(mStackDepth == TRACE_STACK_DEPTH_MAX)) return;

        ScopedTracingLock lock(&mTracingLock);

        ensureThreadInfo();
        bool needEmitEventIntern = false;
        mCurrentEventNameIid[mStackDepth] = internEvent(name, &needEmitEventIntern);
        if (CC_UNLIKELY(needEmitEventIntern)) {
            beginPacket();
            mPacket.set_trusted_packet_sequence_id(kSequenceId);
            mPacket.set_sequence_flags(2 /* incremental */);
            auto interned_data = mPacket.set_interned_data();
            auto eventname = interned_data->add_event_names();
            eventname->set_iid(mCurrentEventNameIid[mStackDepth]);
            eventname->set_name(name);
            endPacket();
        }
        // Finally do the actual thing
        beginPacket();
        mPacket.set_trusted_packet_sequence_id(kSequenceId);
        mPacket.set_sequence_flags(2 /* incremental */);
        mPacket.set_timestamp(getTimestamp());
        auto trackevent = mPacket.set_track_event();
        trackevent->set_track_uuid(mThreadId); // thread id
        trackevent->add_category_iids(mCurrentCategoryIid[mStackDepth]);
        trackevent->set_name_iid(mCurrentEventNameIid[mStackDepth]);
        trackevent->set_type(::perfetto::protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);
        endPacket();
        ++mStackDepth;
    }

    inline void ensureThreadInfo() __attribute__((always_inline)) {
        // Write trusted sequence id if this is the first packet.
        if (CC_UNLIKELY(1 == __atomic_add_fetch(&sTraceConfig.packetsWritten, 1, __ATOMIC_SEQ_CST))) {
            mFirst = true;
            sTraceConfig.sequenceIdWritten = true;
            beginPacket();
            mPacket.set_trusted_packet_sequence_id(1);
            mPacket.set_sequence_flags(1);
            endPacket();
        } else if (!CC_LIKELY(sTraceConfig.sequenceIdWritten)) { // Not the first packet, but some other thread is writing the sequence id at the moment, wait for it.
            while (!sTraceConfig.sequenceIdWritten);
        }
        // TODO: Allow changing category
        static const char kCategory[] = "gfxstream";
        bool needEmitCategoryIntern = false;
        mCurrentCategoryIid[mStackDepth] = internCategory(kCategory, &needEmitCategoryIntern);
        if (CC_UNLIKELY(needEmitCategoryIntern)) {
            beginPacket();
            mPacket.set_trusted_packet_sequence_id(kSequenceId);
            mPacket.set_sequence_flags(2 /* incremental */);
            auto interned_data = mPacket.set_interned_data();
            auto category = interned_data->add_event_categories();
            category->set_iid(mCurrentCategoryIid[mStackDepth]);
            category->set_name(kCategory);
            endPacket();
        }
        if (CC_UNLIKELY(mNeedToSetThreadId)) {
            mThreadId = __atomic_add_fetch(&sTraceConfig.currentThreadId, 1, __ATOMIC_RELAXED);
            mNeedToSetThreadId = false;
            fprintf(stderr, "%s: found thread id: %u\n", __func__, mThreadId);
            beginPacket();
            mPacket.set_trusted_packet_sequence_id(kSequenceId);
            mPacket.set_sequence_flags(2 /* incremental */);
            auto desc = mPacket.set_track_descriptor();
            desc->set_uuid(mThreadId);
            desc->set_name(getTrackNameFromThreadId(mThreadId));
            endPacket();
        }
    }
    void endTrace() {
        if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
        if (CC_UNLIKELY(mStackDepth == TRACE_STACK_DEPTH_MAX)) return;
        if (CC_UNLIKELY(mStackDepth == 0)) return;
        --mStackDepth;

        ScopedTracingLock lock(&mTracingLock);

        // Finally do the actual thing
        beginPacket();
        mPacket.set_trusted_packet_sequence_id(kSequenceId);
        mPacket.set_sequence_flags(2 /* incremental */);
        mPacket.set_timestamp(getTimestamp());
        auto trackevent = mPacket.set_track_event();
        trackevent->add_category_iids(mCurrentCategoryIid[mStackDepth]);
        trackevent->set_track_uuid(mThreadId); // thread id
        trackevent->set_name_iid(mCurrentEventNameIid[mStackDepth]);
        trackevent->set_type(::perfetto::protos::pbzero::TrackEvent::TYPE_SLICE_END);
        endPacket();
    }

    void traceCounter(const char* name, int64_t val) {
        if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;

        ScopedTracingLock lock(&mTracingLock);

        ensureThreadInfo();
        bool first;
        uint32_t counterId;
        uint64_t counterTrackUuid = getOrCreateCounterTrackUuid(name, &counterId, &first);
        if (CC_UNLIKELY(first)) {
            fprintf(stderr, "%s: thread id: %u has a counter: %u. uuid: 0x%llx\n", __func__, mThreadId, counterId, (unsigned long long)(counterTrackUuid));
            beginPacket();
            auto desc = mPacket.set_track_descriptor();
            desc->set_uuid(counterTrackUuid);
            desc->set_name(getTrackNameFromThreadIdAndCounterName(mThreadId, name));
            desc->set_counter();
            endPacket();
        }
        // Do the actual counter track event
        beginPacket();
        mPacket.set_trusted_packet_sequence_id(kSequenceId);
        mPacket.set_sequence_flags(2 /* incremental */);
        mPacket.set_timestamp(getTimestamp());
        auto trackevent = mPacket.set_track_event();
        trackevent->set_track_uuid(counterTrackUuid);
        trackevent->set_type(::perfetto::protos::pbzero::TrackEvent::TYPE_COUNTER);
        trackevent->set_counter_value(val);
        endPacket();
    }

    void waitFinish() {
        ScopedTracingLock lock(&mTracingLock);
    }

private:
    class ScopedTracingLock {
    public:
        ScopedTracingLock(std::atomic_flag* flag) : mFlag(flag) {
            while (mFlag->test_and_set(std::memory_order_acquire)) {
                // spin
            }
        }

        ~ScopedTracingLock() {
            mFlag->clear(std::memory_order_release);
        }
    private:
        std::atomic_flag* mFlag;
    };

    SavedTraceInfo saveLocked(bool partialReset) {
        // Invalidates mTraceBuffer, transfers ownership of it.
        SavedTraceInfo info = {
            mTraceBuffer,
            mTraceBufferSize,
            size_t(((uintptr_t)mWriter.write_ptr()) - (uintptr_t)mTraceBuffer),
            mFirst,
        };

        resetLocked(partialReset);

        return info;
    }

    void resetLocked(bool partialReset) {
        mTraceBuffer = nullptr;
        mTraceBufferSize = 0;
        mFirst = false;

        if (partialReset) return;

        mNeedToSetThreadId = true;
        mThreadId = 0;
        mNeedToConfigureGuestTime = true;
        mCurrentCounterId = 1;
        mTimeDiff = 0;
        mStackDepth = 0;
        mEventCategoryInterningIds.clear();
        mEventNameInterningIds.clear();
        mCounterNameToTrackUuids.clear();
        mPacket.Reset(&mWriter);
    }

    void finishAndRefresh() {
        if (mTraceBuffer) {
            sTraceStorage.saveTrace(saveLocked(true /* partial reset */));
        }
        allocTraceBuffer();
    };

    void allocTraceBuffer() {
        // Freed after ownership is transferred to Trace Storage
        mTraceBufferSize = sTraceConfig.perThreadStorageMb * 1048576;
        mTraceBuffer = (uint8_t*)malloc(mTraceBufferSize);
        mWriter.Reset(protozero::ContiguousMemoryRange{mTraceBuffer, mTraceBuffer + mTraceBufferSize});
    }

    inline uint64_t getTimestamp() {
        uint64_t t = (uint64_t)(::perfetto::base::GetWallTimeNs().count());
        t += sTraceConfig.guestTimeDiff;
        return t;
    }

    template <typename T>
    static inline uint8_t* writeVarInt(T value, uint8_t* target) {
        // If value is <= 0 we must first sign extend to int64_t (see [1]).
        // Finally we always cast to an unsigned value to to avoid arithmetic
        // (sign expanding) shifts in the while loop.
        // [1]: "If you use int32 or int64 as the type for a negative number, the
        // resulting varint is always ten bytes long".
        // - developers.google.com/protocol-buffers/docs/encoding
        // So for each input type we do the following casts:
        // uintX_t -> uintX_t -> uintX_t
        // int8_t  -> int64_t -> uint64_t
        // int16_t -> int64_t -> uint64_t
        // int32_t -> int64_t -> uint64_t
        // int64_t -> int64_t -> uint64_t
        using MaybeExtendedType =
            typename std::conditional<std::is_unsigned<T>::value, T, int64_t>::type;
        using UnsignedType = typename std::make_unsigned<MaybeExtendedType>::type;

        MaybeExtendedType extended_value = static_cast<MaybeExtendedType>(value);
        UnsignedType unsigned_value = static_cast<UnsignedType>(extended_value);

        while (unsigned_value >= 0x80) {
            *target++ = static_cast<uint8_t>(unsigned_value) | 0x80;
            unsigned_value >>= 7;
        }
        *target = static_cast<uint8_t>(unsigned_value);
        return target + 1;
    }

    void beginPacket() {
        if (CC_UNLIKELY(mTraceBuffer == nullptr)) {
            allocTraceBuffer();
        }

        // Make sure there's enough space for the preamble and size field, and to hold a track event.
        static const size_t kTrackEventPadding = 200; // conservatively 200 bytes
        static const size_t kPacketHeaderSize = 4;
        size_t neededSpace = kPacketHeaderSize + 4 + kTrackEventPadding;
        if (mWriter.bytes_available() < neededSpace) {
            finishAndRefresh();
        }

        mPacket.Reset(&mWriter);
        // Write the preamble
        constexpr uint32_t tag = protozero::proto_utils::MakeTagLengthDelimited(1 /* trace packet id */);
        uint8_t tagScratch[10];
        auto scratchNext = writeVarInt(tag, tagScratch);
        mWriter.WriteBytes(tagScratch, scratchNext - tagScratch);
        // Reserve the size field
        uint8_t* header = mWriter.ReserveBytes(kPacketHeaderSize);
        memset(header, 0, kPacketHeaderSize);
        mPacket.set_size_field(header);
        mWritingPacket = true;
    }

    void endPacket() {
        mWritingPacket = false;
        mPacket.Finalize();
    }

    uint32_t internCategory(const char* str, bool* firstTime) {
        auto it = mEventCategoryInterningIds.find(str);
        if (it != mEventCategoryInterningIds.end()) {
            *firstTime = false;
            return it->second;
        }
        uint32_t res = sTraceConfig.currentInterningId;
        mEventCategoryInterningIds[str] = res;
        __atomic_add_fetch(&sTraceConfig.currentInterningId, 1, __ATOMIC_RELAXED);
        *firstTime = true;
        return res;
    }

    uint32_t internEvent(const char* str, bool* firstTime) {
        auto it = mEventNameInterningIds.find(str);
        if (it != mEventNameInterningIds.end()) {
            *firstTime = false;
            return it->second;
        }
        uint32_t res = sTraceConfig.currentInterningId;
        mEventNameInterningIds[str] = res;
        __atomic_add_fetch(&sTraceConfig.currentInterningId, 1, __ATOMIC_RELAXED);
        *firstTime = true;
        return res;
    }

    static std::string getTrackNameFromThreadId(uint32_t threadId) {
        std::stringstream ss;
        ss << kTrackNamePrefix << threadId;
        return ss.str();
    }

    static std::string getTrackNameFromThreadIdAndCounterName(uint32_t threadId, const char* counterName) {
        std::stringstream ss;
        ss << kTrackNamePrefix << threadId << kCounterNamePrefix << counterName;
        return ss.str();
    }

    uint64_t getOrCreateCounterTrackUuid(const char* name, uint32_t* counterId, bool* firstTime) {
        auto it = mCounterNameToTrackUuids.find(name);
        uint64_t res;
        if (CC_UNLIKELY(it == mCounterNameToTrackUuids.end())) {
            // The counter track uuid is the thread id | shifted counter id.
            res = (((uint64_t)mCurrentCounterId) << 32) | mThreadId;
            mCounterNameToTrackUuids[name] = res;
            *counterId = mCurrentCounterId;
            *firstTime = true;
            // Increment counter id for this thread.
            ++mCurrentCounterId;
        } else {
            res = it->second;
            *counterId = res >> 32;
            *firstTime = false;
        }
        return res;
    }

    uint8_t* mTraceBuffer = nullptr;
    size_t mTraceBufferSize = 0;
    bool mFirst = false;
    bool mWritingPacket = false;
    bool mNeedToSetThreadId = true;
    uint32_t mThreadId = 0;
    bool mNeedToConfigureGuestTime = true;
    uint32_t mCurrentCounterId = 1;
    uint64_t mTimeDiff = 0;
    std::atomic_flag mTracingLock;
    uint32_t mStackDepth = 0;
    uint32_t mCurrentCategoryIid[TRACE_STACK_DEPTH_MAX];
    uint32_t mCurrentEventNameIid[TRACE_STACK_DEPTH_MAX];
    protozero::RootMessage<::perfetto::protos::pbzero::TracePacket> mPacket;
    protozero::ScatteredStreamWriter mWriter;
    std::unordered_map<const char*, uint32_t> mEventCategoryInterningIds;
    std::unordered_map<const char*, uint32_t> mEventNameInterningIds;
    std::unordered_map<const char*, uint64_t> mCounterNameToTrackUuids;
};

void asyncTraceSaveFunc() {
    fprintf(stderr, "%s: Saving combined trace async...\n", __func__);

    static const int kWaitSecondsPerIteration = 1;
    static const int kMaxIters = 20;
    static const int kMinItersForGuestFileSize = 2;

    const char* hostFilename = sTraceConfig.hostFilename;
    const char* guestFilename = sTraceConfig.guestFilename;
    const char* combinedFilename = sTraceConfig.combinedFilename;

    std::streampos currGuestSize = 0;
    int numGoodGuestFileSizeIters = 0;
    bool good = false;

    for (int i = 0; i < kMaxIters; ++i) {
        fprintf(stderr, "%s: Waiting for 1 second...\n", __func__);
        std::this_thread::sleep_for(std::chrono::seconds(kWaitSecondsPerIteration));
        fprintf(stderr, "%s: Querying file size of guest trace...\n", __func__);
        std::ifstream guestFile(guestFilename, std::ios::in | std::ios::binary | std::ios::ate);
        std::streampos size = guestFile.tellg();

        if (!size) {
            fprintf(stderr, "%s: No size, try again\n", __func__);
            continue;
        }

        if (size != currGuestSize) {
            fprintf(stderr, "%s: Sized changed (%llu to %llu), try again\n", __func__,
                    (unsigned long long)currGuestSize, (unsigned long long)size);
            currGuestSize = size;
            continue;
        }

        ++numGoodGuestFileSizeIters;

        if (numGoodGuestFileSizeIters == kMinItersForGuestFileSize) {
            fprintf(stderr, "%s: size is stable, continue saving\n", __func__);
            good = true;
            break;
        }
    }

    if (!good) {
        fprintf(stderr, "%s: Timed out when waiting for guest file to stabilize, skipping combined trace saving.\n", __func__);
        return;
    }

    std::ifstream hostFile(hostFilename, std::ios_base::binary);
    std::ifstream guestFile(guestFilename, std::ios_base::binary);
    std::ofstream combinedFile(combinedFilename, std::ios::out | std::ios_base::binary);

    combinedFile << guestFile.rdbuf() << hostFile.rdbuf();

    fprintf(stderr, "%s: Wrote combined trace (%s)\n", __func__, combinedFilename);
}

void TraceStorage::saveTracesToDisk() {
    fprintf(stderr, "%s: Tracing ended================================================================================\n", __func__);
    fprintf(stderr, "%s: Saving trace to disk. Configuration:\n", __func__);
    fprintf(stderr, "%s: host filename: %s\n", __func__, sTraceConfig.hostFilename);
    fprintf(stderr, "%s: guest filename: %s\n", __func__, sTraceConfig.guestFilename);
    fprintf(stderr, "%s: combined filename: %s\n", __func__, sTraceConfig.combinedFilename);

    fprintf(stderr, "%s: Saving host trace first...\n", __func__);

    std::lock_guard<std::mutex> lock(mContextsLock);

    for (auto context: mContexts) {
        saveTraceLocked(context->save());
    };

    std::ofstream hostOut;
    hostOut.open(sTraceConfig.hostFilename, std::ios::out | std::ios::binary);

    for (const auto& info : mSavedTraces) {
        if (info.first) {
            hostOut.write((const char*)(info.data), info.written);
        }
    }

    for (const auto& info : mSavedTraces) {
        if (!info.first) {
            hostOut.write((const char*)(info.data), info.written);
        }
        free(info.data);
    }

    hostOut.close();

    mSavedTraces.clear();

    fprintf(stderr, "%s: Saving host trace first...(done)\n", __func__);

    if (!sTraceConfig.guestFilename || !sTraceConfig.combinedFilename) {
        fprintf(stderr, "%s: skipping guest combined trace, "
                        "either guest file name (%p) not specified or "
                        "combined file name (%p) not specified\n", __func__,
                sTraceConfig.guestFilename,
                sTraceConfig.combinedFilename);
        return;
    }

    std::thread saveThread(asyncTraceSaveFunc);
    saveThread.detach();
}

static thread_local TraceContext sThreadLocalTraceContext;

PERFETTO_TRACING_ONLY_EXPORT void setTraceConfig(std::function<void(VirtualDeviceTraceConfig&)> f) {
    f(sTraceConfig);
}

VirtualDeviceTraceConfig queryTraceConfig() {
    return sTraceConfig;
}

void initialize(const bool** tracingDisabledPtr) {
    *tracingDisabledPtr = &sTraceConfig.tracingDisabled;
}

bool useFilenameByEnv(const char* s) {
    return s && ("" != std::string(s));
}

void enableTracing() {
    const char* hostFilenameByEnv = std::getenv("VPERFETTO_HOST_FILE");
    const char* guestFilenameByEnv = std::getenv("VPERFETTO_GUEST_FILE");
    const char* combinedFilenameByEnv = std::getenv("VPERFETTO_COMBINED_FILE");

    if (useFilenameByEnv(hostFilenameByEnv)) {
        sTraceConfig.hostFilename = hostFilenameByEnv;
    }

    if (useFilenameByEnv(guestFilenameByEnv)) {
        sTraceConfig.guestFilename = guestFilenameByEnv;
    }

    if (useFilenameByEnv(combinedFilenameByEnv)) {
        sTraceConfig.combinedFilename = combinedFilenameByEnv;
    }
    
    fprintf(stderr, "%s: Tracing begins================================================================================\n", __func__);
    fprintf(stderr, "%s: Configuration:\n", __func__);
    fprintf(stderr, "%s: host filename: %s (possibly set via $VPERFETTO_HOST_FILE)\n", __func__, sTraceConfig.hostFilename);
    fprintf(stderr, "%s: guest filename: %s (possibly set via $VPERFETTO_GUEST_FILE)\n", __func__, sTraceConfig.guestFilename);
    fprintf(stderr, "%s: combined filename: %s (possibly set via $VPERFETTO_COMBINED_FILE)\n", __func__, sTraceConfig.combinedFilename);
    fprintf(stderr, "%s: guest time diff to add to host time: %llu\n", __func__, (unsigned long long)sTraceConfig.guestTimeDiff);

    sTraceConfig.packetsWritten = 0;
    sTraceConfig.sequenceIdWritten = 0;
    sTraceConfig.currentInterningId = 1;
    sTraceConfig.currentThreadId = 1;

    sTraceStorage.onTracingEnabled();
    sTraceConfig.tracingDisabled = 0;
}

void disableTracing() {
    uint32_t tracingWasDisabled = sTraceConfig.tracingDisabled;
    sTraceConfig.tracingDisabled = 1;

    if (!tracingWasDisabled) {
        sTraceStorage.onTracingDisabled();
    }

    sTraceConfig.packetsWritten = 0;
    sTraceConfig.sequenceIdWritten = 0;
    sTraceConfig.currentInterningId = 1;
    sTraceConfig.currentThreadId = 1;
    sTraceConfig.guestTimeDiff = 0;
}

void beginTrace(const char* name) {
    if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
    sThreadLocalTraceContext.beginTrace(name);
}

void endTrace() {
    if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
    sThreadLocalTraceContext.endTrace();
}

void traceCounter(const char* name, int64_t val) {
    if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
    sThreadLocalTraceContext.traceCounter(name, val);
}

void setGuestTime(uint64_t t) {
    virtualdeviceperfetto::setTraceConfig([t](virtualdeviceperfetto::VirtualDeviceTraceConfig& config) {
        // can only be set before tracing
        if (!config.tracingDisabled) {
            return;
        }
        config.guestStartTime = t;
        config.hostStartTime = (uint64_t)(::perfetto::base::GetWallTimeNs().count());
        config.guestTimeDiff = config.guestStartTime - config.hostStartTime;
    });
}

} // namespace virtualdeviceperfetto
