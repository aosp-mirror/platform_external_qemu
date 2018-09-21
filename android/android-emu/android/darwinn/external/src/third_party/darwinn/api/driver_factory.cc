#include "third_party/darwinn/api/driver_factory.h"

namespace platforms {
namespace darwinn {
namespace api {

namespace {

Driver* NewDriver(const Device& device, const Driver::Options& options) {
  // Build Driver.
  auto factory = DriverFactory::GetOrCreate();
  auto backing_driver = factory->CreateDriver(device, options).ValueOrDie();
  return backing_driver.release();
}

}  // namespace

Driver* DriverFactory::CreateDriverAsSingleton(const Device& device,
                                               const Driver::Options& options) {
  static api::Driver* driver = NewDriver(device, options);
  return driver;
}

Device::Type GetTypeByName(const std::string& device_type) {
  if (device_type == "PCI" || device_type == "pci") {
    return Device::Type::PCI;
  } else if (device_type == "USB" || device_type == "usb") {
    return Device::Type::USB;
  } else if (device_type == "REFERENCE" || device_type == "reference") {
    return Device::Type::REFERENCE;
  } else {
    LOG(FATAL) << "Unknown device type: " << device_type
               << ", which should be either \"PCI\", \"USB\" or \"REFERENCE\"";
    __builtin_unreachable();
  }
}

std::string GetTypeName(Device::Type device_type) {
  switch (device_type) {
    case Device::Type::PCI:
      return "pci";
    case Device::Type::USB:
      return "usb";
    case Device::Type::PLATFORM:
      return "platform";
    case Device::Type::REMOTE_PCI:
      return "remote_pci";
    case Device::Type::REMOTE_USB:
      return "remote_usb";
    case Device::Type::REMOTE_PLATFORM:
      return "remote_platform";
    case Device::Type::REFERENCE:
      return "reference";
    default:
      return "unknown";
  }
}

}  // namespace api
}  // namespace darwinn
}  // namespace platforms
