#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_TOP_LEVEL_INTERRUPT_MANAGER_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_TOP_LEVEL_INTERRUPT_MANAGER_H_

#include <memory>

#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Base class for top level interrupt management.
class TopLevelInterruptManager {
 public:
  explicit TopLevelInterruptManager(
      std::unique_ptr<InterruptControllerInterface> interrupt_controller)
      : interrupt_controller_(std::move(interrupt_controller)) {}
  virtual ~TopLevelInterruptManager() = default;

  // Opens/closes the controller.
  virtual util::Status Open() {
    return util::Status();  // OK
  }
  virtual util::Status Close() {
    return util::Status();  // OK
  }

  // Enable/disables interrupts.
  util::Status EnableInterrupts();
  util::Status DisableInterrupts();

  // Handles interrupt.
  util::Status HandleInterrupt(int id);

  // Returns number of top level interrupts.
  int NumInterrupts() const { return interrupt_controller_->NumInterrupts(); }

 protected:
  // Actually enables/disables interrupts, which are system-specific.
  virtual util::Status DoEnableInterrupts() {
    return util::Status();  // OK
  }
  virtual util::Status DoDisableInterrupts() {
    return util::Status();  // OK
  }

  // Actually handles interrupts, which are system-specific.
  virtual util::Status DoHandleInterrupt(int id) {
    return util::Status();  // OK
  }

 private:
  // Interrupt controller.
  std::unique_ptr<InterruptControllerInterface> interrupt_controller_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_TOP_LEVEL_INTERRUPT_MANAGER_H_
