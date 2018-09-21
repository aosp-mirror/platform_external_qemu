#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_MMU_MAPPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_MMU_MAPPER_H_

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Abstract class for mapping memory on device MMU.
class MmuMapper {
 public:
  virtual ~MmuMapper() = default;

  // Opens / Closes the MMU interface.
  // - Reserve |num_simple_page_table_entries_requested| page table entries for
  // simple indexing. Remaining entries will be used for extended addressing.
  virtual util::Status Open(int num_simple_page_table_entries_requested) = 0;
  virtual util::Status Close() = 0;

  // Maps |num_pages| from the memory backing |buffer| to
  // |device_virtual_address|.
  util::Status Map(const Buffer &buffer, uint64 device_virtual_address) {
    return Map(buffer, device_virtual_address, DmaDirection::kBidirectional);
  }

  // Same as above but with a hint indicating the buffer transfer direction.
  util::Status Map(const Buffer &buffer, uint64 device_virtual_address,
                   DmaDirection direction);

  // Unmaps previously mapped Buffer.
  util::Status Unmap(const Buffer &buffer, uint64 device_virtual_address);

  // Translates device address to host virtual address. This function is
  // typically not implemented and will return an UNIMPLEMENTED Status. It is
  // only useful when MMU needs to be modeled directly (as is the case when
  // using IpCore without the HIB, or abrolhos with no MMU).
  //
  // Note that the device address here is the address that is output by the
  // hardware, which may be physical or virtual, depending if an MMU is present
  // or not.
  virtual util::StatusOr<void *> TranslateDeviceAddress(
      uint64 device_address) const {
    return util::Status(util::error::UNIMPLEMENTED, "Translate not supported.");
  }

  // Determines if a virtual address (obtained from TranslateDeviceAddress)
  // points into the extended page tables of this MMU. Generally this is false,
  // except in certain simulations where the MMU is modeled directly.
  virtual bool IsExtendedPageTable(const void *address) const {
    return false;
  }

 protected:
  // Maps |num_pages| from |buffer| (the host virtual address) to
  // |device_virtual_address|. All addresses must be page aligned.
  // Called by public version of Map when the buffer is backed by host memory.
  virtual util::Status DoMap(const void *buffer, int num_pages,
                             uint64 device_virtual_address,
                             DmaDirection direction, bool on_chip_cached) = 0;

  // Maps file descriptor to |device_virtual_address|. Default = unimplemented.
  virtual util::Status DoMap(int fd, int num_pages,
                             uint64 device_virtual_address,
                             DmaDirection direction) {
    return util::Status(util::error::UNIMPLEMENTED,
                        "File descriptor-backed mapping not supported.");
  }

  // Unmaps previously mapped addresses.
  // Called by public version of Unmap when the buffer is backed by host memory.
  virtual util::Status DoUnmap(const void *buffer, int num_pages,
                               uint64 device_virtual_address) = 0;

  // Unmaps previously mapped file descriptor based buffer. Default =
  // unimplemented.

  virtual util::Status DoUnmap(int fd, int num_pages,
                               uint64 device_virtual_address) {
    return util::Status(util::error::UNIMPLEMENTED,
                        "File descriptor-backed unmapping not supported.");
  }
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_MMU_MAPPER_H_
