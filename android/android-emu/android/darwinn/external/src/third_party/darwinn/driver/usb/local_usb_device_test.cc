#include "third_party/darwinn/driver/usb/local_usb_device.h"

#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

TEST(LocalUsbDeviceFactoryTest, ComposeSimplePath) {
  // This test verifies that LocalUsbDeviceFactory composes a path string.
  LocalUsbDeviceFactory::ParsedPath parsed_path = {15, {0, 1, 2, 3, 4, 5, 6}};
  string path = LocalUsbDeviceFactory::ComposePathString(parsed_path);
  EXPECT_EQ(path, "/sys/bus/usb/devices/15-0.1.2.3.4.5.6");
}

TEST(LocalUsbDeviceFactoryTest, ParseSimplePath) {
  // This test verifies that LocalUsbDeviceFactory parses a path string.
  constexpr int kDepth = 7;
  // TODO(macwang): consider changing to ASSERT_OK_AND_ASSIGN when it's
  // supported in the portable build environment.
  auto parse_result = LocalUsbDeviceFactory::ParsePathString(
      "/sys/bus/usb/devices/15-0.1.2.3.4.5.6");
  ASSERT_OK(parse_result.status());
  LocalUsbDeviceFactory::ParsedPath parsed_path = parse_result.ValueOrDie();
  EXPECT_EQ(parsed_path.bus_number, 15);
  ASSERT_EQ(parsed_path.port_numbers.size(), kDepth);
  for (int i = 0; i < kDepth; ++i) {
    ASSERT_EQ(parsed_path.port_numbers[i], i);
  }
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
