#ifndef THIRD_PARTY_DARWINN_DRIVER_MEMORY_FAKE_MMU_MAPPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_MEMORY_FAKE_MMU_MAPPER_H_

#include <map>

#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// A fake MMU mapper implementation that does not accurately model
// the underlying hardware, but behaves the same way.
class FakeMmuMapper : public MmuMapper {
 public:
  FakeMmuMapper() {}
  ~FakeMmuMapper() override {}

  // This class is neither copyable nor movable.
  FakeMmuMapper(const FakeMmuMapper&) = delete;
  FakeMmuMapper& operator=(const FakeMmuMapper&) = delete;

  // Overrides from MmuMapper
  util::Status Open(int num_simple_page_table_entries_requested) override {
    return util::Status();  // OK
  }
  util::Status Close() override { return util::Status(); }
  util::StatusOr<void*> TranslateDeviceAddress(
      uint64 device_virtual_address) const override;

 protected:
  util::Status DoMap(const void* buffer, int num_pages,
                     uint64 device_virtual_address, DmaDirection direction,
                     bool on_chip_cached) override;
  util::Status DoUnmap(const void* buffer, int num_pages,
                       uint64 device_virtual_address) override;

  // Fake mapping: assuming physical address = fd * kHostPageSize.
  util::Status DoMap(int fd, int num_pages, uint64 device_virtual_address,
                     DmaDirection direction) override;
  util::Status DoUnmap(int fd, int num_pages,
                       uint64 device_virtual_address) override;

  // "Page table" to track device addr to host mappings.
  std::map<uint64, const uint8*> device_to_host_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MEMORY_FAKE_MMU_MAPPER_H_
