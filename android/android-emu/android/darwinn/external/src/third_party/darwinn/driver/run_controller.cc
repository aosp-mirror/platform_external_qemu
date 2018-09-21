#include "third_party/darwinn/driver/run_controller.h"

#include "third_party/darwinn/driver/config/common_csr_helper.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status_macros.h"

namespace platforms {
namespace darwinn {
namespace driver {

RunController::RunController(const config::ChipConfig& config,
                             Registers* registers)
    : scalar_core_csr_offsets_(config.GetScalarCoreCsrOffsets()),
      tile_config_csr_offsets_(config.GetTileConfigCsrOffsets()),
      tile_csr_offsets_(config.GetTileCsrOffsets()),
      registers_(registers) {
  CHECK(registers != nullptr);
}

util::Status RunController::DoRunControl(RunControl run_state) {
  const uint64 run_state_value = static_cast<uint64>(run_state);
  RETURN_IF_ERROR(registers_->Write(
      scalar_core_csr_offsets_.scalarCoreRunControl, run_state_value));
  RETURN_IF_ERROR(registers_->Write(
      scalar_core_csr_offsets_.avDataPopRunControl, run_state_value));
  RETURN_IF_ERROR(registers_->Write(
      scalar_core_csr_offsets_.parameterPopRunControl, run_state_value));
  RETURN_IF_ERROR(registers_->Write(scalar_core_csr_offsets_.infeedRunControl,
                                    run_state_value));
  RETURN_IF_ERROR(registers_->Write(scalar_core_csr_offsets_.outfeedRunControl,
                                    run_state_value));

  // TODO(jasonjk): helper uses 7-bits as defined by CSR. Extract bitwidth
  // automatically for different chips.
  config::registers::TileConfig<7> helper;
  helper.set_broadcast();
  RETURN_IF_ERROR(
      registers_->Write(tile_config_csr_offsets_.tileconfig0, helper.raw()));

  // Wait until tileconfig0 is set correctly. Subsequent writes are going to
  // tiles, but hardware does not guarantee correct ordering with previous
  // write.
  RETURN_IF_ERROR(
      registers_->Poll(tile_config_csr_offsets_.tileconfig0, helper.raw()));

  RETURN_IF_ERROR(
      registers_->Write(tile_csr_offsets_.opRunControl, run_state_value));
  RETURN_IF_ERROR(
      registers_->Write(tile_csr_offsets_.meshBus0RunControl, run_state_value));
  RETURN_IF_ERROR(
      registers_->Write(tile_csr_offsets_.meshBus1RunControl, run_state_value));
  RETURN_IF_ERROR(
      registers_->Write(tile_csr_offsets_.meshBus2RunControl, run_state_value));
  RETURN_IF_ERROR(
      registers_->Write(tile_csr_offsets_.meshBus3RunControl, run_state_value));
  RETURN_IF_ERROR(registers_->Write(
      tile_csr_offsets_.ringBusConsumer0RunControl, run_state_value));
  RETURN_IF_ERROR(registers_->Write(
      tile_csr_offsets_.ringBusConsumer1RunControl, run_state_value));
  RETURN_IF_ERROR(registers_->Write(tile_csr_offsets_.ringBusProducerRunControl,
                                    run_state_value));
  RETURN_IF_ERROR(registers_->Write(tile_csr_offsets_.narrowToWideRunControl,
                                    run_state_value));
  RETURN_IF_ERROR(registers_->Write(tile_csr_offsets_.wideToNarrowRunControl,
                                    run_state_value));
  if (tile_csr_offsets_.narrowToNarrowRunControl != static_cast<uint64>(-1)) {
    RETURN_IF_ERROR(registers_->Write(
        tile_csr_offsets_.narrowToNarrowRunControl, run_state_value));
  }

  return util::Status();  // OK
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
