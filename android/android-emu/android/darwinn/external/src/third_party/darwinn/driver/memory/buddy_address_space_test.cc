#include "third_party/darwinn/driver/memory/buddy_address_space.h"

#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/driver/memory/fake_mmu_mapper.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

const uint64 kDeviceAddrStart = 0x1000;
const size_t kNumPages = 16;
const size_t kDeviceAddrSizeBytes = kNumPages * kHostPageSize;

class BuddyAddressSpaceTest : public ::testing::Test {
 protected:
  BuddyAddressSpaceTest() = default;
  ~BuddyAddressSpaceTest() override = default;

  std::unique_ptr<BuddyAddressSpace> MakeBuddyAddressSpace() {
    return gtl::MakeUnique<BuddyAddressSpace>(
        kDeviceAddrStart, kDeviceAddrSizeBytes, &mmu_mapper_);
  }

  // MMU mapper required by AddressSpace.
  FakeMmuMapper mmu_mapper_;
};

// Test initialization.
TEST_F(BuddyAddressSpaceTest, BlocksInit) {
  auto address_space = MakeBuddyAddressSpace();

  // There should be one block of 2^16
  EXPECT_EQ(address_space->NumFreeBlocksByOrder(kHostPageShiftBits + 4), 1);
  // Bins of other orders should be empty
  for (uint32 i = 0; i < 4; ++i) {
    SCOPED_TRACE(i);
    EXPECT_EQ(address_space->NumFreeBlocksByOrder(kHostPageShiftBits + i), 0);
  }
}

TEST_F(BuddyAddressSpaceTest, SimpleMapping) {
  auto address_space = MakeBuddyAddressSpace();

  // 1/2 device size + 1 is guaranteed to require breaking down the highest
  // order block
  std::vector<uint8> buf(kDeviceAddrSizeBytes / 2 + 1);

  EXPECT_OK(address_space->MapMemory(Buffer(buf.data(), buf.size())).status());

  // There shouldn't be any 2^16-sized unused blocks
  EXPECT_EQ(address_space->NumFreeBlocksByOrder(kHostPageShiftBits + 4), 0);
}

// TODO(ijsung): add a test that maps and unmaps randomly. Need to make it a
// random walk with "fuel", where fuel being the amount of remaining memory.

TEST_F(BuddyAddressSpaceTest, MappingAcrossOrders) {
  auto address_space = MakeBuddyAddressSpace();

  std::vector<uint8> buf0(kHostPageSize + 1);  // need at least two pages
  std::vector<uint8> buf1(kHostPageSize + 2);

  EXPECT_OK(
      address_space->MapMemory(Buffer(buf0.data(), buf0.size())).status());
  auto get_blocks = [&address_space](int i) {
    return address_space->NumFreeBlocksByOrder(i + kHostPageShiftBits);
  };
  // There shouldn't be any order 4 (2^16-sized) unused blocks
  EXPECT_EQ(get_blocks(4), 0);
  EXPECT_EQ(get_blocks(3), 1);
  EXPECT_EQ(get_blocks(2), 1);
  EXPECT_EQ(get_blocks(1), 1);
  EXPECT_EQ(get_blocks(0), 0);
  EXPECT_OK(
      address_space->MapMemory(Buffer(buf1.data(), buf1.size())).status());
  EXPECT_EQ(get_blocks(4), 0);
  EXPECT_EQ(get_blocks(3), 1);
  EXPECT_EQ(get_blocks(2), 1);
  EXPECT_EQ(get_blocks(1), 0);
  EXPECT_EQ(get_blocks(0), 0);
}

TEST_F(BuddyAddressSpaceTest, MapAndUnmap) {
  auto address_space = MakeBuddyAddressSpace();
  std::vector<uint8> buf0(kHostPageSize + 1);  // need at least two pages
  ASSERT_OK_AND_ASSIGN(auto device_buffer, address_space->MapMemory(Buffer(
                                               buf0.data(), buf0.size())));
  EXPECT_EQ(GetPageOffset(buf0.data()),
            GetPageOffset(device_buffer.device_address()));

  EXPECT_OK(address_space->UnmapMemory(std::move(device_buffer)));
  auto get_blocks = [&address_space](int i) {
    return address_space->NumFreeBlocksByOrder(i + kHostPageShiftBits);
  };

  // Coalescing should happen
  EXPECT_EQ(get_blocks(4), 1);
  EXPECT_EQ(get_blocks(3), 0);
  EXPECT_EQ(get_blocks(2), 0);
  EXPECT_EQ(get_blocks(1), 0);
  EXPECT_EQ(get_blocks(0), 0);
}

