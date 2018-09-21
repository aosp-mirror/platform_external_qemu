#ifndef THIRD_PARTY_DARWINN_DRIVER_REGISTERS_MOCK_REGISTERS_H_
#define THIRD_PARTY_DARWINN_DRIVER_REGISTERS_MOCK_REGISTERS_H_

#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Mock registers to be used in tests.
class MockRegisters : public Registers {
 public:
  MockRegisters() = default;
  ~MockRegisters() override = default;

  MOCK_METHOD0(Open, util::Status());
  MOCK_METHOD0(Close, util::Status());
  MOCK_METHOD2(Write, util::Status(uint64, uint64));
  MOCK_METHOD1(Read, util::StatusOr<uint64>(uint64));
  MOCK_METHOD2(Write32, util::Status(uint64, uint32));
  MOCK_METHOD1(Read32, util::StatusOr<uint32>(uint64));
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_REGISTERS_MOCK_REGISTERS_H_
