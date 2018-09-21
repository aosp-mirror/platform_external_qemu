#ifndef THIRD_PARTY_DARWINN_DRIVER_REGISTERS_REGISTERS_H_
#define THIRD_PARTY_DARWINN_DRIVER_REGISTERS_REGISTERS_H_

#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Interface for CSR access.
class Registers {
 public:
  virtual ~Registers() = default;

  // Open / Close the register interface.
  virtual util::Status Open() = 0;
  virtual util::Status Close() = 0;

  // Write / Read from a register at the given 64 bit aligned offset.
  // Offset may be implementation dependent.
  virtual util::Status Write(uint64 offset, uint64 value) = 0;
  virtual util::StatusOr<uint64> Read(uint64 offset) = 0;

  // Polls the specified register until it has the given value.
  virtual util::Status Poll(uint64 offset, uint64 expected_value);

  // 32-bit version of above. Usually, it is same as running 64 bit version.
  virtual util::Status Write32(uint64 offset, uint32 value) = 0;
  virtual util::StatusOr<uint32> Read32(uint64 offset) = 0;
  virtual util::Status Poll32(uint64 offset, uint32 expected_value);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_REGISTERS_REGISTERS_H_
