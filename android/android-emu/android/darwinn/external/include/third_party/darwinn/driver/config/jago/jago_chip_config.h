#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_JAGO_JAGO_CHIP_CONFIG_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_JAGO_JAGO_CHIP_CONFIG_H_

#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/jago/jago_chip_structures.h"
#include "third_party/darwinn/driver/config/jago/jago_csr_offsets.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

// Jago-specific configuration.
class JagoChipConfig : public ChipConfig {
 public:
  ~JagoChipConfig() override = default;

  api::Chip GetChip() const override { return api::Chip::kJago; }

  // Extracts CSR offsets for various modules in DarwiNN.
  const HibKernelCsrOffsets& GetHibKernelCsrOffsets() const override {
    return kJagoHibKernelCsrOffsets;
  }
  const HibUserCsrOffsets& GetHibUserCsrOffsets() const override {
    return kJagoHibUserCsrOffsets;
  }
  const QueueCsrOffsets& GetInstructionQueueCsrOffsets() const override {
    return kJagoInstructionQueueCsrOffsets;
  }
  const ScalarCoreCsrOffsets& GetScalarCoreCsrOffsets() const override {
    return kJagoScalarCoreCsrOffsets;
  }
  const TileConfigCsrOffsets& GetTileConfigCsrOffsets() const override {
    return kJagoTileConfigCsrOffsets;
  }
  const TileCsrOffsets& GetTileCsrOffsets() const override {
    return kJagoTileCsrOffsets;
  }
  const InterruptCsrOffsets& GetScalarCoreInterruptCsrOffsets() const override {
    return kJagoScHostIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetTopLevelInterruptCsrOffsets() const override {
    return kJagoTopLevelIntInterruptCsrOffsets;
  }
  const InterruptCsrOffsets& GetFatalErrorInterruptCsrOffsets() const override {
    return kJagoFatalErrIntInterruptCsrOffsets;
  }

  // Extracts CSR offsets that supports specific functionality in DarwiNN.
  const WireCsrOffsets& GetWireCsrOffsets() const override {
    return kJagoWireCsrOffsets;
  }

  // Extracts chip-specific constants in DarwiNN.
  const ChipStructures& GetChipStructures() const override {
    return kJagoChipStructures;
  }

  // Extracts CSR offsets used by scalar core debugger in DarwiNN.
  const BreakpointCsrOffsets& GetScalarCoreBreakpointCsrOffsets()
      const override {
    return kJagoScalarcoreBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreActivationTtuBreakpointCsrOffsets()
      const override {
    return kJagoAvdatapopBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreInfeedTtuBreakpointCsrOffsets()
      const override {
    return kJagoInfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreOutfeedTtuBreakpointCsrOffsets()
      const override {
    return kJagoOutfeedBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetScalarCoreParameterTtuBreakpointCsrOffsets()
      const override {
    return kJagoParameterpopBreakpointCsrOffsets;
  }

  const RegisterFileCsrOffsets& GetScalarRegisterFileCsrOffsets()
      const override {
    return kJagoScalarRegisterFileCsrOffsets;
  }
  const RegisterFileCsrOffsets& GetPredicateRegisterFileCsrOffsets()
      const override {
    return kJagoPredicateRegisterFileCsrOffsets;
  }

  const MemoryCsrOffsets& GetScalarCoreMemoryCsrOffsets() const override {
    return kJagoScmemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by tile debugger in DarwiNN.
  const BreakpointCsrOffsets& GetTileOpTtuBreakpointCsrOffsets() const {
    return kJagoOpBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileWideToNarrowTtuBreakpointCsrOffsets()
      const {
    return kJagoWidetonarrowBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileNarrowToWideTtuBreakpointCsrOffsets()
      const {
    return kJagoNarrowtowideBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer0TtuBreakpointCsrOffsets()
      const {
    return kJagoRingbusconsumer0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusConsumer1TtuBreakpointCsrOffsets()
      const {
    return kJagoRingbusconsumer1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileRingBusProducerTtuBreakpointCsrOffsets()
      const {
    return kJagoRingbusproducerBreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus0TtuBreakpointCsrOffsets() const {
    return kJagoMeshbus0BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus1TtuBreakpointCsrOffsets() const {
    return kJagoMeshbus1BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus2TtuBreakpointCsrOffsets() const {
    return kJagoMeshbus2BreakpointCsrOffsets;
  }
  const BreakpointCsrOffsets& GetTileMeshBus3TtuBreakpointCsrOffsets() const {
    return kJagoMeshbus3BreakpointCsrOffsets;
  }

  const MemoryCsrOffsets& GetTileMemoryCsrOffsets() const override {
    return kJagoMemoryMemoryCsrOffsets;
  }

  // Extracts CSR offsets used by scalar core performance tracing.
  const TraceCsrOffsets& GetScalarCoreActivationTtuTraceCsrOffsets()
      const override {
    return kJagoAvdatapopTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreInfeedTtuTraceCsrOffsets()
      const override {
    return kJagoInfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreOutfeedTtuTraceCsrOffsets()
      const override {
    return kJagoOutfeedTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetScalarCoreParameterTtuTraceCsrOffsets()
      const override {
    return kJagoParameterpopTraceCsrOffsets;
  }

  // Extracts CSR offsets used by tile performance tracing.
  const TraceCsrOffsets& GetTileOpTtuTraceCsrOffsets() const {
    return kJagoOpTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileWideToNarrowTtuTraceCsrOffsets() const {
    return kJagoDmawidetonarrowTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileNarrowToWideTtuTraceCsrOffsets() const {
    return kJagoDmanarrowtowideTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer0TtuTraceCsrOffsets() const {
    return kJagoDmaringbusconsumer0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusConsumer1TtuTraceCsrOffsets() const {
    return kJagoDmaringbusconsumer1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileRingBusProducerTtuTraceCsrOffsets() const {
    return kJagoDmaringbusproducerTraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus0TtuTraceCsrOffsets() const {
    return kJagoDmameshbus0TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus1TtuTraceCsrOffsets() const {
    return kJagoDmameshbus1TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus2TtuTraceCsrOffsets() const {
    return kJagoDmameshbus2TraceCsrOffsets;
  }
  const TraceCsrOffsets& GetTileMeshBus3TtuTraceCsrOffsets() const {
    return kJagoDmameshbus3TraceCsrOffsets;
  }

  // Extracts CSR offsets used to access sync flags in scalar core.
  const SyncFlagCsrOffsets& GetScalarCoreAvdataPopSyncFlagCsrOffsets()
      const override {
    return kJagoAvdataPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterPopSyncFlagCsrOffsets()
      const override {
    return kJagoParameterPopSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreAvdataInfeedSyncFlagCsrOffsets()
      const override {
    return kJagoAvdataInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreParameterInfeedSyncFlagCsrOffsets()
      const override {
    return kJagoParameterInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarInfeedSyncFlagCsrOffsets()
      const override {
    return kJagoScalarInfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerASyncFlagCsrOffsets()
      const override {
    return kJagoProducerASyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreProducerBSyncFlagCsrOffsets()
      const override {
    return kJagoProducerBSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreRingOutfeedSyncFlagCsrOffsets()
      const override {
    return kJagoRingOutfeedSyncFlagCsrOffsets;
  }
  const SyncFlagCsrOffsets& GetScalarCoreScalarPipelineSyncFlagCsrOffsets()
      const override {
    return kJagoScalarPipelineSyncFlagCsrOffsets;
  }

  // Extracts CSR offsets used by bug report generator in DarwiNN.
  const DebugHibUserCsrOffsets& GetDebugHibUserCsrOffsets() const override {
    return kJagoDebugHibUserCsrOffsets;
  }
  const DebugScalarCoreCsrOffsets& GetDebugScalarCoreCsrOffsets()
      const override {
    return kJagoDebugScalarCoreCsrOffsets;
  }
  const DebugTileCsrOffsets& GetDebugTileCsrOffsets() const override {
    return kJagoDebugTileCsrOffsets;
  }

  // Jago-specific.
  const AonResetCsrOffsets& GetAonResetCsrOffsets() const override {
    return kJagoAonResetCsrOffsets;
  }
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_JAGO_JAGO_CHIP_CONFIG_H_
