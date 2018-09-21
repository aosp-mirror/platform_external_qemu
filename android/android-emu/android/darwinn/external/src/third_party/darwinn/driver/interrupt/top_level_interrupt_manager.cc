#include "third_party/darwinn/driver/interrupt/top_level_interrupt_manager.h"

#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::Status TopLevelInterruptManager::EnableInterrupts() {
  RETURN_IF_ERROR(interrupt_controller_->EnableInterrupts());
  return DoEnableInterrupts();
}

util::Status TopLevelInterruptManager::DisableInterrupts() {
  RETURN_IF_ERROR(interrupt_controller_->DisableInterrupts());
  return DoDisableInterrupts();
}

util::Status TopLevelInterruptManager::HandleInterrupt(int id) {
  RETURN_IF_ERROR(DoHandleInterrupt(id));
  return interrupt_controller_->ClearInterruptStatus(id);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
