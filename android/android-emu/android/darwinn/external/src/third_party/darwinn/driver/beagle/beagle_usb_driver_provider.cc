#include <stdint.h>
#include <unistd.h>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>

#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/driver/aligned_allocator.h"
#include "third_party/darwinn/driver/beagle/beagle_top_level_handler.h"
#include "third_party/darwinn/driver/beagle/beagle_top_level_interrupt_manager.h"
#include "third_party/darwinn/driver/config/beagle/beagle_chip_config.h"
#include "third_party/darwinn/driver/driver_factory.h"
#include "third_party/darwinn/driver/executable_registry.h"
#include "third_party/darwinn/driver/executable_verifier.h"
#include "third_party/darwinn/driver/interrupt/grouped_interrupt_controller.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/driver/run_controller.h"
#include "third_party/darwinn/driver/scalar_core_controller.h"
#include "third_party/darwinn/driver/usb/local_usb_device.h"
#include "third_party/darwinn/driver/usb/usb_device_interface.h"
#include "third_party/darwinn/driver/usb/usb_driver.h"
#include "third_party/darwinn/driver/usb/usb_ml_commands.h"
#include "third_party/darwinn/driver/usb/usb_registers.h"
#include "third_party/darwinn/port/gflags.h"
#include "third_party/darwinn/port/ptr_util.h"

/*
 * There are only 3 modes of operation regarding
 * usb_enable_bulk_out_descriptors_from_device and
 * usb_enable_processing_of_hints:
 *
 * 1) both true, we follow the hints, and
 * use descriptors sent from device as validation. This mode doesn't work if
 * the device sends a lot of bulk-out or bulk-in descriptors out which could
 * clog the descriptor/bulk-in pipeline.
 *
 * 2) disable descriptors but enable hints. We blindly follow the hints and
 * send data to device as fast as we can. The mode is similar to the
 * previous one, but could be slightly faster.
 *
 * 3) enable descriptors but disable the hints. we use descriptors from
 * device and pretend there is no hint from code gen, except for first one
 * (for instructions). This mode doesn't work with multiple instruction
 * chunks, as device is not capable of generating descriptors for
 * instructions.
 *
 */
DEFINE_bool(
    usb_enable_bulk_descriptors_from_device, false,
    "USB set to true if bulk in/out descriptors from device are needed.");
DEFINE_bool(usb_enable_processing_of_hints, true,
            "USB set to true for driver to proactively send data to device.");

DEFINE_int32(usb_timeout_millis, 6000, "USB timeout in milliseconds");
DEFINE_bool(
    usb_reset_back_to_dfu_mode, false,
    "USB find device in app mode, reset back to DFU mode, and terminate");
DEFINE_int32(
    usb_software_credits_low_limit, 8192,
    "USB lower bound of bulk out transfer size in bytes, when used in mode 1");
DEFINE_int32(usb_operating_mode, 2,
             "USB driver operating mode: 0:Multiple-Ep w/ HW, 1:Multiple-Ep w/ "
             "SW, 2:Single-Ep");
DEFINE_int32(usb_max_bulk_out_transfer, 1024 * 1024,
             "USB max bulk out transfer size in bytes");
DEFINE_int32(usb_max_num_async_transfers, 3,
             "USB max number of pending async bulk out transfer");
DEFINE_bool(
    usb_force_largest_bulk_in_chunk_size, false,
    "If true, bulk-in data is transmitted in largest chunks possible. Setting "
    "this to true increase performance on USB2.");
DEFINE_bool(usb_enable_overlapping_requests, true,
            "Allows the next queued request to be partially overlapped with "
            "the current one.");
DEFINE_bool(usb_enable_overlapping_bulk_in_and_out, true,
            "Allows bulk-in trasnfer to be submitted before previous bulk-out "
            "requests complete.");
DEFINE_bool(usb_enable_queued_bulk_in_requests, true,
            "Allows bulk-in transfers to be queued to improve performance.");
DEFINE_bool(
    usb_fail_if_slower_than_superspeed, false,
    "USB driver open would fail if the connection is slower than superspeed.");

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using platforms::darwinn::api::Chip;
using platforms::darwinn::api::Device;

constexpr uint16_t kTargetAppVendorId = 0x18D1;
constexpr uint16_t kTargetAppProductId = 0x9302;

constexpr uint16_t kTargetDfuVendorId = 0x1A6E;
constexpr uint16_t kTargetDfuProductId = 0x089A;

// TODO(macwang) add proper error handling to this function.
// Convenience function to read a file to a vector.
std::vector<uint8_t> ReadToVector(const std::string& file_name) {
  VLOG(10) << __func__ << file_name;

  // TODO(macwang) directly read into the vector instead of transcopying through
  // a string.
  std::ifstream ifs(file_name);
  std::string content_string((std::istreambuf_iterator<char>(ifs)),
                             (std::istreambuf_iterator<char>()));

  std::vector<uint8_t> result;
  auto data = reinterpret_cast<const uint8_t*>(content_string.c_str());
  result.insert(result.end(), data, data + content_string.size());
  return result;
}
}  // namespace

class BeagleUsbDriverProvider : public DriverProvider {
 public:
  static std::unique_ptr<DriverProvider> CreateDriverProvider() {
    return gtl::WrapUnique<DriverProvider>(new BeagleUsbDriverProvider());
  }

  ~BeagleUsbDriverProvider() override = default;

  std::vector<Device> Enumerate() override;
  bool CanCreate(const Device& device) override;
  util::StatusOr<std::unique_ptr<api::Driver>> CreateDriver(
      const Device& device, const api::DriverOptions& options) override;

 private:
  BeagleUsbDriverProvider() = default;
};

REGISTER_DRIVER_PROVIDER(BeagleUsbDriverProvider);

