#include "third_party/darwinn/driver/interrupt/grouped_interrupt_controller.h"

#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

GroupedInterruptController::GroupedInterruptController(
    std::vector<std::unique_ptr<InterruptControllerInterface>>*
        interrupt_controllers)
    : InterruptControllerInterface(interrupt_controllers->size()),
      interrupt_controllers_([interrupt_controllers]() {
        CHECK(interrupt_controllers != nullptr);
        return std::move(*interrupt_controllers);
      }()) {}

util::Status GroupedInterruptController::EnableInterrupts() {
  for (auto& interrupt_controller : interrupt_controllers_) {
    RETURN_IF_ERROR(interrupt_controller->EnableInterrupts());
  }
  return util::Status();  // OK
}

util::Status GroupedInterruptController::DisableInterrupts() {
  for (auto& interrupt_controller : interrupt_controllers_) {
    RETURN_IF_ERROR(interrupt_controller->DisableInterrupts());
  }
  return util::Status();  // OK
}

util::Status GroupedInterruptController::ClearInterruptStatus(int id) {
  if (id < interrupt_controllers_.size()) {
    return interrupt_controllers_[id]->ClearInterruptStatus();
  }
  return util::FailedPreconditionError(
      StringPrintf("Unknown interrupt id: %d", id));
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
