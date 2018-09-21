#include "third_party/darwinn/driver/request.h"

#include <vector>

#include "third_party/darwinn/api/allocated_buffer.h"
#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/allocator.h"
#include "third_party/darwinn/driver/executable_util.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/port/array_slice.h"
#include "third_party/darwinn/port/cleanup.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/macros.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"
#include "third_party/darwinn/port/tracing.h"

namespace platforms {
namespace darwinn {
namespace driver {

using ::flatbuffers::VectorLength;

Request::Request(int id, const ExecutableReference* executable_reference,
                 Allocator* allocator,
                 std::unique_ptr<DeviceBufferMapper> device_buffer_mapper,
                 const DmaInfoExtractor* extractor, Done done)
    : id_(id),
      executable_reference_(*[executable_reference]() {
        CHECK(executable_reference != nullptr);
        return executable_reference;
      }()),
      allocator_([allocator]() {
        CHECK(allocator != nullptr);
        return allocator;
      }()),
      device_buffer_mapper_(std::move(device_buffer_mapper)),
      extractor_(*[extractor]() {
        CHECK(extractor != nullptr);
        return extractor;
      }()),
      done_(std::move(done)),
      parameter_device_buffer_(
          executable_reference_.GetParameterDeviceBuffer()) {
  VLOG(5) << StringPrintf("[%d] Request constructed.", id_);
}

Request::Request(int id, const ExecutableReference* executable_reference,
                 Allocator* allocator,
                 std::unique_ptr<DeviceBufferMapper> device_buffer_mapper,
                 const DmaInfoExtractor* extractor)
    : Request(id, executable_reference, allocator,
              std::move(device_buffer_mapper), extractor,
              /*done=*/nullptr) {}

Request::~Request() {
  VLOG(5) << StringPrintf("[%d] Request destroyed.", id_);
  CHECK_OK(Cleanup());
}

void Request::SetDone(Done done) {
  StdMutexLock lock(&mutex_);
  CHECK_OK(ValidateState(kUninitialized));
  done_ = std::move(done);
}

util::Status Request::AddInput(const std::string& name, const Buffer& input) {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kUninitialized));
  RETURN_IF_ERROR(executable_reference_.ValidateInput(name, input));

  VLOG(3) << StringPrintf("Adding input \"%s\" with %zu bytes.", name.c_str(),
                          input.size_bytes());
  host_inputs_[name].push_back(input);
  return util::Status();  // OK
}

util::Status Request::AddOutput(const std::string& name, Buffer output) {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kUninitialized));
  RETURN_IF_ERROR(executable_reference_.ValidateOutput(name, output));

  VLOG(3) << StringPrintf("Adding output \"%s\" with %zu bytes.", name.c_str(),
                          output.size_bytes());
  host_outputs_[name].push_back(std::move(output));
  return util::Status();  // OK
}

util::Status Request::MapDataBuffers() {
  // Map activations except parameters, which is done at registration time.
  TRACE_SCOPE("Darwinn:Request::MapDataBuffers");
  RETURN_IF_ERROR(device_buffer_mapper_->MapScratch(
      executable_reference_.scratch()));
  TRACE_WITHIN_SCOPE("Darwinn:Request::MapDataBuffers::MappedScratch");
  RETURN_IF_ERROR(device_buffer_mapper_->MapInputs(host_inputs_));
  TRACE_WITHIN_SCOPE("Darwinn:Request::MapDataBuffers::MappedInputs");
  RETURN_IF_ERROR(device_buffer_mapper_->MapOutputs(host_outputs_));
  TRACE_WITHIN_SCOPE("Darwinn:Request::MapDataBuffers::MappedOutputs");
  return util::Status();  // OK
}

util::Status Request::MapInstructionBuffers() {
  RETURN_IF_ERROR(device_buffer_mapper_->MapInstructions(instruction_buffers_));
  return util::Status();  // OK
}

util::Status Request::Cleanup() {
  // Note that these calls are a no-op if request is already in a clean state.
  RETURN_IF_ERROR(device_buffer_mapper_->UnmapAll());
  instruction_buffers_.clear();

  return util::Status();  // OK
}

