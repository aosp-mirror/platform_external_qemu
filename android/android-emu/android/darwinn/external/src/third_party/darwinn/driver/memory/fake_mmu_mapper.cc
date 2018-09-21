#include "third_party/darwinn/driver/memory/fake_mmu_mapper.h"

#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::Status FakeMmuMapper::DoMap(const void *buffer, int num_pages,
                                  uint64 device_virtual_address,
                                  DmaDirection direction, bool on_chip_cached) {
  CHECK(IsPageAligned(buffer));
  CHECK(IsPageAligned(device_virtual_address));

  const uint8 *host_addr_start = static_cast<const uint8 *>(buffer);
  for (int i = 0; i < num_pages; ++i) {
    const uint8 *host_addr = i * kHostPageSize + host_addr_start;
    uint64 device_addr = i * kHostPageSize + device_virtual_address;

    CHECK(device_to_host_.find(device_addr) == device_to_host_.end());
    device_to_host_.emplace(device_addr, host_addr);
  }

  return util::Status();  // OK
}

util::Status FakeMmuMapper::DoMap(int fd, int num_pages,
                                  uint64 device_virtual_address,
                                  DmaDirection direction) {
  return DoMap(reinterpret_cast<void *>(fd * kHostPageSize), num_pages,
               device_virtual_address, direction, /*on_chip_cached=*/false);
}

util::Status FakeMmuMapper::DoUnmap(const void *buffer, int num_pages,
                                    uint64 device_virtual_address) {
  CHECK(IsPageAligned(buffer));
  CHECK(IsPageAligned(device_virtual_address));

  for (int i = 0; i < num_pages; ++i) {
    uint64 device_addr = i * kHostPageSize + device_virtual_address;

    // TODO(jnjoseph): Validate that the device virtual address and buffer
    // corresponds to the buffer that was originally mapped.
    CHECK(device_to_host_.find(device_addr) != device_to_host_.end());
    device_to_host_.erase(device_addr);
  }

  return util::Status();  // OK
}

util::Status FakeMmuMapper::DoUnmap(int fd, int num_pages,
                                    uint64 device_virtual_address) {
  return DoUnmap(reinterpret_cast<void *>(fd * kHostPageSize), num_pages,
                 device_virtual_address);
}

util::StatusOr<void *> FakeMmuMapper::TranslateDeviceAddress(
    uint64 device_virtual_address) const {
  uint64 aligned_device_addr = GetPageAddress(device_virtual_address);

  auto iter = device_to_host_.find(aligned_device_addr);
  if (iter == device_to_host_.end()) {
    return util::NotFoundError("Device address not mapped.");
  }

  uint8 *aligned_host_addr = const_cast<uint8 *>(iter->second);
  auto host_address = aligned_host_addr + GetPageOffset(device_virtual_address);

  CHECK(host_address != nullptr);  // StatusOr doesn't like nullptr!
  return host_address;
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
