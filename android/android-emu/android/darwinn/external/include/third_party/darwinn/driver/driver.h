#ifndef THIRD_PARTY_DARWINN_DRIVER_DRIVER_H_
#define THIRD_PARTY_DARWINN_DRIVER_DRIVER_H_

#include <atomic>
#include <memory>
#include <mutex>  // NOLINT
#include <unordered_set>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Base driver implementation.
class Driver : public api::Driver {
 public:
  ~Driver() override = default;

  bool IsOpen() const override;

  bool IsError() const override;

  util::Status Open(bool debug_mode = false)
      LOCKS_EXCLUDED(state_mutex_) override;

  util::StatusOr<const api::PackageReference*> RegisterExecutableFile(
      const std::string& executable_filename)
      LOCKS_EXCLUDED(state_mutex_) override;

  util::StatusOr<const api::PackageReference*> RegisterExecutableSerialized(
      const std::string& executable_content) override;
  util::StatusOr<const api::PackageReference*> RegisterExecutableSerialized(
      const char* executable_content, size_t length) override;

  util::Status UnregisterExecutable(const api::PackageReference* executable_ref)
      LOCKS_EXCLUDED(state_mutex_) override;

  util::StatusOr<std::shared_ptr<api::Request>> CreateRequest(
      const api::PackageReference* executable_ref) override;

  // TODO(b/113105556) If we end up spliting driver::Driver to 2 layers, this
  // method can go up a layer.
  util::Status Submit(std::shared_ptr<api::Request> request,
                      api::Request::Done done_callback)
      LOCKS_EXCLUDED(top_level_submit_mutex_) override;

  util::Status Execute(std::shared_ptr<api::Request> request)
      LOCKS_EXCLUDED(state_mutex_, submit_mutex_,
                     top_level_submit_mutex_) override;

  util::Status Execute(
      const std::vector<std::shared_ptr<api::Request>>& requests)
      LOCKS_EXCLUDED(state_mutex_, submit_mutex_,
                     top_level_submit_mutex_) override;

  util::Status Cancel(std::shared_ptr<api::Request> request)
      LOCKS_EXCLUDED(state_mutex_) override;

  util::Status CancelAllRequests() LOCKS_EXCLUDED(state_mutex_) override;

  util::Status Close() LOCKS_EXCLUDED(state_mutex_) override;

  void SetFatalErrorCallback(FatalErrorCallback callback) override;

  void SetThermalWarningCallback(ThermalWarningCallback callback) override;

  Buffer MakeBuffer(size_t size_bytes) const override;

 protected:
  Driver(api::Chip chip, std::unique_ptr<PackageRegistry> executable_registry);

  // The base driver implementation does the necessary state checks and
  // validations before issuing the following calls that are implemented by the
  // derived class.

