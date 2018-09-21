#ifndef THIRD_PARTY_DARWINN_API_CHIP_H_
#define THIRD_PARTY_DARWINN_API_CHIP_H_

#include <string>

namespace platforms {
namespace darwinn {
namespace api {

// Target chip for runtime stack.
enum class Chip {
  kBeagle,
  kJago,
  kKelvin,
  kNoronha,
  kAbrolhos,
  kShoal,
  kUnknown,
};

// Returns correct Chip for given |chip_name|.
static inline Chip GetChipByName(const std::string& chip_name) {
  if (chip_name == "beagle" || chip_name == "beagle_fpga") {
    return Chip::kBeagle;
  } else if (chip_name == "jago" || chip_name == "jago_fpga") {
    return Chip::kJago;
  } else if (chip_name == "kelvin" || chip_name == "kelvin_fpga") {
    return Chip::kKelvin;
  } else if (chip_name == "noronha" || chip_name == "noronha_fpga") {
    return Chip::kNoronha;
  } else if (chip_name == "abrolhos" || chip_name == "abrolhos_fpga") {
    return Chip::kAbrolhos;
  }else if (chip_name == "shoal" || chip_name == "shoal_fpga") {
    return Chip::kShoal;
  }
  return Chip::kUnknown;
}

// Returns the name of the given |chip|.
static inline std::string GetChipName(Chip chip) {
  switch (chip) {
    case Chip::kBeagle:
      return "beagle";
    case Chip::kJago:
      return "jago";
    case Chip::kKelvin:
      return "kelvin";
    case Chip::kNoronha:
      return "noronha";
    case Chip::kAbrolhos:
      return "abrolhos";
    case Chip::kShoal:
      return "shoal";
    default:
      return "unknown";
  }
}

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_CHIP_H_
