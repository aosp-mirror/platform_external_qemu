#ifndef THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_REGISTRY_H_
#define THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_REGISTRY_H_

#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/layer_information.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/driver/aligned_allocator.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/executable_verifier.h"
#include "third_party/darwinn/driver/instruction_buffers.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Reference to a single executable.
//
// TODO(b/112942167) Rename this to avoid confusion with the API interface now
// that it is not inheriting it.
class ExecutableReference {
 public:
  // This class is neither copyable nor movable.
  ExecutableReference(const ExecutableReference&) = delete;
  ExecutableReference& operator=(const ExecutableReference&) = delete;

  // Returns the index of input layer with given name.
  util::StatusOr<int> InputIndex(const std::string& name) const;

  // Returns the index of output layer with given name.
  util::StatusOr<int> OutputIndex(const std::string& name) const;

  // Returns number of input layers.
  int NumInputLayers() const { return inputs_.size(); }

  // Returns number of output layers.
  int NumOutputLayers() const { return outputs_.size(); }

  // Returns list of input layer names.
  const std::vector<std::string>& InputLayerNames() const {
    return input_layer_names_;
  }

  // Returns list of output layer names.
  const std::vector<std::string>& OutputLayerNames() const {
    return output_layer_names_;
  }

  // Returns information on given input layer. Returns nullptr if index is out
  // of bounds.
  const api::InputLayerInformation* InputLayer(int index) const;

  // Returns information on given output layer. Returns nullptr if index is out
  // of bounds.
  const api::OutputLayerInformation* OutputLayer(int index) const;

  // Returns the expected byte size of activations for given input layer index.
  // This is post-padding, if any.
  int InputLayerSizeBytes(int index) const {
    CHECK(InputLayer(index) != nullptr);
    return InputLayer(index)->size_bytes();
  }

  // Returns the expected byte size of activations for given output layer index.
  // This is post-padding, if any.
  int OutputLayerSizeBytes(int index) const {
    CHECK(OutputLayer(index) != nullptr);
    return OutputLayer(index)->size_bytes();
  }

  // Returns the expected size (in value count) of activations for given input
  // layer index. This is pre-padding, if any.
  int InputLayerSize(int index) const {
    auto layer = InputLayer(index);
    CHECK(layer != nullptr);
    return layer->y_dim() * layer->x_dim() * layer->z_dim();
  }

  // Returns the expected size (in value count) of activations for given input
  // layer index. This is pre-padding, if any.
  int OutputLayerSize(int index) const {
    auto layer = OutputLayer(index);
    CHECK(layer != nullptr);
    return layer->y_dim() * layer->x_dim() * layer->z_dim();
  }

  // Returns the expected size of activations for given input layer.
  // Prefer index based APIs for performance.
  util::StatusOr<int> InputLayerSizeBytes(const std::string& name) const;

  // Returns the expected size of activations for given output layer.
  // Prefer index based APIs for performance.
  util::StatusOr<int> OutputLayerSizeBytes(const std::string& name) const;

  // Returns name for given input layer index.
  std::string InputLayerName(int index) const {
    CHECK(InputLayer(index) != nullptr);
    return InputLayer(index)->name();
  }

  // Returns name for given output layer index.
  std::string OutputLayerName(int index) const {
    CHECK(OutputLayer(index) != nullptr);
    return OutputLayer(index)->name();
  }

  // Returns batch size.
  int BatchSize() const { return executable_->batch_size(); }

  // Returns the size (in bytes) of the parameters blob.
  int ParametersSize() const;

  // Returns number of instruction bitstreams.
  int NumInstructionBitstreams() const;

  // Returns the bitstream size (in bytes) in the instruction bitstream at
  // provided index.
  int BitstreamSize(int index) const;

  // Executable
  const darwinn::Executable& executable() const { return *executable_; }

  // Memory aligned copy of the parameters.
  const Buffer& parameters() const { return parameters_; }

  // Sets mapped parameters.
  // Move-only. The given mapped_parameter will be unmapped during destruction
  // time, so we cannot allow copy-construction, to avoid doubly unmapping a
  // Device Buffer.
  void SetMappedParameters(MappedDeviceBuffer&& mapped_parameters) {
    mapped_parameters_ = std::move(mapped_parameters);
  }

  const DeviceBuffer& GetParameterDeviceBuffer() const {
    return mapped_parameters_.device_buffer();
  }

