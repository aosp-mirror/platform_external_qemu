#ifndef THIRD_PARTY_DARWINN_DRIVER_ALLOCATOR_H_
#define THIRD_PARTY_DARWINN_DRIVER_ALLOCATOR_H_

#include <stddef.h>

#include "third_party/darwinn/api/buffer.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Interface for a class that can allocate host memory.
class Allocator {
 public:
  virtual ~Allocator() = default;

  // Allocates buffer of specified size.
  virtual void* Allocate(size_t size) = 0;

  // Frees a previous allocated buffer.
  virtual void Free(void* buffer) = 0;

  // Allocates and returns a buffer of the specified size. The lifecycle of the
  // the returned buffer is tied to the Allocator instance. It is thus important
  // to ensure that the allocator class outlives the returned buffer instances.
  Buffer MakeBuffer(size_t size_bytes);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_ALLOCATOR_H_
