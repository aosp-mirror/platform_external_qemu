#include "third_party/darwinn/api/layer_information.h"

#include <string>
#include <vector>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace api {

LayerInformation::LayerInformation(const Layer* layer) : layer_(layer) {
  CHECK(layer != nullptr);
}

InputLayerInformation::InputLayerInformation(const Layer* layer)
    : LayerInformation(layer) {}

OutputLayerInformation::OutputLayerInformation(const Layer* layer)
    : LayerInformation(layer),
      output_layer_(layer->any_layer_as_OutputLayer()) {
  CHECK(output_layer_ != nullptr);
}

int OutputLayerInformation::GetBufferIndex(int y, int x, int z) const {
  const auto& layout = output_layer_->layout();

  const int linear_tile_id =
      layout->y_coordinate_to_linear_tile_id_map()->Get(y) +
      layout->x_coordinate_to_linear_tile_id_map()->Get(x);
  const int global_tile_byte_offset =
      layout->linearized_tile_byte_offset()->Get(linear_tile_id);

  const int local_y_coordinate =
      layout->y_coordinate_to_local_y_offset()->Get(y);

  const int local_x_byte_offset =
      layout->x_coordinate_to_local_byte_offset()->Get(x);
  const int local_y_byte_offset =
      local_y_coordinate * layout->x_coordinate_to_local_y_row_size()->Get(x);

  return global_tile_byte_offset + local_y_byte_offset + local_x_byte_offset +
         z;
}

void OutputLayerInformation::Relayout(unsigned char* dest,
                                      unsigned char* src) const {
  const int z_bytes = z_dim() * DataTypeSize();

  if (y_dim() == 1 && x_dim() == 1) {
    // 1 dimensional output (only z-dimension).
    if (src != dest) {
      memcpy(dest, src, z_bytes);
    }
  } else {
    for (int y = 0; y < y_dim(); ++y) {
      for (int x = 0; x < x_dim(); ++x) {
        const int src_offset = GetBufferIndex(y, x, /*z=*/0);
        memcpy(dest, src + src_offset, z_bytes);
        dest += z_bytes;
      }
    }
  }
}

int OutputLayerInformation::DataTypeSize() const {
  // TODO(b/113062082): Remove the code once data_type is deprecated from
  // output_layer.
  DataType type;
  if (flatbuffers::IsFieldPresent(layer(), Layer::VT_DATA_TYPE)) {
    type = layer()->data_type();
  } else {
    // Old binary, using the data_type from OutputLayer.
    type = output_layer_->data_type();
  }
  switch (type) {
    case DataType_FIXED_POINT8:
      return 1;
    case DataType_FIXED_POINT16:
      return 2;
    case DataType_FIXED_POINT32:
      return 4;
    case DataType_BFLOAT:
      return 2;
    case DataType_HALF:
      return 2;
    case DataType_SINGLE:
      return 4;
  }
}

}  // namespace api
}  // namespace darwinn
}  // namespace platforms
