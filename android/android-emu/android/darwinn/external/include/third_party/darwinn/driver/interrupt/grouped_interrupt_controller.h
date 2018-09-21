#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_GROUPED_INTERRUPT_CONTROLLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_GROUPED_INTERRUPT_CONTROLLER_H_

#include <memory>
#include <vector>

#include "third_party/darwinn/driver/interrupt/interrupt_controller.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Helper class for enabling/disabling interrupts, and clearing interrupt
// status.
class GroupedInterruptController : public InterruptControllerInterface {
 public:
  // "interrupt_controllers" will be empty after construction.
  explicit GroupedInterruptController(
      std::vector<std::unique_ptr<InterruptControllerInterface>>*
          interrupt_controllers);

  // This class is neither copyable nor movable.
  GroupedInterruptController(const GroupedInterruptController&) = delete;
  GroupedInterruptController& operator=(const GroupedInterruptController&) =
      delete;

  ~GroupedInterruptController() = default;

  // Enable/disables interrupts.
  util::Status EnableInterrupts() override;
  util::Status DisableInterrupts() override;

  // Clears interrupt status register to notify that host has received the
  // interrupt.
  util::Status ClearInterruptStatus(int id) override;

 private:
  // CSR offsets.
  const std::vector<std::unique_ptr<InterruptControllerInterface>>
      interrupt_controllers_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_GROUPED_INTERRUPT_CONTROLLER_H_
