#ifndef THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_COHERENT_ALLOCATOR_H_
#define THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_COHERENT_ALLOCATOR_H_

#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#include "third_party/darwinn/driver/mmio/coherent_allocator.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/stringprintf.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Functions to allocate coherent memory that is DMA-able by a Darwinn device.
class KernelCoherentAllocator : public CoherentAllocator {
 public:
  KernelCoherentAllocator(const std::string &device_path, int alignment_bytes,
                          size_t size_bytes);
  ~KernelCoherentAllocator() = default;

 private:
  // Implements Open.
  util::StatusOr<char *> DoOpen(size_t size_bytes) override;

  // Implements close.
  util::Status DoClose(char *mem_base, size_t size_bytes) override;

  // File descriptor of the opened device.
  int fd_{-1};

  // Device specific DMA address of the coherent memory block.
  uint64 dma_address_{0};

  // Device path.
  const std::string device_path_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_COHERENT_ALLOCATOR_H_
