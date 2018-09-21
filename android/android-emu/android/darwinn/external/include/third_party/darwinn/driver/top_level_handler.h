#ifndef THIRD_PARTY_DARWINN_DRIVER_TOP_LEVEL_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_TOP_LEVEL_HANDLER_H_

#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Interface for handling resets.
class TopLevelHandler {
 public:
  virtual ~TopLevelHandler() = default;

  // Opens reset handler.
  virtual util::Status Open() {
    return util::Status();  // OK
  }

  // Closes reset handler.
  virtual util::Status Close() {
    return util::Status();  // OK
  }

  // Quits from reset state.
  virtual util::Status QuitReset() {
    return util::Status();  // OK
  }

  // Goes into reset state.
  virtual util::Status EnableReset() {
    return util::Status();  // OK
  }

  // Enables/disables software clock gating - implementation must be idempotent.
  virtual util::Status EnableSoftwareClockGate() {
    return util::Status();  // OK
  }
  virtual util::Status DisableSoftwareClockGate() {
    return util::Status();  // OK
  }

  // Enables/disables hardware clock gating - implementation must be idempotent.
  virtual util::Status EnableHardwareClockGate() {
    return util::Status();  // OK
  }
  virtual util::Status DisableHardwareClockGate() {
    return util::Status();  // OK
  }
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_TOP_LEVEL_HANDLER_H_
