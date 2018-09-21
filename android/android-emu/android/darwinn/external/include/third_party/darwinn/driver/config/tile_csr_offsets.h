#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_TILE_CSR_OFFSETS_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_TILE_CSR_OFFSETS_H_

#include "third_party/darwinn/port/integral_types.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

// This struct holds various CSR offsets for tiles. Members are intentionally
// named to match the GCSR register names.
struct TileCsrOffsets {
  // RunControls to change run state.
  uint64 opRunControl;
  uint64 narrowToNarrowRunControl;
  uint64 wideToNarrowRunControl;
  uint64 narrowToWideRunControl;
  uint64 ringBusConsumer0RunControl;
  uint64 ringBusConsumer1RunControl;
  uint64 ringBusProducerRunControl;
  uint64 meshBus0RunControl;
  uint64 meshBus1RunControl;
  uint64 meshBus2RunControl;
  uint64 meshBus3RunControl;

  // Deep sleep register to control power state.
  uint64 deepSleep;
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_TILE_CSR_OFFSETS_H_
