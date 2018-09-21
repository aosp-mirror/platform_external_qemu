#include "third_party/darwinn/driver/driver_factory.h"

#include <stdlib.h>
#include <memory>
#include <string>
#include <vector>

#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/driver/mock_driver.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using platforms::darwinn::api::Device;
using testing::Field;
using testing::Sequence;

class TestDriverProvider : public DriverProvider {
 public:
  ~TestDriverProvider() override = default;

  std::vector<Device> Enumerate() = 0;

  bool CanCreate(const Device& device) {
    auto supported_devices = Enumerate();
    for (const auto& supported_device : supported_devices) {
      if (device.chip == supported_device.chip &&
          device.type == supported_device.type &&
          device.path == supported_device.path) {
        return true;
      }
    }
    return false;
  }

 protected:
  TestDriverProvider() = default;
};

class TestDriverProviderOne : public TestDriverProvider {
 public:
  static std::unique_ptr<DriverProvider> CreateDriverProvider() {
    provider_ = new TestDriverProviderOne();
    return gtl::WrapUnique<DriverProvider>(provider_);
  }

  ~TestDriverProviderOne() override = default;

  std::vector<Device> Enumerate() {
    std::vector<Device> device_list;
    device_list.push_back({api::Chip::kBeagle, Device::Type::USB, "beagle_0"});
    return device_list;
  }

  MOCK_METHOD2(CreateDriver, util::StatusOr<std::unique_ptr<api::Driver>>(
                                 const Device&, const api::DriverOptions&));

  // Stores a pointer to the statically constructed provider to setup mocks.
  static TestDriverProviderOne* provider_;

 private:
  TestDriverProviderOne() = default;
};

TestDriverProviderOne* TestDriverProviderOne::provider_ = nullptr;

class TestDriverProviderTwo : public TestDriverProvider {
 public:
  static std::unique_ptr<DriverProvider> CreateDriverProvider() {
    provider_ = new TestDriverProviderTwo();
    return gtl::WrapUnique<DriverProvider>(provider_);
  }

  ~TestDriverProviderTwo() override = default;

  std::vector<Device> Enumerate() {
    std::vector<Device> device_list;
    device_list.push_back({api::Chip::kJago, Device::Type::PCI, "jago_0"});
    device_list.push_back({api::Chip::kJago, Device::Type::PCI, "jago_1"});
    return device_list;
  }

  MOCK_METHOD2(CreateDriver, util::StatusOr<std::unique_ptr<api::Driver>>(
                                 const Device&, const api::DriverOptions&));

  // Stores a pointer to the statically constructed provider to setup mocks.
  static TestDriverProviderTwo* provider_;

 private:
  TestDriverProviderTwo() = default;
};

TestDriverProviderTwo* TestDriverProviderTwo::provider_ = nullptr;

REGISTER_DRIVER_PROVIDER(TestDriverProviderOne);
REGISTER_DRIVER_PROVIDER(TestDriverProviderTwo);

TEST(DriverFactoryTest, TestEnumerateAll) {
  const auto& devices = DriverFactory::GetOrCreate()->Enumerate();
  EXPECT_EQ(devices.size(), 3);

  EXPECT_EQ(devices[0].path, "beagle_0");
  EXPECT_EQ(devices[1].path, "jago_0");
  EXPECT_EQ(devices[2].path, "jago_1");
}

// Verifies that CreateDriver calls are routed to the approriate
// DriverProviders.
TEST(DriverFactoryTest, TestConstructDrivers) {
  Sequence s;

  EXPECT_CALL(*TestDriverProviderTwo::provider_,
              CreateDriver(Field(&Device::path, "jago_0"), testing::_))
      .InSequence(s);
  EXPECT_CALL(*TestDriverProviderOne::provider_,
              CreateDriver(Field(&Device::path, "beagle_0"), testing::_))
      .InSequence(s);
  EXPECT_CALL(*TestDriverProviderTwo::provider_,
              CreateDriver(Field(&Device::path, "jago_1"), testing::_))
      .InSequence(s);

  auto result = DriverFactory::GetOrCreate()->CreateDriver(
      {api::Chip::kJago, Device::Type::PCI, "jago_0"});
  result = DriverFactory::GetOrCreate()->CreateDriver(
      {api::Chip::kBeagle, Device::Type::USB, "beagle_0"});
  result = DriverFactory::GetOrCreate()->CreateDriver(
      {api::Chip::kJago, Device::Type::PCI, "jago_1"});
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
