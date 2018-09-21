#include "third_party/darwinn/driver/shoal/shoal_top_level_handler.h"

#include "third_party/darwinn/driver/config/common_csr_helper.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status_macros.h"

namespace platforms {
namespace darwinn {
namespace driver {

ShoalTopLevelHandler::ShoalTopLevelHandler(const config::ChipConfig& config,
                                           Registers* registers)
    : reset_offsets_(config.GetAonResetCsrOffsets()),
      hib_user_offsets_(config.GetHibUserCsrOffsets()),
      scalar_core_offsets_(config.GetScalarCoreCsrOffsets()),
      registers_(registers) {
  CHECK(registers != nullptr);
}

util::Status ShoalTopLevelHandler::QuitReset() {
  // Enable clock, with dynamic activity-based clock gating.
  // (by clearing cb_idle_override bit)
  config::registers::ClockEnableReg helper;
  helper.set_clock_enable(1);
  // TODO(jasonjk): should use HW clock gating enable flag to conditionally
  // set this field to 0, leaving it at 1 would mean HW controlled clock gating
  // will stay disabled and the clock will always run when we're out of reset.
  helper.set_idle_override(0);
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.clockEnableReg, helper.raw()));

  // Disable reset.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.resetReg, 0));

  // Disable quiesce bit.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.forceQuiesceReg, 0));

  // Confirm that moved out of reset by reading any CSR with known initial
  // value. scalar core run control should be zero.
  RETURN_IF_ERROR(
      registers_->Poll(scalar_core_offsets_.scalarCoreRunControl, 0));

  return util::Status();  // OK
}

util::Status ShoalTopLevelHandler::EnableReset() {
  // Enable in following order:
  // NOTE(jasonjk): In production, we may want to guard DMA pause by checking
  // resetReg first to use EnableReset to enforce reset when device resulted in
  // an error from previous run.
  // Check whether we are already in reset to guard HIB access.
  ASSIGN_OR_RETURN(const uint64 reset_reg,
                   registers_->Read(reset_offsets_.resetReg));

  if (reset_reg == 0) {
    // If not in reset, Enable DMA pause.
    RETURN_IF_ERROR(registers_->Write(hib_user_offsets_.dma_pause, 1));

    // If not in reset, Wait until DMA is paused.
    RETURN_IF_ERROR(registers_->Poll(hib_user_offsets_.dma_paused, 1));
  }

  // Enable quiesce bit.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.forceQuiesceReg, 1));

  // Enable reset.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.resetReg, 1));

  // Disable clock enable.
  config::registers::ClockEnableReg helper;
  helper.set_clock_enable(0);
  helper.set_idle_override(1);
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.clockEnableReg, helper.raw()));

  return util::Status();  // OK
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
