#ifndef THIRD_PARTY_DARWINN_DRIVER_DRIVER_HELPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_DRIVER_HELPER_H_

#include <unistd.h>

#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/executable_util.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/driver/test_vector.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Wraps a driver instance with additional functions that performs tests and
// verify results.
class DriverHelper : public api::Driver {
 public:
  DriverHelper(std::unique_ptr<api::Driver> driver, int max_pending_requests);

  // Destructor. Waits for pending tasks to avoid Submit callbacks
  // acquiring otherwise-destructed mutex_. See b/111616778.
  ~DriverHelper() override {
    if (IsOpen()) CHECK_OK(Close());
  }

  util::Status Open(bool debug_mode) final LOCKS_EXCLUDED(mutex_);

  util::Status Close() final LOCKS_EXCLUDED(mutex_);

  bool IsOpen() const final LOCKS_EXCLUDED(mutex_);

  bool IsError() const final;

  util::StatusOr<const api::PackageReference*> RegisterExecutableFile(
      const std::string& executable_filename) final;

  util::StatusOr<const api::PackageReference*> RegisterExecutableSerialized(
      const std::string& executable_content) final;
  util::StatusOr<const api::PackageReference*> RegisterExecutableSerialized(
      const char* executable_content, size_t length) final;

  util::Status UnregisterExecutable(
      const api::PackageReference* executable_ref) final;

  util::StatusOr<std::shared_ptr<api::Request>> CreateRequest(
      const api::PackageReference* executable_ref) final;

  util::Status Submit(std::shared_ptr<api::Request> request,
                      api::Request::Done done_callback) final;

  util::Status Execute(std::shared_ptr<api::Request> request) final;

  util::Status Execute(
      const std::vector<std::shared_ptr<api::Request>>& requests) final;

  util::Status Cancel(std::shared_ptr<api::Request> request) final;

  util::Status CancelAllRequests() final;

  Buffer MakeBuffer(size_t size_bytes) const final;

  void SetFatalErrorCallback(FatalErrorCallback callback) final;

  void SetThermalWarningCallback(ThermalWarningCallback callback) final;

  // Extensions to the Device interface to facilitate easier testing.

  // Submits an inference request with given test vector.
  util::Status Submit(const TestVector& test_vector, int batches)
      LOCKS_EXCLUDED(mutex_);

  // Submits an inference request and execute the specified callback on
  // completion. |tag| is a user friendly name for tracking this request
  // (typically the model name).
  util::Status Submit(const std::string& tag,
                      const api::PackageReference* executable_ref,
                      const Buffer::NamedMap& input,
                      const Buffer::NamedMap& output,
                      api::Request::Done request_done) LOCKS_EXCLUDED(mutex_);

  // Submits an inference request and verify output.
  util::Status Submit(const std::string& tag,
                      const api::PackageReference* executable_ref,
                      const Buffer::NamedMap& input,
                      const Buffer::NamedMap& expected_output,
                      const Buffer::NamedMap& output) LOCKS_EXCLUDED(mutex_);

 private:
  // Wrapped driver instance.
  std::unique_ptr<api::Driver> driver_;

  // Maximum number of pending requests.
  const int max_pending_requests_{1};

  // Current number of pending requests.
  int pending_requests_ GUARDED_BY(mutex_){0};

  // Total number of requets processed so far.
  int total_requests_ GUARDED_BY(mutex_){0};

  // Condition variable to synchronously wait for pending requests.
  std::condition_variable cv_ GUARDED_BY(mutex_);

  // Guards pending_requests_, cv_ and other internal states.
  mutable std::mutex mutex_;

  // Time at which first submit was called.
  std::chrono::steady_clock::time_point first_submit_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_DRIVER_HELPER_H_
