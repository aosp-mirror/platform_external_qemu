#include "third_party/darwinn/driver/abrolhos/abrolhos_top_level_handler.h"

#include "third_party/darwinn/driver/config/common_csr_helper.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status_macros.h"

namespace platforms {
namespace darwinn {
namespace driver {

AbrolhosTopLevelHandler::AbrolhosTopLevelHandler(
    const config::ChipConfig& config, Registers* registers)
    : reset_offsets_(config.GetAonResetCsrOffsets()),
      hib_user_offsets_(config.GetHibUserCsrOffsets()),
      scalar_core_offsets_(config.GetScalarCoreCsrOffsets()),
      registers_(registers) {
  CHECK(registers != nullptr);
}

util::Status AbrolhosTopLevelHandler::QuitReset() {
  // Disable in following orders:
  // NOTE(jasonjk): In production, logic shutdown and memory shutdown should be
  // done in subgroups to avoid drawing current too much.
  // e.g.
  //   const int kGroupSize = 1;
  //   const int kNumGroups = 9;
  //   int value = 0x1ff;
  //   for (int i = 0; i < kNumGroups; ++i) {
  //     value >>= kGroupSize;
  //     RETURN_IF_ERROR(registers_->Write(..., value));
  //   }
  // 1. Disable logic shutdown.
  // 1a. Disable first pass switches, and wait until ack matches.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.logicShutdownPreReg, 0));
  ASSIGN_OR_RETURN(uint64 logic_shutdown_pre_reg,
                   registers_->Read(reset_offsets_.logicShutdownPreReg));
  config::registers::ShutdownReg shutdown_helper(logic_shutdown_pre_reg);
  while (shutdown_helper.logic_shutdown() !=
         shutdown_helper.logic_shutdown_ack()) {
    ASSIGN_OR_RETURN(logic_shutdown_pre_reg,
                     registers_->Read(reset_offsets_.logicShutdownPreReg));
    shutdown_helper.set_raw(logic_shutdown_pre_reg);
  }
  // 1b. Disable second pass switches, and wait until ack matches.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.logicShutdownAllReg, 0));
  ASSIGN_OR_RETURN(uint64 logic_shutdown_all_reg,
                   registers_->Read(reset_offsets_.logicShutdownAllReg));
  shutdown_helper.set_raw(logic_shutdown_all_reg);
  while (shutdown_helper.logic_shutdown() !=
         shutdown_helper.logic_shutdown_ack()) {
    ASSIGN_OR_RETURN(logic_shutdown_all_reg,
                     registers_->Read(reset_offsets_.logicShutdownAllReg));
    shutdown_helper.set_raw(logic_shutdown_all_reg);
  }

  // 2. Enable clock enable, and set idle_override to force the clock on.
  config::registers::ClockEnableReg helper;
  helper.set_clock_enable(1);
  helper.set_idle_override(1);
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.clockEnableReg, helper.raw()));

  // 3. Read clock enable to force previous write to commit.
  ASSIGN_OR_RETURN(uint64 dummy,
                   registers_->Read(reset_offsets_.clockEnableReg));
  (void)dummy;

  // 4. Disable clock enable.
  helper.set_clock_enable(0);
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.clockEnableReg, helper.raw()));

  // 5. Read clock enable to force previous write to commit.
  ASSIGN_OR_RETURN(dummy, registers_->Read(reset_offsets_.clockEnableReg));
  (void)dummy;

  // 6. Disable memory shutdown, and wait for ack.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.memoryShutdownReg, 0));
  RETURN_IF_ERROR(registers_->Poll(reset_offsets_.memoryShutdownAckReg, 0x0));

  // 7. Enable tile control for deepsleep.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.tileMemoryRetnReg, 0));

  // 8. Enable clock again, with dynamic activty-based clock gating
  // (by clearing cb_idle_override bit)
  helper.set_clock_enable(1);
  // TODO(jasonjk): should use HW clock gating enable flag to conditionally
  // set this field to 0, leaving it at 1 would mean HW controlled clock gating
  // will stay disabled and the clock will always run when we're out of reset.
  helper.set_idle_override(0);
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.clockEnableReg, helper.raw()));

  // 9. Disable clamp.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.clampEnableReg, 0));

  // 10. Disable reset.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.resetReg, 0));

  // 11. Disable quiesce bit.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.forceQuiesceReg, 0));

  // 12. Confirm that moved out of reset by reading any CSR with known initial
  // value. scalar core run control should be zero.
  RETURN_IF_ERROR(
      registers_->Poll(scalar_core_offsets_.scalarCoreRunControl, 0));

  return util::Status();  // OK
}

util::Status AbrolhosTopLevelHandler::EnableReset() {
  // Enable in following order:
  // NOTE(jasonjk): In production, we may want to guard DMA pause by checking
  // resetReg first to use EnableReset to enforce reset when device resulted in
  // an error from previous run.
  // 1. Check whether we are already in reset to guard HIB access.
  ASSIGN_OR_RETURN(const uint64 reset_reg,
                   registers_->Read(reset_offsets_.resetReg));

  if (reset_reg == 0) {
    // 1. If not in reset, Enable DMA pause.
    RETURN_IF_ERROR(registers_->Write(hib_user_offsets_.dma_pause, 1));

    // 2. If not in reset, Wait until DMA is paused.
    RETURN_IF_ERROR(registers_->Poll(hib_user_offsets_.dma_paused, 1));
  }

  // 3. Enable quiesce bit.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.forceQuiesceReg, 1));

  // 4. Enable reset.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.resetReg, 1));

  // 5. Disable clock enable.
  config::registers::ClockEnableReg helper;
  helper.set_clock_enable(0);
  helper.set_idle_override(1);
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.clockEnableReg, helper.raw()));

  // 6. Read clock enable to force previous write to commit.
  ASSIGN_OR_RETURN(uint64 dummy,
                   registers_->Read(reset_offsets_.clockEnableReg));
  (void)dummy;

  // 7. Enable clamp.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.clampEnableReg, 0x1ff));

  // 8. Disable tile control for deepsleep, and read the register to force write
  // to commit.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.tileMemoryRetnReg, 0xff));
  ASSIGN_OR_RETURN(dummy, registers_->Read(reset_offsets_.tileMemoryRetnReg));
  (void)dummy;

  // 9. Enable memory shutdown, and read the register to force write to commit.
  RETURN_IF_ERROR(registers_->Write(reset_offsets_.memoryShutdownReg, 0x1ff));
  ASSIGN_OR_RETURN(dummy, registers_->Read(reset_offsets_.memoryShutdownReg));
  (void)dummy;

  // 10a. Enable second pass shutdown switches, and read the register to force
  // write to commit.
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.logicShutdownAllReg, 0x1ff));
  ASSIGN_OR_RETURN(dummy, registers_->Read(reset_offsets_.logicShutdownAllReg));
  (void)dummy;
  // 10b. Enable first pass shutdown switches, and read the register to force
  // write to commit.
  RETURN_IF_ERROR(
      registers_->Write(reset_offsets_.logicShutdownPreReg, 0x1ff));
  ASSIGN_OR_RETURN(dummy, registers_->Read(reset_offsets_.logicShutdownPreReg));
  (void)dummy;

  return util::Status();  // OK
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
