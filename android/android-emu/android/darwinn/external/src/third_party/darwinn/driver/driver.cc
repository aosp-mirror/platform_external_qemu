#include "third_party/darwinn/driver/driver.h"

#include <atomic>
#include <memory>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/executable_reference.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/executable_registry.h"
#include "third_party/darwinn/driver/request.h"
#include "third_party/darwinn/port/blocking_counter.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

Driver::Driver(api::Chip chip,
               std::unique_ptr<ExecutableRegistry> executable_registry)
    : chip_(chip), executable_registry_(std::move(executable_registry)) {}

util::Status Driver::ValidateState(State expected_state) const {
  if (state_ != expected_state) {
    return util::FailedPreconditionError(StringPrintf(
        "Bad state. expected=%d, actual=%d.", expected_state, state_));
  }
  return util::Status();  // OK
}

util::Status Driver::SetState(State next_state) {
  switch (state_) {
    case kOpen:
      if (next_state == kClosing) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kClosing:
      if (next_state == kClosed) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kClosed:
      if (next_state == kOpen) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;
  }

  // Illegal state transition.
  return util::FailedPreconditionError(StringPrintf(
      "Invalid state transition. current=%d, next=%d.", state_, next_state));
}

bool Driver::IsOpen() const {
  StdMutexLock state_lock(&state_mutex_);
  return state_ == kOpen;
}

bool Driver::IsError() const { return in_error_; }

util::Status Driver::Open(bool debug_mode) {
  StdMutexLock state_lock(&state_mutex_);
  if (num_clients_ > 0) {
    num_clients_++;
    return util::Status();  // OK
  }
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kClosed));

  RETURN_IF_ERROR(DoOpen(debug_mode));
  num_clients_++;
  // All good. Move state to open.
  return SetState(kOpen);
}

// TODO (b/112562718) mapping parameters at executable
// registration time can lead to OOM even if we have enough memory for one
// request.
util::Status Driver::MapParameters(
    api::ExecutableReference* api_executable_ref) {
  PackageReference* package_ref =
      static_cast<PackageReference*>(api_executable_ref);

  for (auto* driver_executable_ref : package_ref->AllExecutableReferences()) {
    const Buffer& buffer = driver_executable_ref->parameters();
    ASSIGN_OR_RETURN(MappedDeviceBuffer mapped_device_buffer,
                     DoMapBuffer(buffer, DmaDirection::kToDevice));

    const DeviceBuffer& device_buffer = mapped_device_buffer.device_buffer();
    VLOG(3) << StringPrintf("Mapped params : %p -> 0x%016llx, %zu bytes.",
                            buffer.ptr(), device_buffer.device_address(),
                            device_buffer.size_bytes());
    driver_executable_ref->SetMappedParameters(std::move(mapped_device_buffer));
  }
  return util::Status();  // OK.
}

util::StatusOr<const api::ExecutableReference*> Driver::RegisterExecutableFile(
    const std::string& executable_filename) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

  ASSIGN_OR_RETURN(const api::ExecutableReference* executable_ref,
                   executable_registry_->RegisterFile(executable_filename));
  // Sets the mapped parameter. As of now executable registry is agnostic
  // of address spaces, so we have to add that after creating the executable
  // reference.
  RETURN_IF_ERROR(
      MapParameters(const_cast<api::ExecutableReference*>(executable_ref)));
  return executable_ref;
}

util::StatusOr<const api::ExecutableReference*>
Driver::RegisterExecutableSerialized(const std::string& executable_content) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

  ASSIGN_OR_RETURN(
      const api::ExecutableReference* executable_ref,
      executable_registry_->RegisterSerialized(executable_content));
  // Sets the mapped parameter. As of now executable registry is agnostic
  // of address spaces, so we have to add that after creating the executable
  // reference.
  RETURN_IF_ERROR(
      MapParameters(const_cast<api::ExecutableReference*>(executable_ref)));
  return executable_ref;
}

util::StatusOr<const api::ExecutableReference*>
Driver::RegisterExecutableSerialized(const char* executable_content,
                                     size_t length) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

  ASSIGN_OR_RETURN(
      const api::ExecutableReference* executable_ref,
      executable_registry_->RegisterSerialized(executable_content, length));
  // Sets the mapped parameter. As of now executable registry is agnostic
  // of address spaces, so we have to add that after creating the executable
  // reference.
  RETURN_IF_ERROR(
      MapParameters(const_cast<api::ExecutableReference*>(executable_ref)));
  return executable_ref;
}

Buffer Driver::MakeBuffer(size_t size_bytes) const {
  StdMutexLock state_lock(&state_mutex_);
  if (!ValidateState(/*expected_state=*/kOpen).ok()) {
    LOG(FATAL) << "Cannot allocate buffers when driver is not open.";
    return Buffer();
  }
  return DoMakeBuffer(size_bytes);
}

util::Status Driver::UnregisterExecutable(
    const api::ExecutableReference* executable_ref) {
  StdMutexLock state_lock(&state_mutex_);
  // TODO (b/112311598): should defer unregistering if there are pending
  // requests.
  return executable_registry_->Unregister(executable_ref);
}

