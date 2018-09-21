#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_NORONHA_NORONHA_CHIP_CONFIG_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_NORONHA_NORONHA_CHIP_CONFIG_H_

#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/noronha/noronha_chip_structures.h"
#include "third_party/darwinn/driver/config/noronha/noronha_csr_offsets.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

// Noronha-specific configuration.
class NoronhaChipConfig : public ChipConfig {
 public:
  ~NoronhaChipConfig() override = default;

  api::Chip GetChip() const override { return api::Chip::kNoronha; }

  // Extracts CSR offsets for various modules in DarwiNN.
  const HibKernelCsrOffsets& GetHibKernelCsrOffsets() const override {
    return kNoronhaHibKernelCsrOffsets;
  }
  const HibUserCsrOffsets& GetHibUserCsrOffsets() const override {
    return kNoronhaHibUserCsrOffsets;
  }
  const QueueCsrOffsets& GetInstructionQueueCsrOffsets() const override {
    return kNoronhaInstructionQueueCsrOffsets;
  }
  const ScalarCoreCsrOffsets& GetScalarCoreCsrOffsets() const override {
    return kNoronhaScalarCoreCsrOffsets;
  }
  const TileConfigCsrOffsets& GetTileConfigCsrOffsets() const override {
    return kNoronhaTileConfigCsrOffsets;
  }
  const TileCsrOffsets& GetTileCsrOffsets() const override {
    return kNoronhaTileCsrOffsets;
  }
  const InterruptCsrOffsets& GetScalarCoreInterruptCsrOffsets() const override {
    return kNoronhaScHostIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetTopLevelInterruptCsrOffsets() const override {
    return kNoronhaTopLevelIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetFatalErrorInterruptCsrOffsets() const override {
    return kNoronhaFatalErrIntInterruptCsrOffsets;
  }

  // Extracts CSR offsets that supports specific functionality in DarwiNN.
  const WireCsrOffsets& GetWireCsrOffsets() const override {
    return kNoronhaWireCsrOffsets;
  }

  // Extracts chip-specific constants in DarwiNN.
  const ChipStructures& GetChipStructures() const override {
    return kNoronhaChipStructures;
  }

  // Extracts CSR offsets used by scalar core debugger in DarwiNN.
  const BreakpointCsrOffsets& GetScalarCoreBreakpointCsrOffsets()
      const override {
    return kNoronhaScalarcoreBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreActivationTtuBreakpointCsrOffsets()
      const override {
    return kNoronhaAvdatapopBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreInfeedTtuBreakpointCsrOffsets()
      const override {
    return kNoronhaInfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreOutfeedTtuBreakpointCsrOffsets()
      const override {
    return kNoronhaOutfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreParameterTtuBreakpointCsrOffsets()
      const override {
    return kNoronhaParameterpopBreakpointCsrOffsets;
  }

  const RegisterFileCsrOffsets& GetScalarRegisterFileCsrOffsets()
      const override {
    return kNoronhaScalarRegisterFileCsrOffsets;
  }
  const RegisterFileCsrOffsets& GetPredicateRegisterFileCsrOffsets()
      const override {
    return kNoronhaPredicateRegisterFileCsrOffsets;
  }

  const MemoryCsrOffsets& GetScalarCoreMemoryCsrOffsets() const override {
    return kNoronhaScmemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by tile debugger in DarwiNN.
  const BreakpointCsrOffsets& GetTileOpTtuBreakpointCsrOffsets() const {
    return kNoronhaOpBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileWideToNarrowTtuBreakpointCsrOffsets()
      const {
    return kNoronhaWidetonarrowBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileNarrowToWideTtuBreakpointCsrOffsets()
      const {
    return kNoronhaNarrowtowideBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer0TtuBreakpointCsrOffsets()
      const {
    return kNoronhaRingbusconsumer0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer1TtuBreakpointCsrOffsets()
      const {
    return kNoronhaRingbusconsumer1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusProducerTtuBreakpointCsrOffsets()
      const {
    return kNoronhaRingbusproducerBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus0TtuBreakpointCsrOffsets() const {
    return kNoronhaMeshbus0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus1TtuBreakpointCsrOffsets() const {
    return kNoronhaMeshbus1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus2TtuBreakpointCsrOffsets() const {
    return kNoronhaMeshbus2BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus3TtuBreakpointCsrOffsets() const {
    return kNoronhaMeshbus3BreakpointCsrOffsets;
  }

  const MemoryCsrOffsets& GetTileMemoryCsrOffsets() const override {
    return kNoronhaMemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by scalar core performance tracing.
  const TraceCsrOffsets& GetScalarCoreActivationTtuTraceCsrOffsets()
      const override {
    return kNoronhaAvdatapopTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreInfeedTtuTraceCsrOffsets()
      const override {
    return kNoronhaInfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreOutfeedTtuTraceCsrOffsets()
      const override {
    return kNoronhaOutfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreParameterTtuTraceCsrOffsets()
      const override {
    return kNoronhaParameterpopTraceCsrOffsets;
  }

  // Extracts CSR offsets used by tile performance tracing.
  const TraceCsrOffsets& GetTileOpTtuTraceCsrOffsets() const {
    return kNoronhaOpTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileWideToNarrowTtuTraceCsrOffsets() const {
    return kNoronhaDmawidetonarrowTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileNarrowToWideTtuTraceCsrOffsets() const {
    return kNoronhaDmanarrowtowideTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer0TtuTraceCsrOffsets() const {
    return kNoronhaDmaringbusconsumer0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer1TtuTraceCsrOffsets() const {
    return kNoronhaDmaringbusconsumer1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusProducerTtuTraceCsrOffsets() const {
    return kNoronhaDmaringbusproducerTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus0TtuTraceCsrOffsets() const {
    return kNoronhaDmameshbus0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus1TtuTraceCsrOffsets() const {
    return kNoronhaDmameshbus1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus2TtuTraceCsrOffsets() const {
    return kNoronhaDmameshbus2TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus3TtuTraceCsrOffsets() const {
    return kNoronhaDmameshbus3TraceCsrOffsets;
  }

  // Extracts CSR offsets used to access sync flags in scalar core.
  const SyncFlagCsrOffsets& GetScalarCoreAvdataPopSyncFlagCsrOffsets()
      const override {
    return kNoronhaAvdataPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterPopSyncFlagCsrOffsets()
      const override {
    return kNoronhaParameterPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreAvdataInfeedSyncFlagCsrOffsets()
      const override {
    return kNoronhaAvdataInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterInfeedSyncFlagCsrOffsets()
      const override {
    return kNoronhaParameterInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarInfeedSyncFlagCsrOffsets()
      const override {
    return kNoronhaScalarInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerASyncFlagCsrOffsets()
      const override {
    return kNoronhaProducerASyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerBSyncFlagCsrOffsets()
      const override {
    return kNoronhaProducerBSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreRingOutfeedSyncFlagCsrOffsets()
      const override {
    return kNoronhaRingOutfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarPipelineSyncFlagCsrOffsets()
      const override {
    return kNoronhaScalarPipelineSyncFlagCsrOffsets;
  }

  // Extracts CSR offsets used by bug report generator in DarwiNN.
  const DebugHibUserCsrOffsets& GetDebugHibUserCsrOffsets() const override {
    return kNoronhaDebugHibUserCsrOffsets;
  }
  const DebugScalarCoreCsrOffsets& GetDebugScalarCoreCsrOffsets()
      const override {
    return kNoronhaDebugScalarCoreCsrOffsets;
  }
  const DebugTileCsrOffsets& GetDebugTileCsrOffsets() const override {
    return kNoronhaDebugTileCsrOffsets;
  }

  // Noronha-specific.
  const AonResetCsrOffsets& GetAonResetCsrOffsets() const override {
    return kNoronhaAonResetCsrOffsets;
  }
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_NORONHA_NORONHA_CHIP_CONFIG_H_
