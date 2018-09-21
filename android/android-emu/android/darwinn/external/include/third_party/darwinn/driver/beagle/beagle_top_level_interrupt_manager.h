#ifndef THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_TOP_LEVEL_INTERRUPT_MANAGER_H_
#define THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_TOP_LEVEL_INTERRUPT_MANAGER_H_

#include <memory>

#include "third_party/darwinn/driver/config/apex_csr_offsets.h"
#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/scu_csr_offsets.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/driver/interrupt/top_level_interrupt_manager.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/status.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Beagle-specific top level interrupt management.
class BeagleTopLevelInterruptManager : public TopLevelInterruptManager {
 public:
  BeagleTopLevelInterruptManager(
      std::unique_ptr<InterruptControllerInterface> interrupt_controller,
      const config::ChipConfig& config, Registers* registers);
  ~BeagleTopLevelInterruptManager() override = default;

 protected:
  // Implements interfaces.
  util::Status DoEnableInterrupts() override;
  util::Status DoDisableInterrupts() override;
  util::Status DoHandleInterrupt(int id) override;

 private:
  // Implements extra CSR managements to enable top level interrupts.
  util::Status EnableThermalWarningInterrupt();
  util::Status EnableMbistInterrupt();
  util::Status EnablePcieErrorInterrupt();
  util::Status EnableThermalShutdownInterrupt();

  // Implements extra CSR managements to disable top level interrupts.
  util::Status DisableThermalWarningInterrupt();
  util::Status DisableMbistInterrupt();
  util::Status DisablePcieErrorInterrupt();
  util::Status DisableThermalShutdownInterrupt();

  // Implements top level interrupt handling.
  util::Status HandleThermalWarningInterrupt();
  util::Status HandleMbistInterrupt();
  util::Status HandlePcieErrorInterrupt();
  util::Status HandleThermalShutdownInterrupt();

  // Apex CSR offsets.
  const config::ApexCsrOffsets& apex_csr_offsets_;

  // SCU CSR offsets.
  const config::ScuCsrOffsets scu_csr_offsets_;

  // CSR interface.
  Registers* const registers_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_BEAGLE_BEAGLE_TOP_LEVEL_INTERRUPT_MANAGER_H_
