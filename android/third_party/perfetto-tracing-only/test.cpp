#include "trace_packet.pbzero.h"
#include "track_event.pbzero.h"
#include "interned_data.pbzero.h"

#include "perfetto/base/time.h"

#include "perfetto/protozero/message_handle.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_writer.h"

#include "perfetto/ext/tracing/core/shared_memory_abi.h"

#include <mutex>
#include <unordered_map>

#include <fcntl.h>

// using perfetto::protos::pbzero::TracePacket;

namespace perfetto {

static __attribute__((aligned(4096))) uint8_t sBuffer[4096];
static size_t sWritten = 0;

class TestChunkStore {
public:
    TestChunkStore(uint8_t* buffer, size_t size) : mBuffer(buffer), mSize(size), mAbi(mBuffer, mSize, 4096) {
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

    SharedMemoryABI::Chunk GetNewChunk() {
        ++mHeader.chunk_id;
        SharedMemoryABI::Chunk res = mAbi.TryAcquireChunkForWriting(0, 0, &mHeader);
        if (!res.is_valid()) {
            fprintf(stderr, "%s: not valid\n", __func__);
        }
        return res;
    }

    void flush(SharedMemoryABI::Chunk&& chunk) {
        fprintf(stderr, "%s: call\n", __func__);
        mAbi.ReleaseChunkAsComplete(std::move(chunk));

        auto chunkForReading = mAbi.TryAcquireChunkForReading(0, 0);

        size_t bytesUsedThisChunk = 0;
        uint8_t* current = chunkForReading.payload_begin();
        do {
            // preamble. if == 0
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

        fprintf(stderr, "Writing out %zu bytes\n", bytesUsedThisChunk);

        for (uint32_t i = 0; i < chunkForReading.payload_size() && i < bytesUsedThisChunk; i += 80) {
            for (uint32_t j = i; j < i + 80; ++j) {
                if (i + j >= bytesUsedThisChunk) break;
                if (i + j >= chunkForReading.payload_size()) break;
                fprintf(stderr, "%hhx ", *(((uint8_t*)chunkForReading.payload_begin()) + i + j));
            }
            fprintf(stderr, "\n");
        }

        FILE* fh = fopen("./testtrace.trace", "wb");
        fwrite(chunkForReading.payload_begin(), bytesUsedThisChunk, 1, fh);
        fclose(fh);

        mAbi.ReleaseChunkAsFree(std::move(chunkForReading));
    }

private:
    uint8_t* mBuffer;
    size_t mSize;
    SharedMemoryABI mAbi;
    SharedMemoryABI::ChunkHeader mHeader;
};

class TestStreamWriterDelegate : public protozero::ScatteredStreamWriter::Delegate {
public:
    TestStreamWriterDelegate(TestChunkStore* store) : mStore(store) { }

    virtual ~TestStreamWriterDelegate() { }

    virtual protozero::ContiguousMemoryRange GetNewBuffer() {
        if (mBuffersUsed > 0) {
            mStore->flush(std::move(mChunk));
        }
        mChunk = mStore->GetNewChunk();
        ++mBuffersUsed;
        return protozero::ContiguousMemoryRange{mChunk.payload_begin(), mChunk.end() };
    }

    void flush() {
        mStore->flush(std::move(mChunk));
    }

private:
    uint64_t mBuffersUsed = 0;
    SharedMemoryABI::Chunk mChunk;
    TestChunkStore* mStore;
};

using TracePacketHandle =
    protozero::MessageHandle<protos::pbzero::TracePacket>;

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

void commitAndRefreshPacket(protozero::RootMessage<protos::pbzero::TracePacket>* packet, protozero::ScatteredStreamWriter* writer) {
    // Finish the previous
    packet->Finalize();
    packet->Reset(writer);

    // Write the preamble
    constexpr uint32_t tag = protozero::proto_utils::MakeTagLengthDelimited(1 /* trace packet id */);
	uint8_t tagScratch[10];
    auto scratchNext = sWriteVarInt(tag, tagScratch);
    writer->WriteBytes(tagScratch, scratchNext - tagScratch);

    // Reserve the size field
    uint8_t* header = writer->ReserveBytes(SharedMemoryABI::kPacketHeaderSize);
    memset(header, 0, SharedMemoryABI::kPacketHeaderSize);
    packet->set_size_field(header);
}

void commitPacket(protozero::RootMessage<protos::pbzero::TracePacket>* packet) {
    // Finish the previous
    packet->Finalize();
}

static uint32_t sTracingEnabled = 0;
static uint32_t sPacketsWritten = 0;
static uint32_t sSequenceIdWritten = 0;
static uint32_t sCurrentInterningId = 1;

class TraceContext : public protozero::ScatteredStreamWriter::Delegate {
public:
    TraceContext() :
        mChunkStore(mBuffer, sizeof(mBuffer)),
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

    void beginTrace(const char* name) {
        if (CC_UNLIKELY(sTracingEnabled)) {
            static const uint32_t kSequenceId = 1;

            // Write trusted sequence id if this is the first packet.
            if (CC_UNLIKELY(1 == __atomic_add_fetch(&sPacketsWritten, 1, __ATOMIC_SEQ_CST))) {
                sSequenceIdWritten = true;
                beginPacket();
                mPacket.set_trusted_packet_sequence_id(1);
                mPacket.set_sequence_flags(1);
                endPacket();
                finishAndRefresh();
            } else if (!CC_LIKELY(sSequenceIdWritten)) { // Not the first packet, but some other thread is writing the sequence id at the moment, wait for it.
                while (!sSequenceIdWritten);
            }

            // TODO: Allow changing category
            static const char kCategory[] = "gfxstream";

            bool needEmitCategoryIntern = false;
            uint32_t categoryIid = internCategory(kCategory, &needEmitCategoryIntern);

            bool needEmitEventIntern = false;
            uint32_t eventNameIid = internEvent(name, &needEmitEventIntern);

            if (CC_UNLIKELY(needEmitCategoryIntern)) {
                beginPacket();
                mPacket.set_sequence_flags(2 /* incremental */);
                mPacket.set_trusted_packet_sequence_id(kSequenceId);
                auto interned_data = mPacket.set_interned_data();
                auto category = interned_data->add_event_categories();
                category->set_iid(categoryIid);
                category->set_name(kCategory);
                endPacket();
            }

            if (CC_UNLIKELY(needEmitEventIntern)) {
                beginPacket();
                mPacket.set_sequence_flags(2 /* incremental */);
                mPacket.set_trusted_packet_sequence_id(kSequenceId);
                auto interned_data = mPacket.set_interned_data();
                auto eventname = interned_data->add_event_names();
                eventname->set_iid(eventNameIid);
                eventname->set_name(name);
                endPacket();
            }

            // Finally do the actual thing
        }
    }

    void endTrace(const char* name) {
        if (!sTracingEnabled) return;
    }
private: 
    void flushAndGetNewChunk() {
        if (mBuffersUsed > 0) {
            mChunkStore.flush(std::move(mChunk));
        }
        ++mBuffersUsed;
        mChunk = mChunkStore.GetNewChunk();
    }

    void finishAndRefresh() {
        flushAndGetNewChunk();
        mWriter.Reset(protozero::ContiguousMemoryRange{mChunk.payload_begin(), mChunk.end() });
    }

    void beginPacket() {
        // Make sure there's enough space for the preamble and size field
        size_t neededSpace = SharedMemoryABI::kPacketHeaderSize + 4;

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

        uint32_t res = sCurrentInterningId;
        mEventCategoryInterningIds[str] = res;
        __atomic_add_fetch(&sCurrentInterningId, 1, __ATOMIC_RELAXED);
        *firstTime = true;
        return res;
    }

    uint32_t internEvent(const char* str, bool* firstTime) {
        auto it = mEventNameInterningIds.find(str);
        if (it != mEventNameInterningIds.end()) {
            *firstTime = false;
            return it->second;
        }

        uint32_t res = sCurrentInterningId;
        mEventNameInterningIds[str] = res;
        __atomic_add_fetch(&sCurrentInterningId, 1, __ATOMIC_RELAXED);
        *firstTime = true;
        return res;
    }

    uint64_t mBuffersUsed = 0;
    bool mCurrentlyTracing = false;
    bool mWritingPacket = false;
    uint8_t mBuffer[4096];
    protozero::RootMessage<protos::pbzero::TracePacket> mPacket;
    SharedMemoryABI::Chunk mChunk;
    TestChunkStore mChunkStore;
    protozero::ScatteredStreamWriter mWriter; 
    std::unordered_map<const char*, uint32_t> mEventCategoryInterningIds;
    std::unordered_map<const char*, uint32_t> mEventNameInterningIds;
};

static thread_local TraceContext sThreadLocalTraceContext;

void hello2() { }

void hello() {
    auto packet = new protozero::RootMessage<protos::pbzero::TracePacket>();

    TestChunkStore chunkStore(sBuffer, 4096);

    TestStreamWriterDelegate delegate(&chunkStore);
    protozero::ScatteredStreamWriter writer(&delegate);

    commitAndRefreshPacket(packet, &writer);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(1 /* seq incr cleared */);
    commitAndRefreshPacket(packet, &writer);

    packet->set_trusted_packet_sequence_id(1);
    auto interned_data = packet->set_interned_data();
    auto category = interned_data->add_event_categories();
    category->set_iid(1);
    category->set_name("category1");
    packet->set_sequence_flags(2 /* seq incr cleared */);

    commitAndRefreshPacket(packet, &writer);
    packet->set_trusted_packet_sequence_id(1);
    interned_data = packet->set_interned_data();
    auto name = interned_data->add_event_names();
    name->set_iid(2);
    name->set_name("name2");
    auto name3 = interned_data->add_event_names();
    name->set_iid(3);
    name->set_name("name3");
    packet->set_sequence_flags(2 /* seq incr needed */);

    commitAndRefreshPacket(packet, &writer);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    packet->set_timestamp(5);
    auto trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(2000);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_END);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(4000);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(4200);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(3);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(4500);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(3);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_END);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(5000);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_END);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(3000);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->set_track_uuid(1);
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);

    commitAndRefreshPacket(packet, &writer);
    packet->set_timestamp(5000);
    packet->set_trusted_packet_sequence_id(1);
    packet->set_sequence_flags(2 /* seq incr needed */);
    trackevent = packet->set_track_event();
    trackevent->set_track_uuid(1); // thread id
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_END);

    commitPacket(packet);
    delegate.flush();

    delete packet;
}

} // namespace perfetto
