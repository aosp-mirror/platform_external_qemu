#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_BUDDY_ADDRESS_SPACE_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_BUDDY_ADDRESS_SPACE_H_

#include <stddef.h>
#include <mutex>  // NOLINT
#include <set>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/memory/mmio_address_space.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// A buddy memory allocator for DarwiNN virtual address space segment.
// https://en.wikipedia.org/wiki/Buddy_memory_allocation
class BuddyAddressSpace final : public MmioAddressSpace {
 public:
  using AddressSpace::MapMemory;  // Allows for proper overload resolution.

  BuddyAddressSpace(uint64 device_virtual_address_start,
                    uint64 device_virtual_address_size_bytes,
                    MmuMapper* mmu_mapper);

  // This class is neither copyable nor movable.
  BuddyAddressSpace(const BuddyAddressSpace&) = delete;
  BuddyAddressSpace& operator=(const BuddyAddressSpace&) = delete;

  ~BuddyAddressSpace() override = default;

  // Maps the given host buffer to the device buffer. Returns the mapped device
  // buffer on success.
  util::StatusOr<DeviceBuffer> MapMemory(const Buffer& buffer,
                                         DmaDirection direction,
                                         MappingTypeHint mapping_type) override
      LOCKS_EXCLUDED(mutex_);

  // Unmaps the given device buffer.
  util::Status UnmapMemory(DeviceBuffer buffer) override LOCKS_EXCLUDED(mutex_);

  // Returns # of free blocks at a certain order.
  // TODO(ijsung): at this moment this method is only used by testcases to
  // inspect some internal status of the allocator. Need a better way to verify
  // the correctness. Maybe a consistency checker that's turned on when in debug
  // mode?
  size_t NumFreeBlocksByOrder(int order) const LOCKS_EXCLUDED(mutex_);

 private:
  mutable std::mutex mutex_;

  // Sets of buddy blocks. blocks_[0] are kHostPageSize'd blocks.
  std::vector<std::set<uint64>> blocks_ GUARDED_BY(mutex_);

  // Tracks allocated blocks.
  std::vector<std::set<uint64>> allocated_ GUARDED_BY(mutex_);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_BUDDY_ADDRESS_SPACE_H_
