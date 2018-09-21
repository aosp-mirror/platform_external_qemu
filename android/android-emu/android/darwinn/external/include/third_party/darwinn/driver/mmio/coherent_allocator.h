#ifndef THIRD_PARTY_DARWINN_DRIVER_MMIO_COHERENT_ALLOCATOR_H_
#define THIRD_PARTY_DARWINN_DRIVER_MMIO_COHERENT_ALLOCATOR_H_

#include <stddef.h>
#include <mutex>  // NOLINT

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Manage Device Specific DMA-able Coherent memory.
class CoherentAllocator {
 public:
  CoherentAllocator();
  CoherentAllocator(int alignment_bytes, size_t size_bytes);
  virtual ~CoherentAllocator() = default;

  // Opens coherent allocator.
  util::Status Open() LOCKS_EXCLUDED(mutex_);

  // Closes coherent allocator.
  util::Status Close() LOCKS_EXCLUDED(mutex_);

  // Returns a chunk of coherent memory.
  util::StatusOr<Buffer> Allocate(size_t size_bytes) LOCKS_EXCLUDED(mutex_);

 protected:
  // Implements Open.
  virtual util::StatusOr<char *> DoOpen(size_t size_bytes);

  // Implements close.
  virtual util::Status DoClose(char *mem_base, size_t size_bytes);

 private:
  // Alignment bytes for host memory.
  const int alignment_bytes_;

  // User-space virtual address of memory block.
  char *coherent_memory_base_{nullptr};

  // Total size of coherent memory region.
  const size_t total_size_bytes_;

  // Coherent Bytes allocated so far.
  size_t allocated_bytes_ GUARDED_BY(mutex_){0};

  // Guards all APIs functions Open/Close/Allocate.
  mutable std::mutex mutex_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MMIO_COHERENT_ALLOCATOR_H_
