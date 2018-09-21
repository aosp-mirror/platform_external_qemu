#include "third_party/darwinn/driver/dma_info.h"

#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

// Returns a debugging-friendly string.
std::string ToString(DmaState state) {
  switch (state) {
    case DmaState::kPending:
      return "pending";

    case DmaState::kActive:
      return "active";

    case DmaState::kCompleted:
      return "completed";

    case DmaState::kError:
      return "error";
  }
}

// Returns a debugging-friendly string.
std::string ToString(const DeviceBuffer& buffer) {
  return StringPrintf("device_address = 0x%llx, bytes = %zd",
                      buffer.device_address(), buffer.size_bytes());
}

}  // namespace

std::string DmaInfo::Dump() const {
  std::string prefix = StringPrintf("DMA[%d]: ", id_);
  switch (type_) {
    case DmaDescriptorType::kInstruction:
      return prefix + "Instruction: " + ToString(buffer_) + ", " +
             ToString(state_);
    case DmaDescriptorType::kInputActivation:
      return prefix + "Input activation: " + ToString(buffer_) + ", " +
             ToString(state_);
    case DmaDescriptorType::kParameter:
      return prefix + "Parameter: " + ToString(buffer_) + ", " +
             ToString(state_);
    case DmaDescriptorType::kOutputActivation:
      return prefix + "Output activation: " + ToString(buffer_) + ", " +
             ToString(state_);
    case DmaDescriptorType::kScalarCoreInterrupt0:
      return prefix + "SC interrupt 0";
    case DmaDescriptorType::kScalarCoreInterrupt1:
      return prefix + "SC interrupt 1";
    case DmaDescriptorType::kScalarCoreInterrupt2:
      return prefix + "SC interrupt 2";
    case DmaDescriptorType::kScalarCoreInterrupt3:
      return prefix + "SC interrupt 3";
    case DmaDescriptorType::kLocalFence:
      return prefix + "Local fence";
    case DmaDescriptorType::kGlobalFence:
      return prefix + "Global fence";
  }
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
