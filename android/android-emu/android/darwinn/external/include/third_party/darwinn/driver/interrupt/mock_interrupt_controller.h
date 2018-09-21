#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_MOCK_INTERRUPT_CONTROLLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_MOCK_INTERRUPT_CONTROLLER_H_

#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Mock interrupt controller used for driver testing.
class MockInterruptController : public InterruptControllerInterface {
 public:
  MockInterruptController(int num_interrupts)
      : InterruptControllerInterface(num_interrupts) {}
  ~MockInterruptController() override = default;

  MOCK_METHOD0(EnableInterrupts, util::Status());
  MOCK_METHOD0(DisableInterrupts, util::Status());
  MOCK_METHOD1(ClearInterruptStatus, util::Status(int));
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_MOCK_INTERRUPT_CONTROLLER_H_
