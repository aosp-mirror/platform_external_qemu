#ifndef THIRD_PARTY_DARWINN_API_REQUEST_H_
#define THIRD_PARTY_DARWINN_API_REQUEST_H_

#include <functional>
#include <memory>
#include <string>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/status_macros.h"

namespace platforms {
namespace darwinn {
namespace api {

// Compute request. Thread-unsafe.
class Request {
 public:
  // A type for request completion callback.
  // The int argument is the same as return value of id().
  using Done = std::function<void(int, util::Status)>;

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
};

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_REQUEST_H_
