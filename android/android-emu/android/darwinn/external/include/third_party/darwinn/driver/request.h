#ifndef THIRD_PARTY_DARWINN_DRIVER_REQUEST_H_
#define THIRD_PARTY_DARWINN_DRIVER_REQUEST_H_

#include <stddef.h>
#include <functional>
#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/allocator.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/dma_info.h"
#include "third_party/darwinn/driver/dma_info_extractor.h"
#include "third_party/darwinn/driver/executable_util.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/port/array_slice.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// DarwiNN compute request. Thread safe.
class Request : public api::Request {
 public:
  // Constructs a request object for executing the given |executable_reference|.
  // |done| is the callback function executed when request is complete.
  // TODO(jnjoseph): Make this constructor private and create request objects
  // through a factory function in the driver.
  Request(int id, const ExecutableReference* executable_reference,
          Allocator* allocator,
          std::unique_ptr<DeviceBufferMapper> device_buffer_mapper,
          const DmaInfoExtractor* extractor, uint64 alignment_bytes);
  Request(int id, const ExecutableReference* executable_reference,
          Allocator* allocator,
          std::unique_ptr<DeviceBufferMapper> device_buffer_mapper,
          const DmaInfoExtractor* extractor, uint64 alignment_bytes, Done done);

  // This class is neither copyable nor movable.
  Request(const Request&) = delete;
  Request& operator=(const Request&) = delete;

  ~Request() override;

  // Sets the callback function executed when request is complete.
  void SetDone(Done done) LOCKS_EXCLUDED(mutex_);

  // Adds an input or output buffer. This may be called repeatedly depending
  // on the batch size as long as the request instance is not submitted. If
  // supplied "name" does not exist or size constraints on the input and output
  // buffers do not match executable, will return failure. Memory backing the
  // |Buffer| instance must be valid throughout the life of the request.
  util::Status AddInput(const std::string& name, const Buffer& input)
      LOCKS_EXCLUDED(mutex_) override;
  util::Status AddOutput(const std::string& name, Buffer output)
      LOCKS_EXCLUDED(mutex_) override;

  // Returns the input Buffer with the given layer name and batch ID.
  const Buffer& InputBuffer(const std::string& name, int batch) const override
      LOCKS_EXCLUDED(mutex_);

  // Returns the output Buffer with the given layer name and batch ID. This is
  // intended to be used by the ReferenceDriver.
  Buffer OutputBuffer(const std::string& name, int batch) const override
      LOCKS_EXCLUDED(mutex_);

  // Validates the constraints and builds the request.
  util::Status Validate() LOCKS_EXCLUDED(mutex_);
  util::Status Prepare() LOCKS_EXCLUDED(mutex_);

  // Cancels the pending request. Cancellation is best effort. Completion
  // callback is called if not already. Canceling a completed request has
  // no effect.
  util::Status Cancel() LOCKS_EXCLUDED(mutex_);

  // TODO(b/32309535): The following functions needs to restricted for use
  // by the driver only.

  // Notifies that request is submitted to the driver, but not yet issued.
  util::Status NotifyRequestSubmitted() LOCKS_EXCLUDED(mutex_);

  // Notifies that request is active. That is, request is issued to DarwiNN.
  util::Status NotifyRequestActive() LOCKS_EXCLUDED(mutex_);

  // Notifies completion of the request with the given status.
  util::Status NotifyCompletion(util::Status status) LOCKS_EXCLUDED(mutex_);

  // Returns request id.
  int id() const override { return id_; }

  // Returns the number of instruction bitstream chunks.
  int num_instruction_bitstream_chunks() const {
    return executable().instruction_bitstreams()->Length();
  }

  // Returns a list of DMAs to be performed.
  util::StatusOr<std::list<DmaInfo>> GetDmaInfos() const LOCKS_EXCLUDED(mutex_);

  // Returns executable reference.
  const ExecutableReference& executable_reference() const {
    return executable_reference_;
  }

  // Returns the number of batches that have been added to this request, or an
  // error if not all inputs and outputs have the same number of batches.
  util::StatusOr<int> GetNumBatches() const override LOCKS_EXCLUDED(mutex_);

 private:
  // Compute request state. State transitions :
  //  kUninitialized -> kCreated -> kSubmitted -> kActive -> kDone
  //  kUninitialized -> kCreated -> kSubmitted -> kDone [if cancelled].
  enum State {
    kUninitialized,  // Request not initialized.
    kCreated,        // Request created, but pending issue to DarwiNN.
    kSubmitted,      // Request submitted and in queue for issuing to DarwiNN.
    kActive,         // Request issued to DarwiNN, pending results.
    kDone,           // Request in terminal state.
  };

  // Attempts a state transition to the given state.
  util::Status SetState(State next_state) EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Validates that we are in the expected state.
  util::Status ValidateState(State expected_state) const
      SHARED_LOCKS_REQUIRED(mutex_);

  // Maps all data buffers (input, output, parameters) to the DarwiNN address
  // space.
  util::Status MapDataBuffers() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Map instruction buffers to the DarwiNN address space.
  util::Status MapInstructionBuffers() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Unmaps all buffers and frees the allocated instruction and parameter
  // buffers if any. Reverse of what is done in #Prepare().
  util::Status Cleanup() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Convenience function that returns the backing executable in
  // |executable_reference_|.
  const darwinn::Executable& executable() const {
    return executable_reference_.executable();
  }

  // Checks alignment requirement for a provided buffer and returns an error if
  // it is mis-aligned.
  util::Status ValidateBufferAlignment(const Buffer& buffer);

  // Unique ID for request.
  const int id_;

  // Executable for the compute request.
  const ExecutableReference& executable_reference_;

  // Buffer allocator.
  Allocator* const allocator_;

  // Maps and stores all device buffers.
  std::unique_ptr<DeviceBufferMapper> device_buffer_mapper_;

  // DMA info extractor.
  const DmaInfoExtractor& extractor_;

  // Maintains integrity of the request object.
  mutable std::mutex mutex_;

  // Request state.
  State state_ GUARDED_BY(mutex_){kUninitialized};

  // Infeed and outfeed host buffers.
  // host_*[layer_name][batch_id] = buffer.
  Buffer::NamedMap host_inputs_ GUARDED_BY(mutex_);
  Buffer::NamedMap host_outputs_ GUARDED_BY(mutex_);

  // Final request completion callback.
  Done done_ GUARDED_BY(mutex_);

  // A copy of the mapped parameters owned by executable reference.
  const DeviceBuffer parameter_device_buffer_ GUARDED_BY(mutex_);
  // Buffers for instructions.
  std::unique_ptr<InstructionBuffers> instruction_buffers_ GUARDED_BY(mutex_);

  // The alignment requirement for input and output buffers provided by the
  // user.
  const uint64 alignment_bytes_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_REQUEST_H_
