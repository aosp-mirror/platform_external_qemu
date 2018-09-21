#ifndef THIRD_PARTY_DARWINN_DRIVER_ABROLHOS_ABROLHOS_TOP_LEVEL_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_ABROLHOS_ABROLHOS_TOP_LEVEL_HANDLER_H_

#include "third_party/darwinn/driver/config/aon_reset_csr_offsets.h"
#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/hib_user_csr_offsets.h"
#include "third_party/darwinn/driver/config/scalar_core_csr_offsets.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Handles abrolhos resets. Only used in remote driver as this will be handled
// in kernel space in kernel driver.
class AbrolhosTopLevelHandler : public TopLevelHandler {
 public:
  AbrolhosTopLevelHandler(const config::ChipConfig& config,
                          Registers* registers);
  ~AbrolhosTopLevelHandler() override = default;

  // Quits from reset state.
  util::Status QuitReset() override;

  // Goes into reset state.
  util::Status EnableReset() override;

 private:
  // CSR offsets.
  const config::AonResetCsrOffsets& reset_offsets_;
  const config::HibUserCsrOffsets& hib_user_offsets_;
  const config::ScalarCoreCsrOffsets scalar_core_offsets_;

  // CSR interface.
  Registers* const registers_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_ABROLHOS_ABROLHOS_TOP_LEVEL_HANDLER_H_
