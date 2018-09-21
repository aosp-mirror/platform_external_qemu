#ifndef THIRD_PARTY_DARWINN_DRIVER_DEBUG_TRACER_H_
#define THIRD_PARTY_DARWINN_DRIVER_DEBUG_TRACER_H_

#include <memory>
#include <mutex>  // NOLINT
#include <string>

#include "third_party/darwinn/driver/debug/trace.pb.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/protobuf_helper.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace debug {

// Trace various driver and hardware events.
class Tracer {
 public:
  // Constructs a tracer that logs to the given file. Tracing is diabled if
  // trace_file is an empty string.
  explicit Tracer(const std::string& trace_file);

  // Noop tracer.
  Tracer() : Tracer("") {}

  ~Tracer();

  void Close() LOCKS_EXCLUDED(mutex_);

  void LogRegisterRead(uint64 offset, uint64 value);
  void LogRegisterWrite(uint64 offset, uint64 value);
  void LogRegisterPoll(uint64 offset, uint64 expected_value);
  void LogDmaRead(uint64 address, const char* buffer, uint64 size);
  void LogDmaWrite(uint64 address, const char* buffer, uint64 size);
  void LogInterrupt(int32 interrupt_id);

  // This class is neither copyable nor movable.
  Tracer(const Tracer&) = delete;
  Tracer& operator=(const Tracer&) = delete;

 private:
  // Logs a trace event to file.
  void LogTraceEvent(const TraceEvent& trace_event) LOCKS_EXCLUDED(mutex_);

  // True, if trace is enabled.
  bool trace_enabled_{false};

  // Trace file descriptor.
  int trace_fd_{0};
  std::unique_ptr<FileOutputStream> file_output_stream_;
  std::unique_ptr<CodedOutputStream> coded_output_stream_ GUARDED_BY(mutex_);

  // Guards access to the coded_output_stream_;
  std::mutex mutex_;
};

}  // namespace debug
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_DEBUG_TRACER_H_
