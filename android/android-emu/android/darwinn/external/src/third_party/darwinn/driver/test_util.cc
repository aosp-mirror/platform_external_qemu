#include "third_party/darwinn/driver/test_util.h"

#include <string>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace testutil {

std::string MakeTypedExecutable(ExecutableType type, uint64 token) {
  ExecutableT executable;
  executable.type = type;
  executable.parameter_caching_token = token;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Executable::Pack(builder, &executable));
  return std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                     builder.GetSize());
}

std::string MakePackage(const std::vector<std::string>& input_executables) {
  flatbuffers::FlatBufferBuilder multi_executable_builder;

  std::vector<flatbuffers::Offset<flatbuffers::String>> executables;
  executables.reserve(input_executables.size());
  for (const auto& serialized_executable : input_executables) {
    executables.push_back(
        multi_executable_builder.CreateString(serialized_executable));
  }

  // Create MultiExecutable flatbuffer.
  auto serialized_executables =
      multi_executable_builder.CreateVector(executables);
  auto multi_executable =
      CreateMultiExecutable(multi_executable_builder, serialized_executables);
  multi_executable_builder.Finish(multi_executable);

  // Create the package flatbuffer.
  flatbuffers::FlatBufferBuilder package_builder;
  std::vector<uint8> serialized_multi_executable(
      &multi_executable_builder.GetBufferPointer()[0],
      &multi_executable_builder
           .GetBufferPointer()[multi_executable_builder.GetSize()]);
  // We need to make sure serialized_multi_executable has the same alignment
  // as executables and parameters so that the parameter alignment is not
  // violated when the package is loaded into memory.
  package_builder.ForceVectorAlignment(serialized_multi_executable.size(),
                                       sizeof(uint8), driver::kHostPageSize);
  auto executables_vector =
      package_builder.CreateVector(serialized_multi_executable);

  auto package =
      CreatePackage(package_builder,
                    /*version=*/1,
                    /*serialized_multi_executable=*/executables_vector,
                    /*signature=*/0,
                    /*keypair_version=*/0);
  package_builder.Finish(package);

  return string(
      reinterpret_cast<const char*>(package_builder.GetBufferPointer()),
      package_builder.GetSize());
}

void ExecutableGenerator::AddInstructionBitstreams(
    const std::vector<int>& instruction_bytes) {
  constexpr char kDummyValue = 'a';
  std::vector<flatbuffers::Offset<InstructionBitstream>> instruction_vector;

  for (const int num_bytes : instruction_bytes) {
    std::string dummy_instructions(num_bytes, kDummyValue);
    auto bitstream = builder_.CreateVector(
        reinterpret_cast<const uint8_t*>(dummy_instructions.data()),
        dummy_instructions.size());
    auto instruction_bitstream =
        CreateInstructionBitstream(builder_, bitstream);
    instruction_vector.push_back(instruction_bitstream);
  }

  instruction_bitstreams_ = builder_.CreateVector(instruction_vector);
}

ExecutableBuilder* ExecutableGenerator::ExecBuilder() {
  CreateExecutableBuilder();
  return exec_builder_.get();
}

void ExecutableGenerator::Finish() {
  CreateExecutableBuilder();

  if (!instruction_bitstreams_.IsNull()) {
    exec_builder_->add_instruction_bitstreams(instruction_bitstreams_);
  }

  auto exec = exec_builder_->Finish();
  builder_.Finish(exec);
}

const Executable* ExecutableGenerator::Executable() const {
  return GetExecutable(builder_.GetBufferPointer());
}

string ExecutableGenerator::String() const {
  const string serialized(
      reinterpret_cast<const char*>(builder_.GetBufferPointer()),
      builder_.GetSize());
  return serialized;
}

string ExecutableGenerator::PackageString() const {
  return PackageString(String());
}

string ExecutableGenerator::PackageString(const string& executable) {
  return MakePackage({executable});
}

void ExecutableGenerator::CreateExecutableBuilder() {
  if (exec_builder_ != nullptr) {
    return;
  }

  exec_builder_ =
      gtl::MakeUnique<ExecutableBuilder>(builder_);
}

}  // namespace testutil
}  // namespace darwinn
}  // namespace platforms