util::Status Request::Validate() {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kUninitialized));

  // Validate instruction bit stream.
  if (VectorLength(executable().instruction_bitstreams()) == 0) {
    return util::InvalidArgumentError(
        "Executable does not contain instruction bitstream.");
  }
  for (const auto& chunk : *executable().instruction_bitstreams()) {
    if (VectorLength(chunk->bitstream()) == 0) {
      return util::InvalidArgumentError(
          "Executable contains empty instruction bitstream chunk.");
    }
  }

  // Number of input / outputs should match with executable.
  if (host_inputs_.size() != VectorLength(executable().input_layers())) {
    return util::InvalidArgumentError(
        "Added inputs does not match the number of required inputs for "
        "executable.");
  }

  if (host_outputs_.size() != VectorLength(executable().output_layers())) {
    return util::InvalidArgumentError(
        "Added outputs does not match the number of required outputs for "
        "executable.");
  }

  // Number of input / output buffers must match configured batch size.
  for (const auto& name_and_input : host_inputs_) {
    if (name_and_input.second.size() != executable().batch_size()) {
      return util::InvalidArgumentError(
          StringPrintf("Number of input buffers for \"%s\" does not match "
                       "configured batch size. expected=%d, actual=%zu.",
                       name_and_input.first.c_str(), executable().batch_size(),
                       name_and_input.second.size()));
    }
  }

  for (const auto& name_and_output : host_outputs_) {
    if (name_and_output.second.size() != executable().batch_size()) {
      return util::InvalidArgumentError(
          StringPrintf("Number of output buffers for \"%s\" does not match "
                       "configured batch size. expected=%d, actual=%zu.",
                       name_and_output.first.c_str(), executable().batch_size(),
                       name_and_output.second.size()));
    }
  }

  return util::Status();  // OK
}

util::Status Request::Prepare() {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kUninitialized));

  // Allocate and create an aligned copy of instruction bitstream.
  instruction_buffers_.reserve(
      VectorLength(executable().instruction_bitstreams()));
  for (const auto& chunk : *executable().instruction_bitstreams()) {
    auto buffer = allocator_->MakeBuffer(chunk->bitstream()->Length());
    instruction_buffers_.push_back(std::move(buffer));
    memcpy(instruction_buffers_.back().ptr(), chunk->bitstream()->data(),
           chunk->bitstream()->Length());
  }

  // Cleanup instructions.
  auto instructions_cleanup = gtl::MakeCleanup(
      [this]() NO_THREAD_SAFETY_ANALYSIS { instruction_buffers_.clear(); });

  RETURN_IF_ERROR(MapDataBuffers());
  VLOG(10) << "MapDataBuffers() done.";

  // Data buffer cleanup.
  auto data_buffers_cleanup = gtl::MakeCleanup(
      [this]() { CHECK_OK(device_buffer_mapper_->UnmapAll()); });

  // Update the instruction stream to link the input, output and parameter
  // addresses.
  for (int i = 0; i < VectorLength(executable().instruction_bitstreams());
       ++i) {
    // Link scratch address if necessary.
    if (device_buffer_mapper_->GetScratchDeviceBuffer().IsValid()) {
      ExecutableUtil::LinkScratchAddress(
          device_buffer_mapper_->GetScratchDeviceBuffer().device_address(),
          executable().instruction_bitstreams()->Get(i)->field_offsets(),
          gtl::MutableArraySlice<uint8>(
              instruction_buffers_[i].ptr(),
              VectorLength(
                  executable().instruction_bitstreams()->Get(i)->bitstream())));
    }

    // Link parameters if necessary.
    if (parameter_device_buffer_.IsValid()) {
      const uint64 linked_parameter_address =
          parameter_device_buffer_.device_address();
      ExecutableUtil::LinkParameterAddress(
          linked_parameter_address,
          executable().instruction_bitstreams()->Get(i)->field_offsets(),
          gtl::MutableArraySlice<uint8>(
              instruction_buffers_[i].ptr(),
              VectorLength(
                  executable().instruction_bitstreams()->Get(i)->bitstream())));
    }

    for (const auto& name_and_mapped_input :
         device_buffer_mapper_->GetInputDeviceBuffers()) {
      std::vector<uint64> linked_input_addresses;
      for (const auto& mapped_input : name_and_mapped_input.second) {
        linked_input_addresses.push_back(mapped_input.device_address());
      }
      ExecutableUtil::LinkInputAddress(
          name_and_mapped_input.first, linked_input_addresses,
          executable().instruction_bitstreams()->Get(i)->field_offsets(),
          gtl::MutableArraySlice<uint8>(
              instruction_buffers_[i].ptr(),
              VectorLength(
                  executable().instruction_bitstreams()->Get(i)->bitstream())));
    }

    for (const auto& name_and_mapped_output :
         device_buffer_mapper_->GetOutputDeviceBuffers()) {
      std::vector<uint64> linked_output_addresses;
      for (const auto& mapped_output : name_and_mapped_output.second) {
        linked_output_addresses.push_back(mapped_output.device_address());
      }
      ExecutableUtil::LinkOutputAddress(
          name_and_mapped_output.first, linked_output_addresses,
          executable().instruction_bitstreams()->Get(i)->field_offsets(),
          gtl::MutableArraySlice<uint8>(
              instruction_buffers_[i].ptr(),
              VectorLength(
                  executable().instruction_bitstreams()->Get(i)->bitstream())));
    }
  }

  // Mapping of instruction buffers must happen after instructions have been
  // been patched with linked addresses. Any further modifications to
  // instructions may not be visible to device due to cache coherency issues.
  RETURN_IF_ERROR(MapInstructionBuffers());
  VLOG(10) << "MapInstructionBuffers() done.";

  // All good.
  data_buffers_cleanup.release();
  instructions_cleanup.release();

  return SetState(kCreated);
}

