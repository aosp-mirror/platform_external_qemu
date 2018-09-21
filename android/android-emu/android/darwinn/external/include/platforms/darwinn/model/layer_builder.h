// Functions to build up layers.
#ifndef PLATFORMS_DARWINN_MODEL_LAYER_BUILDER_H_
#define PLATFORMS_DARWINN_MODEL_LAYER_BUILDER_H_

#include <string>
#include <vector>

#if defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "platforms/darwinn/model/config/array.pb.h"
#include "platforms/darwinn/model/config/internal.pb.h"
#include "platforms/darwinn/model/config/representation.pb.h"
#else
#include "platforms/darwinn/model/config/array.proto.h"
#include "platforms/darwinn/model/config/internal.proto.h"
#include "platforms/darwinn/model/config/representation.proto.h"
#endif

#include "platforms/darwinn/model/minmax.h"

#ifndef HAS_GLOBAL_STRING
using std::string;
#endif

namespace platforms {
namespace darwinn {
namespace model {

// Builds a classifier layer.
config::Layer BuildClassifierLayer(const string& name,
                                   const config::ArrayInfo& array_info,
                                   config::ClassifierLayer::Op op, float beta,
                                   const string& input_layer_name);

// Builds a concatenation layer.
//
// Note: ArrayInfo is not an argument, because this layer infers its ArrayInfo
// from its input layer(s). It does not make sense to have a different output
// DataType, nor different QuantizationParameters.
config::Layer BuildConcatenationLayer(
    const string& name, config::ConcatenationLayer::Mode mode,
    const std::vector<string>& input_layer_names);

// Builds a convolution layer.
config::Layer BuildConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int z_out_dim, int kernel_y_dim, int kernel_x_dim, int y_stride,
    int x_stride, const string& input_layer_name);

// Builds a convolution layer with custom padding.
config::Layer BuildConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const config::PaddingAmount& pad, int z_out_dim, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride,
    const string& input_layer_name);

// Builds a dilated convolution layer.
config::Layer BuildDilatedConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int z_out_dim, int kernel_y_dim, int kernel_x_dim, int y_stride,
    int x_stride, int dilation_rate, const string& input_layer_name);

// Builds a dilated convolution layer with custom padding.
config::Layer BuildDilatedConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const config::PaddingAmount& pad, int z_out_dim, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride, int dilation_rate,
    const string& input_layer_name);

// Builds a transposed convolution layer.
config::Layer BuildTransposedConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int y_out_dim, int x_out_dim, int z_out_dim, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride,
    const string& input_layer_name);

// Builds a depthwise layer.
config::Layer BuildDepthwiseConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int kernel_y_dim, int kernel_x_dim, int y_stride, int x_stride,
    int depth_multiplier, const string& input_layer_name);

// Builds a depthwise layer with custom padding.
config::Layer BuildDepthwiseConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const config::PaddingAmount& pad, int kernel_y_dim, int kernel_x_dim,
    int y_stride, int x_stride, int depth_multiplier,
    const string& input_layer_name);

// Builds a fully-connected layer.
config::Layer BuildFullyConnectedLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, int z_out_dim,
    const string& input_layer_name);

// Builds rescaling layer.
config::Layer BuildRescalingLayer(
    const string& name, const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const string& input_layer_name);

// Builds an input layer.
config::Layer BuildInputLayer(const string& name,
                              const config::ArrayInfo& array_info,
                              int y_in_size, int x_in_size, int z_in_size);

// Builds an input layer with padded z dimension.
config::Layer BuildInputLayer(const config::Layer& config, int z_out_size);

// Builds a input shard layer.
config::Layer BuildInputShardLayer(const string& name,
                                   const config::Layer& original_input_layer,
                                   int y_shard_begin, int y_shard_dim);

// Builds a input shard layer with padded z dimension.
config::Layer BuildInputShardLayer(const config::Layer& config, int z_out_dim);

// Builds a constant layer.
config::Layer BuildConstantLayer(const string& name,
                                 const config::ArrayInfo& array_info,
                                 int y_in_size, int x_in_size, int z_in_size);

// Builds an output layer.
config::Layer BuildOutputLayer(const string& name, int z_out_dim,
                               const string& input_layer_name);

// Builds an output shard layer.
config::Layer BuildOutputShardLayer(const string& name,
                                    const config::Layer& original_output_config,
                                    int y_original_dim, int y_shard_begin,
                                    int y_shard_dim,
                                    const string& input_layer_name);

// Builds a spill layer.
config::Layer BuildSpillLayer(const string& name,
                              config::SpillLocation spill_location,
                              const string& input_layer_name);

// Builds a fetch layer.
config::Layer BuildFetchLayer(const string& name,
                              config::SpillLocation spill_location,
                              const string& input_layer_name);

// Builds a biased normalization layer.
config::Layer BuildBiasedNormalizationLayer(
    const string& name, const config::ArrayInfo& output_info, float alpha,
    const string& input_layer_name);

// Builds a L2 normalization layer.
config::Layer BuildL2NormalizationLayer(const string& name,
                                        const config::ArrayInfo& array_info,
                                        const string& input_layer_name);

