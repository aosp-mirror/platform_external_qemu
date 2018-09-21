#include "third_party/darwinn/driver/dma_info_extractor.h"

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

DmaInfoExtractor::DmaInfoExtractor(ExtractorType type, bool overlap_requests)
    : type_(type), overlap_requests_(overlap_requests) {}

std::list<DmaInfo> DmaInfoExtractor::ExtractDmaInfos(
    const ExecutableReference& executable_reference,
    const DeviceBufferMapper& buffers) const {
  switch (type_) {
    case ExtractorType::kInstructionDma:
      return ExtractInstructionDmaInfos(buffers);

    case ExtractorType::kDmaHints:
      return ExtractDmaHints(executable_reference, buffers);

    case ExtractorType::kFirstInstruction:
      return ExtractFirstInstruction(buffers);
  }
}

std::list<DmaInfo> DmaInfoExtractor::ExtractInstructionDmaInfos(
    const DeviceBufferMapper& buffers) const {
  std::list<DmaInfo> dmas;
  const auto& instructions = buffers.GetInstructionDeviceBuffers();

  int id = 0;
  for (const auto& buffer : instructions) {
    dmas.push_back(DmaInfo(id++, DmaDescriptorType::kInstruction, buffer));
  }
  if (!overlap_requests_) {
    dmas.push_back(DmaInfo(id++, DmaDescriptorType::kGlobalFence));
  }
  return dmas;
}

std::list<DmaInfo> DmaInfoExtractor::ExtractDmaHints(
    const ExecutableReference& executable_reference,
    const DeviceBufferMapper& buffers) const {
  CHECK(executable_reference.executable().dma_hints() != nullptr);
  const DmaHints& dma_hints = *executable_reference.executable().dma_hints();
  std::list<DmaInfo> dmas;
  int id = 0;
  for (const auto& dma_hint : *dma_hints.hints()) {
    switch (dma_hint->any_hint_type()) {
      case AnyHint_DmaDescriptorHint: {
        const auto& descriptor = dma_hint->any_hint_as_DmaDescriptorHint();
        const auto& meta = descriptor->meta();
        switch (meta->desc()) {
          case Description_BASE_ADDRESS_INPUT_ACTIVATION: {
            const auto& buffer = buffers.GetInputDeviceBuffer(
                meta->name()->str(), meta->batch());
            dmas.push_back(DmaInfo(id++, DmaDescriptorType::kInputActivation,
                                   buffer.Slice(descriptor->offset_in_bytes(),
                                                descriptor->size_in_bytes())));
            break;
          }

          case Description_BASE_ADDRESS_OUTPUT_ACTIVATION: {
            const auto& buffer = buffers.GetOutputDeviceBuffer(
                meta->name()->str(), meta->batch());
            dmas.push_back(DmaInfo(id++, DmaDescriptorType::kOutputActivation,
                                   buffer.Slice(descriptor->offset_in_bytes(),
                                                descriptor->size_in_bytes())));
            break;
          }

          case Description_BASE_ADDRESS_PARAMETER: {
            const auto& buffer =
                executable_reference.GetParameterDeviceBuffer();
            dmas.push_back(DmaInfo(id++, DmaDescriptorType::kParameter,
                                   buffer.Slice(descriptor->offset_in_bytes(),
                                                descriptor->size_in_bytes())));
            break;
          }

          case Description_BASE_ADDRESS_SCRATCH: {
            const auto& buffer = buffers.GetScratchDeviceBuffer();
            if (dma_hint->direction() == Direction_INFEED) {
              dmas.push_back(
                  DmaInfo(id++, DmaDescriptorType::kInputActivation,
                          buffer.Slice(descriptor->offset_in_bytes(),
                                       descriptor->size_in_bytes())));
            } else {
              DCHECK_EQ(dma_hint->direction(), Direction_OUTFEED);
              dmas.push_back(
                  DmaInfo(id++, DmaDescriptorType::kOutputActivation,
                          buffer.Slice(descriptor->offset_in_bytes(),
                                       descriptor->size_in_bytes())));
            }
            break;
          }
        }
        break;
      }

      case AnyHint_InstructionHint: {
        const int chunk_id =
            dma_hint->any_hint_as_InstructionHint()->instruction_chunk_index();
        const auto& buffer = buffers.GetInstructionDeviceBuffer(chunk_id);
        dmas.push_back(DmaInfo(id++, DmaDescriptorType::kInstruction, buffer));
        break;
      }

      case AnyHint_InterruptHint: {
        const auto& interrupt = dma_hint->any_hint_as_InterruptHint();
        const DmaDescriptorType type = static_cast<DmaDescriptorType>(
            static_cast<int>(DmaDescriptorType::kScalarCoreInterrupt0) +
            static_cast<int>(interrupt->type()));
        dmas.push_back(DmaInfo(id++, type));
        break;
      }

      case AnyHint_FenceHint: {
        dmas.push_back(DmaInfo(id++, DmaDescriptorType::kLocalFence));
        break;
      }

      case AnyHint_NONE:
        LOG(FATAL) << StringPrintf("Unrecognized hint");
        break;
    }
  }

  // Add GlobalFence to enforce ordering when hints are not fully deterministic.
  if (!dma_hints.fully_deterministic() || !overlap_requests_) {
    dmas.push_back(DmaInfo(id++, DmaDescriptorType::kGlobalFence));
  }

  if (VLOG_IS_ON(10)) {
    for (const auto& dma : dmas) {
      VLOG(10) << dma.Dump();
    }
  }
  return dmas;
}

std::list<DmaInfo> DmaInfoExtractor::ExtractFirstInstruction(
    const DeviceBufferMapper& buffers) const {
  const auto& instructions = buffers.GetInstructionDeviceBuffers();
  return {DmaInfo(/*id=*/0, DmaDescriptorType::kInstruction, instructions[0]),
          DmaInfo(/*id=*/1, DmaDescriptorType::kGlobalFence)};
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
