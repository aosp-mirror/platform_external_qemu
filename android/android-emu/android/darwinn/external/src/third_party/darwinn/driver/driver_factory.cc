#include "third_party/darwinn/driver/driver_factory.h"

#include <dirent.h>
#include <sys/stat.h>
#include <memory>
#include <utility>

#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/api/driver_options_helper.h"
#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/driver.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

std::vector<api::Device> DriverProvider::EnumerateSysfs(
    const std::string& class_name, api::Chip chip, api::Device::Type type) {
  // Look through sysfs for devices of the given class.
  // Sysfs paths look like this: /sys/class/<class_name>/<class_name>_<n>
  // For example, the first beagle device is: /sys/class/apex/apex_0
  // The corresponding device file is assumed to be: /dev/<class_name>_<n>
  // For example, if /sys/class/apex/apex_0 exists, we look for /dev/apex_0.
  //
  // Note that these assumptions could potentially break in the future; there is
  // some rumbling that /sys/class will be migrated to /sys/subsystem, at which
  // point we will need to make some updates here.
  std::vector<api::Device> device_list;
  std::string class_dir_name = "/sys/class/" + class_name;
  DIR* dir = opendir(class_dir_name.c_str());
  if (dir == nullptr) {
    VLOG(2) << "Failed to open " << class_dir_name << ": " << strerror(errno);
    return device_list;  // empty list
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string entry_name = std::string(entry->d_name);
    if (entry_name == "." || entry_name == "..") {
      continue;
    }
    std::string dev_file_name = "/dev/" + entry_name;
    struct stat statbuf;
    int ret = stat(dev_file_name.c_str(), &statbuf);
    if (ret != 0) {
      VLOG(1) << "Failed to stat " << dev_file_name << ": " << strerror(errno);
      continue;
    }
    if (!S_ISCHR(statbuf.st_mode)) {
      LOG(ERROR) << dev_file_name << " is not a character device.";
      continue;
    }
    device_list.push_back({chip, type, dev_file_name});
  }

  closedir(dir);
  return device_list;
}

std::vector<api::Device> DriverFactory::Enumerate() {
  StdMutexLock lock(&mutex_);

  std::vector<api::Device> device_list;
  for (auto& provider : providers_) {
    auto provider_supported_devices = provider->Enumerate();
    for (auto& device : provider_supported_devices) {
      device_list.push_back(device);
    }
  }

  return device_list;
}

util::StatusOr<std::unique_ptr<api::Driver>> DriverFactory::CreateDriver(
    const api::Device& device) {
  return CreateDriver(device, api::DriverOptionsHelper::Defaults());
}

util::StatusOr<std::unique_ptr<api::Driver>> DriverFactory::CreateDriver(
    const api::Device& device, const api::Driver::Options& opaque_options) {
  StdMutexLock lock(&mutex_);

  // Deserialize options.
  const api::DriverOptions* options =
      api::GetDriverOptions(opaque_options.data());
  if (options == nullptr) {
    return util::InvalidArgumentError("Invalid Driver::Options instance.");
  }

  if (options->version() != Driver::kOptionsVersion) {
    return util::InvalidArgumentError("Invalid Driver::Options version.");
  }

  // Update verbosity level.
  // TODO(ijsung): Verbosity level should be of per driver instance.
#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
  if (options->verbosity() >= 0) {
    ::platforms::darwinn::internal::SetLoggingLevel(options->verbosity());
  }
#endif

  for (auto& provider : providers_) {
    // Skip if the provider cannot create driver for this device spec.
    if (!provider->CanCreate(device)) {
      continue;
    }

    // Always invoke only the first provider which claims the ability.
    if (device.path != kDefaultDevicePath) {
      return provider->CreateDriver(device, *options);
    }

    // Try to enumerate with this provider.
    std::vector<api::Device> device_list = provider->Enumerate();
    // Skip this provider if there is no any device.
    if (device_list.empty()) {
      continue;
    }

    // Create driver associated with the device in the resulting list.
    for (const auto& provider_device : device_list) {
      if (device.chip == provider_device.chip) {
        return provider->CreateDriver(provider_device, *options);
      }
    }
  }

  return util::NotFoundError("Unable to construct driver for device.");
}

void DriverFactory::RegisterDriverProvider(
    std::unique_ptr<DriverProvider> provider) {
  StdMutexLock lock(&mutex_);
  providers_.push_back(std::move(provider));
}

DriverFactory* DriverFactory::GetOrCreate() {
  static std::unique_ptr<DriverFactory> singleton =
      gtl::WrapUnique<DriverFactory>(new driver::DriverFactory());
  return singleton.get();
}

}  // namespace driver

namespace api {

DriverFactory* DriverFactory::GetOrCreate() {
  return driver::DriverFactory::GetOrCreate();
}

}  // namespace api
}  // namespace darwinn
}  // namespace platforms
