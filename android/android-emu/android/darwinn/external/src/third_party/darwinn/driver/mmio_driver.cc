#include "third_party/darwinn/driver/mmio_driver.h"

#include <functional>
#include <queue>
#include <utility>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/config/common_csr_helper.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/dma_info_extractor.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller_interface.h"
#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
#include "third_party/darwinn/driver/interrupt/top_level_interrupt_manager.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/driver/mmio/host_queue.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/driver/request.h"
#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/cleanup.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"
#include "third_party/darwinn/port/tracing.h"

namespace platforms {
namespace darwinn {
namespace driver {

namespace {

// Indicates no HIB Fatal Error.
constexpr uint64 kHibErrorStatusNone = 0;

}  // namespace

MmioDriver::MmioDriver(
    std::unique_ptr<config::ChipConfig> chip_config,
    std::unique_ptr<Registers> registers, std::unique_ptr<MmuMapper> mmu_mapper,
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
    std::unique_ptr<PackageRegistry> executable_registry)
    : Driver(
          [](config::ChipConfig* chip_config) {
            CHECK(chip_config != nullptr);
            return chip_config->GetChip();
          }(chip_config.get()),
          std::move(executable_registry)),
      hib_user_csr_offsets_(chip_config->GetHibUserCsrOffsets()),
      chip_structure_(chip_config->GetChipStructures()),
      registers_(std::move(registers)),
      mmu_mapper_(std::move(mmu_mapper)),
      address_space_(std::move(address_space)),
      allocator_(std::move(allocator)),
      instruction_queue_(std::move(instruction_queue)),
      interrupt_handler_(std::move(interrupt_handler)),
      top_level_interrupt_manager_(std::move(top_level_interrupt_manager)),
      fatal_error_interrupt_controller_(
          std::move(fatal_error_interrupt_controller)),
      scalar_core_controller_(std::move(scalar_core_controller)),
      run_controller_(std::move(run_controller)),
      top_level_handler_(std::move(top_level_handler)),
      dma_info_extractor_(DmaInfoExtractor::ExtractorType::kInstructionDma) {}

MmioDriver::~MmioDriver() {
  if (Close().ok()) {
    LOG(WARNING) << "Driver destroyed when open. Forced Close().";
  }
}

util::Status MmioDriver::ValidateState(State expected_state) const {
  if (state_ != expected_state) {
    return util::FailedPreconditionError(StringPrintf(
        "Bad state. expected=%d, actual=%d.", expected_state, state_));
  }
  return util::Status();  // OK
}

util::Status MmioDriver::SetState(State next_state) {
  switch (state_) {
    case kOpen:
      if (next_state == kClosing) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kClosing:
      if (next_state == kClosed) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;

    case kClosed:
      if (next_state == kOpen) {
        state_ = next_state;
        return util::Status();  // OK
      }
      break;
  }

  // Illegal state transition.
  return util::FailedPreconditionError(StringPrintf(
      "Invalid state transition. current=%d, next=%d.", state_, next_state));
}

util::Status MmioDriver::RegisterAndEnableAllInterrupts() {
  // Instruction queue completion.
  RETURN_IF_ERROR(interrupt_handler_->Register(
      DW_INTERRUPT_INSTR_QUEUE,
      std::bind(&HostQueue<HostQueueDescriptor,
                           HostQueueStatusBlock>::ProcessStatusBlock,
                instruction_queue_.get())));

  // Execution completions.
  RETURN_IF_ERROR(
      interrupt_handler_->Register(DW_INTERRUPT_SC_HOST_0, [this]() {
        auto count_result = scalar_core_controller_->CheckInterruptCounts(0);
        CHECK_OK(count_result.status());
        uint64 count = count_result.ValueOrDie();
        // We need to clear the interrupts _before_ calling
        // HandleExecutionCompletion() because that may put the device in clock
        // gated mode, which causes CSR access to be rejected.
        CHECK_OK(scalar_core_controller_->ClearInterruptStatus(0));
        for (int i = 0; i < count; ++i) {
          HandleExecutionCompletion();
        }
      }));

  // Clear status for other scalar core interrupts.
  RETURN_IF_ERROR(
      interrupt_handler_->Register(DW_INTERRUPT_SC_HOST_1, [this]() {
        CHECK_OK(scalar_core_controller_->ClearInterruptStatus(1));
      }));
  RETURN_IF_ERROR(
      interrupt_handler_->Register(DW_INTERRUPT_SC_HOST_2, [this]() {
        CHECK_OK(scalar_core_controller_->ClearInterruptStatus(2));
      }));
  RETURN_IF_ERROR(
      interrupt_handler_->Register(DW_INTERRUPT_SC_HOST_3, [this]() {
        CHECK_OK(scalar_core_controller_->ClearInterruptStatus(3));
      }));

  // Top level interrupts.
  for (int i = 0; i < top_level_interrupt_manager_->NumInterrupts(); ++i) {
    RETURN_IF_ERROR(interrupt_handler_->Register(
        static_cast<Interrupt>(DW_INTERRUPT_TOP_LEVEL_BASE + i), [this, i]() {
          LOG(WARNING) << StringPrintf("Top level interrupt: %d", i);
          CHECK_OK(top_level_interrupt_manager_->HandleInterrupt(i));
        }));
  }

  // HIB Errors.
  RETURN_IF_ERROR(
      interrupt_handler_->Register(DW_INTERRUPT_FATAL_ERR, [this]() {
        // Fatal Error is sticky when raised. Once fatal error is raised,
        // disable first and then clear interrupts. Note that it is still
        // possible for this function to be called multiple times when fatal
        // error is raised because of the host side delay involved in disabling
        // and clearing the interrupts. This is handle inside CheckFatalError().
        CHECK_OK(fatal_error_interrupt_controller_->DisableInterrupts());
        CHECK_OK(fatal_error_interrupt_controller_->ClearInterruptStatus(0));
        CheckFatalError(CheckHibError());
      }));

  // Enable interrupts, if needed.
  RETURN_IF_ERROR(scalar_core_controller_->EnableInterrupts());
  RETURN_IF_ERROR(instruction_queue_->EnableInterrupts());
  RETURN_IF_ERROR(fatal_error_interrupt_controller_->EnableInterrupts());
  RETURN_IF_ERROR(top_level_interrupt_manager_->EnableInterrupts());

  return util::Status();  // OK
}

util::Status MmioDriver::CheckHibError() {
  ASSIGN_OR_RETURN(uint64 hib_error_status,
                   registers_->Read(hib_user_csr_offsets_.hib_error_status));
  if (hib_error_status == kHibErrorStatusNone) {
    return util::Status();  // OK
  }

  uint64 hib_first_error_status =
      registers_->Read(hib_user_csr_offsets_.hib_first_error_status)
          .ValueOrDie();

  auto error_string = StringPrintf(
      "HIB Error. hib_error_status = %016llx, hib_first_error_status = %016llx",
      static_cast<unsigned long long>(hib_error_status),  // NOLINT(runtime/int)
      static_cast<unsigned long long>(                    // NOLINT(runtime/int)
          hib_first_error_status));
  LOG(ERROR) << error_string;
  return util::InternalError(error_string);
}

util::Status MmioDriver::DoOpen(bool debug_mode) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kClosed));

