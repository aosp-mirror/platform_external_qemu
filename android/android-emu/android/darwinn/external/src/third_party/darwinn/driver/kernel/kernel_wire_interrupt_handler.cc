#include "third_party/darwinn/driver/kernel/kernel_wire_interrupt_handler.h"

#include <string>

#include "third_party/darwinn/driver/config/wire_csr_offsets.h"
#include "third_party/darwinn/driver/interrupt/wire_interrupt_handler.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/cleanup.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

KernelWireInterruptHandler::KernelWireInterruptHandler(
    Registers* registers, const config::WireCsrOffsets& wire_csr_offsets,
    const std::string& device_path, int num_wires)
    : wire_handler_(registers, wire_csr_offsets, num_wires),
      event_handler_(device_path, num_wires),
      num_wires_(num_wires) {}

util::Status KernelWireInterruptHandler::Open() {
  RETURN_IF_ERROR(wire_handler_.Open());
  auto wire_handler_closer = gtl::MakeCleanup(
      [this]() NO_THREAD_SAFETY_ANALYSIS { CHECK_OK(wire_handler_.Close()); });

  RETURN_IF_ERROR(event_handler_.Open());
  auto event_handler_closer = gtl::MakeCleanup(
      [this]() NO_THREAD_SAFETY_ANALYSIS { CHECK_OK(event_handler_.Close()); });

  for (int wire = 0; wire < num_wires_; ++wire) {
    RETURN_IF_ERROR(event_handler_.RegisterEvent(wire, [this, wire]() {
      wire_handler_.InvokeAllPendingInterrupts(wire);
    }));
  }

  // All good. Release cleanup functions.
  wire_handler_closer.release();
  event_handler_closer.release();

  return util::Status();  // OK
}

util::Status KernelWireInterruptHandler::Close(bool in_error) {
  util::Status status;
  status.Update(event_handler_.Close());
  status.Update(wire_handler_.Close());
  return status;
}

util::Status KernelWireInterruptHandler::Register(Interrupt interrupt,
                                                  Handler handler) {
  return wire_handler_.Register(interrupt, std::move(handler));
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
