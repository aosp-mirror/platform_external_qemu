#include "third_party/darwinn/driver/usb/usb_ml_commands.h"

#include <inttypes.h>
#include <utility>

#include "third_party/darwinn/driver/usb/usb_device_interface.h"
#include "third_party/darwinn/driver/usb/usb_standard_commands.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

namespace {

constexpr size_t kRegister64RawDataSizeInBytes = 8;
constexpr size_t kRegister32RawDataSizeInBytes = 4;
constexpr size_t kInterruptRawDataSizeInBytes = 4;
constexpr size_t kEventRawDataSizeInBytes = 16;
constexpr size_t kPacketHeaderRawDataSizeInBytes = 8;

}  // namespace

const uint8_t UsbMlCommands::kSingleBulkOutEndpoint;
const uint8_t UsbMlCommands::kInstructionsEndpoint;
const uint8_t UsbMlCommands::kInputActivationsEndpoint;
const uint8_t UsbMlCommands::kParametersEndpoint;
const uint8_t UsbMlCommands::kBulkInEndpoint;
const uint8_t UsbMlCommands::kEventInEndpoint;
const uint8_t UsbMlCommands::kInterruptInEndpoint;

UsbMlCommands::UsbMlCommands(std::unique_ptr<UsbDeviceInterface> device,
                             TimeoutMillis default_timeout_msec)
    : UsbStandardCommands(std::move(device), default_timeout_msec) {
  VLOG(10) << __func__;
}

UsbMlCommands::~UsbMlCommands() { VLOG(10) << __func__; }

util::Status UsbMlCommands::DfuDetach(int interface_number,
                                      uint16_t timeout_msec) {
  VLOG(10) << StringPrintf("%s interface %d, timeout %u msec", __func__,
                           interface_number, timeout_msec);

  SetupPacket command{
      // Request type (00100001b).
      ComposeUsbRequestType(CommandDataDir::kHostToDevice, CommandType::kClass,
                            CommandRecipient::kInterface),

      // Request id for DFU detach command
      0,

      // Timeout in milliseconds.
      timeout_msec,

      // Interface number.
      static_cast<uint16_t>(interface_number),

      // Data length.
      0};

  RETURN_IF_ERROR(SendControlCommand(command, __func__));

  return Close(CloseAction::kGracefulPortReset);
}

util::StatusOr<UsbMlCommands::Register32> UsbMlCommands::ReadRegister32(
    uint32_t offset) {
  VLOG(10) << StringPrintf("%s offset 0x%x", __func__, offset);

  Register32 result;
  SetupPacket command{
      // Request type (0xC0).
      ComposeUsbRequestType(CommandDataDir::kDeviceToHost, CommandType::kVendor,
                            CommandRecipient::kDevice),

      // Request id for read CSR 32-bit command.
      1,

      // Low 16-bit of the offset.
      static_cast<uint16_t>(offset & 0xffff),

      // High 16-bit of the offset.
      static_cast<uint16_t>(offset >> 16),

      // Data length.
      kRegister32RawDataSizeInBytes};

  size_t num_bytes_transferred = 0;
  RETURN_IF_ERROR(SendControlCommandWithDataIn(
      command,
      MutableBuffer(reinterpret_cast<uint8_t*>(&result), sizeof(result)),
      &num_bytes_transferred, __func__));

  if (num_bytes_transferred != sizeof(result)) {
    return util::UnknownError("Invalid register data");
  }

  VLOG(7) << StringPrintf("%s [0x%X] == 0x%" PRIX32, __func__, offset, result);

  return result;
}

util::StatusOr<UsbMlCommands::Register64> UsbMlCommands::ReadRegister64(
    uint32_t offset) {
  VLOG(10) << StringPrintf("%s offset 0x%x", __func__, offset);

  Register64 result;
  SetupPacket command{
      // Request type (0xC0).
      ComposeUsbRequestType(CommandDataDir::kDeviceToHost, CommandType::kVendor,
                            CommandRecipient::kDevice),

      // Request id for read CSR 64-bit command.
      0,

      // Low 16-bit of the offset.
      static_cast<uint16_t>(offset & 0xffff),

      // High 16-bit of the offset.
      static_cast<uint16_t>(offset >> 16),

      // Data length.
      kRegister64RawDataSizeInBytes};

  size_t num_bytes_transferred = 0;
  RETURN_IF_ERROR(SendControlCommandWithDataIn(
      command,
      MutableBuffer(reinterpret_cast<uint8_t*>(&result), sizeof(result)),
      &num_bytes_transferred, __func__));

  if (num_bytes_transferred != sizeof(result)) {
    return util::UnknownError("Invalid register data");
  }

  VLOG(7) << StringPrintf("%s [0x%X] == 0x%" PRIX64, __func__, offset, result);

  return result;
}

util::Status UsbMlCommands::WriteRegister32(uint32_t offset, Register32 value) {
  VLOG(7) << StringPrintf("%s [0x%X] := 0x%" PRIX32, __func__, offset, value);

  SetupPacket command{
      // Request type (10100001b).
      ComposeUsbRequestType(CommandDataDir::kHostToDevice, CommandType::kVendor,
                            CommandRecipient::kDevice),

      // Request id for write CSR 32-bit command.
      1,

      // Low 16-bit of the offset.
      static_cast<uint16_t>(offset & 0xffff),

      // High 16-bit of the offset.
      static_cast<uint16_t>(offset >> 16),

      // Data length.
      sizeof(value)};

  return SendControlCommandWithDataOut(
      command, ConstBuffer(reinterpret_cast<uint8_t*>(&value), sizeof(value)),
      __func__);
}

