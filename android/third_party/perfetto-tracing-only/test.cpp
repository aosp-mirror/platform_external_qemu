#include "trace_packet.pbzero.h"

#include "perfetto/protozero/message_handle.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_writer.h"

#include "perfetto/ext/tracing/core/shared_memory_abi.h"

// using perfetto::protos::pbzero::TracePacket;

namespace perfetto {

static uint8_t sBuffer[4096];
static size_t sWritten = 0;

class TestStreamWriterDelegate : public protozero::ScatteredStreamWriter::Delegate {
public:
    virtual ~TestStreamWriterDelegate() { }

    virtual protozero::ContiguousMemoryRange GetNewBuffer() {
        // flush current contents to disk up to sWritten...
        return protozero::ContiguousMemoryRange{sBuffer, sBuffer + 4096};
    }
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


