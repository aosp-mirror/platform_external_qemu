#include "third_party/darwinn/driver/device_buffer_mapper.h"

#include <memory>
#include <utility>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/stringprintf.h"
#include "third_party/darwinn/port/tracing.h"

namespace platforms {
namespace darwinn {
namespace driver {

DeviceBufferMapper::DeviceBufferMapper(AddressSpace* address_space)
    : address_space_(address_space) {
  CHECK(address_space != nullptr);
}

util::Status DeviceBufferMapper::UnmapAll() {
  TRACE_SCOPE("Darwinn::DeviceBufferMapper::UnmapAll");
  for (auto& instruction : instructions_) {
    RETURN_IF_ERROR(Unmap(std::move(instruction)));
  }

  RETURN_IF_ERROR(Unmap(std::move(scratch_)));
  TRACE_WITHIN_SCOPE("Darwinn::DeviceBufferMapper::UnmapAll::UnmappedScratch");

  for (auto& name_and_mapped_input : inputs_) {
    for (auto& mapped_input : name_and_mapped_input.second) {
      RETURN_IF_ERROR(Unmap(std::move(mapped_input)));
    }
  }
  TRACE_WITHIN_SCOPE("Darwinn::DeviceBufferMapper::UnmapAll::UnmappedInputs");

  for (auto& name_and_mapped_output : outputs_) {
    for (auto& mapped_output : name_and_mapped_output.second) {
      RETURN_IF_ERROR(Unmap(std::move(mapped_output)));
    }
  }
  TRACE_WITHIN_SCOPE("Darwinn::DeviceBufferMapper::UnmapAll::UnmappedOutputs");

  inputs_.clear();
  outputs_.clear();
  instructions_.clear();
  TRACE_WITHIN_SCOPE("Darwinn::DeviceBufferMapper::UnmapAll::done");

  return util::Status();  // OK
}

util::Status DeviceBufferMapper::MapInputs(const Buffer::NamedMap& buffers) {
  DCHECK(inputs_.empty());
  for (const auto& name_and_input : buffers) {
    for (const auto& input : name_and_input.second) {
      ASSIGN_OR_RETURN(auto mapped_input, Map(input, DmaDirection::kToDevice));

      VLOG(3) << StringPrintf(
          "Mapped input \"%s\" : 0x%016llx, %zu bytes.",
          name_and_input.first.c_str(),
          static_cast<unsigned long long>(  // NOLINT(runtime/int)
              mapped_input.device_address()),
          mapped_input.size_bytes());

      inputs_[name_and_input.first].push_back(std::move(mapped_input));
    }
  }

  return util::Status();  // OK
}

util::Status DeviceBufferMapper::MapOutputs(const Buffer::NamedMap& buffers) {
  DCHECK(outputs_.empty());
  for (const auto& name_and_output : buffers) {
    for (const auto& output : name_and_output.second) {
      ASSIGN_OR_RETURN(auto mapped_output,
                       Map(output, DmaDirection::kFromDevice));

      VLOG(3) << StringPrintf(
          "Mapped output \"%s\" : 0x%016llx, %zu bytes.",
          name_and_output.first.c_str(),
          static_cast<unsigned long long>(  // NOLINT(runtime/int)
              mapped_output.device_address()),
          mapped_output.size_bytes());

      outputs_[name_and_output.first].push_back(std::move(mapped_output));
    }
  }

  return util::Status();  // OK
}

util::Status DeviceBufferMapper::MapScratch(const Buffer& buffer) {
  DCHECK(!scratch_.IsValid());
  ASSIGN_OR_RETURN(scratch_, Map(buffer, DmaDirection::kBidirectional));

  VLOG(3) << StringPrintf(
      "Mapped scratch : %p -> 0x%016llx, %zu bytes.", buffer.ptr(),
      static_cast<unsigned long long>(  // NOLINT(runtime/int)
          scratch_.device_address()),
      scratch_.size_bytes());

  return util::Status();  // OK
}

util::Status DeviceBufferMapper::MapInstructions(
    const std::vector<Buffer>& buffers) {
  DCHECK(instructions_.empty());
  instructions_.resize(buffers.size());
  for (int i = 0; i < buffers.size(); ++i) {
    ASSIGN_OR_RETURN(instructions_[i],
                     Map(buffers[i], DmaDirection::kToDevice));

    VLOG(3) << StringPrintf(
        "Mapped instructions[%d] : %p -> 0x%016llx, %zu bytes.", i,
        buffers[i].ptr(),
        static_cast<unsigned long long>(  // NOLINT(runtime/int)
            instructions_[i].device_address()),
        instructions_[i].size_bytes());
  }

  return util::Status();  // OK
}

util::StatusOr<DeviceBuffer> DeviceBufferMapper::Map(const Buffer& buffer,
                                                     DmaDirection direction) {
  if (buffer.IsValid()) {
    return address_space_->MapMemory(buffer, direction, MappingTypeHint::kAny);
  }
  return DeviceBuffer();  // Invalid buffer.
}

util::Status DeviceBufferMapper::Unmap(DeviceBuffer buffer) {
  if (buffer.IsValid()) {
    return address_space_->UnmapMemory(std::move(buffer));
  }
  return util::Status();  // OK
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
