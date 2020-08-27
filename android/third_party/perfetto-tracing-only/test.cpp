#include "trace_packet.pbzero.h"

#include "perfetto/protozero/message_handle.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_writer.h"

#include "perfetto/ext/tracing/core/shared_memory_abi.h"

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
    }

    SharedMemoryABI::Chunk flushOutstandingAndGetNewChunk() {
        ++mHeader.chunk_id;
        // TODO: flush the current chunk/page
        SharedMemoryABI::Chunk res = mAbi.TryAcquireChunkForWriting(0, 0, &mHeader);
        return res;
    }

private:
    uint8_t* mBuffer;
    size_t mSize;
    SharedMemoryABI mAbi;
    SharedMemoryABI::ChunkHeader mHeader;

};

class TestStreamWriterDelegate : public protozero::ScatteredStreamWriter::Delegate {
public:
    TestStreamWriterDelegate() : mChunkStore(sBuffer, 4096) { }

    virtual ~TestStreamWriterDelegate() { }

    virtual protozero::ContiguousMemoryRange GetNewBuffer() {
        mChunk = mChunkStore.flushOutstandingAndGetNewChunk();
        // flush current contents to disk up to sWritten...
        return protozero::ContiguousMemoryRange{mChunk.payload_begin(), mChunk.end() };
    }
private:
    SharedMemoryABI::Chunk mChunk;
    TestChunkStore mChunkStore;
};

using TracePacketHandle =
    protozero::MessageHandle<protos::pbzero::TracePacket>;

void hello() {
    auto packet = new protozero::RootMessage<protos::pbzero::TracePacket>();
    packet->Finalize();

    TestStreamWriterDelegate delegate;
    protozero::ScatteredStreamWriter writer(&delegate);

    packet->Reset(&writer);
    uint8_t* header = writer.ReserveBytes(SharedMemoryABI::kPacketHeaderSize);
    memset(header, 0, SharedMemoryABI::kPacketHeaderSize);
    packet->set_size_field(header);

    TracePacketHandle handle(packet);
}

} // namespace perfetto


