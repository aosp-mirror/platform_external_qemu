#include "third_party/darwinn/driver/aligned_allocator.h"

#include <functional>
#include <memory>

#include "third_party/darwinn/port/aligned_malloc.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {

AlignedAllocator::AlignedAllocator(uint64 alignment_bytes)
    : alignment_bytes_(alignment_bytes) {
  // Check for power of 2, since we use arithmetic that relies on it elsewhere.
  CHECK_EQ((alignment_bytes - 1) & alignment_bytes, 0);
}

void* AlignedAllocator::Allocate(size_t size) {
  int aligned_size = (size + alignment_bytes_ - 1) & ~(alignment_bytes_ - 1);
  return aligned_malloc(aligned_size, alignment_bytes_);
}

void AlignedAllocator::Free(void* aligned_memory) {
  aligned_free(aligned_memory);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
