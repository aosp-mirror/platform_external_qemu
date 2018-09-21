#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_KELVIN_KELVIN_CHIP_CONFIG_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_KELVIN_KELVIN_CHIP_CONFIG_H_

#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/kelvin/kelvin_chip_structures.h"
#include "third_party/darwinn/driver/config/kelvin/kelvin_csr_offsets.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

// Kelvin-specific configuration.
class KelvinChipConfig : public ChipConfig {
 public:
  ~KelvinChipConfig() override = default;

  api::Chip GetChip() const override { return api::Chip::kKelvin; }

  // Extracts CSR offsets for various modules in DarwiNN.
  const HibKernelCsrOffsets& GetHibKernelCsrOffsets() const override {
    return kKelvinHibKernelCsrOffsets;
  }
  const HibUserCsrOffsets& GetHibUserCsrOffsets() const override {
    return kKelvinHibUserCsrOffsets;
  }
  const QueueCsrOffsets& GetInstructionQueueCsrOffsets() const override {
    return kKelvinInstructionQueueCsrOffsets;
  }
  const ScalarCoreCsrOffsets& GetScalarCoreCsrOffsets() const override {
    return kKelvinScalarCoreCsrOffsets;
  }
  const TileConfigCsrOffsets& GetTileConfigCsrOffsets() const override {
    return kKelvinTileConfigCsrOffsets;
  }
  const TileCsrOffsets& GetTileCsrOffsets() const override {
    return kKelvinTileCsrOffsets;
  }
  const InterruptCsrOffsets& GetScalarCoreInterruptCsrOffsets() const override {
    return kKelvinScHostIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetTopLevelInterruptCsrOffsets() const override {
    return kKelvinTopLevelIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetFatalErrorInterruptCsrOffsets() const override {
    return kKelvinFatalErrIntInterruptCsrOffsets;
  }

  // Extracts CSR offsets that supports specific functionality in DarwiNN.
  const WireCsrOffsets& GetWireCsrOffsets() const override {
    return kKelvinWireCsrOffsets;
  }

  // Extracts chip-specific constants in DarwiNN.
  const ChipStructures& GetChipStructures() const override {
    return kKelvinChipStructures;
  }

  // Extracts CSR offsets used by scalar core debugger in DarwiNN.
  const BreakpointCsrOffsets& GetScalarCoreBreakpointCsrOffsets()
      const override {
    return kKelvinScalarcoreBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreActivationTtuBreakpointCsrOffsets()
      const override {
    return kKelvinAvdatapopBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreInfeedTtuBreakpointCsrOffsets()
      const override {
    return kKelvinInfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreOutfeedTtuBreakpointCsrOffsets()
      const override {
    return kKelvinOutfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreParameterTtuBreakpointCsrOffsets()
      const override {
    return kKelvinParameterpopBreakpointCsrOffsets;
  }

  const RegisterFileCsrOffsets& GetScalarRegisterFileCsrOffsets()
      const override {
    return kKelvinScalarRegisterFileCsrOffsets;
  }
  const RegisterFileCsrOffsets& GetPredicateRegisterFileCsrOffsets()
      const override {
    return kKelvinPredicateRegisterFileCsrOffsets;
  }

  const MemoryCsrOffsets& GetScalarCoreMemoryCsrOffsets() const override {
    return kKelvinScmemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by tile debugger in DarwiNN.
  const BreakpointCsrOffsets& GetTileOpTtuBreakpointCsrOffsets() const {
    return kKelvinOpBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileWideToNarrowTtuBreakpointCsrOffsets()
      const {
    return kKelvinWidetonarrowBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileNarrowToWideTtuBreakpointCsrOffsets()
      const {
    return kKelvinNarrowtowideBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer0TtuBreakpointCsrOffsets()
      const {
    return kKelvinRingbusconsumer0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer1TtuBreakpointCsrOffsets()
      const {
    return kKelvinRingbusconsumer1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusProducerTtuBreakpointCsrOffsets()
      const {
    return kKelvinRingbusproducerBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus0TtuBreakpointCsrOffsets() const {
    return kKelvinMeshbus0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus1TtuBreakpointCsrOffsets() const {
    return kKelvinMeshbus1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus2TtuBreakpointCsrOffsets() const {
    return kKelvinMeshbus2BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus3TtuBreakpointCsrOffsets() const {
    return kKelvinMeshbus3BreakpointCsrOffsets;
  }

  const MemoryCsrOffsets& GetTileMemoryCsrOffsets() const override {
    return kKelvinMemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by scalar core performance tracing.
  const TraceCsrOffsets& GetScalarCoreActivationTtuTraceCsrOffsets()
      const override {
    return kKelvinAvdatapopTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreInfeedTtuTraceCsrOffsets()
      const override {
    return kKelvinInfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreOutfeedTtuTraceCsrOffsets()
      const override {
    return kKelvinOutfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreParameterTtuTraceCsrOffsets()
      const override {
    return kKelvinParameterpopTraceCsrOffsets;
  }

  // Extracts CSR offsets used by tile performance tracing.
  const TraceCsrOffsets& GetTileOpTtuTraceCsrOffsets() const {
    return kKelvinOpTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileWideToNarrowTtuTraceCsrOffsets() const {
    return kKelvinDmawidetonarrowTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileNarrowToWideTtuTraceCsrOffsets() const {
    return kKelvinDmanarrowtowideTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer0TtuTraceCsrOffsets() const {
    return kKelvinDmaringbusconsumer0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer1TtuTraceCsrOffsets() const {
    return kKelvinDmaringbusconsumer1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusProducerTtuTraceCsrOffsets() const {
    return kKelvinDmaringbusproducerTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus0TtuTraceCsrOffsets() const {
    return kKelvinDmameshbus0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus1TtuTraceCsrOffsets() const {
    return kKelvinDmameshbus1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus2TtuTraceCsrOffsets() const {
    return kKelvinDmameshbus2TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus3TtuTraceCsrOffsets() const {
    return kKelvinDmameshbus3TraceCsrOffsets;
  }

  // Extracts CSR offsets used to access sync flags in scalar core.
  const SyncFlagCsrOffsets& GetScalarCoreAvdataPopSyncFlagCsrOffsets()
      const override {
    return kKelvinAvdataPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterPopSyncFlagCsrOffsets()
      const override {
    return kKelvinParameterPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreAvdataInfeedSyncFlagCsrOffsets()
      const override {
    return kKelvinAvdataInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterInfeedSyncFlagCsrOffsets()
      const override {
    return kKelvinParameterInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarInfeedSyncFlagCsrOffsets()
      const override {
    return kKelvinScalarInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerASyncFlagCsrOffsets()
      const override {
    return kKelvinProducerASyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerBSyncFlagCsrOffsets()
      const override {
    return kKelvinProducerBSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreRingOutfeedSyncFlagCsrOffsets()
      const override {
    return kKelvinRingOutfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarPipelineSyncFlagCsrOffsets()
      const override {
    return kKelvinScalarPipelineSyncFlagCsrOffsets;
  }

  // Extracts CSR offsets used by bug report generator in DarwiNN.
  const DebugHibUserCsrOffsets& GetDebugHibUserCsrOffsets() const override {
    return kKelvinDebugHibUserCsrOffsets;
  }
  const DebugScalarCoreCsrOffsets& GetDebugScalarCoreCsrOffsets()
      const override {
    return kKelvinDebugScalarCoreCsrOffsets;
  }
  const DebugTileCsrOffsets& GetDebugTileCsrOffsets() const override {
    return kKelvinDebugTileCsrOffsets;
  }
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_KELVIN_KELVIN_CHIP_CONFIG_H_
