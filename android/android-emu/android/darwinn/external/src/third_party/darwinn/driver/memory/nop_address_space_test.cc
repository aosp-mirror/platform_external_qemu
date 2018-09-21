#include "third_party/darwinn/driver/memory/nop_address_space.h"

#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

// Test Map and Unmap methods in NopAddressSpace
TEST(NopAddressSpaceTest, MapAndUnmap) {
  constexpr uint32 buf_size = 42;
  NopAddressSpace address_space;
  std::vector<uint8> buf(buf_size);
  ASSERT_OK_AND_ASSIGN(auto device_buffer,
                       address_space.MapMemory(Buffer(buf.data(), buf.size())));
  EXPECT_TRUE(device_buffer.IsValid());
  EXPECT_EQ(device_buffer.size_bytes(), buf_size);
  EXPECT_OK(address_space.UnmapMemory(std::move(device_buffer)));
  EXPECT_FALSE(device_buffer.IsValid());  // NOLINT
}

// Test Translate methods in NopAddressSpace
TEST(NopAddressSpaceTest, Translate) {
  constexpr uint32 buf_size = 42;
  NopAddressSpace address_space;
  std::vector<uint8> buf(buf_size);
  ASSERT_OK_AND_ASSIGN(auto device_buffer,
                       address_space.MapMemory(Buffer(buf.data(), buf.size())));
  ASSERT_OK_AND_ASSIGN(auto host_buffer,
                       address_space.Translate(device_buffer));
  EXPECT_TRUE(host_buffer.IsValid());
  EXPECT_EQ(host_buffer.size_bytes(), buf_size);
  EXPECT_OK(address_space.UnmapMemory(std::move(device_buffer)));
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
