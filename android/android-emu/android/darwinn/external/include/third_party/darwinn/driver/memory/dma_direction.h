#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_DMA_DIRECTION_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_DMA_DIRECTION_H_

namespace platforms {
namespace darwinn {
namespace driver {

// Mimics the Linux kernel dma_data_direction enum in the DMA API.
// This indicates the direction which data moves during a DMA transfer, and is a
// useful hint to pass to the kernel when mapping buffers.
enum class DmaDirection {
  // DMA_TO_DEVICE: CPU caches are flushed at mapping time.
  kToDevice = 1,
  // DMA_FROM_DEVICE: CPU caches are invalidated at unmapping time.
  kFromDevice = 2,
  // DMA_BIDIRECTIONAL: Both of the above.
  kBidirectional = 0,
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_DMA_DIRECTION_H_