std::vector<Device> BeagleUsbDriverProvider::Enumerate() {
  LocalUsbDeviceFactory usb_device_factory;
  std::vector<Device> device_list;

  auto usb_dfu_device_list_or_error = usb_device_factory.EnumerateDevices(
      kTargetDfuVendorId, kTargetDfuProductId);

  auto usb_app_device_list_or_error = usb_device_factory.EnumerateDevices(
      kTargetAppVendorId, kTargetAppProductId);

  if (usb_dfu_device_list_or_error.ok()) {
    for (const auto& path : usb_dfu_device_list_or_error.ValueOrDie()) {
      device_list.push_back({Chip::kBeagle, Device::Type::USB, path});
      VLOG(10) << StringPrintf("%s: adding path [%s]", __func__, path.c_str());
    }
  }

  if (usb_app_device_list_or_error.ok()) {
    for (const auto& path : usb_app_device_list_or_error.ValueOrDie()) {
      device_list.push_back({Chip::kBeagle, Device::Type::USB, path});
      VLOG(10) << StringPrintf("%s: adding path [%s]", __func__, path.c_str());
    }
  }

  return device_list;
}

bool BeagleUsbDriverProvider::CanCreate(const Device& device) {
  return device.type == Device::Type::USB && device.chip == Chip::kBeagle;
}

util::StatusOr<std::unique_ptr<api::Driver>>
BeagleUsbDriverProvider::CreateDriver(
    const Device& device, const api::DriverOptions& driver_options) {
  if (!CanCreate(device)) {
    return util::NotFoundError("Unsupported device.");
  }

  auto config = gtl::MakeUnique<config::BeagleChipConfig>();

  UsbDriver::UsbDriverOptions options;
  options.usb_force_largest_bulk_in_chunk_size =
      FLAGS_usb_force_largest_bulk_in_chunk_size;
  options.usb_enable_bulk_descriptors_from_device =
      FLAGS_usb_enable_bulk_descriptors_from_device;
  options.usb_enable_processing_of_hints = FLAGS_usb_enable_processing_of_hints;
  options.usb_max_num_async_transfers = FLAGS_usb_max_num_async_transfers;
  options.mode =
      static_cast<UsbDriver::OperatingMode>(FLAGS_usb_operating_mode);
  options.max_bulk_out_transfer_size_in_bytes = FLAGS_usb_max_bulk_out_transfer;
  options.software_credits_lower_limit_in_bytes =
      FLAGS_usb_software_credits_low_limit;
  options.usb_enable_overlapping_requests =
      FLAGS_usb_enable_overlapping_requests;
  options.usb_enable_overlapping_bulk_in_and_out =
      FLAGS_usb_enable_overlapping_bulk_in_and_out;
  options.usb_fail_if_slower_than_superspeed =
      FLAGS_usb_fail_if_slower_than_superspeed;
  options.usb_enable_queued_bulk_in_requests =
      FLAGS_usb_enable_queued_bulk_in_requests;

  auto usb_registers = gtl::MakeUnique<UsbRegisters>();
  std::vector<std::unique_ptr<InterruptControllerInterface>>
      top_level_interrupt_controllers;
  top_level_interrupt_controllers.push_back(
      gtl::MakeUnique<InterruptController>(
          config->GetUsbTopLevel0InterruptCsrOffsets(), usb_registers.get()));
  top_level_interrupt_controllers.push_back(
      gtl::MakeUnique<InterruptController>(
          config->GetUsbTopLevel1InterruptCsrOffsets(), usb_registers.get()));
  top_level_interrupt_controllers.push_back(
      gtl::MakeUnique<InterruptController>(
          config->GetUsbTopLevel2InterruptCsrOffsets(), usb_registers.get()));
  top_level_interrupt_controllers.push_back(
      gtl::MakeUnique<InterruptController>(
          config->GetUsbTopLevel3InterruptCsrOffsets(), usb_registers.get()));
  auto top_level_interrupt_controller =
      gtl::MakeUnique<GroupedInterruptController>(
          &top_level_interrupt_controllers);

  auto top_level_interrupt_manager =
      gtl::MakeUnique<BeagleTopLevelInterruptManager>(
          std::move(top_level_interrupt_controller), *config,
          usb_registers.get());

  auto fatal_error_interrupt_controller = gtl::MakeUnique<InterruptController>(
      config->GetUsbFatalErrorInterruptCsrOffsets(), usb_registers.get());

  auto top_level_handler = gtl::MakeUnique<BeagleTopLevelHandler>(
      *config, usb_registers.get(),
      /*use_usb=*/true, driver_options.performance_expectation());

  const api::DriverUsbOptions* usb_options = driver_options.usb();
  if (usb_options != nullptr) {
    if (usb_options->dfu_firmware() != nullptr) {
      auto provided_dfu_path = usb_options->dfu_firmware()->str();
      if (!provided_dfu_path.empty()) {
        // try loading firmware into memory.
        options.usb_firmware_image = ReadToVector(provided_dfu_path);
      }
    }
    options.usb_always_dfu = usb_options->always_dfu();
  }

  std::string path(device.path);
  ASSIGN_OR_RETURN(auto verifier,
                   MakeExecutableVerifier(driver_options.public_key()->str()));
  auto executable_registry =
      gtl::MakeUnique<ExecutableRegistry>(device.chip, std::move(verifier));

  return {gtl::MakeUnique<UsbDriver>(
      std::move(config),
      [path] {
        LocalUsbDeviceFactory usb_device_factory;

        return usb_device_factory.OpenDevice(path, FLAGS_usb_timeout_millis);
      },
      std::move(usb_registers), std::move(top_level_interrupt_manager),
      std::move(fatal_error_interrupt_controller), std::move(top_level_handler),
      std::move(executable_registry), options)};
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
