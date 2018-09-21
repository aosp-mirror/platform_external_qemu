#include "third_party/darwinn/driver/interrupt/interrupt_controller.h"

#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {

InterruptController::InterruptController(
    const config::InterruptCsrOffsets& csr_offsets, Registers* registers,
    int num_interrupts)
    : InterruptControllerInterface(num_interrupts),
      csr_offsets_(csr_offsets),
      registers_(registers) {
  CHECK(registers != nullptr);
}

util::Status InterruptController::EnableInterrupts() {
  const uint64 enable_all = (1ULL << NumInterrupts()) - 1;
  return registers_->Write(csr_offsets_.control, enable_all);
}

util::Status InterruptController::DisableInterrupts() {
  constexpr uint64 kDisableAll = 0;
  return registers_->Write(csr_offsets_.control, kDisableAll);
}

util::Status InterruptController::ClearInterruptStatus(int id) {
  // Interrupt status register has W0C policy meaning that writing 0 clears the
  // bit, while writing 1 does not have any effect.
  const uint64 clear_bit = ~(1ULL << id);

  uint64 value = (1ULL << NumInterrupts()) - 1;
  value &= clear_bit;
  return registers_->Write(csr_offsets_.status, value);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
