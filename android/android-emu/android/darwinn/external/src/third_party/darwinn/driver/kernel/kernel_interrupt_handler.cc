#include "third_party/darwinn/driver/kernel/kernel_interrupt_handler.h"

#include <string>
#include <utility>

#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
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

KernelInterruptHandler::KernelInterruptHandler(const std::string& device_path)
    : event_handler_(device_path, DW_INTERRUPT_COUNT) {}

util::Status KernelInterruptHandler::Open() { return event_handler_.Open(); }

util::Status KernelInterruptHandler::Close(bool in_error) {
  return event_handler_.Close();
}

util::Status KernelInterruptHandler::Register(Interrupt interrupt,
                                              Handler handler) {
  return event_handler_.RegisterEvent(interrupt, std::move(handler));
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
