#include "trace_packet.pbzero.h"
#include "track_event.pbzero.h"
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

        FILE* fh = fopen("./testtrace.trace", "wb");
        fwrite(chunkForReading.payload_begin(), chunkForReading.payload_size(), 1, fh);
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

void setPacketHeader(protozero::RootMessage<protos::pbzero::TracePacket>* packet, protozero::ScatteredStreamWriter* writer) {
    uint8_t* header = writer->ReserveBytes(SharedMemoryABI::kPacketHeaderSize);
    memset(header, 0, SharedMemoryABI::kPacketHeaderSize);
    packet->set_size_field(header);
}

void hello() {
    auto outBuffer = TraceBuffer::Create(4096);
    auto packet = new protozero::RootMessage<protos::pbzero::TracePacket>();
    packet->Finalize();

    TestChunkStore chunkStore(sBuffer, 4096);

    TestStreamWriterDelegate delegate(&chunkStore);
    protozero::ScatteredStreamWriter writer(&delegate);

    // packet->Reset(&writer); setPacketHeader(packet, &writer);
    // packet->set_timestamp(0);
    // auto config = packet->set_trace_config();
    // config->add_buffers()->set_size_kb(4);
    // auto ds = config->add_data_sources();
    // auto ds_config = ds->mutable_config();
    // ds_config->set_name("mydatasource");
    // packet->Finalize();

    packet->Reset(&writer); setPacketHeader(packet, &writer);

    packet->set_timestamp(123);
    packet->set_sequence_flags(1 /* seq incr cleared */);

    auto trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_BEGIN);

    packet->Finalize();

    packet->Reset(&writer); setPacketHeader(packet, &writer);

    packet->set_timestamp(234);
    packet->set_sequence_flags(1 /* seq incr cleared */);

    trackevent = packet->set_track_event();
    trackevent->add_category_iids(1);
    trackevent->set_name_iid(2);
    trackevent->set_type(protos::pbzero::TrackEvent::TYPE_SLICE_END);

    packet->Finalize();

    delegate.flush();

    delete packet;
}

} // namespace perfetto


