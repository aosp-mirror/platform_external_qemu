#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_CONTROLLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_CONTROLLER_H_

#include "third_party/darwinn/driver/config/interrupt_csr_offsets.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Helper class for enabling/disabling interrupts, and clearing interrupt
// status.
class InterruptController : public InterruptControllerInterface {
 public:
  InterruptController(const config::InterruptCsrOffsets& csr_offsets,
                      Registers* registers, int num_interrupts = 1);

  // This class is neither copyable nor movable.
  InterruptController(const InterruptController&) = delete;
  InterruptController& operator=(const InterruptController&) = delete;

  ~InterruptController() = default;

  // Enable/disables interrupts.
  util::Status EnableInterrupts() override;
  util::Status DisableInterrupts() override;

  // Clears interrupt status register to notify that host has received the
  // interrupt.
  util::Status ClearInterruptStatus(int id) override;

 private:
  // CSR offsets.
  const config::InterruptCsrOffsets& csr_offsets_;

  // CSR interface.
  Registers* const registers_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_CONTROLLER_H_
