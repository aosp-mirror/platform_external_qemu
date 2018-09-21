#ifndef THIRD_PARTY_DARWINN_API_LAYER_INFORMATION_H_
#define THIRD_PARTY_DARWINN_API_LAYER_INFORMATION_H_

#include <string>

#include "third_party/darwinn/api/executable_generated.h"

namespace platforms {
namespace darwinn {
namespace api {

// Provides information on input and output layers.
class LayerInformation {
 public:
  virtual ~LayerInformation() = default;

  // Copyable.
  LayerInformation(const LayerInformation& rhs) = default;
  LayerInformation& operator=(const LayerInformation& rhs) = default;

  // Returns layer name.
  std::string name() const { return layer_->name()->str(); }

  // Returns the expected size of activations for given layer.
  int size_bytes() const { return layer_->size_bytes(); }

  // Layer dimensions.
  int x_dim() const { return layer_->x_dim(); }
  int y_dim() const { return layer_->y_dim(); }
  int z_dim() const { return layer_->z_dim(); }

  // Returns the zero point value.
  int zero_point() const { return layer_->numerics()->zero_point(); }

  // Returns the dequantization factor.
  float dequantization_factor() const {
    return layer_->numerics()->dequantization_factor();
  }

  // Returns data type in this layer.
  darwinn::DataType data_type() const { return layer_->data_type(); }

 protected:
  explicit LayerInformation(const Layer* layer);

  const Layer* layer() const { return layer_; }

 private:
  const Layer* layer_;
};

// Provides detailed information on input layers.
class InputLayerInformation : public LayerInformation {
 public:
  explicit InputLayerInformation(const Layer* layer);
  ~InputLayerInformation() override = default;
};

// Provides detailed information on output layers.
class OutputLayerInformation : public LayerInformation {
 public:
  explicit OutputLayerInformation(const Layer* layer);
  ~OutputLayerInformation() override = default;

  // Returns an index value of output buffer for a given tensor coordinate.
  int GetBufferIndex(int y, int x, int z) const;

  // Relayout the source DarwiNN output buffer (TYXZ layout, T = Tile) into
  // user output buffer (YXZ layout).
  void Relayout(unsigned char* dest, unsigned char* src) const;

 private:
  // Returns the size of the data type in this layer in bytes.
  int DataTypeSize() const;

  const OutputLayer* output_layer_;
};

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_LAYER_INFORMATION_H_
