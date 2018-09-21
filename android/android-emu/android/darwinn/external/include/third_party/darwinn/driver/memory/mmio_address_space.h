#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_MMIO_ADDRESS_SPACE_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_MMIO_ADDRESS_SPACE_H_

#include <stddef.h>
#include <map>
#include <mutex>  // NOLINT

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// A class to manage a DarwiNN virtual address space segment when mmio is used.
class MmioAddressSpace : public AddressSpace {
 public:
  MmioAddressSpace(uint64 device_virtual_address_start,
                   uint64 device_virtual_address_size_bytes,
                   MmuMapper* mmu_mapper)
      : AddressSpace(),
        device_virtual_address_start_(device_virtual_address_start),
        device_virtual_address_size_bytes_(device_virtual_address_size_bytes),
        mmu_mapper_(mmu_mapper) {
    CHECK(mmu_mapper != nullptr);
    CHECK(IsPageAligned(device_virtual_address_start));
    CHECK(IsPageAligned(device_virtual_address_size_bytes));
  }

  // This class is neither copyable nor movable.
  MmioAddressSpace(const MmioAddressSpace&) = delete;
  MmioAddressSpace& operator=(const MmioAddressSpace&) = delete;

  virtual ~MmioAddressSpace() = default;

  // Implements AddressSpace interface.
  util::StatusOr<const Buffer> Translate(
      const DeviceBuffer& buffer) const override;

 protected:
  // Maps the entire given Buffer, and stores the mapping information. Returns
  // an error if trying to map already mapped Buffer.
  util::Status Map(const Buffer& buffer, uint64 device_address,
                   DmaDirection direction) LOCKS_EXCLUDED(mutex_);

  // Checks and unmaps device virtual address. |device_address| must be
  // page-aligned.
  util::Status Unmap(uint64 device_address, int num_released_pages)
      LOCKS_EXCLUDED(mutex_);

  // Member accessors for inherited classes.
  uint64 device_virtual_address_start() const {
    return device_virtual_address_start_;
  }
  uint64 device_virtual_address_size_bytes() const {
    return device_virtual_address_size_bytes_;
  }

  // Returns last device virtual address.
  uint64 GetLastDeviceVirtualAddress() const {
    return device_virtual_address_start_ + device_virtual_address_size_bytes_;
  }

 private:
  // Device address space start.
  const uint64 device_virtual_address_start_;

  // Device address space size in bytes.
  const uint64 device_virtual_address_size_bytes_;

  // Underlying MMU mapper.
  MmuMapper* const mmu_mapper_ GUARDED_BY(mutex_);

  // Guards |mmu_mapper_| and |mapped_|.
  mutable std::mutex mutex_;

  // Tracks already mapped segments.
  // key - aligned device virtual address.
  // value - {host address, number of mapped pages}
  std::map<uint64, Buffer> mapped_ GUARDED_BY(mutex_);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_MMIO_ADDRESS_SPACE_H_
