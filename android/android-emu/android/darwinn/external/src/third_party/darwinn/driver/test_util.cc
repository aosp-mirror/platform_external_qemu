#include "third_party/darwinn/driver/test_util.h"

#include <string>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace testutil {
namespace {

flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Layer>>>
CreateLayers(flatbuffers::FlatBufferBuilder& builder,
             const std::vector<std::string>& names, AnyLayer layer_type) {
  std::vector<flatbuffers::Offset<Layer>> layers;
  for (const auto& name : names) {
    flatbuffers::Offset<void> any_layer =
        layer_type == AnyLayer_OutputLayer ? CreateOutputLayer(builder).Union()
                                           : CreateInputLayer(builder).Union();
    layers.push_back(CreateLayerDirect(builder,
                                       /*name=*/name.c_str(),
                                       /*size_bytes=*/1,
                                       /*y_dim=*/1,
                                       /*x_dim=*/1,
                                       /*z_dim=*/1,
                                       /*numerics=*/0,
                                       /*data_type=*/DataType_FIXED_POINT8,
                                       /*any_layer_type=*/layer_type,
                                       /*any_layer=*/any_layer));
  }
  return builder.CreateVector(layers);
}

}  // namespace

std::string MakeTypedExecutable(ExecutableType type, uint64 token) {
  ExecutableT executable;
  executable.type = type;
  executable.parameter_caching_token = token;
  executable.batch_size = 1;

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
  package_builder.Finish(package, api::kHeadPackageIdentifier);

  return std::string(
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

void ExecutableGenerator::SetInputLayers(
    const std::vector<std::string>& names) {
  input_layers_ = CreateLayers(builder_, names, AnyLayer_InputLayer);
}

void ExecutableGenerator::SetOutputLayers(
    const std::vector<std::string>& names) {
  output_layers_ = CreateLayers(builder_, names, AnyLayer_OutputLayer);
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

  if (!input_layers_.IsNull()) {
    exec_builder_->add_input_layers(input_layers_);
  }
  if (!output_layers_.IsNull()) {
    exec_builder_->add_output_layers(output_layers_);
  }

  exec_builder_->add_batch_size(batch_size_);
  auto exec = exec_builder_->Finish();
  builder_.Finish(exec);
}

const Executable* ExecutableGenerator::Executable() const {
  return flatbuffers::GetRoot<platforms::darwinn::Executable>(
      builder_.GetBufferPointer());
}

std::string ExecutableGenerator::String() const {
  const std::string serialized(
      reinterpret_cast<const char*>(builder_.GetBufferPointer()),
      builder_.GetSize());
  return serialized;
}

std::string ExecutableGenerator::PackageString() const {
  return PackageString(String());
}

std::string ExecutableGenerator::PackageString(const std::string& executable) {
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
