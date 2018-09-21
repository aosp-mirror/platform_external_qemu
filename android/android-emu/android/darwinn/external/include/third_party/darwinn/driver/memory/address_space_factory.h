#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_ADDRESS_SPACE_FACTORY_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_ADDRESS_SPACE_FACTORY_H_

#include <stddef.h>
#include <memory>
#include <vector>

#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/buddy_address_space.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/driver/memory/nop_address_space.h"
#include "third_party/darwinn/port/integral_types.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Abstract factory to create address spaces.
class AddressSpaceFactory {
 public:
  AddressSpaceFactory() = default;
  virtual ~AddressSpaceFactory() = default;

  // Creates various address spaces.
  virtual std::unique_ptr<AddressSpace> Create(
      uint64 device_virtual_address_start,
      uint64 device_virtual_address_size_bytes) const = 0;
};

// Factory to create buddy-allocated address spaces.
class BuddyAddressSpaceFactory : public AddressSpaceFactory {
 public:
  explicit BuddyAddressSpaceFactory(MmuMapper* mmu_mapper);
  ~BuddyAddressSpaceFactory() override = default;

  // Creates various address spaces. Taking usages only for conforming with
  // existing interface.
  std::unique_ptr<AddressSpace> Create(
      uint64 device_virtual_address_start,
      uint64 device_virtual_address_size_bytes)
      const override;

 private:
  // MmuMapper used by address space.
  MmuMapper* const mmu_mapper_;
};

// Factory to create no-op address spaces.
class NopAddressSpaceFactory : public AddressSpaceFactory {
 public:
  NopAddressSpaceFactory() = default;
  ~NopAddressSpaceFactory() override = default;

  std::unique_ptr<AddressSpace> Create(
      uint64 device_virtual_address_start,
      uint64 device_virtual_address_size_bytes) const override;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_ADDRESS_SPACE_FACTORY_H_
