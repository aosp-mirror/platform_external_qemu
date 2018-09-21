#ifndef THIRD_PARTY_DARWINN_DRIVER_RUN_CONTROLLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_RUN_CONTROLLER_H_

#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/scalar_core_csr_offsets.h"
#include "third_party/darwinn/driver/config/tile_config_csr_offsets.h"
#include "third_party/darwinn/driver/config/tile_csr_offsets.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Controls run states of both scalar core and tiles.
class RunController {
 public:
  RunController(const config::ChipConfig& config, Registers* registers);
  virtual ~RunController() = default;

  // This class is neither copyable nor movable.
  RunController(const RunController&) = delete;
  RunController& operator=(const RunController&) = delete;

  // Performs run control.
  virtual util::Status DoRunControl(RunControl run_state);

 private:
  // CSR offsets.
  const config::ScalarCoreCsrOffsets& scalar_core_csr_offsets_;
  const config::TileConfigCsrOffsets& tile_config_csr_offsets_;
  const config::TileCsrOffsets& tile_csr_offsets_;

  // CSR interface.
  Registers* const registers_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_RUN_CONTROLLER_H_
