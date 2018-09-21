#ifndef THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_WIRE_INTERRUPT_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_WIRE_INTERRUPT_HANDLER_H_

#include <mutex>  // NOLINT
#include <string>

#include "third_party/darwinn/driver/config/wire_csr_offsets.h"
#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
#include "third_party/darwinn/driver/interrupt/wire_interrupt_handler.h"
#include "third_party/darwinn/driver/kernel/kernel_event_handler.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Wire Interrupt handler implementation that reads and processes the pending
// bit array on a single wire interrupt in userspace.
class KernelWireInterruptHandler : public InterruptHandler {
 public:
  // Default close to avoid name hiding.
  using InterruptHandler::Close;

  KernelWireInterruptHandler(Registers* registers,
                             const config::WireCsrOffsets& wire_csr_offsets,
                             const std::string& device_path, int num_wires);
  ~KernelWireInterruptHandler() override = default;

  // This class is neither copyable nor movable.
  KernelWireInterruptHandler(const KernelWireInterruptHandler&) = delete;
  KernelWireInterruptHandler& operator=(const KernelWireInterruptHandler&) =
      delete;

  util::Status Open() override;
  util::Status Close(bool in_error) override;
  util::Status Register(Interrupt interrupt, Handler handler) override;

 private:
  // Backing wire interrupt handler.
  WireInterruptHandler wire_handler_;

  // KernelEventHandler
  KernelEventHandler event_handler_;

  // Number of wires.
  const int num_wires_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_WIRE_INTERRUPT_HANDLER_H_