util::Status UsbMlCommands::WriteRegister64(uint32_t offset, Register64 value) {
  VLOG(7) << StringPrintf("%s [0x%X] := 0x%" PRIX64, __func__, offset, value);

  SetupPacket command{
      // Request type (10100001b).
      ComposeUsbRequestType(CommandDataDir::kHostToDevice, CommandType::kVendor,
                            CommandRecipient::kDevice),

      // Request id for write CSR 64-bit command.
      0,

      // Low 16-bit of the offset.
      static_cast<uint16_t>(offset & 0xffff),

      // High 16-bit of the offset.
      static_cast<uint16_t>(offset >> 16),

      // Data length.
      sizeof(value)};

  return SendControlCommandWithDataOut(
      command, ConstBuffer(reinterpret_cast<uint8_t*>(&value), sizeof(value)),
      __func__);
}

std::vector<uint8_t> UsbMlCommands::PrepareHeader(DescriptorTag tag,
                                                  uint32_t length) {
  // Write 8-byte-long tag.
  constexpr size_t kLengthSizeInBytes = 4;

  // length must be 4-byte long, otherwise we could be sending wrong data.
  CHECK_EQ(sizeof(length), kLengthSizeInBytes);

  std::vector<uint8_t> header_packet(kPacketHeaderRawDataSizeInBytes);
  memcpy(header_packet.data(), &length, kLengthSizeInBytes);
  *(header_packet.data() + kLengthSizeInBytes) =
      (static_cast<uint8_t>(tag) & 0xF);

  VLOG(10) << StringPrintf(
      "%s ep %d: header hex %2x %2x %2x %2x - %2x %2x %2x %2x", __func__,
      kSingleBulkOutEndpoint, header_packet[0], header_packet[1],
      header_packet[2], header_packet[3], header_packet[4], header_packet[5],
      header_packet[6], header_packet[7]);

  return header_packet;
}

util::Status UsbMlCommands::WriteHeader(DescriptorTag tag, uint32_t length) {
  std::vector<uint8_t> header_packet = PrepareHeader(tag, length);
  return BulkOutTransfer(kSingleBulkOutEndpoint, ConstBuffer(header_packet),
                         __func__);
}

util::Status UsbMlCommands::AsyncReadEvent(const EventInDone& callback) {
  auto event_data =
      std::make_shared<std::vector<uint8_t>>(kEventRawDataSizeInBytes);
  CHECK(event_data);
  return AsyncBulkInTransfer(
      kEventInEndpoint, MutableBuffer(event_data->data(), event_data->size()),
      [event_data, callback](util::Status status,
                             size_t num_bytes_transferred) {
        EventDescriptor event_descriptor;
        if (!status.ok()) {
          callback(status, event_descriptor);
          return;
        }
        if (num_bytes_transferred != kEventRawDataSizeInBytes) {
          VLOG(1) << StringPrintf("%s data lost. calling with empty event",
                                  __func__);
          callback(util::DataLossError(__func__), event_descriptor);
          return;
        }
        memcpy(&event_descriptor.offset, event_data->data(),
               sizeof(event_descriptor.offset));
        constexpr size_t kAddressSizeInBytes = 8;
        constexpr size_t kLengthSizeInBytes = 4;
        memcpy(&event_descriptor.length,
               event_data->data() + kAddressSizeInBytes, kLengthSizeInBytes);
        event_descriptor.tag = static_cast<DescriptorTag>(
            *(event_data->data() + kAddressSizeInBytes + kLengthSizeInBytes) &
            0xF);

        VLOG(7) << StringPrintf(
            "%s tag:%d, offset:0x%" PRIX64 ", length %u", __func__,
            static_cast<int>(event_descriptor.tag), event_descriptor.offset,
            event_descriptor.length);

        // OK.
        callback(status, event_descriptor);

        VLOG(7) << StringPrintf("%s callback done", __func__);
      },
      __func__);
}

util::Status UsbMlCommands::AsyncReadInterrupt(
    const InterruptInDone& callback) {
  auto interrupt_data =
      std::make_shared<std::vector<uint8_t>>(kInterruptRawDataSizeInBytes);
  CHECK(interrupt_data);
  return AsyncInterruptInTransfer(
      kInterruptInEndpoint,
      MutableBuffer(interrupt_data->data(), interrupt_data->size()),
      [interrupt_data, callback](util::Status status,
                                 size_t num_bytes_transferred) {
        InterruptInfo interrupt_info = {0};
        if (!status.ok()) {
          callback(status, interrupt_info);
          return;
        }
        if (num_bytes_transferred != kInterruptRawDataSizeInBytes) {
          callback(util::DataLossError(__func__), interrupt_info);
          return;
        }
        memcpy(&interrupt_info.raw_data, interrupt_data->data(),
               sizeof(interrupt_info.raw_data));
        VLOG(7) << StringPrintf("%s raw data 0x%X", __func__,
                                interrupt_info.raw_data);

        // OK.
        callback(status, interrupt_info);

        VLOG(7) << StringPrintf("%s callback done", __func__);
      },
      __func__);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
