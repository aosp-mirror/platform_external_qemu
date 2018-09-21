#ifndef THIRD_PARTY_DARWINN_DRIVER_DMA_INFO_EXTRACTOR_H_
#define THIRD_PARTY_DARWINN_DRIVER_DMA_INFO_EXTRACTOR_H_

#include <list>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/dma_info.h"
#include "third_party/darwinn/driver/package_registry.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Extracts DMAs to be performed by driver.
class DmaInfoExtractor {
 public:
  // Determines how to extract DMA infos for the executable.
  enum class ExtractorType {
    // Extracts only instruction DMAs for baseline PCIe usecase.
    kInstructionDma = 0,

    // Extracts through DMA hints for USB usecase.
    kDmaHints = 1,

    // Extracts only first instruction DMA for USB usecase.
    kFirstInstruction = 2,
  };

  explicit DmaInfoExtractor(ExtractorType type)
      : DmaInfoExtractor(type, true) {}
  DmaInfoExtractor(ExtractorType type, bool overlap_requests);
  virtual ~DmaInfoExtractor() = default;

  // Extracts a list of DMAs to be performed.
  virtual std::list<DmaInfo> ExtractDmaInfos(
      const ExecutableReference& executable_reference,
      const DeviceBufferMapper& buffers) const;

 private:
  // Extracts instruction DMAs.
  std::list<DmaInfo> ExtractInstructionDmaInfos(
      const DeviceBufferMapper& buffers) const;

  // Extracts DMA hints.
  std::list<DmaInfo> ExtractDmaHints(
      const ExecutableReference& executable_reference,
      const DeviceBufferMapper& buffers) const;

  // Extracts first instruction DMA.
  std::list<DmaInfo> ExtractFirstInstruction(
      const DeviceBufferMapper& buffers) const;

  // Extractor type.
  const ExtractorType type_;

  // True if requests can be overlapped. Should be set to false just for
  // debugging.
  const bool overlap_requests_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_DMA_INFO_EXTRACTOR_H_
