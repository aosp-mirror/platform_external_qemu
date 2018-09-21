#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_MOCK_ADDRESS_SPACE_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_MOCK_ADDRESS_SPACE_H_

#include <stddef.h>
#include <memory>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Mock DarwiNN address space management.
class MockAddressSpace : public AddressSpace {
 public:
  using AddressSpace::MapMemory;  // Allows for proper overload resolution.

  MockAddressSpace() = default;
  ~MockAddressSpace() override = default;

  MOCK_METHOD3(MapMemory,
               util::StatusOr<DeviceBuffer>(const Buffer&, DmaDirection,
                                            MappingTypeHint mapping_type));
  MOCK_METHOD1(UnmapMemory, util::Status(DeviceBuffer));
  MOCK_CONST_METHOD1(Translate,
                     util::StatusOr<const Buffer>(const DeviceBuffer&));
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_MOCK_ADDRESS_SPACE_H_