  virtual util::Status DoOpen(bool debug_mode)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  virtual util::Status DoClose(bool in_error)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  // Cancels pending requests and waits for active requests to finish.
  virtual util::Status DoCancelAndWaitRequests(bool in_error)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  virtual util::StatusOr<MappedDeviceBuffer> DoMapBuffer(const Buffer& buffer,
                                                         DmaDirection direction)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  virtual util::StatusOr<std::shared_ptr<api::Request>> DoCreateRequest(
      const ExecutableReference* executable)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  virtual util::Status DoSubmit(std::shared_ptr<api::Request> request,
                                api::Request::Done request_done)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  virtual Buffer DoMakeBuffer(size_t size_bytes) const
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_) = 0;

  // Gets the executable reference for a provided request based on the driver
  // implementation.
  //
  // TODO(b/113078031) This method should go away once we have another layer of
  // abstraction for Request that top level driver can handle. Right now it's
  // only handling api::Request so it does not have any visibility into request
  // internals like executable reference.
  virtual const ExecutableReference* DoGetExecutableReference(
      std::shared_ptr<api::Request> api_request) = 0;

  // Notifies that the driver / device has entered an error state.
  void NotifyFatalError(const util::Status& status);

 private:
  // Driver state. Transitions :
  //  kClosed -> kOpen -> kClosing -> kClosed.
  enum State {
    kOpen,     // Driver is Open.
    kClosing,  // Driver is Closing.
    kClosed,   // Driver is Closed. (Initial state.)
  };

  // Attempts a state transition to the given state.
  util::Status SetState(State next_state)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_);

  // Validates that we are in the expected state.
  util::Status ValidateState(State expected_state) const
      SHARED_LOCKS_REQUIRED(state_mutex_);

  // Internal helper for mapping parameters.
  util::Status MapParameters(api::PackageReference* api_executable_ref)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_);

  // Creates a single request based on a provided driver::ExecutableReference (
  // as opposed to CreateRequest that may end up in a preceding request for
  // parameter caching).
  util::StatusOr<std::shared_ptr<api::Request>> CreateSingleRequest(
      const ExecutableReference* executable_ref) LOCKS_EXCLUDED(state_mutex_);

  // Submits a single request (as opposed to Submit that may end up submiting a
  // parameter-caching request as well).
  util::Status SubmitSingleRequest(std::shared_ptr<api::Request> request,
                                   api::Request::Done done_callback)
      LOCKS_EXCLUDED(state_mutex_, submit_mutex_)
          EXCLUSIVE_LOCKS_REQUIRED(top_level_submit_mutex_);

  // Reset the state of cached parameters. This does not do anything to TPU
  // memory, only invalidates the cache state in driver which then results in
  // reloading parameters if needed.
  void ResetCachedParameters()
      EXCLUSIVE_LOCKS_REQUIRED(top_level_submit_mutex_);

  // Checks if we need to load to-be-cached parameters to the TPU. If so, it
  // will submit the proper request for it.
  util::Status CheckAndSubmitParameterCachingRequest(
      const ExecutableReference* parameter_caching_ref)
      EXCLUSIVE_LOCKS_REQUIRED(top_level_submit_mutex_);

  // Internal helper to submit a batch of inputs that is not equal to the
  // hardware batch size.
  util::Status SubmitBatches(int request_batches,
                             const std::shared_ptr<api::Request>& request,
                             api::Request::Done done_callback)
      EXCLUSIVE_LOCKS_REQUIRED(top_level_submit_mutex_, state_mutex_,
                               submit_mutex_);

  // Maintains integrity of the driver state.
  mutable std::mutex state_mutex_;

  // Guarantees that multiple requests will be submitted in the order provided.
  mutable std::mutex submit_mutex_;

  // Guarantees that top level operations (checking and submitting parameter-
  // caching requests) are processed in order.
  mutable std::mutex top_level_submit_mutex_;

  // Counts the number of clients that opened this driver.
  int num_clients_ GUARDED_BY(state_mutex_){0};

  // Driver state.
  State state_ GUARDED_BY(state_mutex_){kClosed};

  // Chip that this driver controls.
  const api::Chip chip_;

  // Executable registry. Null, when device is in closed state.
  std::unique_ptr<PackageRegistry> executable_registry_;

  // Registered fatal Error Callback.
  FatalErrorCallback fatal_error_callback_;

  // Registered thermal warning Callback.
  ThermalWarningCallback thermal_warning_callback_;

  // True, if device is in error state.
  std::atomic<bool> in_error_{false};

  // The currently active parameter-caching token. This token determines if a
  // new submission will require reloading cached parameters in TPU SRAM.
  uint64 current_parameter_caching_token_ GUARDED_BY(top_level_submit_mutex_);

  // A set of parameter-caching ExecutableReferences that shows if that model
  // has already cached its parameters on TPU SRAM, and the cache is still
  // valid.
  std::unordered_set<const ExecutableReference*> currently_cached_refs_
      GUARDED_BY(top_level_submit_mutex_);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_DRIVER_H_
