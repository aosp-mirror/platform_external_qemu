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
#include "track_event.pbzero.h"
#include "interned_data.pbzero.h"

#include "perfetto/base/time.h"

#include "perfetto/protozero/message_handle.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_writer.h"

#include "perfetto/ext/tracing/core/shared_memory_abi.h"

#include <thread>
#include <unordered_map>

#include <fcntl.h>

namespace perfetto {

static __attribute__((aligned(4096))) uint8_t sBuffer[4096];
static size_t sWritten = 0;

template <typename T>
static inline uint8_t* sWriteVarInt(T value, uint8_t* target) {
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

static FILE* sDefaultFileHandle = nullptr;

static TraceConfig sTraceConfig = {
    .initialized = false,
    .tracingDisabled = true,
    .packetsWritten = 0,
    .sequenceIdWritten = 0,
    .currentInterningId = 1,
    .currentThreadId = 1,
    .filename = "testtrace.trace",
    .guestStartTime = 0,
    .useGuestStartTime = false,
};

static TraceWriterOperations sDefaultOperations = {
    .openSink = []() {
        sDefaultFileHandle = fopen(sTraceConfig.filename, "wb");
    },
    .closeSink = []() {
        fclose(sDefaultFileHandle);
    },
    .writeToSink = [](uint8_t* src, size_t bytes) {
        fwrite(src, bytes, 1, sDefaultFileHandle);
    },
};

class ChunkStore {
public:
    ChunkStore(uint8_t* buffer, size_t size) : mBuffer(buffer), mSize(size), mAbi(mBuffer, mSize, 4096) {
        mHeader.chunk_id = 0;
        mHeader.writer_id = 0;
        SharedMemoryABI::ChunkHeader::Packets packets;
        packets.count = 1;
        packets.flags = 0;
        mHeader.packets.store(packets, std::memory_order_relaxed);

        if (!mAbi.TryPartitionPage(0, SharedMemoryABI::PageLayout::kPageDiv1)) {
            fprintf(stderr, "%s: couldn't get first page\n", __func__);
        }
    }

    ~ChunkStore() { }

    SharedMemoryABI::Chunk GetNewChunk() {
        ++mHeader.chunk_id;
        SharedMemoryABI::Chunk res = mAbi.TryAcquireChunkForWriting(0, 0, &mHeader);
        if (!res.is_valid()) {
            fprintf(stderr, "%s: not valid\n", __func__);
        }
        return res;
    }

    void flush(SharedMemoryABI::Chunk&& chunk) {
        // fprintf(stderr, "%s: call\n", __func__);
        mAbi.ReleaseChunkAsComplete(std::move(chunk));

        auto chunkForReading = mAbi.TryAcquireChunkForReading(0, 0);

        size_t bytesUsedThisChunk = 0;
        uint8_t* current = chunkForReading.payload_begin();
        do {
            if (0x0a != *current) break;

            ++bytesUsedThisChunk;
            ++current;

            // ... a varint stating its size ...
            uint32_t field_size = 0;
            uint32_t shift = 0;
            for (;;) {
                char c = *(char*)(current); ++ current;
                field_size |= static_cast<uint32_t>(c & 0x7f) << shift;
                shift += 7;
                ++bytesUsedThisChunk;
                if (!(c & 0x80))
                    break;
            }

            bytesUsedThisChunk += field_size;
            current += field_size;
        } while(true);

        // fprintf(stderr, "Writing out %zu bytes\n", bytesUsedThisChunk);

        // for (uint32_t i = 0; i < chunkForReading.payload_size() && i < bytesUsedThisChunk; i += 80) {
        //     for (uint32_t j = i; j < i + 80; ++j) {
        //         if (i + j >= bytesUsedThisChunk) break;
        //         if (i + j >= chunkForReading.payload_size()) break;
        //         fprintf(stderr, "%hhx ", *(((uint8_t*)chunkForReading.payload_begin()) + i + j));
        //     }
        //     fprintf(stderr, "\n");
        // }

        sTraceConfig.ops.writeToSink(chunkForReading.payload_begin(), bytesUsedThisChunk);

        mAbi.ReleaseChunkAsFree(std::move(chunkForReading));
        if (!mAbi.TryPartitionPage(0, SharedMemoryABI::PageLayout::kPageDiv1)) {
            fprintf(stderr, "%s: couldn't get first page\n", __func__);
        }
    }

protected:
    virtual void doWrite(uint8_t* src, size_t bytes) {
        fwrite(src, bytes, 1, mFileHandle);
    }

private:
    FILE* mFileHandle;
    uint8_t* mBuffer;
    size_t mSize;
    SharedMemoryABI mAbi;
    SharedMemoryABI::ChunkHeader mHeader;
};

#define TRACE_STACK_DEPTH_MAX 16

class TraceContext : public protozero::ScatteredStreamWriter::Delegate {
public:
    TraceContext() :
        mChunkStore((uint8_t*)(4096 * ((((uintptr_t)mBuffer) + 4096 - 1) / 4096)), 4096),
        mWriter(this) { }

    virtual ~TraceContext() { }

    virtual protozero::ContiguousMemoryRange GetNewBuffer() {
        bool fragment = mWritingPacket;
        if (fragment) {
            // Finalize the current packet
            mPacket.Finalize();
            // Flush and reset the writer to the new chunk
            finishAndRefresh();
            // Store preamble and size field in the new chunk
            beginPacket();
            // Return a memory range accounting for the new preamble and size field
            return protozero::ContiguousMemoryRange{mChunk.payload_begin() + mWriter.written(), mChunk.end() };
        } else {
            flushAndGetNewChunk();
            return protozero::ContiguousMemoryRange{mChunk.payload_begin(), mChunk.end() };
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

    void beginTrace(const char* name) {
        if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
        if (CC_UNLIKELY(mStackDepth == TRACE_STACK_DEPTH_MAX)) return;

        // Write trusted sequence id if this is the first packet.
        if (CC_UNLIKELY(1 == __atomic_add_fetch(&sTraceConfig.packetsWritten, 1, __ATOMIC_SEQ_CST))) {
            sTraceConfig.sequenceIdWritten = true;
            beginPacket();
            mPacket.set_trusted_packet_sequence_id(1);
            mPacket.set_sequence_flags(1);
            endPacket();
            finishAndRefresh();
        } else if (!CC_LIKELY(sTraceConfig.sequenceIdWritten)) { // Not the first packet, but some other thread is writing the sequence id at the moment, wait for it.
            while (!sTraceConfig.sequenceIdWritten);
        }

        // TODO: Allow changing category
        static const char kCategory[] = "gfxstream";

        bool needEmitCategoryIntern = false;
        mCurrentCategoryIid[mStackDepth] = internCategory(kCategory, &needEmitCategoryIntern);

        bool needEmitEventIntern = false;
        mCurrentEventNameIid[mStackDepth] = internEvent(name, &needEmitEventIntern);

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

        if (CC_UNLIKELY(mNeedToSetThreadId)) {
            mThreadId = __atomic_add_fetch(&sTraceConfig.currentThreadId, 1, __ATOMIC_RELAXED);
            mNeedToSetThreadId = false;
        }

        // Finally do the actual thing
        beginPacket();
        mPacket.set_trusted_packet_sequence_id(kSequenceId);
        mPacket.set_sequence_flags(2 /* incremental */);
        mPacket.set_timestamp((uint64_t)(base::GetWallTimeNs().count()));
        auto trackevent = mPacket.set_track_event();
        trackevent->set_track_uuid(mThreadId); // thread id
        trackevent->add_category_iids(mCurrentCategoryIid[mStackDepth]);
        trackevent->set_name_iid(mCurrentEventNameIid[mStackDepth]);
        trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);
        endPacket();

        ++mStackDepth;
    }

    void endTrace() {
        if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
        if (CC_UNLIKELY(mStackDepth == TRACE_STACK_DEPTH_MAX)) return;

        --mStackDepth;
        // Finally do the actual thing
        beginPacket();
        mPacket.set_trusted_packet_sequence_id(kSequenceId);
        mPacket.set_sequence_flags(2 /* incremental */);
        mPacket.set_timestamp((uint64_t)(base::GetWallTimeNs().count()));
        auto trackevent = mPacket.set_track_event();
        trackevent->add_category_iids(mCurrentCategoryIid[mStackDepth]);
        trackevent->set_track_uuid(mThreadId); // thread id
        trackevent->set_name_iid(mCurrentEventNameIid[mStackDepth]);
        trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_END);
        endPacket();
    }

    void finishAndRefresh() {
        flushAndGetNewChunk();
        mWriter.Reset(protozero::ContiguousMemoryRange{mChunk.payload_begin(), mChunk.end() });
    }

private: 
    void flushAndGetNewChunk() {
        if (mBuffersUsed > 0) {
            mChunkStore.flush(std::move(mChunk));
        }
        ++mBuffersUsed;
        mChunk = mChunkStore.GetNewChunk();
    }

    void beginPacket() {
        // Make sure there's enough space for the preamble and size field, and to hold a track event.
        static const size_t kTrackEventPadding = 200; // conservatively 200 bytes
        size_t neededSpace = SharedMemoryABI::kPacketHeaderSize + 4 + kTrackEventPadding;

        if (mWriter.bytes_available() < neededSpace) {
            finishAndRefresh();
        }

        mPacket.Reset(&mWriter);

        // Write the preamble
        constexpr uint32_t tag = protozero::proto_utils::MakeTagLengthDelimited(1 /* trace packet id */);
        uint8_t tagScratch[10];
        auto scratchNext = sWriteVarInt(tag, tagScratch);
        mWriter.WriteBytes(tagScratch, scratchNext - tagScratch);

        // Reserve the size field
        uint8_t* header = mWriter.ReserveBytes(SharedMemoryABI::kPacketHeaderSize);
        memset(header, 0, SharedMemoryABI::kPacketHeaderSize);
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

    bool mNeedToSetThreadId = true;
    uint32_t mThreadId = 0;
    uint64_t mBuffersUsed = 0;
    bool mCurrentlyTracing = false;
    bool mWritingPacket = false;
    uint32_t mStackDepth = 0;
    uint32_t mCurrentCategoryIid[TRACE_STACK_DEPTH_MAX];
    uint32_t mCurrentEventNameIid[TRACE_STACK_DEPTH_MAX];
    uint8_t mBuffer[8192];
    protozero::RootMessage<protos::pbzero::TracePacket> mPacket;
    SharedMemoryABI::Chunk mChunk;
    ChunkStore mChunkStore;
    protozero::ScatteredStreamWriter mWriter; 
    std::unordered_map<const char*, uint32_t> mEventCategoryInterningIds;
    std::unordered_map<const char*, uint32_t> mEventNameInterningIds;
};

static thread_local TraceContext sThreadLocalTraceContext;

void setTraceConfig(std::function<void(TraceConfig&)> f) {
    f(sTraceConfig);
}

TraceConfig queryTraceConfig() {
    return sTraceConfig;
}

void enableTracing() {
    sTraceConfig.ops.openSink();
    sTraceConfig.tracingDisabled = 0;
}

void disableTracing() {
    sTraceConfig.tracingDisabled = 1;
    sTraceConfig.ops.closeSink();
}

void basic_test() {
    setTraceConfig([](TraceConfig& config) {
        config.ops = sDefaultOperations;
    });
    enableTracing();
    for (uint32_t i = 0; i < 40; ++i) {
        sThreadLocalTraceContext.beginTrace("test trace 1");
        sThreadLocalTraceContext.beginTrace("test trace 1.1");
        sThreadLocalTraceContext.endTrace();
        sThreadLocalTraceContext.endTrace();
        sThreadLocalTraceContext.beginTrace("test trace 2");
        sThreadLocalTraceContext.endTrace();
    }
    sThreadLocalTraceContext.finishAndRefresh();
    disableTracing();
}

} // namespace perfetto
