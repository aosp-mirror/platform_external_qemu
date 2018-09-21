// Tests for fake MMU mapper.

#include <memory>
#include <vector>

#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/driver/memory/fake_mmu_mapper.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

class FakeMmuMapperTest : public ::testing::Test {
 protected:
  FakeMmuMapperTest() { mmu_mapper_ = gtl::MakeUnique<FakeMmuMapper>(); }
  ~FakeMmuMapperTest() override {}

  std::unique_ptr<FakeMmuMapper> mmu_mapper_;
};

TEST_F(FakeMmuMapperTest, TestMapTranslate) {
  uint8* buffer = reinterpret_cast<uint8*>(kHostPageSize * 10);
  uint64 device_virtual_address = kHostPageSize * 20;
  size_t num_pages = 10;
  const Buffer buf(buffer, num_pages * kHostPageSize);

  EXPECT_OK(mmu_mapper_->MmuMapper::Map(buf, device_virtual_address));
  for (int i = 0; i < num_pages; ++i) {
    auto offset = kHostPageSize * i;
    auto result =
        mmu_mapper_->TranslateDeviceAddress(device_virtual_address + offset);
    EXPECT_OK(result.status());

    EXPECT_EQ(result.ValueOrDie(), buffer + offset);
  }

  // Translate non aligned address.
  size_t offset = 20;
  auto result =
      mmu_mapper_->TranslateDeviceAddress(device_virtual_address + offset);
  EXPECT_OK(result.status());
  EXPECT_EQ(result.ValueOrDie(), buffer + offset);

  EXPECT_OK(mmu_mapper_->MmuMapper::Unmap(buf, device_virtual_address));
}

TEST_F(FakeMmuMapperTest, TestTranslateUnMapped) {
  size_t offset = 20;
  auto result = mmu_mapper_->TranslateDeviceAddress(kHostPageSize + offset);
  EXPECT_THAT(result.status().CanonicalCode(), util::error::NOT_FOUND);
}

TEST_F(FakeMmuMapperTest, TestSimpleFDMapping) {
  const size_t num_pages = 10;
  const int fd = 42;
  const uint64 device_virtual_address = kHostPageSize * 20;

  const Buffer buf(fd, num_pages * kHostPageSize);
  EXPECT_OK(mmu_mapper_->Map(buf, device_virtual_address));

  // Translate some non-aligned address
  const size_t offset = 20;
  auto result =
      mmu_mapper_->TranslateDeviceAddress(device_virtual_address + offset);

  EXPECT_OK(result.status());
  EXPECT_EQ(result.ValueOrDie(),
            reinterpret_cast<uint8*>(fd * kHostPageSize) + offset);
  EXPECT_OK(mmu_mapper_->MmuMapper::Unmap(buf, device_virtual_address));
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
