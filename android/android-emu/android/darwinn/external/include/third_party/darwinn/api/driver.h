#ifndef THIRD_PARTY_DARWINN_API_DRIVER_H_
#define THIRD_PARTY_DARWINN_API_DRIVER_H_

#include <stddef.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace api {

// DarwiNN driver. Thread-safe, but not all functions can be called in
// callback context.
//
// Typical usage:
// Driver driver = driverFactory.get();
//
// m1 = driver.RegisterExecutable(<path to executable file>)
// m2 = driver.RegisterExecutable(<path to executable file>)
//
// driver.Open();
// r1 = driver.CreateRequest(m1, done_callback);
// r2 = driver.CreateRequest(m1, done_callback);
// driver.Submit(r1);
// driver.Submit(r2).
// driver.Close();
//...
// After some time, application can try to re-open the driver again.
// driver.Open();
// ...
// driver.Close();
class Driver {
 public:
  // Callback for thermal warnings. Set with SetThermalWarningCallback().
  using ThermalWarningCallback = std::function<void()>;

  // Callback for fatal, unrecoverable failure. Set with
  // SetFatalErrorCallback().
  using FatalErrorCallback = std::function<void(const util::Status&)>;

  // Driver options. Opaque pointer to an options::Options FB object.
  using Options = std::vector<uint8_t>;

  // Current driver option version. Should match the version in
  // driver_options.fbs.
  static constexpr int kOptionsVersion = 1;

  Driver() = default;
  virtual ~Driver() = default;

  // This class is neither copyable nor movable.
  Driver(const Driver&) = delete;
  Driver& operator=(const Driver&) = delete;

  // Returns true if the driver is open state.
  virtual bool IsOpen() const = 0;

  // Returns true if underlying hardware is in an error state.
  virtual bool IsError() const = 0;

  // Registers a file containing pre-compiled DarwiNN executable and returns a
  // reference to the registered executable. The reference can be used for
  // constructing requests later on.
  // TODO(ijsung): As it exists today, it is possible to register an executable
  // when driver is either open or closed. In the future registration would only
  // be possible when the driver is in open state.
  virtual util::StatusOr<const PackageReference*> RegisterExecutableFile(
      const std::string& executable_filename) = 0;

  // Registers a string with serialized contents of a pre-compiled DarwiNN
  // executable and returns a reference to the registered executable. The
  // reference can be used for constructing requests later on.
  // TODO(ijsung): As it exists today, it is possible to register an executable
  // when driver is either open or closed. In the future registration would only
  // be possible when the driver is in open state.
  virtual util::StatusOr<const PackageReference*> RegisterExecutableSerialized(
      const std::string& executable_content) = 0;
  virtual util::StatusOr<const PackageReference*> RegisterExecutableSerialized(
      const char* executable_content, size_t length) = 0;

  // Unregisters a previously registered model.
  // TODO(b/80200253): This is not supported yet.
  virtual util::Status UnregisterExecutable(
      const PackageReference* executable_ref) = 0;

  // Opens and initializes the underlying hardware. If debug_mode is true,
  // the hardware is setup for use with a debugger.
  virtual util::Status Open(bool debug_mode = false) = 0;

  // Creates a request object initialized with the given ExecutableReference.
  virtual util::StatusOr<std::shared_ptr<Request>> CreateRequest(
      const PackageReference* executable_ref) = 0;

  // Submits a request for asynchronous execution. On success, done_callback
  // will eventually be executed with the request status. The caller is expected
  // to exit the done_callback as soon as possible. It is acceptable to only
  // call #Submit() in the context of this callback.
  virtual util::Status Submit(std::shared_ptr<Request> request,
                              Request::Done done_callback) = 0;

  // Executes a request synchronously. Calling thread will block until execution
  // is complete.
  virtual util::Status Execute(std::shared_ptr<Request> request) = 0;

  // Executes a series of requests synchronously in the given order. Calling
  // thread will block until execution is complete.
  virtual util::Status Execute(
      const std::vector<std::shared_ptr<Request>>& request) = 0;

  // Attempts to cancel a request. This is best effort cancellation. As in,
  // requests already submitted to the hardware will be allowed to complete.
  // Other requests will be cancelled, and will invoke done_callback with
  // cancelled error.
  virtual util::Status Cancel(std::shared_ptr<Request> request) = 0;

  // Best effort cancellation of all submitted requests.
  virtual util::Status CancelAllRequests() = 0;

  // Closes and shutdowns underlying hardware possibly, switching it off.
  // Pending requests are cancelled or completed and callbacks issued. Once
  // closed, requests can no longer be submitted.
  virtual util::Status Close() = 0;

  // Allocates size_bytes bytes and returns a Buffer for application use. The
  // allocated memory is tied to the lifecycle of the Buffer object which in
  // turn is tied to the life cycle of the driver instance.
  virtual Buffer MakeBuffer(size_t size_bytes) const = 0;

  // Sets the callback for fatal, unrecoverable failure. When a fatal
  // error is raised, the driver is pushed into an error state. All new
  // submitted requests will fail. Application can generate a bug report and
  // should close the driver, at which point all pending requests will fail and
  // their callbacks executed.
  virtual void SetFatalErrorCallback(FatalErrorCallback callback) = 0;

  // Sets the callback for thermal warnings. Application may be required to
  // to reduce performance level and/or throttle new requests.
  virtual void SetThermalWarningCallback(ThermalWarningCallback callback) = 0;

  // TODO(b/70535438): Add function for dumping bugreport.
};

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_DRIVER_H_
