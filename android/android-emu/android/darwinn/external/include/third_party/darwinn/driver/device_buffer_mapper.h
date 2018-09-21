#ifndef THIRD_PARTY_DARWINN_DRIVER_DEVICE_BUFFER_MAPPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_DEVICE_BUFFER_MAPPER_H_

#include <functional>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Thread-unsafe.
// Maps request-specific Buffers to DeviceBuffers, and keeps track of
// DeviceBuffers. These include: input, output, instruction and scratch.
// Note that parameters are mapped and owned by ExecutableReference.
class DeviceBufferMapper {
 public:
  explicit DeviceBufferMapper(AddressSpace* address_space);
  ~DeviceBufferMapper() = default;

  // This class is neither copyable nor movable.
  DeviceBufferMapper(const DeviceBufferMapper&) = delete;
  DeviceBufferMapper& operator=(const DeviceBufferMapper&) = delete;

  // Unmaps all per-request buffers. It is safe to call this method for cleanup
  // even if DeviceBuffers are partially mapped.
  util::Status UnmapAll();

  // Maps given buffers to DeviceBuffers.
  util::Status MapInputs(const Buffer::NamedMap& buffers);
  util::Status MapOutputs(const Buffer::NamedMap& buffers);
  util::Status MapScratch(const Buffer& buffer);
  util::Status MapInstructions(const std::vector<Buffer>& buffers);

  // Returns mapped DeviceBuffers.
  const DeviceBuffer::NamedMap& GetInputDeviceBuffers() const {
    return inputs_;
  }
  const DeviceBuffer::NamedMap& GetOutputDeviceBuffers() const {
    return outputs_;
  }
  const DeviceBuffer& GetScratchDeviceBuffer() const { return scratch_; }
  const std::vector<DeviceBuffer>& GetInstructionDeviceBuffers() const {
    return instructions_;
  }

  // Returns mapped DeviceBuffer for given argument.
  const DeviceBuffer& GetInputDeviceBuffer(const std::string& name,
                                           int batch) const {
    return inputs_.at(name)[batch];
  }
  const DeviceBuffer& GetOutputDeviceBuffer(const std::string& name,
                                            int batch) const {
    return outputs_.at(name)[batch];
  }
  const DeviceBuffer& GetInstructionDeviceBuffer(int chunk_id) const {
    DCHECK_LT(chunk_id, instructions_.size());
    return instructions_[chunk_id];
  }

 private:
  // Convenience function that wraps AddressSpace#Map() handling invalid
  // buffers.
  util::StatusOr<DeviceBuffer> Map(const Buffer& buffer,
                                   DmaDirection direction);

  // Convenience function that wraps AddressSpace#UnmapMemory() handling invalid
  // buffers.
  util::Status Unmap(DeviceBuffer buffer);

  // Address space used for mapping.
  AddressSpace* const address_space_;

  // Scratch buffer. Could be invalid.
  DeviceBuffer scratch_;

  // Input/output buffers.
  // input/output[layer_name][batch_id] = DeviceBuffer
  DeviceBuffer::NamedMap inputs_;
  DeviceBuffer::NamedMap outputs_;

  // Instruction buffers.
  std::vector<DeviceBuffer> instructions_;
};

// Holds a mapped device buffer as well as a callback for unmapping.
class MappedDeviceBuffer {
 public:
  MappedDeviceBuffer() = default;
  MappedDeviceBuffer(
      const DeviceBuffer& device_buffer,
      const std::function<util::Status(const DeviceBuffer&)>& unmapper)
      : device_buffer_(device_buffer),
        unmap_(std::bind(unmapper, device_buffer)) {}

  ~MappedDeviceBuffer() {
    // We should have unmapped the buffer at this moment.
    CHECK(!unmap_);
  }

  // This type is not copyable; we can't have the same device buffer unmapped
  // more than once.
  MappedDeviceBuffer(const MappedDeviceBuffer&) = delete;
  MappedDeviceBuffer& operator=(const MappedDeviceBuffer&) = delete;

  // This type is movable.
  MappedDeviceBuffer(MappedDeviceBuffer&& other) = default;
  MappedDeviceBuffer& operator=(MappedDeviceBuffer&& other) = default;

  const DeviceBuffer& device_buffer() const { return device_buffer_; }

  // Unmaps the associated DeviceBuffer using the given unmapper.
  util::Status Unmap() {
    if (unmap_) RETURN_IF_ERROR(unmap_());
    unmap_ = nullptr;
    return util::Status();  // OK.
  }

 private:
  DeviceBuffer device_buffer_;
  std::function<util::Status()> unmap_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_DEVICE_BUFFER_MAPPER_H_
