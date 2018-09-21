#include "third_party/darwinn/driver/memory/address_space_factory.h"

#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace driver {

BuddyAddressSpaceFactory::BuddyAddressSpaceFactory(MmuMapper* mmu_mapper)
    : AddressSpaceFactory(), mmu_mapper_(mmu_mapper) {
  CHECK(mmu_mapper != nullptr);
}

std::unique_ptr<AddressSpace> BuddyAddressSpaceFactory::Create(
    uint64 device_virtual_address_start,
    uint64 device_virtual_address_size_bytes) const {
  return gtl::MakeUnique<BuddyAddressSpace>(device_virtual_address_start,
                                            device_virtual_address_size_bytes,
                                            mmu_mapper_);
}

std::unique_ptr<AddressSpace> NopAddressSpaceFactory::Create(
    uint64 /* device_virtual_address_start */,
    uint64 /* device_virtual_address_size_bytes */) const {
  return gtl::MakeUnique<NopAddressSpace>();
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