  // Scratch buffer, if the executable requires scratch space. If not, then the
  // buffer will be invalid.
  Buffer scratch() const { return scratch_; }

  // Validates that the given input buffer is compatible with the executable.
  util::Status ValidateInput(const std::string& input_name,
                             const Buffer& input) const;

  // Validates that the given output buffer is compatible with the executable.
  util::Status ValidateOutput(const std::string& output_name,
                              const Buffer& output) const;

  // Returns the parameter-caching token which is unique across models that are
  // compiled together and can cache their parameters on TPU SRAM at the same
  // time. If 0, it means this executable's parameters cannot safely co-exist
  // with those of others. Please note that tokens are not limited to parameter
  // cached models. We could have a stand-alone compiled model that still has
  // a token to ensure us it will not overwite cached parameters of other
  // models.
  uint64 ParameterCachingToken() const;

  // Reuses or creates instruction buffers.
  std::unique_ptr<InstructionBuffers> GetInstructionBuffers(
      Allocator* allocator);

  // Returns instruction buffers back to executable reference.
  // TODO(yuchicheng): Add pool size limit. This currently doesn't have size
  // limit, and if there are many requests happened at the same time, we might
  // increase the total memory footprint. Notice that this won't increase
  // the memory peak size but will hold them longer. If this becomes an issue
  // we should investigate what's the correct size limit.
  void ReturnInstructionBuffers(
      std::unique_ptr<InstructionBuffers> instruction_buffers);

 private:
  friend class PackageReference;

  // Allow constructions through the friend classes only.
  ExecutableReference(const Executable* executable,
                      driver::Allocator* allocator);

  // Memory aligned copy of parameters.
  Buffer parameters_;

  // Mapped parameters.
  MappedDeviceBuffer mapped_parameters_;

  // Scratch buffer, if the executable requires scratch space. If not, then the
  // buffer will be invalid.
  Buffer scratch_;

  // Holds the backing executable.
  const Executable* executable_;

  // Vector with list of input layer names.
  std::vector<std::string> input_layer_names_;

  // Vector with list of output layer names.
  std::vector<std::string> output_layer_names_;

  // Vector with detailed input layer information.
  std::vector<api::InputLayerInformation> inputs_;

  // Vector with detailed outpu layer information.
  std::vector<api::OutputLayerInformation> outputs_;

  // Maps input layer names to indices.
  std::unordered_map<std::string, int> input_map_;

  // Maps output layer names to indices.
  std::unordered_map<std::string, int> output_map_;

  mutable std::mutex instruction_buffers_vector_mutex_;
  std::vector<std::unique_ptr<InstructionBuffers>> instruction_buffers_vector_
      GUARDED_BY(instruction_buffers_vector_mutex_);
};

// Contains an executable package.
class PackageReference : public api::PackageReference {
 public:
  // This class is neither copyable nor movable.
  PackageReference(const PackageReference&) = delete;
  PackageReference& operator=(const PackageReference&) = delete;

  // Verifies the digital signature in the package.
  util::Status VerifySignature() const override {
    return verifier_->VerifySignature(package_buffer_.ptr());
  }

  // The following methods just call their counterpart in
  // MainExecutableReference().
  util::StatusOr<int> InputIndex(const std::string& name) const override {
    return MainExecutableReference()->InputIndex(name);
  }

  util::StatusOr<int> OutputIndex(const std::string& name) const override {
    return MainExecutableReference()->OutputIndex(name);
  }

  int NumInputLayers() const override {
    return MainExecutableReference()->NumInputLayers();
  }

  int NumOutputLayers() const override {
    return MainExecutableReference()->NumOutputLayers();
  }

  const std::vector<std::string>& InputLayerNames() const override {
    return MainExecutableReference()->InputLayerNames();
  }

  const std::vector<std::string>& OutputLayerNames() const override {
    return MainExecutableReference()->OutputLayerNames();
  }

  const api::InputLayerInformation* InputLayer(int index) const override {
    return MainExecutableReference()->InputLayer(index);
  }

  const api::OutputLayerInformation* OutputLayer(int index) const override {
    return MainExecutableReference()->OutputLayer(index);
  }

  int InputLayerSizeBytes(int index) const override {
    return MainExecutableReference()->InputLayerSizeBytes(index);
  }

  int OutputLayerSizeBytes(int index) const override {
    return MainExecutableReference()->OutputLayerSizeBytes(index);
  }

