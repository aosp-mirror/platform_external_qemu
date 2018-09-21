#include "third_party/darwinn/driver/executable_registry.h"

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/driver/aligned_allocator.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

// Alignment for buffers allocated by the registry.
constexpr uint64 kAlignment = 4096;

}  // namespace

ExecutableRegistry::ExecutableRegistry()
    : ExecutableRegistry(api::Chip::kUnknown) {}

ExecutableRegistry::ExecutableRegistry(api::Chip chip)
    : ExecutableRegistry(chip, gtl::MakeUnique<NoopExecutableVerifier>()) {}

ExecutableRegistry::ExecutableRegistry(
    api::Chip chip, std::unique_ptr<ExecutableVerifier> executable_verifier)
    : allocator_(kAlignment),
      chip_(chip),
      verifier_(std::move(executable_verifier)) {}

util::StatusOr<const api::PackageReference*>
ExecutableRegistry::RegisterPackage(const Buffer& package_buffer) {
  // Check the file identifier of the package.
  std::string package_identifier(
      flatbuffers::GetBufferIdentifier(package_buffer.ptr()),
      flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  if (package_identifier != api::kHeadPackageIdentifier) {
    LOG(WARNING) << StringPrintf("Package file identifier %s not supported.",
                                 package_identifier.c_str());
  }

  // Verify and get the package from the memory mapped buffer.
  flatbuffers::Verifier package_verifier(
      reinterpret_cast<const uint8*>(package_buffer.ptr()),
      package_buffer.size_bytes());
  if (!package_verifier.VerifyBuffer<Package>()) {
    return util::InternalError("Package verification failed.");
  }
  auto* package = flatbuffers::GetRoot<Package>(package_buffer.ptr());

  // Verify and get the MultiExecutable table from the package.
  flatbuffers::Verifier multi_executable_verifier(
      package->serialized_multi_executable()->data(),
      flatbuffers::VectorLength(package->serialized_multi_executable()));
  if (!multi_executable_verifier.VerifyBuffer<MultiExecutable>()) {
    return util::InternalError("MultiExecutable verification failed.");
  }
  auto* multi_executable = flatbuffers::GetRoot<MultiExecutable>(
      package->serialized_multi_executable()->data());

  // Extract the buffer pointer for the serialized executable from the
  // MultiExecutable.

  if (flatbuffers::VectorLength(multi_executable->serialized_executables()) ==
      0) {
    return util::NotFoundError("No executables provided.");
  }

  ASSIGN_OR_RETURN(auto executables, ExtractExecutables(*multi_executable));

  PackageReference* package_reference;
  switch (executables.size()) {
    case 1:
      // TODO(b/112762925) Here we are considering the sole executable in a
      // package as stand-alone no matter what the type specifies. This is for
      // being backward-compatible with the old-style parameter-caching. Once
      // that is deprecated, here we should look for the STAND_ALONE type.
      package_reference =
          new PackageReference(package_buffer, executables.begin()->second,
                               &allocator_, verifier_.get());
      break;

    case 2:
      package_reference = new PackageReference(
          package_buffer, executables[ExecutableType_PARAMETER_CACHING],
          executables[ExecutableType_EXECUTION_ONLY], &allocator_,
          verifier_.get());
      break;

    // TODO(b/113115673) Once this feature is implemented, we need to update the
    // constructor used here. Right now we still allow 3 executables in a
    // package to avoid future backward-incompatibility. The current behavior is
    // to always use the stand-alone one.
    case 3:
      package_reference = new PackageReference(
          package_buffer, executables[ExecutableType_STAND_ALONE], &allocator_,
          verifier_.get());
      break;

    default:
      return util::InternalError("Unexpected combination of executables.");
  }

  auto executable_reference =
      std::unique_ptr<api::PackageReference>(package_reference);
  auto api_reference =
      registrations_
          .emplace(executable_reference.get(), std::move(executable_reference))
          .first->first;
  SetParameterCachingReference(api_reference);

  return api_reference;
}

util::StatusOr<std::unordered_map<ExecutableType, const Executable*>>
ExecutableRegistry::ExtractExecutables(
    const MultiExecutable& multi_executable) const {
  std::unordered_map<ExecutableType, const Executable*> executables;

  // Fetch executables to a map of type -> executable.
  for (const auto* executable_serialized :
       *multi_executable.serialized_executables()) {
    ASSIGN_OR_RETURN(auto executable,
                     FetchAndVerifyExecutable(executable_serialized->c_str(),
                                              executable_serialized->size()));

    if (executables.find(executable->type()) != executables.end()) {
      return util::InvalidArgumentError(
          "Multiple executables of the same type were found in the package.");
    }
    executables[executable->type()] = executable;
  }

  // Sanity check for legal combinations.
  switch (executables.size()) {
    case 0:
      return util::InternalError("No executables provided.");

    case 1:
      break;

    case 2:
      if (executables.find(ExecutableType_PARAMETER_CACHING) ==
              executables.end() ||
          executables.find(ExecutableType_EXECUTION_ONLY) ==
              executables.end()) {
        return util::InvalidArgumentError(
            "Invalid combination of executables in the package.");
      }
      break;

    case 3:
      if (executables.find(ExecutableType_PARAMETER_CACHING) ==
              executables.end() ||
          executables.find(ExecutableType_EXECUTION_ONLY) ==
              executables.end() ||
          executables.find(ExecutableType_STAND_ALONE) == executables.end()) {
        return util::InvalidArgumentError(
            "Invalid combination of executables in the package.");
      }
      break;

    default:
      return util::InvalidArgumentError(
          "Found executable types that are not yet supported.");
  }

  return executables;
}

util::StatusOr<const Executable*> ExecutableRegistry::FetchAndVerifyExecutable(
    const char* executable_serialized, size_t length) const {
  flatbuffers::Verifier verifier(
      reinterpret_cast<const uint8*>(executable_serialized), length);
  if (!verifier.VerifyBuffer<Executable>()) {
    return util::InvalidArgumentError("Executable verification failed.");
  }

  const auto* executable = flatbuffers::GetRoot<Executable>(
      reinterpret_cast<const uint8*>(executable_serialized));

  // All executables must have a batch size of at least one.
  if (executable->batch_size() < 1) {
    return util::InvalidArgumentError("Executable has invalid batch size.");
  }

  // If the executable does not say which chip is being used (e.g. if it was
  // compiled before we added this check) then we allow the executable to be
  // registered.
  if (chip_ != api::Chip::kUnknown && executable->chip() != nullptr &&
      executable->chip()->size() > 0) {
    api::Chip executable_chip = api::GetChipByName(executable->chip()->str());
    if (executable_chip != chip_) {
      return util::InvalidArgumentError(StringPrintf(
          "Cannot run an executable compiled for '%s' on a '%s' device.",
          executable->chip()->c_str(), api::GetChipName(chip_).c_str()));
    }
  }

  return executable;
}

util::StatusOr<const api::PackageReference*>
ExecutableRegistry::RegisterSerialized(const std::string& executable_content) {
  return RegisterSerialized(executable_content.data(),
                            executable_content.size());
}

util::StatusOr<const api::PackageReference*>
ExecutableRegistry::RegisterSerialized(const char* executable_content,
                                       size_t length) {
  Buffer package_buffer = allocator_.MakeBuffer(length);
  CHECK(package_buffer.ptr() != nullptr);
  memcpy(package_buffer.ptr(), executable_content, length);
  return RegisterPackage(package_buffer);
}

util::StatusOr<const api::PackageReference*> ExecutableRegistry::RegisterFile(
    const std::string& executable_filename) {
  std::ifstream ifs;
  ifs.open(executable_filename, std::ifstream::in);
  if (!ifs.is_open()) {
    return util::InvalidArgumentError(
        StringPrintf("Cannot open %s.", executable_filename.c_str()));
  }

  ifs.seekg(0, std::ios_base::end);
  size_t file_size(ifs.tellg());
  ifs.seekg(std::ios_base::beg);

  Buffer package_buffer = allocator_.MakeBuffer(file_size);
  CHECK(package_buffer.ptr() != nullptr);
  ifs.read(reinterpret_cast<char*>(package_buffer.ptr()), file_size);
  ifs.close();

  return RegisterPackage(package_buffer);
}

util::Status ExecutableRegistry::Unregister(
    const api::PackageReference* executable_reference) {
  // Bail out early if executable_reference isn't valid.
  if (registrations_.count(executable_reference) == 0) {
    return util::NotFoundError(
        "Attempting to unregister a nonexistent executable reference.");
  }

  CHECK(executable_reference != nullptr);
  UnsetParameterCachingReference(executable_reference);

  PackageReference* package_reference = const_cast<PackageReference*>(
      static_cast<const PackageReference*>(executable_reference));
  RETURN_IF_ERROR(package_reference->UnmapParameters());

  // TODO (b/112311598): Need to track outstanding requests and error when
  // there are pending/in-flight requests at un-registration time.
  if (registrations_.erase(package_reference) == 0) {
    return util::NotFoundError(
        "Attempting to unregister a nonexistent executable reference.");
  }

  return util::Status();  // OK.
}

void ExecutableRegistry::UnregisterAll() {
  for (auto& it : registrations_) {
    CHECK(it.first != nullptr);
    PackageReference* package = const_cast<PackageReference*>(
        static_cast<const PackageReference*>(it.first));
    CHECK_OK(package->UnmapParameters());
  }
  // TODO (b/112311598): Need to track outstanding requests and error when
  // there are pending/in-flight requests at un-registration time.
  registrations_.clear();

  StdMutexLock parameter_caching_lock(&parameter_caching_ref_mutex_);
  parameter_caching_refs_.clear();
}

void ExecutableRegistry::SetParameterCachingReference(
    const api::PackageReference* api_executable_ref) {
  const auto* package_ref =
      static_cast<const PackageReference*>(api_executable_ref);
  if (!package_ref->ParameterCachingEnabled()) {
    return;
  }

  StdMutexLock parameter_caching_lock(&parameter_caching_ref_mutex_);
  parameter_caching_refs_[package_ref->InferenceExecutableReference()] =
      package_ref->ParameterCachingExecutableReference();
}

void ExecutableRegistry::UnsetParameterCachingReference(
    const api::PackageReference* api_executable_ref) {
  const auto* package_ref =
      static_cast<const PackageReference*>(api_executable_ref);
  if (!package_ref->ParameterCachingEnabled()) {
    return;
  }

  StdMutexLock parameter_caching_lock(&parameter_caching_ref_mutex_);
  parameter_caching_refs_.erase(package_ref->InferenceExecutableReference());
}

const ExecutableReference*
ExecutableRegistry::GetParameterCachingExecutableReference(
    const ExecutableReference* inference_executable_reference) {
  StdMutexLock parameter_caching_lock(&parameter_caching_ref_mutex_);
  if (parameter_caching_refs_.find(inference_executable_reference) ==
      parameter_caching_refs_.end()) {
    return nullptr;
  }
  return parameter_caching_refs_[inference_executable_reference];
}

ExecutableReference::ExecutableReference(const Executable* executable,
                                         driver::Allocator* allocator)
    : executable_(executable) {
  auto parameter_size_bytes =
      flatbuffers::VectorLength(executable->parameters());
  if (parameter_size_bytes > 0) {
    // TODO(b/112390166): The on_chip_cached flag may need to be derived
    // from the executable.
    parameters_ =
        Buffer(reinterpret_cast<const uint8*>(executable->parameters()->data()),
               parameter_size_bytes, /*on_chip_cached=*/true);
  }

  // Allocate scratch if necessary.
  if (executable->scratch_size_bytes() > 0) {
    scratch_ = allocator->MakeBuffer(executable->scratch_size_bytes());
    CHECK(scratch_.ptr() != nullptr);
  }

  // Set layer information.
  const int input_layer_count =
      flatbuffers::VectorLength(executable_->input_layers());
  inputs_.reserve(input_layer_count);
  input_layer_names_.reserve(input_layer_count);

  for (int i = 0; i < input_layer_count; ++i) {
    const auto& layer_name = executable_->input_layers()->Get(i)->name()->str();
    inputs_.emplace_back(executable_->input_layers()->Get(i));
    input_layer_names_.emplace_back(layer_name);
    input_map_[layer_name] = i;
  }

  const int output_layer_count =
      flatbuffers::VectorLength(executable_->output_layers());
  outputs_.reserve(output_layer_count);
  output_layer_names_.reserve(output_layer_count);

  for (int i = 0; i < output_layer_count; ++i) {
    const auto& layer_name =
        executable_->output_layers()->Get(i)->name()->str();
    outputs_.emplace_back(executable_->output_layers()->Get(i));
    output_layer_names_.emplace_back(layer_name);
    output_map_[layer_name] = i;
  }
}

int ExecutableReference::ParametersSize() const {
  if (!flatbuffers::IsFieldPresent(executable_,
                                   darwinn::Executable::VT_PARAMETERS)) {
    return 0;
  }
  return executable_->parameters()->size();
}

int ExecutableReference::NumInstructionBitstreams() const {
  if (!flatbuffers::IsFieldPresent(
          executable_, darwinn::Executable::VT_INSTRUCTION_BITSTREAMS)) {
    return 0;
  }
  return executable_->instruction_bitstreams()->size();
}

int ExecutableReference::BitstreamSize(int index) const {
  auto* instruction_bitstream =
      executable_->instruction_bitstreams()->Get(index);
  if (!flatbuffers::IsFieldPresent(
          instruction_bitstream, darwinn::InstructionBitstream::VT_BITSTREAM)) {
    return 0;
  }
  return instruction_bitstream->bitstream()->size();
}

util::StatusOr<int> ExecutableReference::InputIndex(
    const std::string& name) const {
  auto iter = input_map_.find(name);
  if (iter != input_map_.end()) {
    return iter->second;
  }
  return util::NotFoundError(
      StringPrintf("Input layer '%s' not found.", name.c_str()));
}

util::StatusOr<int> ExecutableReference::OutputIndex(
    const std::string& name) const {
  auto iter = output_map_.find(name);
  if (iter != output_map_.end()) {
    return iter->second;
  }
  return util::NotFoundError(
      StringPrintf("Output layer '%s' not found.", name.c_str()));
}

const api::InputLayerInformation* ExecutableReference::InputLayer(
    int index) const {
  if (index < inputs_.size()) {
    return &inputs_[index];
  }
  return nullptr;
}

const api::OutputLayerInformation* ExecutableReference::OutputLayer(
    int index) const {
  if (index < outputs_.size()) {
    return &outputs_[index];
  }
  return nullptr;
}

util::StatusOr<int> ExecutableReference::InputLayerSizeBytes(
    const std::string& name) const {
  ASSIGN_OR_RETURN(int index, InputIndex(name));
  return inputs_[index].size_bytes();
}

util::StatusOr<int> ExecutableReference::OutputLayerSizeBytes(
    const std::string& name) const {
  ASSIGN_OR_RETURN(int index, OutputIndex(name));
  return outputs_[index].size_bytes();
}

util::Status ExecutableReference::ValidateInput(const std::string& input_name,
                                                const Buffer& input) const {
  ASSIGN_OR_RETURN(const int expected_size_bytes,
                   InputLayerSizeBytes(input_name));
  if (input.size_bytes() != expected_size_bytes) {
    return util::InvalidArgumentError(StringPrintf(
        "Unexpected input size for \"%s\". expected=%d, actual=%zu.",
        input_name.c_str(), expected_size_bytes, input.size_bytes()));
  }
  return util::Status();  // OK
}

util::Status ExecutableReference::ValidateOutput(const std::string& output_name,
                                                 const Buffer& output) const {
  ASSIGN_OR_RETURN(const int expected_size_bytes,
                   OutputLayerSizeBytes(output_name));
  if (output.size_bytes() != expected_size_bytes) {
    return util::InvalidArgumentError(StringPrintf(
        "Unexpected output size for \"%s\". expected=%d, actual=%zu.",
        output_name.c_str(), expected_size_bytes, output.size_bytes()));
  }
  return util::Status();  // OK
}

uint64 ExecutableReference::ParameterCachingToken() const {
  return executable().parameter_caching_token();
}

// Reuses the instruction buffers if available. Creates a new one if not.
std::unique_ptr<InstructionBuffers> ExecutableReference::GetInstructionBuffers(
    Allocator* const allocator) {
  StdMutexLock lock(&instruction_buffers_vector_mutex_);

  if (!instruction_buffers_vector_.empty()) {
    std::unique_ptr<InstructionBuffers> old_instruction_buffers =
        std::move(instruction_buffers_vector_.back());
    instruction_buffers_vector_.pop_back();
    VLOG(10) << "Reusing old instruction buffers.";

    return old_instruction_buffers;
  }

  auto instruction_buffers = gtl::MakeUnique<InstructionBuffers>(
      allocator, *executable().instruction_bitstreams());

  VLOG(10) << "Created new instruction buffers.";
  return instruction_buffers;
}

// Returns instruction buffers back to the executable references so that the
// next request could reuse it.
void ExecutableReference::ReturnInstructionBuffers(
    std::unique_ptr<InstructionBuffers> instruction_buffers) {
  StdMutexLock lock(&instruction_buffers_vector_mutex_);

  instruction_buffers_vector_.push_back(std::move(instruction_buffers));
  VLOG(10) << "Returned instruction buffers back to executable reference";
}

PackageReference::PackageReference(const Buffer& package_buffer,
                                   const Executable* standalone_executable,
                                   driver::Allocator* allocator,
                                   ExecutableVerifier* verifier)
    : package_buffer_(package_buffer),
      verifier_(verifier),
      standalone_reference_(
          new driver::ExecutableReference(standalone_executable, allocator)) {}

PackageReference::PackageReference(
    const Buffer& package_buffer,
    const Executable* parameter_caching_executable,
    const Executable* inference_executable, driver::Allocator* allocator,
    ExecutableVerifier* verifier)
    : package_buffer_(package_buffer),
      verifier_(verifier),
      parameter_caching_reference_(new driver::ExecutableReference(
          parameter_caching_executable, allocator)),
      inference_reference_(
          new driver::ExecutableReference(inference_executable, allocator)) {}

int PackageReference::ParametersSize() const {
  if (standalone_reference_ != nullptr) {
    return standalone_reference_->ParametersSize();
  }

  auto parameters_size = inference_reference_->ParametersSize();
  if (parameter_caching_reference_ != nullptr) {
    parameters_size += parameter_caching_reference_->ParametersSize();
  }
  return parameters_size;
}

std::vector<driver::ExecutableReference*>
PackageReference::AllExecutableReferences() const {
  std::vector<driver::ExecutableReference*> all_references;
  if (standalone_reference_ != nullptr) {
    all_references.push_back(standalone_reference_.get());
  }
  if (parameter_caching_reference_ != nullptr) {
    all_references.push_back(parameter_caching_reference_.get());
  }
  if (inference_reference_ != nullptr) {
    all_references.push_back(inference_reference_.get());
  }
  return all_references;
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
