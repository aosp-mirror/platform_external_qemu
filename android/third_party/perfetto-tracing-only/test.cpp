#include "trace_packet.pbzero.h"
#include "track_event.pbzero.h"
#include "interned_data.pbzero.h"

#include "trace_buffer.h"

#include "perfetto/protozero/message_handle.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_writer.h"

#include "perfetto/ext/tracing/core/shared_memory_abi.h"

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

    SharedMemoryABI::Chunk flushOutstandingAndGetNewChunk() {
        ++mHeader.chunk_id;
        // TODO: flush the current chunk/page
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
        mChunk = mStore->flushOutstandingAndGetNewChunk();
        return protozero::ContiguousMemoryRange{mChunk.payload_begin(), mChunk.end() };
    }

    void flush() {
        mStore->flush(std::move(mChunk));
    }

private:
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

void hello() {
    auto outBuffer = TraceBuffer::Create(4096);
    auto packet = new protozero::RootMessage<protos::pbzero::TracePacket>();

    TestChunkStore chunkStore(sBuffer, 4096);

    TestStreamWriterDelegate delegate(&chunkStore);
    protozero::ScatteredStreamWriter writer(&delegate);

    commitAndRefreshPacket(packet, &writer);
    auto interned_data = packet->set_interned_data();
    auto category = interned_data->add_event_categories();
    category->set_iid(1);
    category->set_name("category1");
    auto name = interned_data->add_event_names();
    name->set_iid(2);
    name->set_name("name2");
    auto name3 = interned_data->add_event_names();
    name->set_iid(3);
    name->set_name("name3");

    packet->set_sequence_flags(1 /* seq incr cleared */);

    packet->set_trusted_packet_sequence_id(1);
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

    commitPacket(packet);
    delegate.flush();

    delete packet;
}

} // namespace perfetto


