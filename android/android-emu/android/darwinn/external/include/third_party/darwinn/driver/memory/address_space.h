#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_ADDRESS_SPACE_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_ADDRESS_SPACE_H_

#include <stddef.h>
#include <memory>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// A hint that the implementation should use a particular type of address
// space mapping, for systems that have multiple mapping types.
enum class MappingTypeHint {
  // No preference. Most mappings should be of this type.
  kAny,

  // Use simple address space mappings, if the hardware is capable.
  kSimple,

  // Use extended address space mappings, if the hardware is capable.
  kExtended,
};

// An interface for managing a DarwiNN virtual address space segment.
class AddressSpace {
 public:
  AddressSpace() = default;

  // This class is neither copyable nor movable.
  AddressSpace(const AddressSpace&) = delete;
  AddressSpace& operator=(const AddressSpace&) = delete;

  virtual ~AddressSpace() = default;

  // Maps the buffer to the device buffer. Returns the mapped device
  // buffer on success.
  util::StatusOr<DeviceBuffer> MapMemory(const Buffer& buffer) {
    return MapMemory(buffer, DmaDirection::kBidirectional,
                     MappingTypeHint::kAny);
  }

  // Same as above but with a hint indicating the buffer transfer direction and
  // a hint indicating whether to use simple or extended mappings.
  virtual util::StatusOr<DeviceBuffer> MapMemory(
      const Buffer& buffer, DmaDirection direction,
      MappingTypeHint mapping_type) = 0;

  // Unmaps the given device buffer.
  virtual util::Status UnmapMemory(DeviceBuffer buffer) = 0;

  // Translates device buffer to corresponding buffer.
  // Existing usecase is USB, where DeviceBuffer always have corresponding host
  // buffer.
  virtual util::StatusOr<const Buffer> Translate(
      const DeviceBuffer& buffer) const = 0;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_ADDRESS_SPACE_H_
