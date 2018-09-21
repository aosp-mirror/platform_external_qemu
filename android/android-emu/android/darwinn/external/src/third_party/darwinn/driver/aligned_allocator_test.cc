#include "third_party/darwinn/driver/aligned_allocator.h"

#include <stdlib.h>
#include <memory>
#include <string>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

class AlignedAllocatorTest : public ::testing::Test {
 protected:
  AlignedAllocatorTest() = default;
  ~AlignedAllocatorTest() override = default;
};

class AlignedAllocatorParamTest : public ::testing::TestWithParam<int> {};

TEST_P(AlignedAllocatorParamTest, AlignedAllocatorParamTest) {
  AlignedAllocator allocator(GetParam());
  void* buffer = allocator.Allocate(135);
  uint64 addr = reinterpret_cast<uint64>(buffer);
  EXPECT_EQ(addr & ~(GetParam() - 1), addr);
  EXPECT_EQ(addr & (GetParam() - 1), 0);
  allocator.Free(buffer);
}

const int kTestAlignments[]{4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

INSTANTIATE_TEST_CASE_P(AlignedAllocatorParamTest, AlignedAllocatorParamTest,
                        ::testing::ValuesIn(kTestAlignments));

TEST(BufferTest, MakeBuffer) {
  const int kAlignment = 64;
  AlignedAllocator allocator(kAlignment);

  Buffer buffer = allocator.MakeBuffer(1024);
  uint64 addr = reinterpret_cast<uint64>(buffer.ptr());

  EXPECT_EQ(addr & ~(kAlignment - 1), addr);
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
