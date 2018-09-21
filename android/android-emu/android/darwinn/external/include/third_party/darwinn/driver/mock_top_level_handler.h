#ifndef THIRD_PARTY_DARWINN_DRIVER_MOCK_TOP_LEVEL_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_MOCK_TOP_LEVEL_HANDLER_H_

#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Mock reset handler used for driver testing.
class MockTopLevelHandler : public TopLevelHandler {
 public:
  MockTopLevelHandler() = default;
  ~MockTopLevelHandler() override = default;

  MOCK_METHOD0(Open, util::Status());
  MOCK_METHOD0(Close, util::Status());
  MOCK_METHOD0(QuitReset, util::Status());
  MOCK_METHOD0(EnableReset, util::Status());
  MOCK_METHOD0(EnableSoftwareClockGate, util::Status());
  MOCK_METHOD0(DisableSoftwareClockGate, util::Status());
  MOCK_METHOD0(EnableHardwareClockGate, util::Status());
  MOCK_METHOD0(DisableHardwareClockGate, util::Status());
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MOCK_TOP_LEVEL_HANDLER_H_
