#ifndef THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_HANDLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_HANDLER_H_

#include <functional>

#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Interrupt identifiers.
enum Interrupt {
  DW_INTERRUPT_INSTR_QUEUE = 0,
  DW_INTERRUPT_INPUT_ACTV_QUEUE = 1,
  DW_INTERRUPT_PARAM_QUEUE = 2,
  DW_INTERRUPT_OUTPUT_ACTV_QUEUE = 3,
  DW_INTERRUPT_SC_HOST_0 = 4,
  DW_INTERRUPT_SC_HOST_1 = 5,
  DW_INTERRUPT_SC_HOST_2 = 6,
  DW_INTERRUPT_SC_HOST_3 = 7,
  DW_INTERRUPT_TOP_LEVEL_0 = 8,
  DW_INTERRUPT_TOP_LEVEL_1 = 9,
  DW_INTERRUPT_TOP_LEVEL_2 = 10,
  DW_INTERRUPT_TOP_LEVEL_3 = 11,
  DW_INTERRUPT_FATAL_ERR = 12,
  DW_INTERRUPT_COUNT = 13,

  // Aliases.
  DW_INTERRUPT_SC_HOST_BASE = DW_INTERRUPT_SC_HOST_0,
  DW_INTERRUPT_TOP_LEVEL_BASE = DW_INTERRUPT_TOP_LEVEL_0,
};

// Interface for handling interrupts.
class InterruptHandler {
 public:
  using Handler = std::function<void()>;

  virtual ~InterruptHandler() = default;

  // Open / Close the interrupt handler.
  virtual util::Status Open() = 0;
  virtual util::Status Close(bool in_error) = 0;
  util::Status Close() { return Close(/*in_error=*/false); }

  // Registers interrupt.
  virtual util::Status Register(Interrupt interrupt, Handler handler) = 0;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_INTERRUPT_INTERRUPT_HANDLER_H_
