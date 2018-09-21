#ifndef THIRD_PARTY_DARWINN_DRIVER_USB_USB_DFU_UTIL_H_
#define THIRD_PARTY_DARWINN_DRIVER_USB_USB_DFU_UTIL_H_

#include "third_party/darwinn/driver/usb/usb_device_interface.h"
#include "third_party/darwinn/driver/usb/usb_dfu_commands.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Performs DFU on device with specified firmware image.
util::Status UsbUpdateDfuDevice(UsbDfuCommands* dfu_device,
                                UsbDeviceInterface::ConstBuffer firmware_image,
                                bool skip_verify);

// TODO(b/77550491): remove this function, as it's only used by the remote
// interface.
// Tries to perform DFU on all USB devices of the same vendor and
// product ID.
util::Status UsbUpdateAllDfuDevices(UsbManager* usb_manager, uint16_t vendor_id,
                                    uint16_t product_id,
                                    const std::string& firmware_filename,
                                    bool skip_verify);

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_USB_USB_DFU_UTIL_H_
