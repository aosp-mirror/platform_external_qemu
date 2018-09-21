#include "third_party/darwinn/driver/memory/nop_address_space.h"

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::StatusOr<DeviceBuffer> NopAddressSpace::MapMemory(
    const Buffer& buffer, DmaDirection direction,
    MappingTypeHint mapping_type) {
  if (!buffer.IsValid()) {
    return util::InvalidArgumentError("Invalid buffer.");
  }

  return DeviceBuffer(reinterpret_cast<uint64>(buffer.ptr()),
                      buffer.size_bytes());
}

util::StatusOr<const Buffer> NopAddressSpace::Translate(
    const DeviceBuffer& buffer) const {
  return Buffer(reinterpret_cast<uint8*>(buffer.device_address()),
                buffer.size_bytes());
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
