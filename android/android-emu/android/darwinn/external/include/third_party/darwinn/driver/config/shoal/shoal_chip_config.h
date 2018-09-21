#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CHIP_CONFIG_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CHIP_CONFIG_H_

#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/shoal/shoal_chip_structures.h"
#include "third_party/darwinn/driver/config/shoal/shoal_csr_offsets.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

// Shoal-specific configuration.
class ShoalChipConfig : public ChipConfig {
 public:
  ~ShoalChipConfig() override = default;

  api::Chip GetChip() const override { return api::Chip::kShoal; }

  // Extracts CSR offsets for various modules in DarwiNN.
  const HibKernelCsrOffsets& GetHibKernelCsrOffsets() const override {
    return kShoalHibKernelCsrOffsets;
  }
  const HibUserCsrOffsets& GetHibUserCsrOffsets() const override {
    return kShoalHibUserCsrOffsets;
  }
  const QueueCsrOffsets& GetInstructionQueueCsrOffsets() const override {
    return kShoalInstructionQueueCsrOffsets;
  }
  const ScalarCoreCsrOffsets& GetScalarCoreCsrOffsets() const override {
    return kShoalScalarCoreCsrOffsets;
  }
  const TileConfigCsrOffsets& GetTileConfigCsrOffsets() const override {
    return kShoalTileConfigCsrOffsets;
  }
  const TileCsrOffsets& GetTileCsrOffsets() const override {
    return kShoalTileCsrOffsets;
  }
  const InterruptCsrOffsets& GetScalarCoreInterruptCsrOffsets() const override {
    return kShoalScHostIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetTopLevelInterruptCsrOffsets() const override {
    return kShoalTopLevelIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetFatalErrorInterruptCsrOffsets() const override {
    return kShoalFatalErrIntInterruptCsrOffsets;
  }

  // Extracts CSR offsets that supports specific functionality in DarwiNN.
  const WireCsrOffsets& GetWireCsrOffsets() const override {
    return kShoalWireCsrOffsets;
  }

  // Extracts chip-specific constants in DarwiNN.
  const ChipStructures& GetChipStructures() const override {
    return kShoalChipStructures;
  }

  // Extracts CSR offsets used by scalar core debugger in DarwiNN.
  const BreakpointCsrOffsets& GetScalarCoreBreakpointCsrOffsets()
      const override {
    return kShoalScalarcoreBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreActivationTtuBreakpointCsrOffsets()
      const override {
    return kShoalAvdatapopBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreInfeedTtuBreakpointCsrOffsets()
      const override {
    return kShoalInfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreOutfeedTtuBreakpointCsrOffsets()
      const override {
    return kShoalOutfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreParameterTtuBreakpointCsrOffsets()
      const override {
    return kShoalParameterpopBreakpointCsrOffsets;
  }

  const RegisterFileCsrOffsets& GetScalarRegisterFileCsrOffsets()
      const override {
    return kShoalScalarRegisterFileCsrOffsets;
  }
  const RegisterFileCsrOffsets& GetPredicateRegisterFileCsrOffsets()
      const override {
    return kShoalPredicateRegisterFileCsrOffsets;
  }

  const MemoryCsrOffsets& GetScalarCoreMemoryCsrOffsets() const override {
    return kShoalScmemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by tile debugger in DarwiNN.
  const BreakpointCsrOffsets& GetTileOpTtuBreakpointCsrOffsets() const {
    return kShoalOpBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileWideToNarrowTtuBreakpointCsrOffsets()
      const {
    return kShoalWidetonarrowBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileNarrowToWideTtuBreakpointCsrOffsets()
      const {
    return kShoalNarrowtowideBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer0TtuBreakpointCsrOffsets()
      const {
    return kShoalRingbusconsumer0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer1TtuBreakpointCsrOffsets()
      const {
    return kShoalRingbusconsumer1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusProducerTtuBreakpointCsrOffsets()
      const {
    return kShoalRingbusproducerBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus0TtuBreakpointCsrOffsets() const {
    return kShoalMeshbus0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus1TtuBreakpointCsrOffsets() const {
    return kShoalMeshbus1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus2TtuBreakpointCsrOffsets() const {
    return kShoalMeshbus2BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus3TtuBreakpointCsrOffsets() const {
    return kShoalMeshbus3BreakpointCsrOffsets;
  }

  const MemoryCsrOffsets& GetTileMemoryCsrOffsets() const override {
    return kShoalMemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by scalar core performance tracing.
  const TraceCsrOffsets& GetScalarCoreActivationTtuTraceCsrOffsets()
      const override {
    return kShoalAvdatapopTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreInfeedTtuTraceCsrOffsets()
      const override {
    return kShoalInfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreOutfeedTtuTraceCsrOffsets()
      const override {
    return kShoalOutfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreParameterTtuTraceCsrOffsets()
      const override {
    return kShoalParameterpopTraceCsrOffsets;
  }

  // Extracts CSR offsets used by tile performance tracing.
  const TraceCsrOffsets& GetTileOpTtuTraceCsrOffsets() const {
    return kShoalOpTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileWideToNarrowTtuTraceCsrOffsets() const {
    return kShoalDmawidetonarrowTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileNarrowToWideTtuTraceCsrOffsets() const {
    return kShoalDmanarrowtowideTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer0TtuTraceCsrOffsets() const {
    return kShoalDmaringbusconsumer0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer1TtuTraceCsrOffsets() const {
    return kShoalDmaringbusconsumer1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusProducerTtuTraceCsrOffsets() const {
    return kShoalDmaringbusproducerTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus0TtuTraceCsrOffsets() const {
    return kShoalDmameshbus0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus1TtuTraceCsrOffsets() const {
    return kShoalDmameshbus1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus2TtuTraceCsrOffsets() const {
    return kShoalDmameshbus2TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus3TtuTraceCsrOffsets() const {
    return kShoalDmameshbus3TraceCsrOffsets;
  }

  // Extracts CSR offsets used to access sync flags in scalar core.
  const SyncFlagCsrOffsets& GetScalarCoreAvdataPopSyncFlagCsrOffsets()
      const override {
    return kShoalAvdataPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterPopSyncFlagCsrOffsets()
      const override {
    return kShoalParameterPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreAvdataInfeedSyncFlagCsrOffsets()
      const override {
    return kShoalAvdataInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterInfeedSyncFlagCsrOffsets()
      const override {
    return kShoalParameterInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarInfeedSyncFlagCsrOffsets()
      const override {
    return kShoalScalarInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerASyncFlagCsrOffsets()
      const override {
    return kShoalProducerASyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerBSyncFlagCsrOffsets()
      const override {
    return kShoalProducerBSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreRingOutfeedSyncFlagCsrOffsets()
      const override {
    return kShoalRingOutfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarPipelineSyncFlagCsrOffsets()
      const override {
    return kShoalScalarPipelineSyncFlagCsrOffsets;
  }

  // Extracts CSR offsets used by bug report generator in DarwiNN.
  const DebugHibUserCsrOffsets& GetDebugHibUserCsrOffsets() const override {
    return kShoalDebugHibUserCsrOffsets;
  }
  const DebugScalarCoreCsrOffsets& GetDebugScalarCoreCsrOffsets()
      const override {
    return kShoalDebugScalarCoreCsrOffsets;
  }
  const DebugTileCsrOffsets& GetDebugTileCsrOffsets() const override {
    return kShoalDebugTileCsrOffsets;
  }

  // Shoal-specific.
  const AonResetCsrOffsets& GetAonResetCsrOffsets() const override {
    return kShoalAonResetCsrOffsets;
  }
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CHIP_CONFIG_H_
