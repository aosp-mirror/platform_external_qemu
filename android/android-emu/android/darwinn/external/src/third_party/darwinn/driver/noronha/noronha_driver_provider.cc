#include <memory>
#include <utility>
#include <vector>

#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/driver/aligned_allocator.h"
#include "third_party/darwinn/driver/config/chip_structures.h"
#include "third_party/darwinn/driver/config/noronha/noronha_chip_config.h"
#include "third_party/darwinn/driver/driver_factory.h"
#include "third_party/darwinn/driver/executable_registry.h"
#include "third_party/darwinn/driver/executable_verifier.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/driver/interrupt/grouped_interrupt_controller.h"
#include "third_party/darwinn/driver/interrupt/interrupt_controller.h"
#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
#include "third_party/darwinn/driver/interrupt/top_level_interrupt_manager.h"
#include "third_party/darwinn/driver/interrupt/wire_interrupt_handler.h"
#include "third_party/darwinn/driver/kernel/kernel_coherent_allocator.h"
#include "third_party/darwinn/driver/kernel/kernel_interrupt_handler.h"
#include "third_party/darwinn/driver/kernel/kernel_registers.h"
#include "third_party/darwinn/driver/kernel/kernel_wire_interrupt_handler.h"
#include "third_party/darwinn/driver/memory/dual_address_space.h"
#include "third_party/darwinn/driver/mmio/host_queue.h"
#include "third_party/darwinn/driver/mmio_driver.h"
#include "third_party/darwinn/driver/noronha/noronha_mmu_mapper.h"
#include "third_party/darwinn/driver/run_controller.h"
#include "third_party/darwinn/driver/scalar_core_controller.h"
#include "third_party/darwinn/driver/top_level_handler.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using platforms::darwinn::api::Chip;
using platforms::darwinn::api::Device;


}  // namespace

class NoronhaDriverProvider : public DriverProvider {
 public:
  static std::unique_ptr<DriverProvider> CreateDriverProvider() {
    return gtl::WrapUnique<DriverProvider>(new NoronhaDriverProvider());
  }

  ~NoronhaDriverProvider() override = default;

  std::vector<Device> Enumerate() override;
  bool CanCreate(const Device& device) override;
  util::StatusOr<std::unique_ptr<api::Driver>> CreateDriver(
      const Device& device, const api::DriverOptions& options) override;

 private:
  NoronhaDriverProvider() = default;
};

REGISTER_DRIVER_PROVIDER(NoronhaDriverProvider);

std::vector<Device> NoronhaDriverProvider::Enumerate() {
  return EnumerateSysfs("abc-pcie-tpu", Chip::kNoronha, Device::Type::PCI);
}

bool NoronhaDriverProvider::CanCreate(const Device& device) {
  return device.type == Device::Type::PCI && device.chip == Chip::kNoronha;
}

util::StatusOr<std::unique_ptr<api::Driver>>
NoronhaDriverProvider::CreateDriver(const Device& device,
                                    const api::DriverOptions& options) {
  if (!CanCreate(device)) {
    return util::NotFoundError("Unsupported device.");
  }

  // TODO(jasonjk): Following queue size could come from a config.
  constexpr int kInstructionQueueSize = 256;

  // Coherent memory block granted to the Host Queue
  constexpr int kCoherentAllocatorMaxSizeByte = 0x4000;

  // Number of wires.
  constexpr int kNumWires = 3;

  auto config = gtl::MakeUnique<config::NoronhaChipConfig>();

  // Offsets are embedded in the CSR spec.
  constexpr uint64 kTileConfig0Offset = 0x0;
  constexpr uint64 kScalarCoreOffset = 0x4000;
  constexpr uint64 kUserHibOffset = 0x8000;

  // Memory mapping must be aligned with page size. Assuming 4KB page size.
  constexpr uint64 kSectionSize = 0x1000;

  const std::vector<KernelRegisters::MmapRegion> regions = {
      {kTileConfig0Offset, kSectionSize},
      {kScalarCoreOffset, kSectionSize},
      {kUserHibOffset, kSectionSize},
  };
  auto registers = gtl::MakeUnique<KernelRegisters>(device.path, regions,
                                                    /*read_only=*/false);

  auto interrupt_handler = gtl::MakeUnique<KernelWireInterruptHandler>(
      registers.get(), config->GetWireCsrOffsets(), device.path, kNumWires);
  auto top_level_handler = gtl::MakeUnique<TopLevelHandler>();

  auto mmu_mapper = gtl::MakeUnique<NoronhaMmuMapper>(device.path);
  auto address_space = gtl::MakeUnique<DualAddressSpace>(
      config->GetChipStructures(), mmu_mapper.get());
  int allocation_alignment_bytes =
      config->GetChipStructures().allocation_alignment_bytes;
  auto allocator =
      gtl::MakeUnique<AlignedAllocator>(allocation_alignment_bytes);
  auto coherent_allocator = gtl::MakeUnique<KernelCoherentAllocator>(
      device.path, allocation_alignment_bytes, kCoherentAllocatorMaxSizeByte);
  auto host_queue =
      gtl::MakeUnique<HostQueue<HostQueueDescriptor, HostQueueStatusBlock>>(
          config->GetInstructionQueueCsrOffsets(), config->GetChipStructures(),
          registers.get(), std::move(coherent_allocator),
          kInstructionQueueSize);

  constexpr int kNumTopLevelInterrupts = 4;
  auto top_level_interrupt_controller = gtl::MakeUnique<InterruptController>(
      config->GetTopLevelInterruptCsrOffsets(), registers.get(),
      kNumTopLevelInterrupts);

  auto top_level_interrupt_manager = gtl::MakeUnique<TopLevelInterruptManager>(
      std::move(top_level_interrupt_controller));

  auto fatal_error_interrupt_controller = gtl::MakeUnique<InterruptController>(
      config->GetFatalErrorInterruptCsrOffsets(), registers.get());
  auto scalar_core_controller =
      gtl::MakeUnique<ScalarCoreController>(*config, registers.get());
  auto run_controller =
      gtl::MakeUnique<RunController>(*config, registers.get());

  ASSIGN_OR_RETURN(auto verifier,
                   MakeExecutableVerifier(options.public_key()->str()));
  auto executable_registry =
      gtl::MakeUnique<ExecutableRegistry>(device.chip, std::move(verifier));

  return {gtl::MakeUnique<MmioDriver>(
      std::move(config), std::move(registers), std::move(mmu_mapper),
      std::move(address_space), std::move(allocator), std::move(host_queue),
      std::move(interrupt_handler), std::move(top_level_interrupt_manager),
      std::move(fatal_error_interrupt_controller),
      std::move(scalar_core_controller), std::move(run_controller),
      std::move(top_level_handler), std::move(executable_registry))};
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
