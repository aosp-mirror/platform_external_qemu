#ifndef THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_INTERRUPT_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_INTERRUPT_HANDLER_H_

#include <string>

#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
#include "third_party/darwinn/driver/kernel/kernel_event_handler.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Kernel implementation of the interrupt handler interface.
class KernelInterruptHandler : public InterruptHandler {
 public:
  // Default close to avoid name hiding.
  using InterruptHandler::Close;

  explicit KernelInterruptHandler(const std::string& device_path);
  ~KernelInterruptHandler() override = default;

  util::Status Open() override;
  util::Status Close(bool in_error) override;
  util::Status Register(Interrupt interrupt, Handler handler) override;

 private:
  // Backing event handler.
  KernelEventHandler event_handler_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_INTERRUPT_HANDLER_H_
