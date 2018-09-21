#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_CONTROLLER_INTERFACE_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_CONTROLLER_INTERFACE_H_

#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Interface for enabling/disabling interrupts, and clearing interrupt status.
class InterruptControllerInterface {
 public:
  explicit InterruptControllerInterface(int num_interrupts)
      : num_interrupts_(num_interrupts) {}

  // This class is neither copyable nor movable.
  InterruptControllerInterface(const InterruptControllerInterface&) = delete;
  InterruptControllerInterface& operator=(const InterruptControllerInterface&) =
      delete;

  virtual ~InterruptControllerInterface() = default;

  // Enable/disables interrupts.
  virtual util::Status EnableInterrupts() = 0;
  virtual util::Status DisableInterrupts() = 0;

  // Clears interrupt status register to notify that host has received the
  // interrupt.
  virtual util::Status ClearInterruptStatus(int id) = 0;
  util::Status ClearInterruptStatus() { return ClearInterruptStatus(/*id=*/0); }

  // Returns number of interrupts controlled by this interface.
  int NumInterrupts() const { return num_interrupts_; }

 private:
  // Number of interrupts enabled/disabled/cleared by this interface.
  const int num_interrupts_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_CONTROLLER_INTERFACE_H_
