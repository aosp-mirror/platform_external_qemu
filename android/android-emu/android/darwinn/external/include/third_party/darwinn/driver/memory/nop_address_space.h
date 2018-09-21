#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_NOP_ADDRESS_SPACE_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_NOP_ADDRESS_SPACE_H_

#include <stddef.h>
#include <memory>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// No-op address space implementation. MapMemory and UnmapMemory is no-op. Host
// address equals to device virtual address.
class NopAddressSpace : public AddressSpace {
 public:
  using AddressSpace::MapMemory;  // Allows for proper overload resolution.

  NopAddressSpace() = default;

  // This class is neither copyable nor movable.
  NopAddressSpace(const NopAddressSpace&) = delete;
  NopAddressSpace& operator=(const NopAddressSpace&) = delete;

  virtual ~NopAddressSpace() = default;

  // Maps the given host buffer to the device buffer. Returns the mapped device
  // buffer on success.
  util::StatusOr<DeviceBuffer> MapMemory(const Buffer& buffer,
                                         DmaDirection direction,
                                         MappingTypeHint mapping_type) override;

  // Unmaps the given device address range.
  util::Status UnmapMemory(DeviceBuffer buffer) override {
    return util::Status();  // OK
  }

  // Translates device buffer to host buffer.
  util::StatusOr<const Buffer> Translate(
      const DeviceBuffer& buffer) const override;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_NOP_ADDRESS_SPACE_H_