  // Register Access.
  RETURN_IF_ERROR(registers_->Open());
  auto registers_closer =
      gtl::MakeCleanup([this] { CHECK_OK(registers_->Close()); });

  // Reset Handler - Manages power state of the chip.
  RETURN_IF_ERROR(top_level_handler_->Open());
  auto top_level_handler_closer =
      gtl::MakeCleanup([this] { CHECK_OK(top_level_handler_->Close()); });

  // Disable clock gate and reset GCB for clean state.
  RETURN_IF_ERROR(top_level_handler_->DisableSoftwareClockGate());
  RETURN_IF_ERROR(top_level_handler_->DisableHardwareClockGate());
  RETURN_IF_ERROR(top_level_handler_->EnableReset());

  // Quit from reset mode.
  RETURN_IF_ERROR(top_level_handler_->QuitReset());
  RETURN_IF_ERROR(top_level_handler_->EnableHardwareClockGate());

  // HIB should be good to start with.
  RETURN_IF_ERROR(CheckHibError());

  // Limit AXI DMA burst.
  RETURN_IF_ERROR(registers_->Write(hib_user_csr_offsets_.dma_burst_limiter,
                                    chip_structure_.axi_dma_burst_limiter));

  // MMU Access.
  const int num_simple_entries =
      GetNumSimplePageTableEntries(chip_structure_.num_page_table_entries);

  RETURN_IF_ERROR(mmu_mapper_->Open(num_simple_entries));
  auto mmu_mapper_closer =
      gtl::MakeCleanup([this] { CHECK_OK(mmu_mapper_->Close()); });