TEST_F(BuddyAddressSpaceTest, UnmapNotMapped) {
  auto address_space = MakeBuddyAddressSpace();

  EXPECT_THAT(address_space->UnmapMemory(DeviceBuffer(kDeviceAddrStart, 4096))
                  .CanonicalCode(),
              util::error::INVALID_ARGUMENT);
}

TEST_F(BuddyAddressSpaceTest, MapZeroFail) {
  auto address_space = MakeBuddyAddressSpace();
  std::vector<uint8> buf0;
  EXPECT_THAT(
      address_space->MapMemory(Buffer(buf0.data(), 0)).status().CanonicalCode(),
      util::error::INVALID_ARGUMENT);
}

TEST_F(BuddyAddressSpaceTest, MapNullptrFail) {
  auto address_space = MakeBuddyAddressSpace();
  const void* kNull = nullptr;
  EXPECT_THAT(
      address_space->MapMemory(Buffer(kNull, 1000)).status().CanonicalCode(),
      util::error::INVALID_ARGUMENT);
}

TEST_F(BuddyAddressSpaceTest, MapAllMapAgainFail) {
  auto address_space = MakeBuddyAddressSpace();
  // See b/76207074 on how the size is determined.
  constexpr auto kBufSize = kDeviceAddrSizeBytes - kHostPageSize + 1;
  std::vector<uint8> buf0(kBufSize);
  ASSERT_OK_AND_ASSIGN(auto device_buffer, address_space->MapMemory(Buffer(
                                               buf0.data(), buf0.size())));
  EXPECT_EQ(device_buffer.size_bytes(), kBufSize);

  std::vector<uint8> buf1(kBufSize);
  EXPECT_THAT(address_space->MapMemory(Buffer(buf1.data(), buf1.size()))
                  .status()
                  .CanonicalCode(),
              util::error::RESOURCE_EXHAUSTED);
}

TEST_F(BuddyAddressSpaceTest, MapAllUnmapMapAll) {
  auto address_space = MakeBuddyAddressSpace();
  // See b/76207074 on how the size is determined.
  constexpr auto kBufSize = kDeviceAddrSizeBytes - kHostPageSize + 1;
  {
    std::vector<uint8> buf(kBufSize);
    ASSERT_OK_AND_ASSIGN(auto device_buffer, address_space->MapMemory(Buffer(
                                                 buf.data(), buf.size())));
    EXPECT_EQ(device_buffer.size_bytes(), kBufSize);
    EXPECT_OK(address_space->UnmapMemory(std::move(device_buffer)));
  }

  {
    std::vector<uint8> buf(kBufSize);
    ASSERT_OK_AND_ASSIGN(auto device_buffer, address_space->MapMemory(Buffer(
                                                 buf.data(), buf.size())));
    EXPECT_EQ(device_buffer.size_bytes(), kBufSize);
  }
}

TEST_F(BuddyAddressSpaceTest, SimpleFileDescriptorMapping) {
  constexpr int fd = 42;
  constexpr int fd_size = 0xbeef;
  auto address_space = MakeBuddyAddressSpace();

  ASSERT_OK_AND_ASSIGN(auto device_buffer,
                       address_space->MapMemory(Buffer(fd, fd_size)));
  // Test validity of the mapped buffer.
  EXPECT_EQ(device_buffer.IsValid(), true);
  EXPECT_EQ(device_buffer.size_bytes(), fd_size);
  // Test translation.
  ASSERT_OK_AND_ASSIGN(
      auto device_phys_address,
      mmu_mapper_.TranslateDeviceAddress(device_buffer.device_address()));
  // Test if MMU is updated to map device VA returned by MapMemory() to the
  // bogus physical address provided by the fake MMU mapper.
  EXPECT_EQ(reinterpret_cast<intptr_t>(device_phys_address),
            fd * kHostPageSize);

  EXPECT_OK(address_space->UnmapMemory(std::move(device_buffer)));
  EXPECT_EQ(device_buffer.IsValid(), false); // NOLINT
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
