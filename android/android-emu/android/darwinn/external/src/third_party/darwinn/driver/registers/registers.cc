#include "third_party/darwinn/driver/registers/registers.h"

#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::Status Registers::Poll(uint64 offset, uint64 expected_value) {
  // NOTE(jnjoseph): Add a timeout.
  ASSIGN_OR_RETURN(uint64 actual_value, Read(offset));
  while (actual_value != expected_value) {
    ASSIGN_OR_RETURN(actual_value, Read(offset));
  }
  return util::Status();
}

util::Status Registers::Poll32(uint64 offset, uint32 expected_value) {
  // NOTE(jnjoseph): Add a timeout.
  ASSIGN_OR_RETURN(uint32 actual_value, Read32(offset));
  while (actual_value != expected_value) {
    ASSIGN_OR_RETURN(actual_value, Read32(offset));
  }
  return util::Status();
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