  // Interrupt Handler.
  RETURN_IF_ERROR(interrupt_handler_->Open());
  auto interrupt_handler_closer =
      gtl::MakeCleanup([this] { CHECK_OK(interrupt_handler_->Close()); });

  // Instruction Queue Access.
  RETURN_IF_ERROR(instruction_queue_->Open(address_space_.get()));
  auto instruction_queue_closer =
      gtl::MakeCleanup([this] { CHECK_OK(instruction_queue_->Close()); });

  // Scalar core control.
  RETURN_IF_ERROR(scalar_core_controller_->Open());
  auto scalar_core_controller_closer =
      gtl::MakeCleanup([this] { CHECK_OK(scalar_core_controller_->Close()); });

  if (!debug_mode) {
    // Move all subsystems to Run state.
    RETURN_IF_ERROR(run_controller_->DoRunControl(RunControl::kMoveToRun));
  }

  // Disable periodic status block updates.
  RETURN_IF_ERROR(
      registers_->Write(hib_user_csr_offsets_.status_block_update, 0));

  // Register and enable all interrupts.
  RETURN_IF_ERROR(RegisterAndEnableAllInterrupts());

  // DMA scheduler.
  RETURN_IF_ERROR(dma_scheduler_.Open());
  auto dma_scheduler_closer =
      gtl::MakeCleanup([this] { CHECK_OK(dma_scheduler_.Close()); });

  // All good. Move state to open.
  RETURN_IF_ERROR(SetState(kOpen));

  // Clock gate until the first request arrives.
  RETURN_IF_ERROR(top_level_handler_->EnableSoftwareClockGate());

  // Release cleanup functions.
  dma_scheduler_closer.release();
  scalar_core_controller_closer.release();
  interrupt_handler_closer.release();
  instruction_queue_closer.release();
  mmu_mapper_closer.release();
  top_level_handler_closer.release();
  registers_closer.release();

  return util::Status();  // OK
}

util::Status MmioDriver::DoClose(bool in_error) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(/*expected_state=*/kOpen));

  // Note our intention to close.
  RETURN_IF_ERROR(SetState(kClosing));

  // Disable Clock Gating so as the closing procedure can access the chip
  RETURN_IF_ERROR(top_level_handler_->DisableSoftwareClockGate());

  // All good. Shut down stuff. This is best effort. So if things starts
  // failing, keep going and try cleaning up as much as we can.
  util::Status status;

  // Stop all pipelines.
  status.Update(run_controller_->DoRunControl(RunControl::kMoveToHalt));

  // Disable all interrupts.
  status.Update(top_level_interrupt_manager_->DisableInterrupts());
  status.Update(fatal_error_interrupt_controller_->DisableInterrupts());
  status.Update(instruction_queue_->DisableInterrupts());
  status.Update(scalar_core_controller_->DisableInterrupts());

  // Wait for inflight events to complete.
  status.Update(scalar_core_controller_->Close());
  status.Update(instruction_queue_->Close(in_error));
  status.Update(interrupt_handler_->Close(in_error));

  // Begin shutdown.
  status.Update(dma_scheduler_.Close());
  status.Update(mmu_mapper_->Close());
  status.Update(top_level_handler_->EnableReset());
  status.Update(top_level_handler_->Close());
  status.Update(registers_->Close());
  RETURN_IF_ERROR(status);

  // Finalize.
  RETURN_IF_ERROR(SetState(kClosed));

  return util::Status();  // OK
}

util::Status MmioDriver::DoCancelAndWaitRequests(bool in_error) {
  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(dma_scheduler_.CancelPendingRequests());
  if (!in_error) {
    RETURN_IF_ERROR(dma_scheduler_.WaitActiveRequests());
  }
  return util::Status();  // OK
}

Buffer MmioDriver::DoMakeBuffer(size_t size_bytes) const {
  return allocator_->MakeBuffer(size_bytes);
}

util::StatusOr<MappedDeviceBuffer> MmioDriver::DoMapBuffer(
    const Buffer& buffer, DmaDirection direction) {
  if (buffer.IsValid()) {
    ASSIGN_OR_RETURN(auto device_buffer,
                     address_space_->MapMemory(buffer, direction,
                                               MappingTypeHint::kExtended));
    // TODO (ijsung): this is dangerous: the std::bind captures a raw pointer to
    // the address space. This will break if executable registry outlives
    // address space in the driver. A better way is to at least use share_ptr
    // for address spaces, and here let the std::bind capture a weak_ptr.
    return MappedDeviceBuffer(
        device_buffer, std::bind(&AddressSpace::UnmapMemory,
                                 address_space_.get(), std::placeholders::_1));
  }
  return MappedDeviceBuffer();
}

