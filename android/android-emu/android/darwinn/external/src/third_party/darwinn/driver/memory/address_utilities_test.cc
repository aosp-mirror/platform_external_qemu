// Tests for address utilities.

#include "third_party/darwinn/driver/memory/address_utilities.h"

#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

TEST(AddressUtilitiesTest, GetPageOffset) {
  uint8* buffer = reinterpret_cast<uint8*>(0);
  EXPECT_EQ(GetPageOffset(buffer), 0);

  buffer = reinterpret_cast<uint8*>(1);
  EXPECT_EQ(GetPageOffset(buffer), 1);

  buffer = reinterpret_cast<uint8*>(kHostPageSize - 1);
  EXPECT_EQ(GetPageOffset(buffer), kHostPageSize - 1);

  buffer = reinterpret_cast<uint8*>(kHostPageSize + 0);
  EXPECT_EQ(GetPageOffset(buffer), 0);

  buffer = reinterpret_cast<uint8*>(kHostPageSize + 89);
  EXPECT_EQ(GetPageOffset(buffer), 89);
}

TEST(AddressUtilitiesTest, IsPageAligned) {
  uint8* buffer = reinterpret_cast<uint8*>(0);
  EXPECT_TRUE(IsPageAligned(buffer));
  EXPECT_FALSE(IsPageAligned(buffer + 1));
  EXPECT_FALSE(IsPageAligned(buffer + kHostPageSize - 1));
  EXPECT_TRUE(IsPageAligned(buffer + kHostPageSize));
}

TEST(AddressUtilitiesTest, GetNumberPages) {
  uint8* buffer = reinterpret_cast<uint8*>(0);
  EXPECT_EQ(GetNumberPages(buffer, 0), 0);

  buffer = reinterpret_cast<uint8*>(1);
  EXPECT_EQ(GetNumberPages(buffer, 0), 1);

  buffer = reinterpret_cast<uint8*>(kHostPageSize + 1);
  EXPECT_EQ(GetNumberPages(buffer, 0), 1);

  buffer = reinterpret_cast<uint8*>(kHostPageSize - 1);
  EXPECT_EQ(GetNumberPages(buffer, 0), 1);
  EXPECT_EQ(GetNumberPages(buffer, 1), 1);
  EXPECT_EQ(GetNumberPages(buffer, 2), 2);
}

TEST(AddressUtilitiesTest, GetPageAddressForBuffer) {
  uint8* buffer = reinterpret_cast<uint8*>(0);
  EXPECT_EQ(GetPageAddressForBuffer(buffer), buffer);
  EXPECT_EQ(GetPageAddressForBuffer(buffer + 1), buffer);
  EXPECT_EQ(GetPageAddressForBuffer(buffer + kHostPageSize - 1), buffer);

  buffer = reinterpret_cast<uint8*>(kHostPageSize);
  EXPECT_EQ(GetPageAddressForBuffer(buffer + 10), buffer);
}

TEST(AddressUtilitiesTest, GetPageAddressFromNumber) {
  EXPECT_EQ(GetPageAddressFromNumber(0), 0);
  EXPECT_EQ(GetPageAddressFromNumber(1), kHostPageSize);
  EXPECT_EQ(GetPageAddressFromNumber(2), kHostPageSize * 2);
}

TEST(AddressUtilitiesTest, GetPageNumberFromAddress) {
  EXPECT_EQ(GetPageNumberFromAddress(0), 0);
  EXPECT_EQ(GetPageNumberFromAddress(kHostPageSize), 1);
  EXPECT_EQ(GetPageNumberFromAddress(kHostPageSize * 2), 2);
}

class IsAlignedTest : public ::testing::Test,
                      public ::testing::WithParamInterface<int> {};

TEST_P(IsAlignedTest, IsAlignedTest) {
  constexpr int kAlignment = 4;
  uint64 addr = GetParam();

  bool expected = (addr % kAlignment) == 0;

  EXPECT_EQ(IsAligned(reinterpret_cast<uint8*>(addr), kAlignment).ok(),
            expected);
}

const int kTestOffsets[]{
    8064, 4243, 7019, 6611, 4894, 4846, 6285, 5409, 1867, 5316, 8830, 8236,
    8175, 3651, 1015, 4268, 7114, 9703, 5779, 4131, 1704, 3346, 7602, 6282,
    4567, 9925, 3009, 3825, 8353, 7910, 1874, 3830, 5685, 6125, 7358, 2643,
    2732, 9301, 3718, 9934, 7066, 8181, 6437, 4356, 5422, 7497, 8785, 1497,
};

INSTANTIATE_TEST_CASE_P(IsAlignedTest, IsAlignedTest,
                        ::testing::ValuesIn(kTestOffsets));

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
