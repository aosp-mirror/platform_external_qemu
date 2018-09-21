#ifndef THIRD_PARTY_DARWINN_DRIVER_MMIO_DRIVER_H_
#define THIRD_PARTY_DARWINN_DRIVER_MMIO_DRIVER_H_

#include <atomic>
#include <condition_variable>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <queue>

#include "third_party/darwinn/api/allocated_buffer.h"
#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/allocator.h"
#include "third_party/darwinn/driver/config/chip_config.h"
#include "third_party/darwinn/driver/config/chip_structures.h"
#include "third_party/darwinn/driver/config/hib_user_csr_offsets.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/dma_info_extractor.h"
#include "third_party/darwinn/driver/driver.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
#include "third_party/darwinn/driver/interrupt/top_level_interrupt_manager.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/driver/mmio/host_queue.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/driver/request.h"
#include "third_party/darwinn/driver/run_controller.h"
#include "third_party/darwinn/driver/scalar_core_controller.h"
#include "third_party/darwinn/driver/single_queue_dma_scheduler.h"
#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// DarwiNN driver implementation that talks to the device through memory-mapped
// IO setup with a kernel device driver. Thread safe.
class MmioDriver : public Driver {
 public:
  MmioDriver(
      std::unique_ptr<config::ChipConfig> chip_config,
      std::unique_ptr<Registers> registers,
      std::unique_ptr<MmuMapper> mmu_mapper,
      std::unique_ptr<AddressSpace> address_space,
      std::unique_ptr<Allocator> allocator,
      std::unique_ptr<HostQueue<HostQueueDescriptor, HostQueueStatusBlock>>
          instruction_queue,
      std::unique_ptr<InterruptHandler> interrupt_handler,
      std::unique_ptr<TopLevelInterruptManager> top_level_interrupt_manager,
      std::unique_ptr<InterruptControllerInterface>
          fatal_error_interrupt_controller,
      std::unique_ptr<ScalarCoreController> scalar_core_controller,
      std::unique_ptr<RunController> run_controller,
      std::unique_ptr<TopLevelHandler> top_level_handler,
      std::unique_ptr<PackageRegistry> executable_registry);

  // This class is neither copyable nor movable.
  MmioDriver(const MmioDriver&) = delete;
  MmioDriver& operator=(const MmioDriver&) = delete;

  ~MmioDriver() override;

 protected:
  util::Status DoOpen(bool debug_mode) LOCKS_EXCLUDED(state_mutex_) override;
  util::Status DoClose(bool in_error) LOCKS_EXCLUDED(state_mutex_) override;
  util::Status DoCancelAndWaitRequests(bool in_error)
      LOCKS_EXCLUDED(state_mutex_) override;

  Buffer DoMakeBuffer(size_t size_bytes) const override;

  util::StatusOr<MappedDeviceBuffer> DoMapBuffer(
      const Buffer& buffer, DmaDirection direction) override;

  util::StatusOr<std::shared_ptr<api::Request>> DoCreateRequest(
      const ExecutableReference* executable)
      LOCKS_EXCLUDED(state_mutex_) override;
  util::Status DoSubmit(std::shared_ptr<api::Request> request_in,
                        Request::Done done)
      LOCKS_EXCLUDED(state_mutex_) override;
  const ExecutableReference* DoGetExecutableReference(
      std::shared_ptr<api::Request> api_request) override;

 private:
  // TODO(jnjoseph): Eliminate state management here. Since this is now done
  // in the base class.
  // Driver state. Transitions :
  //  kClosed -> kOpen -> kClosing -> kClosed.
  enum State {
    kOpen,     // Driver is Open.
    kClosing,  // Driver is Closing.
    kClosed,   // Driver is Closed. (Initial state.)
  };

  // Attempts a state transition to the given state.
  util::Status SetState(State next_state)
      EXCLUSIVE_LOCKS_REQUIRED(state_mutex_);

  // Validates that we are in the expected state.
  util::Status ValidateState(State expected_state) const
      SHARED_LOCKS_REQUIRED(state_mutex_);

  // Attempts to issue as many DMAs as possible.
  void TryIssueDmas() LOCKS_EXCLUDED(dma_issue_mutex_);

  // Handles request execution completions.
  void HandleExecutionCompletion();

  // Handles instruction queue pop notifications.
  void HandleHostQueueCompletion(uint32 error_code);

  // Checks for HIB Errors.
  util::Status CheckHibError();

  // Catch all fatal error handling during runtime.
  void CheckFatalError(const util::Status& status);

  // Registers and enables all interrupts.
  util::Status RegisterAndEnableAllInterrupts();

  // CSR offsets.
  const config::HibUserCsrOffsets& hib_user_csr_offsets_;

  // Chip structure.
  const config::ChipStructures& chip_structure_;

  // Register interface.
  std::unique_ptr<Registers> registers_;

  // MMU Mapper.
  std::unique_ptr<MmuMapper> mmu_mapper_;

  // Address space management.
  std::unique_ptr<AddressSpace> address_space_;

  // Host buffer allocator.
  std::unique_ptr<Allocator> allocator_;

  // Instruction queue.
  std::unique_ptr<HostQueue<HostQueueDescriptor, HostQueueStatusBlock>>
      instruction_queue_;

  // Interrupt handler.
  std::unique_ptr<InterruptHandler> interrupt_handler_;

  // Top level interrupt manager.
  std::unique_ptr<TopLevelInterruptManager> top_level_interrupt_manager_;

  // Fatal error interrupt controller.
  std::unique_ptr<InterruptControllerInterface>
      fatal_error_interrupt_controller_;

  // Scalar core controller.
  std::unique_ptr<ScalarCoreController> scalar_core_controller_;

  // Run controller.
  std::unique_ptr<RunController> run_controller_;

  // Reset handler.
  std::unique_ptr<TopLevelHandler> top_level_handler_;

  // Maintains integrity of the driver state.
  std::mutex state_mutex_;

  // Ensures that DMAs produced by the dma scheduler is submitted
  // in order to the instruction queue.
  std::mutex dma_issue_mutex_;

  // Driver state.
  State state_ GUARDED_BY(state_mutex_){kClosed};

  // When in state |kClosing|, a notification to wait for all active
  // requests to complete.
  std::condition_variable wait_active_requests_complete_;

  // ID for tracking requests.
  std::atomic<int> next_id_{0};

  // DMA info extractor.
  DmaInfoExtractor dma_info_extractor_;

  // DMA scheduler.
  SingleQueueDmaScheduler dma_scheduler_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MMIO_DRIVER_H_