util::StatusOr<std::shared_ptr<api::Request>> MmioDriver::DoCreateRequest(
    const ExecutableReference* executable) {
  StdMutexLock lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(kOpen));
  return {std::make_shared<Request>(
      next_id_++, executable, allocator_.get(),
      gtl::MakeUnique<DeviceBufferMapper>(address_space_.get()),
      &dma_info_extractor_, chip_structure_.minimum_alignment_bytes)};
}

util::Status MmioDriver::DoSubmit(std::shared_ptr<api::Request> request_in,
                                  Request::Done done) {
  TRACE_SCOPE("Darwinn::MmioDriver::DoSubmit");
  // Downcast to driver::Request.
  auto request = std::static_pointer_cast<Request>(request_in);

  StdMutexLock state_lock(&state_mutex_);
  RETURN_IF_ERROR(ValidateState(kOpen));
  request->SetDone(std::move(done));

  // Disables Clock Gating so as the chip is accessible while the request
  // is built.
  RETURN_IF_ERROR(top_level_handler_->DisableSoftwareClockGate());

  // Validate and prepare the request.
  RETURN_IF_ERROR(request->Validate());
  RETURN_IF_ERROR(request->Prepare());

  RETURN_IF_ERROR(dma_scheduler_.Submit(std::move(request)));

  TRACE_WITHIN_SCOPE("Darwinn::MmioDriver::DoSubmit::Issue");
  TryIssueDmas();

  return util::Status();  // OK
}

const ExecutableReference* MmioDriver::DoGetExecutableReference(
    std::shared_ptr<api::Request> api_request) {
  auto* request = static_cast<Request*>(api_request.get());
  return &request->executable_reference();
}

void MmioDriver::TryIssueDmas() {
  TRACE_SCOPE("Darwinn::MmioDriver::TryIssueDmas");
  // Both the dma_scheduler and instruction_queue is threadsafe on its own.
  // However, we also want to to make sure that DMAs popped from the dma
  // scheduler are pushed to the instruction queue in the order it is received.
  // So do the following with the dma_issue_mutex held.
  StdMutexLock state_lock(&dma_issue_mutex_);

  CHECK_OK(top_level_handler_->DisableSoftwareClockGate());
  while (instruction_queue_->GetAvailableSpace() > 0) {
    auto* dma = dma_scheduler_.GetNextDma();
    if (dma == nullptr) {
      break;
    }
    CHECK(dma->type() == DmaDescriptorType::kInstruction);

    HostQueueDescriptor descriptor{};
    descriptor.address = dma->buffer().device_address();
    descriptor.size_in_bytes = dma->buffer().size_bytes();

    // Enqueue should always succeed.
    CheckFatalError(
        instruction_queue_->Enqueue(descriptor, [this, dma](uint32 error_code) {
          CHECK_OK(dma_scheduler_.NotifyDmaCompletion(dma));
          HandleHostQueueCompletion(error_code);
        }));
    TRACE_WITHIN_SCOPE("Darwinn::MmioDriver::TryIssueDmas::Enqueue");
  }
}

void MmioDriver::HandleExecutionCompletion() {
  TRACE_SCOPE("Darwinn::MmioDriver::HandleExecutionCompletion");
  CHECK_OK(dma_scheduler_.NotifyRequestCompletion());
  if (dma_scheduler_.IsEmpty()) {
    CHECK_OK(top_level_handler_->EnableSoftwareClockGate());
  }
}

void MmioDriver::HandleHostQueueCompletion(uint32 error_code) {
  TRACE_SCOPE("Darwinn::MmioDriver::HostQueueCompletion");
  if (error_code != 0) {
    // TODO(jnjoseph): Parse the error code and attach a human readable string.
    CheckFatalError(
        util::InternalError(StringPrintf("Host Queue error %d.", error_code)));
    return;
  }
  TryIssueDmas();
}

void MmioDriver::CheckFatalError(const util::Status& status) {
  if (status.ok()) {
    return;
  }
  NotifyFatalError(status);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