  int InputLayerSize(int index) const override {
    return MainExecutableReference()->InputLayerSize(index);
  }

  int OutputLayerSize(int index) const override {
    return MainExecutableReference()->OutputLayerSize(index);
  }

  util::StatusOr<int> InputLayerSizeBytes(
      const std::string& name) const override {
    return MainExecutableReference()->InputLayerSizeBytes(name);
  }

  util::StatusOr<int> OutputLayerSizeBytes(
      const std::string& name) const override {
    return MainExecutableReference()->OutputLayerSizeBytes(name);
  }

  std::string InputLayerName(int index) const override {
    return MainExecutableReference()->InputLayerName(index);
  }

  std::string OutputLayerName(int index) const override {
    return MainExecutableReference()->OutputLayerName(index);
  }

  int BatchSize() const override {
    return MainExecutableReference()->BatchSize();
  }

  // TODO(b/112942854) This method is ambiguous, either remove or change.
  int NumInstructionBitstreams() const override {
    return MainExecutableReference()->NumInstructionBitstreams();
  }

  // TODO(b/112942854) This method is ambiguous, either remove or change.
  int BitstreamSize(int index) const override {
    return MainExecutableReference()->BitstreamSize(index);
  }

  // Returns the total size (in bytes) of the parameters blob (both in parameter
  // caching and exection executable.
  //
  // TODO(b/112942854) This method is ambiguous, either remove or change.
  int ParametersSize() const override;

  // Returns a vector of all executable references in this package.
  std::vector<driver::ExecutableReference*> AllExecutableReferences() const;

  // Returns the main executable reference to refer to for read-only information
  // (e.g. number of layers).
  const driver::ExecutableReference* MainExecutableReference() const {
    return standalone_reference_ ? standalone_reference_.get()
                                 : inference_reference_.get();
  }

  // Returns true if this package is parameter-caching-enabled.
  bool ParameterCachingEnabled() const {
    return parameter_caching_reference_ != nullptr;
  }

  // Returns the inference executable reference in this package. You can make
  // sure such reference exists by calling ParameterCachingEnabled method.
  const driver::ExecutableReference* InferenceExecutableReference() const {
    return inference_reference_.get();
  }

  // Returns the parameter-caching executable reference in this package. You can
  // make sure such reference exists by calling ParameterCachingEnabled method.
  const driver::ExecutableReference* ParameterCachingExecutableReference()
      const {
    return parameter_caching_reference_.get();
  }

 private:
  friend class ExecutableRegistry;

  // Allow constructions through the ExecutableRegistry class only.
  //
  // The current implementation allows either a stand-alone executable or
  // parameter-caching + inference.

  // Constructor for stand-alone executable.
  PackageReference(const Buffer& package_buffer,
                   const Executable* standalone_executable,
                   driver::Allocator* allocator, ExecutableVerifier* verifier);

  // Constructor for a parameter cached package.
  PackageReference(const Buffer& package_buffer,
                   const Executable* parameter_caching_executable,
                   const Executable* inference_executable,
                   driver::Allocator* allocator, ExecutableVerifier* verifier);

  // Unmaps parameters of all executables in this package.
  util::Status UnmapParameters() {
    if (standalone_reference_ != nullptr) {
      RETURN_IF_ERROR(standalone_reference_->mapped_parameters_.Unmap());
    }
    if (parameter_caching_reference_ != nullptr) {
      RETURN_IF_ERROR(parameter_caching_reference_->mapped_parameters_.Unmap());
    }
    if (inference_reference_ != nullptr) {
      RETURN_IF_ERROR(inference_reference_->mapped_parameters_.Unmap());
    }
    return util::Status();  // OK.
  }

  // Buffer backing the package buffer.
  Buffer package_buffer_;

  // A ExecutableVerifier for checking digital signatures on executable
  // packages.
  const ExecutableVerifier* const verifier_;

  // The ExecutableReference for the parameter-caching executable.
  std::unique_ptr<driver::ExecutableReference> parameter_caching_reference_;

  // The Executable reference for the inference executable.
  std::unique_ptr<driver::ExecutableReference> inference_reference_;

  // The Executable reference for the stand-alone executable.
  std::unique_ptr<driver::ExecutableReference> standalone_reference_;
};

