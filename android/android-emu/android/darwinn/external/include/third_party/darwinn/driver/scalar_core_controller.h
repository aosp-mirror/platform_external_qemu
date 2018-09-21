#ifndef THIRD_PARTY_DARWINN_DRIVER_SCALAR_CORE_CONTROLLER_H_
#define THIRD_PARTY_DARWINN_DRIVER_SCALAR_CORE_CONTROLLER_H_

#include <mutex>  // NOLINT
#include <vector>

#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/hib_user_csr_offsets.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Controls scalar core.
class ScalarCoreController {
 public:
  ScalarCoreController(const config::ChipConfig& config, Registers* registers);

  // This class is neither copyable nor movable.
  ScalarCoreController(const ScalarCoreController&) = delete;
  ScalarCoreController& operator=(const ScalarCoreController&) = delete;

  virtual ~ScalarCoreController() = default;

  // Opens/closes the controller.
  virtual util::Status Open() LOCKS_EXCLUDED(mutex_);
  virtual util::Status Close() LOCKS_EXCLUDED(mutex_);

  // Enable/disables interrupts.
  util::Status EnableInterrupts() LOCKS_EXCLUDED(mutex_);
  util::Status DisableInterrupts() LOCKS_EXCLUDED(mutex_);

  // Clears interrupt status register to notify that host has received the
  // interrupt.
  util::Status ClearInterruptStatus(int id) LOCKS_EXCLUDED(mutex_);

  // Reads and returns scalar core interrupt count register for given |id|. Read
  // is destructive in the sense that the second read to the same |id| will
  // return 0 assuming that there was no change in the CSR from the hardware
  // side.
  virtual util::StatusOr<uint64> CheckInterruptCounts(int id)
      LOCKS_EXCLUDED(mutex_);

 private:
  // Returns an error if not |open|.
  util::Status ValidateOpenState(bool open) const SHARED_LOCKS_REQUIRED(mutex_);

  // CSR offsets.
  const config::HibUserCsrOffsets& hib_user_csr_offsets_;

  // CSR interface.
  Registers* const registers_;

  // Interrupt controller.
  InterruptController interrupt_controller_;

  // Counted interrupts from scalar core.
  std::vector<uint64> interrupt_counts_;

  // Guard/track the open status of ScalarCoreController.
  std::mutex mutex_;
  bool open_ GUARDED_BY(mutex_) {false};
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_SCALAR_CORE_CONTROLLER_H_
