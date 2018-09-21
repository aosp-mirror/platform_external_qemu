#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_DUMMY_INTERRUPT_CONTROLLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_DUMMY_INTERRUPT_CONTROLLER_H_

#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Dummy class that does nothing upon interrupt related requests.
class DummyInterruptController : public InterruptControllerInterface {
 public:
  explicit DummyInterruptController(int num_interrupts)
      : InterruptControllerInterface(num_interrupts) {}

  // This class is neither copyable nor movable.
  DummyInterruptController(const DummyInterruptController&) = delete;
  DummyInterruptController& operator=(const DummyInterruptController&) = delete;

  ~DummyInterruptController() = default;

  util::Status EnableInterrupts() override {
    return util::Status();  // OK
  }

  util::Status DisableInterrupts() override {
    return util::Status();  // OK
  }

  util::Status ClearInterruptStatus(int id) override {
    return util::Status();  // OK
  }
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_DUMMY_INTERRUPT_CONTROLLER_H_
