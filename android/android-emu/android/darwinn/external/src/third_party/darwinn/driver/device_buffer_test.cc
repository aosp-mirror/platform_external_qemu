#include "third_party/darwinn/driver/device_buffer.h"

#include <stddef.h>
#include <stdlib.h>
#include <memory>
#include <string>

#include "third_party/darwinn/driver/allocator.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

TEST(DeviceBufferTest, DeviceDeviceBufferTest) {
  uint64 temp = 0xdeadbeefface1234;
  const DeviceBuffer buffer(temp, sizeof(temp));

  EXPECT_TRUE(buffer.IsValid());
  EXPECT_EQ(buffer.device_address(), temp);
  EXPECT_EQ(buffer.size_bytes(), sizeof(temp));
}

TEST(DeviceBufferTest, DeviceDeviceBufferCopyConstructorTest) {
  uint64 temp = 0xdeadbeefface1234;
  DeviceBuffer buffer(temp, sizeof(temp));

  DeviceBuffer other_buffer(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.device_address(), temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));
}

TEST(DeviceBufferTest, DeviceDeviceBufferCopyAssignmentTest) {
  uint64 temp = 0xdeadbeefface1234;
  DeviceBuffer buffer(temp, sizeof(temp));

  DeviceBuffer other_buffer = buffer;
  EXPECT_TRUE(buffer.IsValid());
  EXPECT_EQ(other_buffer.device_address(), temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));
}

TEST(DeviceBufferTest, Equality) {
  DeviceBuffer bufferB1(0xdeadbeef, 1024);
  DeviceBuffer bufferB2(0xdeadbeef, 1024);
  EXPECT_EQ(bufferB1, bufferB2);

  DeviceBuffer bufferB3(0xdeadbeef, 1025);
  EXPECT_NE(bufferB1, bufferB3);
  DeviceBuffer bufferB4(0xdeadface, 1024);
  EXPECT_NE(bufferB1, bufferB4);
}

TEST(DeviceBufferTest, DeviceBufferMoveConstructorTest) {
  uint64 temp = 0xdeadbeefface1234;
  DeviceBuffer buffer(temp, sizeof(temp));

  DeviceBuffer other_buffer(std::move(buffer));
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.device_address(), temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));

  EXPECT_FALSE(buffer.IsValid());
  EXPECT_EQ(buffer.device_address(), 0);
  EXPECT_EQ(buffer.size_bytes(), 0);
}

TEST(DeviceBufferTest, DeviceBufferMoveAssignmentTest) {
  uint64 temp = 0xdeadbeefface1234;
  DeviceBuffer buffer(temp, sizeof(temp));

  DeviceBuffer other_buffer = std::move(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.device_address(), temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));

  EXPECT_FALSE(buffer.IsValid());
  EXPECT_EQ(buffer.device_address(), 0);
  EXPECT_EQ(buffer.size_bytes(), 0);
}

TEST(DeviceBufferTest, DeviceBufferSlice) {
  constexpr uint64 kBaseAddress = 0x20000;
  constexpr size_t kSizeBytes = 1000;
  DeviceBuffer buffer(kBaseAddress, kSizeBytes);

  for (uint64 byte_offset = 0; byte_offset < kSizeBytes; byte_offset += 100) {
    const size_t new_size = kSizeBytes - byte_offset;
    DeviceBuffer slice0 = buffer.Slice(byte_offset, new_size);
    EXPECT_TRUE(slice0.IsValid());
    EXPECT_EQ(slice0.device_address(), buffer.device_address() + byte_offset);
    EXPECT_EQ(slice0.size_bytes(), new_size);
  }
}

TEST(DeviceBufferTest, DeviceBufferSliceBaseAddressFail) {
  constexpr uint64 kBaseAddress = 0x20000;
  constexpr size_t kSizeBytes = 1000;
  DeviceBuffer buffer(kBaseAddress, kSizeBytes);

  EXPECT_DEATH(buffer.Slice(kBaseAddress + kSizeBytes, 10), "Overflowed ");
}

TEST(DeviceBufferTest, DeviceBufferSliceSizeFail) {
  constexpr uint64 kBaseAddress = 0x20000;
  constexpr size_t kSizeBytes = 1000;
  DeviceBuffer buffer(kBaseAddress, kSizeBytes);

  EXPECT_DEATH(buffer.Slice(0, kSizeBytes * 2), "Overflowed ");
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