util::StatusOr<std::shared_ptr<api::Request>> Driver::CreateRequest(
    const api::ExecutableReference* api_executable_ref) {
  const auto* package_ref =
      static_cast<const PackageReference*>(api_executable_ref);

  return CreateSingleRequest(package_ref->MainExecutableReference());
}

util::StatusOr<std::shared_ptr<api::Request>> Driver::CreateSingleRequest(
    const ExecutableReference* executable_ref) {
  StdMutexLock state_lock(&state_mutex_);

  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));
  return DoCreateRequest(executable_ref);
}

util::Status Driver::Submit(std::shared_ptr<api::Request> api_request,
                            api::Request::Done done_callback) {
  StdMutexLock top_level_submit_lock(&top_level_submit_mutex_);
  const auto* main_ref = DoGetExecutableReference(api_request);

  if (main_ref->ParameterCachingToken() == 0 ||
      main_ref->ParameterCachingToken() != current_parameter_caching_token_) {
    ResetCachedParameters();
  }

  const auto* parameter_caching_ref =
      executable_registry_->GetParameterCachingExecutableReference(main_ref);
  if (parameter_caching_ref != nullptr) {
    RETURN_IF_ERROR(
        CheckAndSubmitParameterCachingRequest(parameter_caching_ref));
  }

  return SubmitSingleRequest(std::move(api_request), std::move(done_callback));
}

util::Status Driver::SubmitSingleRequest(
    std::shared_ptr<api::Request> api_request,
    api::Request::Done done_callback) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

  StdMutexLock submit_lock(&submit_mutex_);
  return DoSubmit(std::move(api_request), std::move(done_callback));
}

util::Status Driver::CheckAndSubmitParameterCachingRequest(
    const ExecutableReference* parameter_caching_ref) {
  if (parameter_caching_ref->ParameterCachingToken() == 0) {
    return util::InternalError("Parameter caching tag is not set.");
  }

  if (currently_cached_refs_.find(parameter_caching_ref) !=
      currently_cached_refs_.end()) {
    return util::Status();  // OK
  }

  current_parameter_caching_token_ =
      parameter_caching_ref->ParameterCachingToken();
  currently_cached_refs_.insert(parameter_caching_ref);

  ASSIGN_OR_RETURN(auto request, CreateSingleRequest(parameter_caching_ref));
  return SubmitSingleRequest(std::move(request), [](int, util::Status) {});
}

void Driver::ResetCachedParameters() {
  current_parameter_caching_token_ = 0;
  currently_cached_refs_.clear();
}

util::Status Driver::Execute(std::shared_ptr<api::Request> request) {
  BlockingCounter counter(1);
  util::Status final_status;

  {
    StdMutexLock state_lock(&state_mutex_);
    RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

    auto done_callback = [&counter, &final_status](int id,
                                                   util::Status status) {
      final_status = std::move(status);
      counter.DecrementCount();
    };

    // Submit asynchronously and wait.
    {
      StdMutexLock submit_lock(&submit_mutex_);
      RETURN_IF_ERROR(DoSubmit(std::move(request), done_callback));
    }
  }

  counter.Wait();

  return final_status;
}

util::Status Driver::Execute(
    const std::vector<std::shared_ptr<api::Request>>& requests) {
  BlockingCounter counter(requests.size());
  std::mutex status_mutex;
  util::Status final_status;

  {
    StdMutexLock state_lock(&state_mutex_);
    RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

    auto done_callback = [&counter, &final_status, &status_mutex](
                             int id, util::Status status) {
      StdMutexLock status_lock(&status_mutex);
      final_status.Update(status);
      counter.DecrementCount();
    };

    // Submit asynchronously and wait.
    {
      StdMutexLock submit_lock(&submit_mutex_);
      for (auto request : requests) {
        RETURN_IF_ERROR(DoSubmit(std::move(request), done_callback));
      }
    }
  }

  counter.Wait();
  return final_status;
}

util::Status Driver::Cancel(std::shared_ptr<api::Request> request) {
  return util::UnimplementedError("Unimplemented.");
}

util::Status Driver::CancelAllRequests() {
  return util::UnimplementedError("Unimplemented.");
}

util::Status Driver::Close() {
  StdMutexLock state_lock(&state_mutex_);
  if (num_clients_ > 1) {
    num_clients_--;
    return util::Status();  // OK
  }
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

  // Note our intention to close.
  RETURN_IF_ERROR(SetState(kClosing));

  // Unregisters all currently registered models.
  // Note this has to be done before address spaces get destructed in DoClose.
  executable_registry_->UnregisterAll();

  // Actually close.
  RETURN_IF_ERROR(DoClose(in_error_));

  num_clients_--;
  return SetState(kClosed);
}

void Driver::SetFatalErrorCallback(FatalErrorCallback callback) {
  fatal_error_callback_ = std::move(callback);
}

void Driver::SetThermalWarningCallback(ThermalWarningCallback callback) {
  thermal_warning_callback_ = std::move(callback);
}

void Driver::NotifyFatalError(const util::Status& status) {
  // Set error state.
  bool was_in_error = std::atomic_exchange(&in_error_, true);
  if (!was_in_error) {
    // Notify Error only the first time the fatal error is triggered.
    // TODO(b/70535438): Issue this is in a new detached thread to decouple
    // itself from other driver contexts.
    if (fatal_error_callback_) {
      fatal_error_callback_(status);
    }
  }
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