util::Status Request::NotifyRequestSubmitted() {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kCreated));

  VLOG(3) << StringPrintf("[%d] NotifyRequestSubmitted()", id_);
  return SetState(kSubmitted);
}

util::Status Request::NotifyRequestActive() {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kSubmitted));

  VLOG(3) << StringPrintf("[%d] NotifyRequestActive()", id_);
  return SetState(kActive);
}

util::Status Request::NotifyCompletion(util::Status status) {
  StdMutexLock lock(&mutex_);
  RETURN_IF_ERROR(ValidateState(kActive));

  VLOG(3) << StringPrintf("[%d] NotifyCompletion()", id_);

  // Cleanup first before notify, because we need to unmap buffers first to
  // guarantee that output buffers are coherent.
  status.Update(Cleanup());
  if (done_) {
    done_(id_, status);
  }

  return SetState(kDone);
}

util::StatusOr<std::list<DmaInfo>> Request::GetDmaInfos() const {
  StdMutexLock lock(&mutex_);
  if (state_ != kCreated && state_ != kSubmitted) {
    return util::FailedPreconditionError(
        StringPrintf("Unexpected call to GetDmaInfos in state_ = %d.", state_));
  }

  return extractor_.ExtractDmaInfos(executable_reference_,
                                    *device_buffer_mapper_);
}

util::Status Request::Cancel() {
  StdMutexLock lock(&mutex_);

  VLOG(3) << StringPrintf("[%d] Cancel()", id_);

  if (state_ == kUninitialized || state_ == State::kCreated) {
    return util::FailedPreconditionError(
        StringPrintf("Cannot cancel in state_=%d.", state_));
  }

  // If State::kSubmitted, OK to cancel.
  if (state_ == State::kSubmitted) {
    // Run completed callback.
    if (done_) {
      done_(id_, util::CancelledError("Request cancelled."));
    }

    RETURN_IF_ERROR(Cleanup());
    return SetState(kDone);
  }

  // If State::kActive, do nothing and allow request to complete.
  // If State::kDone, do nothing because request is already complete.
  return util::Status();  // OK
}

util::Status Request::ValidateState(State expected_state) const {
  if (state_ != expected_state) {
    return util::FailedPreconditionError(
        StringPrintf("Bad state. expected=%d, actual=%d.",
                     expected_state, state_));
  }
  return util::Status();  // OK
}

util::Status Request::SetState(State next_state) {
  VLOG(5) << StringPrintf("[%d] SetState old=%d, new=%d.",
                          id_, state_, next_state);
  switch (state_) {
    case kUninitialized:
      if (next_state == kCreated) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kCreated:
      if (next_state == kSubmitted) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kSubmitted:
      if (next_state == kActive || next_state == kDone) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kActive:
      if (next_state == kDone) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kDone:
      break;
  }

  // Illegal state transition.
  return util::FailedPreconditionError(StringPrintf(
      "Invalid state transition. current=%d, next=%d.", state_, next_state));
}

const Buffer& Request::InputBuffer(const std::string& name, int batch) const {
  StdMutexLock lock(&mutex_);
  return host_inputs_.at(name)[batch];
}

Buffer Request::OutputBuffer(const std::string& name, int batch) {
  StdMutexLock lock(&mutex_);
  return host_outputs_.at(name)[batch];
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
