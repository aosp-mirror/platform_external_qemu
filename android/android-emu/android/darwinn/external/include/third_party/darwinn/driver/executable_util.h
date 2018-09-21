#ifndef THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_UTIL_H_
#define THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_UTIL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/port/array_slice.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Utility functions for working with executable.fbs.
class ExecutableUtil {
 public:
  // Processes the input instruction stream and generates an output instruction
  // stream with the meta fields populated with the given scratch address. Due
  // to the way flatbuffers are packed, field_offsets can be nullptr which is
  // treated the same as empty vector in this function.
  static void LinkScratchAddress(
      uint64 scratch_address,
      const flatbuffers::Vector<flatbuffers::Offset<darwinn::FieldOffset>>*
          field_offsets,
      gtl::MutableArraySlice<uint8> encoded_buffer);

  // Processes the input instruction stream and generates an output instruction
  // stream with the meta fields populated with the given host addresses. Due
  // to the way flatbuffers are packed, field_offsets can be nullptr which is
  // treated the same as empty vector in this function.
  static void LinkParameterAddress(
      uint64 parameter_address,
      const flatbuffers::Vector<flatbuffers::Offset<darwinn::FieldOffset>>*
          field_offsets,
      gtl::MutableArraySlice<uint8> encoded_buffer);

  static void LinkInputAddress(
      const std::string& input_name, const std::vector<uint64>& input_addresses,
      const flatbuffers::Vector<flatbuffers::Offset<darwinn::FieldOffset>>*
          field_offsets,
      gtl::MutableArraySlice<uint8> encoded_buffer);

  static void LinkOutputAddress(
      const std::string& output_name,
      const std::vector<uint64>& output_addresses,
      const flatbuffers::Vector<flatbuffers::Offset<darwinn::FieldOffset>>*
          field_offsets,
      gtl::MutableArraySlice<uint8> encoded_buffer);

  // Convenience function to set a uint32 value on the specified bitoffset.
  static void CopyUint32(gtl::MutableArraySlice<uint8> buffer, int offset_bit,
                         uint32 value);

 private:
  // Static class.
  ExecutableUtil();
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_UTIL_H_
