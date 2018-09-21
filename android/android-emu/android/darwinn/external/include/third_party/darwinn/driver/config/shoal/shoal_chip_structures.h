// AUTO GENERATED.

#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CHIP_STRUCTURES_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CHIP_STRUCTURES_H_

#include "third_party/darwinn/driver/config/chip_structures.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

const ChipStructures kShoalChipStructures = {
    16ULL,    // NOLINT: minimum_alignment_bytes
    4096ULL,  // NOLINT: allocation_alignment_bytes
    6ULL,     // NOLINT: axi_dma_burst_limiter
    1ULL,     // NOLINT: num_wire_interrupts
    0ULL,     // NOLINT: num_page_table_entries
    32ULL,    // NOLINT: physical_address_bits
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CHIP_STRUCTURES_H_
