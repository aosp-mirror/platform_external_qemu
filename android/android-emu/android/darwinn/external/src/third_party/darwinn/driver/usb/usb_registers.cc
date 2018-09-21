#include "third_party/darwinn/driver/usb/usb_registers.h"

#include "third_party/darwinn/driver/usb/usb_ml_commands.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::Status UsbRegisters::Open() {
  return util::UnimplementedError("USB register open without attached device");
}

util::Status UsbRegisters::Open(UsbMlCommands* usb_device) {
  usb_device_ = usb_device;
  return util::Status();  // OK
}

util::Status UsbRegisters::Close() {
  usb_device_ = nullptr;
  return util::Status();  // OK
}

util::Status UsbRegisters::Write(uint64 offset, uint64 value) {
  if (usb_device_) {
    return usb_device_->WriteRegister64(static_cast<uint32>(offset), value);
  }
  return util::FailedPreconditionError(
      "USB register write without attached device");
}

util::StatusOr<uint64> UsbRegisters::Read(uint64 offset) {
  if (usb_device_) {
    return usb_device_->ReadRegister64(static_cast<uint32>(offset));
  }
  return util::FailedPreconditionError(
      "USB register read without attached device");
}

util::Status UsbRegisters::Write32(uint64 offset, uint32 value) {
  if (usb_device_) {
    return usb_device_->WriteRegister32(static_cast<uint32>(offset), value);
  }
  return util::FailedPreconditionError(
      "USB register write32 without attached device");
}

util::StatusOr<uint32> UsbRegisters::Read32(uint64 offset) {
  if (usb_device_) {
    return usb_device_->ReadRegister32(static_cast<uint32>(offset));
  }
  return util::FailedPreconditionError(
      "USB register read32 without attached device");
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
