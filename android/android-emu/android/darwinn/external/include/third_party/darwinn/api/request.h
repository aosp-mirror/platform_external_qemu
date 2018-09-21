#ifndef THIRD_PARTY_DARWINN_API_REQUEST_H_
#define THIRD_PARTY_DARWINN_API_REQUEST_H_

#include <functional>
#include <memory>
#include <string>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace api {

// Compute request. Thread-unsafe.
class Request {
 public:
  // A type for request completion callback.
  // The int argument is the same as return value of id().
  using Done = std::function<void(int, const util::Status&)>;

  Request() = default;
  virtual ~Request() = default;

  // This class is neither copyable nor movable.
  Request(const Request&) = delete;
  Request& operator=(const Request&) = delete;

  // Adds an input buffer. This may be called repeatedly depending
  // on the batch size as long as the request instance is not submitted. The
  // size constraints on the input and output buffers will be evaluated during
  // Device#Submit. Memory backing the buffer instance must be valid throughout
  // the life of the request.
  virtual util::Status AddInput(const std::string& name,
                                const Buffer& input) = 0;

  // Adds an output buffer. This may be called repeatedly depending
  // on the batch size as long as the request instance is not submitted. The
  // size constraints on the input and output buffers will be evaluated during
  // Device#Submit. Memory backing the buffer instance must be valid throughout
  // the life of the request.
  virtual util::Status AddOutput(const std::string& name, Buffer output) = 0;

  // Returns an ID to track the request.
  virtual int id() const = 0;

  // Returns the number of batches that have been added to this request. If not
  // all inputs and outputs have the same number of batches, this function
  // returns an error.
  // This method is intended for internal driver use only.
  // TODO(b/113078031): Move this method to the implementation class.
  virtual util::StatusOr<int> GetNumBatches() const = 0;

  // Returns the input Buffer with the given layer name and batch ID.
  // This method is intended for internal driver use only.
  // TODO(b/113078031): Move this method to the implementation class.
  virtual const Buffer& InputBuffer(const std::string& name,
                                    int batch) const = 0;

  // Returns the output Buffer with the given layer name and batch ID.
  // This method is intended for internal driver use only.
  // TODO(b/113078031): Move this method to the implementation class.
  virtual Buffer OutputBuffer(const std::string& name, int batch) const = 0;
};

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_REQUEST_H_
