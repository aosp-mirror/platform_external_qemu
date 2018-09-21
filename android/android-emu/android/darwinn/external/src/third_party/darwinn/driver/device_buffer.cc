#include "third_party/darwinn/driver/device_buffer.h"

#include <stddef.h>

#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {

void DeviceBuffer::Clear() {
  type_ = Type::kInvalid;
  size_bytes_ = 0;
  device_address_ = 0;
}

DeviceBuffer::DeviceBuffer(uint64 device_address, size_t size_bytes)
    : type_(Type::kDefault),
      size_bytes_(size_bytes),
      device_address_(device_address) {}

bool DeviceBuffer::operator==(const DeviceBuffer& rhs) const {
  return type_ == rhs.type_ && size_bytes_ == rhs.size_bytes_ &&
         device_address_ == rhs.device_address_;
}

bool DeviceBuffer::operator!=(const DeviceBuffer& rhs) const {
  return !(*this == rhs);
}

DeviceBuffer::DeviceBuffer(DeviceBuffer&& other)
    : type_(other.type_),
      size_bytes_(other.size_bytes_),
      device_address_(other.device_address_) {
  other.Clear();
}

DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& other) {
  if (this != &other) {
    type_ = other.type_;
    size_bytes_ = other.size_bytes_;
    device_address_ = other.device_address_;

    other.Clear();
  }
  return *this;
}

DeviceBuffer DeviceBuffer::Slice(uint64 byte_offset, size_t size_bytes) const {
  CHECK_LE(byte_offset + size_bytes, size_bytes_)
      << "Overflowed underlying DeviceBuffer";
  const uint64 new_device_address = device_address_ + byte_offset;
  return DeviceBuffer(new_device_address, size_bytes);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