// Builds a pooling layer.
config::Layer BuildPoolingLayer(
    const string& name, const config::ArrayInfo& array_info,
    config::PoolingLayer::Pool pool,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int kernel_y_dim, int kernel_x_dim, int y_stride, int x_stride,
    const string& input_layer_name);

// Builds a pooling layer with default activation function = none.
config::Layer BuildPoolingLayer(const string& name,
                                const config::ArrayInfo& array_info,
                                config::PoolingLayer::Pool pool,
                                config::Pad pad, int kernel_y_dim,
                                int kernel_x_dim, int y_stride, int x_stride,
                                const string& input_layer_name);

// Builds a pooling layer with custom padding, and default activation function =
// NONE.
config::Layer BuildPoolingLayer(const string& name,
                                const config::ArrayInfo& array_info,
                                config::PoolingLayer::Pool pool,
                                const config::PaddingAmount& pad,
                                int kernel_y_dim, int kernel_x_dim,
                                int y_stride, int x_stride,
                                const string& input_layer_name);

// Builds a reshape layer.
//
// Note: ArrayInfo is not an argument, because this layer infers its ArrayInfo
// from its input layer(s). It does not make sense to have a different output
// DataType, nor different QuantizationParameters.
config::Layer BuildReshapeLayer(const string& name, int y_out_dim,
                                int x_out_dim, int z_out_dim,
                                const string& input_layer_name);

// Builds a tensor slice layer.
//
// Note: ArrayInfo is not an argument, because this layer infers its ArrayInfo
// from its input layer(s). It does not make sense to have a different output
// DataType, nor different QuantizationParameters.
config::Layer BuildSliceLayer(const string& name, config::SliceLayer::Mode mode,
                              int in_begin, int in_size,
                              const string& input_layer_name);

// Builds an identity layer.
config::Layer BuildIdentityLayer(const string& name,
                                 const config::ArrayInfo& array_info,
                                 const string& input_layer_name);

// Builds an image interpolation layer.
config::Layer BuildImageInterpolationLayer(
    const string& name, const config::ArrayInfo& output_info,
    config::ImageInterpolationLayer::Algorithm algorithm,
    config::ImageInterpolationLayer::StrideMethod stride_method, int y_size,
    int x_size, const string& input_layer_name);

// Builds an image interpolation layer with custom start offset and stride.
config::Layer BuildImageInterpolationLayer(
    const string& name, const config::ArrayInfo& output_info,
    config::ImageInterpolationLayer::Algorithm algorithm,
    const config::ImageInterpolationLayer::StartOffsetAndStride& y,
    const config::ImageInterpolationLayer::StartOffsetAndStride& x, int y_size,
    int x_size, const string& input_layer_name);

// Builds a vector layer.
config::Layer BuildVectorLayer(
    const string& name, const config::ArrayInfo& array_info,
    config::VectorLayer::Op op,
    config::Layer::ActivationFunction activation_function,
    const std::vector<string>& input_layer_names);

// Builds an interleave layer.
config::Layer BuildInterleaveLayer(
    const string& name, config::InterleaveLayer::Mode mode,
    const std::vector<string>& input_layer_names);

// Builds a space-to-depth layer.
config::Layer BuildSpaceToDepthLayer(const string& name, int block_size,
                                     const string& input_layer_name);

// Builds a depth-to-spacelayer.
config::Layer BuildDepthToSpaceLayer(const string& name, int block_size,
                                     const string& input_layer_name);

// Builds a space-to-batch layer.
config::Layer BuildSpaceToBatchLayer(
    const string& name, int block_size,
    const config::PaddingAmount& padding_amount,
    const string& input_layer_name);

// Builds a batch-to-space layer.
config::Layer BuildBatchToSpaceLayer(const string& name, int block_size,
                                     const config::PaddingAmount& crop_amount,
                                     const string& input_layer_name);

// Builds a zero-padding layer.
//
// Note: ArrayInfo is not an argument, because this layer infers its ArrayInfo
// from its input layer(s). It does not make sense to have a different output
// DataType, nor different QuantizationParameters.
config::Layer BuildZeroPaddingLayer(
    const string& name, config::ZeroPaddingLayer::Dimension dimension,
    int pre_padding, int post_padding, const string& input_layer_name);

// Returns ArrayInfo with data_type set.
config::ArrayInfo ArrayType(config::DataType data_type);

// Returns ArrayInfo with data_type, real_range set.
config::ArrayInfo ArrayTypeRange(config::DataType data_type, double minimum,
                                 double maximum);

// Returns ArrayInfo with 'data_type' and real ranges set for each channel. This
// can be used for per-channel quantization.
config::ArrayInfo ArrayTypeRanges(config::DataType data_type,
                                  const std::vector<MinMax<double>>& minmax);

// Returns an ArrayInfo object corresponding to fixed point types, with the
// given scale and zero-point. Check fails internally if floating point types
// are supplied.
config::ArrayInfo FixedPointArrayInfo(config::DataType data_type, double scale,
                                      int zero_point);

}  // namespace model
}  // namespace darwinn
}  // namespace platforms

#endif  // PLATFORMS_DARWINN_MODEL_LAYER_BUILDER_H_