// A registry for executable files. Most methods do not have built-in thread-
// safety and rely on base driver class to ensure that. Some methods require
// thread-safety with respect to their call-sites in base driver and that is
// implemented in this class.
class ExecutableRegistry {
 public:
  // Constructs an ExecutableRegistry that does not check if the executables
  // being registered are for the correct device. Please prefer the other
  // overload when the chip is known.
  ExecutableRegistry();

  // Constructs an ExecutableRegistry that will check to make sure all
  // registered executables are for the correct chip.
  explicit ExecutableRegistry(api::Chip chip);

  // Constructs an ExecutableRegistry that can verify digital signatures in
  // executable packages that get registered.
  ExecutableRegistry(api::Chip chip,
                     std::unique_ptr<ExecutableVerifier> executable_verifier);
  ~ExecutableRegistry() = default;

  // This class is neither copyable nor movable.
  ExecutableRegistry(const ExecutableRegistry&) = delete;
  ExecutableRegistry& operator=(const ExecutableRegistry&) = delete;

  // Registers a serialized executable binary. Once the executable is
  // registered, driver has its own copy of it so there would be no need to keep
  // the executable_content in memory.
  util::StatusOr<const api::PackageReference*> RegisterSerialized(
      const std::string& executable_content);
  util::StatusOr<const api::PackageReference*> RegisterSerialized(
      const char* executable_content, size_t length);

  // Same as above, but the executable is read from the given file.
  util::StatusOr<const api::PackageReference*> RegisterFile(
      const std::string& executable_filename);

  // Unregisters an executable. Invokes the callback to unmap the parameter.
  util::Status Unregister(const api::PackageReference* executable_reference);

  // Unregisteres all registered executables.
  void UnregisterAll() LOCKS_EXCLUDED(parameter_caching_ref_mutex_);

  // Returns the number of registered executables.
  int GetRegistrySize() const { return registrations_.size(); }

  // Takes an executable reference to a registered inference execuatble and
  // returns its parameter-caching if it exists. If not it returns nullptr.
  const ExecutableReference* GetParameterCachingExecutableReference(
      const ExecutableReference* inference_executable_reference)
      LOCKS_EXCLUDED(parameter_caching_ref_mutex_);

 private:
  // Registers an executable package binary.
  util::StatusOr<const api::PackageReference*> RegisterPackage(
      const Buffer& package_buffer);

  // Takes in a multi-executable and returns a map of each executable type to
  // its executable pointer. It will return an error in any illegal combination.
  // Legitimate combinations are:
  //
  //   1. A single executable (no matter what type).
  //   2. 1 parameter-caching and 1 inference executable.
  //   3. 1 parameter-caching, 1 inference and 1 stand-alone executable.
  util::StatusOr<std::unordered_map<ExecutableType, const Executable*>>
  ExtractExecutables(const MultiExecutable& multi_executable) const;

  // Fetches an Executable from its serialized version and performs some
  // verification checks (does not include signature verification).
  util::StatusOr<const Executable*> FetchAndVerifyExecutable(
      const char* executable_serialized, size_t length) const;

  // If applicable, sets/unsets a reference from the inference executable
  // reference to it's parameter caching reference.
  void SetParameterCachingReference(
      const api::PackageReference* api_executable_ref)
      LOCKS_EXCLUDED(parameter_caching_ref_mutex_);
  void UnsetParameterCachingReference(
      const api::PackageReference* api_executable_ref)
      LOCKS_EXCLUDED(parameter_caching_ref_mutex_);

  // Allocator.
  AlignedAllocator allocator_;

  // Tracks registrations.
  // Uses a map instead of a set, since looking up an std::set of unique_ptr is
  // tricky.
  std::unordered_map<const api::PackageReference*,
                     std::unique_ptr<api::PackageReference>>
      registrations_;

  // Executables will be checked to ensure they were compiled for this chip.
  // Can be kUnknown to disable checking.
  const api::Chip chip_;

  // A verifier for checking digital signatures on executable packages.
  std::unique_ptr<ExecutableVerifier> verifier_;

  // A mutex to synchronize access to parameter_caching_refs_.
  mutable std::mutex parameter_caching_ref_mutex_;

  // A map from inference executable references to their corresponding parameter
  // caching references. If an executable reference does not exist in this map,
  // we can conclude it is stand-alone.
  std::unordered_map<const ExecutableReference*, const ExecutableReference*>
      parameter_caching_refs_ GUARDED_BY(parameter_caching_ref_mutex_);
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_REGISTRY_H_
