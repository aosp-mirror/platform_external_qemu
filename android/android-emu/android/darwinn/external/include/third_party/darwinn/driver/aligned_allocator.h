#ifndef THIRD_PARTY_DARWINN_DRIVER_ALIGNED_ALLOCATOR_H_
#define THIRD_PARTY_DARWINN_DRIVER_ALIGNED_ALLOCATOR_H_

#include <memory>

#include "third_party/darwinn/driver/allocator.h"
#include "third_party/darwinn/port/integral_types.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Convenience class to allocate aligned buffers.
class AlignedAllocator : public Allocator {
 public:
  // All allocated buffers will be aligned to |alignment_bytes| with a size
  // granulairy of |alignment_bytes|.
  explicit AlignedAllocator(uint64 alignment_bytes);
  ~AlignedAllocator() = default;

  // This class is neither copyable nor movable.
  AlignedAllocator(const AlignedAllocator&) = delete;
  AlignedAllocator& operator=(const AlignedAllocator&) = delete;

  void* Allocate(size_t size) override;
  void Free(void* aligned_memory) override;

 private:
  // Alignment
  const uint64 alignment_bytes_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_ALIGNED_ALLOCATOR_H_
