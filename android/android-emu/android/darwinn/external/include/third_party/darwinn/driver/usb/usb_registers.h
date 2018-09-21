#ifndef THIRD_PARTY_DARWINN_DRIVER_USB_USB_REGISTERS_H_
#define THIRD_PARTY_DARWINN_DRIVER_USB_USB_REGISTERS_H_

#include <memory>

#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/driver/usb/usb_ml_commands.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

class UsbRegisters : public Registers {
 public:
  UsbRegisters() = default;
  // This version without argument should never be used. Use the version with
  // pointer to USB deviec instead.
  util::Status Open() override;
  // Enable the USB register object the actually communicate with the underlying
  // device.
  util::Status Open(UsbMlCommands* usb_device);
  util::Status Close() override;

  // Accesses 64-bit registers.
  util::Status Write(uint64 offset, uint64 value) override;
  util::StatusOr<uint64> Read(uint64 offset) override;

  // Accesses 32-bit registers.
  util::Status Write32(uint64 offset, uint32 value) override;
  util::StatusOr<uint32> Read32(uint64 offset) override;

 private:
  // Underlying device.
  UsbMlCommands* usb_device_{nullptr};
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_USB_USB_REGISTERS_H_
