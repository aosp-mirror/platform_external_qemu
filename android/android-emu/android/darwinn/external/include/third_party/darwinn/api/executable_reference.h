#ifndef THIRD_PARTY_DARWINN_API_EXECUTABLE_REFERENCE_H
#define THIRD_PARTY_DARWINN_API_EXECUTABLE_REFERENCE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/layer_information.h"
#include "third_party/darwinn/port/statusor.h"

namespace platforms {
namespace darwinn {
namespace api {

// Type for a registered executable.
class ExecutableReference {
 public:
  virtual ~ExecutableReference() = default;

  // This class is neither copyable nor movable.
  ExecutableReference(const ExecutableReference&) = delete;
  ExecutableReference& operator=(const ExecutableReference&) = delete;

  // Verifies the digital signature of the backing executable package.
  virtual util::Status VerifySignature() const = 0;

  // Returns the index of input layer with given name.
  virtual util::StatusOr<int> InputIndex(const std::string& name) const = 0;

  // Returns the index of output layer with given name.
  virtual util::StatusOr<int> OutputIndex(const std::string& name) const = 0;

  // Returns number of input layers.
  virtual int NumInputLayers() const = 0;

  // Returns number of output layers.
  virtual int NumOutputLayers() const = 0;

  // Returns list of input layer names.
  virtual const std::vector<std::string>& InputLayerNames() const = 0;

  // Returns list of output layer names.
  virtual const std::vector<std::string>& OutputLayerNames() const = 0;

  // Returns information on given input layer. Returns nullptr if index is out
  // of bounds.
  virtual const InputLayerInformation* InputLayer(int index) const = 0;

  // Returns information on given output layer. Returns nullptr if index is out
  // of bounds.
  virtual const OutputLayerInformation* OutputLayer(int index) const = 0;

  // Returns the expected byte size of activations for given input layer index.
  // This is post-padding, if any.
  virtual int InputLayerSizeBytes(int index) const = 0;

  // Returns the expected byte size of activations for given output layer index.
  // This is post-padding, if any.
  virtual int OutputLayerSizeBytes(int index) const = 0;

  // Returns the expected size (in value count) of activations for given input
  // layer index. This is pre-padding, if any.
  virtual int InputLayerSize(int index) const = 0;

  // Returns the expected size (in value count) of activations for given input
  // layer index. This is pre-padding, if any.
  virtual int OutputLayerSize(int index) const = 0;

  // Returns the expected size of activations for given input layer.
  // Prefer index based APIs for performance.
  virtual util::StatusOr<int> InputLayerSizeBytes(
      const std::string& name) const = 0;

  // Returns the expected size of activations for given output layer.
  // Prefer index based APIs for performance.
  virtual util::StatusOr<int> OutputLayerSizeBytes(
      const std::string& name) const = 0;

  // Returns name for given input layer index.
  virtual std::string InputLayerName(int index) const = 0;

  // Returns name for given output layer index.
  virtual std::string OutputLayerName(int index) const = 0;

  // Returns batch size.
  virtual int BatchSize() const = 0;

  // Returns the size (in bytes) of the parameters blob.
  virtual int ParametersSize() const = 0;

  // Returns number of instruction bitstreams.
  virtual int NumInstructionBitstreams() const = 0;

  // Returns the bitstream size (in bytes) in the instruction bitstream at
  // provided index.
  virtual int BitstreamSize(int index) const = 0;

 protected:
  ExecutableReference() = default;
};

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_EXECUTABLE_REFERENCE_H
