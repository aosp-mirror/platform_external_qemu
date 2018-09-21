#ifndef THIRD_PARTY_DARWINN_DRIVER_USB_USB_ML_COMMANDS_H_
#define THIRD_PARTY_DARWINN_DRIVER_USB_USB_ML_COMMANDS_H_

#include "third_party/darwinn/driver/usb/usb_device_interface.h"
#include "third_party/darwinn/driver/usb/usb_standard_commands.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Thread-safe implementation of Machine Learning commands through USB
// interface.
class UsbMlCommands : public UsbStandardCommands {
 public:
  enum class DescriptorTag {
    kUnknown = -1,
    kInstructions = 0,
    kInputActivations = 1,
    kParameters = 2,
    kOutputActivations = 3,
    kInterrupt0 = 4,
    kInterrupt1 = 5,
    kInterrupt2 = 6,
    kInterrupt3 = 7,
  };

  struct EventDescriptor {
    DescriptorTag tag{DescriptorTag::kUnknown};
    uint32_t length;
    uint64_t offset;
  };

  // Contains information retrieved from USB interrupt.
  // TODO(macwang): further parse this raw_data and provide more readable
  // information.
  struct InterruptInfo {
    uint32_t raw_data;
  };

  using InterruptInDone =
      std::function<void(util::Status, const InterruptInfo&)>;
  using EventInDone = std::function<void(util::Status, const EventDescriptor&)>;

  using Register32 = uint32_t;
  using Register64 = uint64_t;

  // Bulk out endpoint id used in single bulk out endpoint mode.
  static constexpr uint8_t kSingleBulkOutEndpoint = 1;

  // Bulk out endpoint id used for instruction stream in multiple bulk out mode.
  static constexpr uint8_t kInstructionsEndpoint = 1;

  // Bulk out endpoint id used for input activation stream in multiple bulk out
  // mode.
  static constexpr uint8_t kInputActivationsEndpoint = 2;

  // Bulk out endpoint id used for parameter stream in multiple bulk out mode.
  static constexpr uint8_t kParametersEndpoint = 3;

  // Bulk in endpoint id used for output activation stream.
  static constexpr uint8_t kBulkInEndpoint = 1;

  // Bulk in endpoint id used for event stream.
  static constexpr uint8_t kEventInEndpoint = 2;

  // Interrupt in endpoint id used for interrupt stream.
  static constexpr uint8_t kInterruptInEndpoint = 3;

  // Constructs a new object from pointer to an USB device.
  UsbMlCommands(std::unique_ptr<UsbDeviceInterface> device,
                TimeoutMillis default_timeout_msec);

  // This class is neither copyable nor movable.
  UsbMlCommands(const UsbMlCommands&) = delete;
  UsbMlCommands& operator=(const UsbMlCommands&) = delete;

  ~UsbMlCommands() override;

  // Detaches from application mode, then closes the device with graceful port
  // reset. The object becomes closed upon successful return.
  // Note the interface number and timeout for detachment have to come from
  // device configuration or discovered through parsing the interface
  // descriptors.
  util::Status DfuDetach(int interface_number, uint16_t timeout_msec);

  // Reads 32-bit CSR from device.
  util::StatusOr<Register32> ReadRegister32(uint32_t offset);

  // Reads 64-bit CSR from device.
  util::StatusOr<Register64> ReadRegister64(uint32_t offset);

  // Writes 32-bit CSR to device.
  util::Status WriteRegister32(uint32_t offset, Register32 value);

  // Writes 64-bit CSR to device.
  util::Status WriteRegister64(uint32_t offset, Register64 value);

  // Writes header to device, through the single bulk out endpoint. This
  // function is only meaningful if the device is in single bulk out endpoint
  // mode.
  util::Status WriteHeader(DescriptorTag tag, uint32_t length);

  // Prepares header to be sent to device. This function
  // is only meaningful if the device is in single bulk out endpoint mode.
  std::vector<uint8_t> PrepareHeader(DescriptorTag tag, uint32_t length);

  // Asynchrounously reads event from device.
  util::Status AsyncReadEvent(const EventInDone& callback);

  // Asynchrounously read interrupt from device.
  util::Status AsyncReadInterrupt(const InterruptInDone& callback);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_USB_USB_ML_COMMANDS_H_
