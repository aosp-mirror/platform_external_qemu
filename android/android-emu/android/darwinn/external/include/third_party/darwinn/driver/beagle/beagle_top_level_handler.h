#ifndef THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_TOP_LEVEL_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_TOP_LEVEL_HANDLER_H_

#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/driver/config/cb_bridge_csr_offsets.h"
#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/hib_user_csr_offsets.h"
#include "third_party/darwinn/driver/config/misc_csr_offsets.h"
#include "third_party/darwinn/driver/config/scalar_core_csr_offsets.h"
#include "third_party/darwinn/driver/config/scu_csr_offsets.h"
#include "third_party/darwinn/driver/config/tile_config_csr_offsets.h"
#include "third_party/darwinn/driver/config/tile_csr_offsets.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Handles beagle resets. Only used in remote driver as this will be handled in
// kernel space in kernel driver.
class BeagleTopLevelHandler : public TopLevelHandler {
 public:
  BeagleTopLevelHandler(const config::ChipConfig& config, Registers* registers,
                        bool use_usb, api::PerformanceExpectation performance);
  ~BeagleTopLevelHandler() override = default;

  // Implements ResetHandler interface.
  util::Status Open() override;
  util::Status QuitReset() override;
  util::Status EnableReset() override;
  util::Status EnableHardwareClockGate() override;
  util::Status DisableHardwareClockGate() override;

 private:
  // CSR offsets.
  const config::CbBridgeCsrOffsets& cb_bridge_offsets_;
  const config::HibUserCsrOffsets& hib_user_offsets_;
  const config::MiscCsrOffsets& misc_offsets_;
  const config::ScuCsrOffsets& reset_offsets_;
  const config::ScalarCoreCsrOffsets& scalar_core_offsets_;
  const config::TileConfigCsrOffsets& tile_config_offsets_;
  const config::TileCsrOffsets& tile_offsets_;

  // CSR interface.
  Registers* const registers_;

  // Select clock combinations for performance.
  const api::PerformanceExpectation performance_;

  // Whether USB is used for Beagle.
  const bool use_usb_;

  // True if clock gated. Starts at non-clock gated mode.
  bool software_clock_gated_{false};
  bool hardware_clock_gated_{false};
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_TOP_LEVEL_HANDLER_H_
