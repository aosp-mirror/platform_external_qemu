#include "third_party/darwinn/driver/debug/tracer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
#include <string>

#include "third_party/darwinn/driver/debug/trace.pb.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/protobuf_helper.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/std_mutex_lock.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace debug {

Tracer::Tracer(const std::string& trace_file) {
  trace_enabled_ = !trace_file.empty();
  if (trace_enabled_) {
    trace_fd_ = open(trace_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0640);
    CHECK_GE(trace_fd_, 0) << "Open failed!";
    file_output_stream_ = gtl::MakeUnique<FileOutputStream>(trace_fd_);
    coded_output_stream_ =
        gtl::MakeUnique<CodedOutputStream>(file_output_stream_.get());
  }
}

Tracer::~Tracer() {
  if (trace_enabled_) {
    Close();
  }
}

void Tracer::Close() {
  if (trace_enabled_) {
    StdMutexLock lock(&mutex_);
    coded_output_stream_.reset();
    CHECK(file_output_stream_->Close());  // Closes underlying file descriptor.
    trace_enabled_ = false;
  }
}

void Tracer::LogTraceEvent(const TraceEvent& trace_event) {
  if (trace_enabled_) {
    StdMutexLock lock(&mutex_);
    coded_output_stream_->WriteVarint32(trace_event.ByteSize());
    CHECK(trace_event.SerializeToCodedStream(coded_output_stream_.get()));
  }
}

void Tracer::LogRegisterRead(uint64 offset, uint64 value) {
  TraceEvent trace_event;
  trace_event.mutable_register_read()->set_offset(offset);
  trace_event.mutable_register_read()->set_value(value);
  LogTraceEvent(trace_event);
}

void Tracer::LogRegisterWrite(uint64 offset, uint64 value) {
  TraceEvent trace_event;
  trace_event.mutable_register_write()->set_offset(offset);
  trace_event.mutable_register_write()->set_value(value);
  LogTraceEvent(trace_event);
}

void Tracer::LogRegisterPoll(uint64 offset, uint64 expected_value) {
  TraceEvent trace_event;
  trace_event.mutable_register_poll()->set_offset(offset);
  trace_event.mutable_register_poll()->set_expected_value(expected_value);
  LogTraceEvent(trace_event);
}

void Tracer::LogDmaRead(uint64 address, const char* buffer, uint64 size) {
  TraceEvent trace_event;
  auto dma_read = trace_event.mutable_dma_read();
  dma_read->set_address(address);
  dma_read->set_size(size);
  dma_read->set_data(buffer, size);
  LogTraceEvent(trace_event);
}

void Tracer::LogDmaWrite(uint64 address, const char* buffer, uint64 size) {
  TraceEvent trace_event;
  auto dma_write = trace_event.mutable_dma_write();
  dma_write->set_address(address);
  dma_write->set_size(size);
  dma_write->set_data(buffer, size);
  LogTraceEvent(trace_event);
}

void Tracer::LogInterrupt(int32 interrupt_id) {
  TraceEvent trace_event;
  trace_event.mutable_interrupt()->set_id(interrupt_id);
  LogTraceEvent(trace_event);
}

}  // namespace debug
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
