#ifndef THIRD_PARTY_DARWINN_DRIVER_TEST_UTIL_H_
#define THIRD_PARTY_DARWINN_DRIVER_TEST_UTIL_H_

#include <string>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/aligned_allocator.h"

namespace platforms {
namespace darwinn {
namespace testutil {

// Helper function to create an empty executable with its type and parameter-
// caching token set.
std::string MakeTypedExecutable(ExecutableType type, uint64 token);

// Helper function to pack a vector of serialzied executables to a package.
std::string MakePackage(const std::vector<std::string>& input_executables);

// Convenience class for generating executable flat buffers used for testing.
// Usage:
//   ExecutableGenerator generator;
//   generator.AddInstructionBitstreams(....); (optional)
//   generator.ExecBuilder().add_version(...); (optional)
//   ...
//   generator.Finish();
//   auto str = generator.String(); (optional)
//   auto str = generator.PackageString(); (optional)
class ExecutableGenerator {
 public:
  ExecutableGenerator() = default;
  ~ExecutableGenerator() = default;

  // Finishes the construction of the executable flat buffer. No changed are
  // allowed after calling this and has to be called before using the
  // executable.
  void Finish();

  // Builds and returns an executable. Make sure to call Finish before this.
  const Executable* Executable() const;

  // Returns a string encapsulation of the executable flat buffer. Make sure to
  // call Finish before this.
  std::string String() const;

  // Returns the package containing the executable built in this instance. Make
  // sure to call finish before this.
  std::string PackageString() const;

  // Returns the package containing the given executable.
  static std::string PackageString(const std::string& executable);

  // Returns flat buffer builder, this is low-level so use with caution.
  flatbuffers::FlatBufferBuilder* Builder() { return &builder_; }

  // Returns the reference to the executable builder. After calling this method
  // you can no longer call the following private methods.
  ExecutableBuilder* ExecBuilder();

  void SetBatchSize(int batch_size) { batch_size_ = batch_size; }

  // Takes an int vector representing the number of instruction bytes per
  // instruction stream and adds a list of instruction streams to the
  // executable. This method may be called only once per ExecutableGenerator.
  void AddInstructionBitstreams(const std::vector<int>& instruction_bytes);

  // Set the input layers of the executable. Only the name needs to be
  // specified; the rest of the layer properties are filled in with defaults.
  void SetInputLayers(const std::vector<std::string>& names);

  // Set the output layers of the executable. Only the name needs to be
  // specified; the rest of the layer properties are filled in with defaults.
  void SetOutputLayers(const std::vector<std::string>& names);

 private:
  // If exec_builder_ is not initialized creates and sets it.
  void CreateExecutableBuilder();

  // The flat buffer builder in charge of generating the result.
  flatbuffers::FlatBufferBuilder builder_;

  // The builder responsible for generating the Executable which is the root
  // flatbuffer table.
  std::unique_ptr<ExecutableBuilder> exec_builder_;

  // Batch size of the generated executable.
  int batch_size_{1};

  // InstructionBitstreams that need to be added to the executable.
  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<InstructionBitstream>>>
      instruction_bitstreams_;

  // Input layers that will be added to the executable.
  flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Layer>>>
      input_layers_;

  // Output layers that will be added to the executable.
  flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Layer>>>
      output_layers_;
};

}  // namespace testutil
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_TEST_UTIL_H_
