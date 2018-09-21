#include "platforms/darwinn/model/layer_builder.h"

#include <limits>

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
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace model {
namespace {

// Fills layer config with given "input_layer_names".
void SetConfigInputLayers(const std::vector<string>& input_layer_names,
                          config::Layer* layer_config) {
  CHECK(layer_config != nullptr);
  for (const auto& input_layer_name : input_layer_names) {
    layer_config->add_input_layers(input_layer_name);
  }
}

// Returns a default activation function for given "pool".
config::Layer::ActivationFunction GetDefaultActivationFunction(
    config::PoolingLayer::Pool pool) {
  if (pool == config::PoolingLayer::AVERAGE) {
    // For average pooling, the activation function is used for averaging, so
    // such pooling cannot be combined with another activation function.
    return config::Layer::LINEAR;
  }
  return config::Layer::NONE;
}

}  // namespace

config::Layer BuildDilatedConvolutionLayerWithPaddingNotSet(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, int z_out_dim,
    int kernel_y_dim, int kernel_x_dim, int y_stride, int x_stride,
    int dilation_rate, const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  layer_config.set_kernel_y_dim(kernel_y_dim);
  layer_config.set_kernel_x_dim(kernel_x_dim);
  layer_config.set_y_stride(y_stride);
  layer_config.set_x_stride(x_stride);
  layer_config.set_activation_function(activation_function);
  *layer_config.mutable_parameter_array_info() = parameter_array_info;
  *layer_config.mutable_output_array() = output_array_info;
  layer_config.set_dilation_rate(dilation_rate);
  config::ConvolutionLayer* config = layer_config.mutable_convolution();
  config->set_z_out_dim(z_out_dim);

  return layer_config;
}

config::Layer BuildDepthwiseConvolutionLayerWithPaddingNotSet(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride, int depth_multiplier,
    const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  layer_config.set_kernel_y_dim(kernel_y_dim);
  layer_config.set_kernel_x_dim(kernel_x_dim);
  layer_config.set_y_stride(y_stride);
  layer_config.set_x_stride(x_stride);
  layer_config.set_activation_function(activation_function);
  *layer_config.mutable_parameter_array_info() = parameter_array_info;
  *layer_config.mutable_output_array() = output_array_info;
  config::DepthwiseConvolutionLayer* config =
      layer_config.mutable_depthwise_convolution();
  config->set_depth_multiplier(depth_multiplier);

  return layer_config;
}

config::Layer BuildRescalingLayer(
    const string& name, const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);
  layer_config.set_activation_function(activation_function);
  *layer_config.mutable_output_array() = output_array_info;
  layer_config.mutable_rescaling();
  return layer_config;
}

config::Layer BuildClassifierLayer(const string& name,
                                   const config::ArrayInfo& array_info,
                                   config::ClassifierLayer::Op op, float beta,
                                   const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  *layer_config.mutable_output_array() = array_info;
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config::ClassifierLayer* config = layer_config.mutable_classifier();
  config->set_op(op);
  config->set_beta(beta);
  return layer_config;
}

config::Layer BuildConcatenationLayer(
    const string& name, config::ConcatenationLayer::Mode mode,
    const std::vector<string>& input_layer_names) {
  config::Layer layer_config;
  config::ConcatenationLayer* config = layer_config.mutable_concatenation();
  layer_config.set_name(name);
  SetConfigInputLayers(input_layer_names, &layer_config);

  config->set_mode(mode);
  return layer_config;
}

config::Layer BuildConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int z_out_dim, int kernel_y_dim, int kernel_x_dim, int y_stride,
    int x_stride, const string& input_layer_name) {
  // Convolution layer is equal to dilated convolution with dilation rate = 1.
  config::Layer layer_config = BuildDilatedConvolutionLayerWithPaddingNotSet(
      name, parameter_array_info, output_array_info, activation_function,
      z_out_dim, kernel_y_dim, kernel_x_dim, y_stride, x_stride,
      /*dilation_rate=*/1, input_layer_name);
  layer_config.set_pad(pad);
  return layer_config;
}

config::Layer BuildConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const config::PaddingAmount& pad, int z_out_dim, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride,
    const string& input_layer_name) {
  // Convolution layer is equal to dilated convolution with dilation rate = 1.
  config::Layer layer_config = BuildDilatedConvolutionLayerWithPaddingNotSet(
      name, parameter_array_info, output_array_info, activation_function,
      z_out_dim, kernel_y_dim, kernel_x_dim, y_stride, x_stride,
      /*dilation_rate=*/1, input_layer_name);
  layer_config.set_pad(config::CUSTOM);
  *layer_config.mutable_padding_amount() = pad;
  return layer_config;
}

config::Layer BuildDilatedConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int z_out_dim, int kernel_y_dim, int kernel_x_dim, int y_stride,
    int x_stride, int dilation_rate, const string& input_layer_name) {
  config::Layer layer_config = BuildDilatedConvolutionLayerWithPaddingNotSet(
      name, parameter_array_info, output_array_info, activation_function,
      z_out_dim, kernel_y_dim, kernel_x_dim, y_stride, x_stride, dilation_rate,
      input_layer_name);
  layer_config.set_pad(pad);
  return layer_config;
}

config::Layer BuildDilatedConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const config::PaddingAmount& pad, int z_out_dim, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride, int dilation_rate,
    const string& input_layer_name) {
  config::Layer layer_config = BuildDilatedConvolutionLayerWithPaddingNotSet(
      name, parameter_array_info, output_array_info, activation_function,
      z_out_dim, kernel_y_dim, kernel_x_dim, y_stride, x_stride, dilation_rate,
      input_layer_name);
  layer_config.set_pad(config::CUSTOM);
  *layer_config.mutable_padding_amount() = pad;
  return layer_config;
}

config::Layer BuildTransposedConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int y_out_dim, int x_out_dim, int z_out_dim, int kernel_y_dim,
    int kernel_x_dim, int y_stride, int x_stride,
    const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  layer_config.set_kernel_y_dim(kernel_y_dim);
  layer_config.set_kernel_x_dim(kernel_x_dim);
  layer_config.set_y_stride(y_stride);
  layer_config.set_x_stride(x_stride);
  layer_config.set_pad(pad);
  layer_config.set_activation_function(activation_function);
  *layer_config.mutable_parameter_array_info() = parameter_array_info;
  *layer_config.mutable_output_array() = output_array_info;

  config::TransposedConvolutionLayer* config =
      layer_config.mutable_transposed_convolution();
  config->set_y_out_dim(y_out_dim);
  config->set_x_out_dim(x_out_dim);
  config->set_z_out_dim(z_out_dim);
  return layer_config;
}

config::Layer BuildDepthwiseConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int kernel_y_dim, int kernel_x_dim, int y_stride, int x_stride,
    int depth_multiplier, const string& input_layer_name) {
  config::Layer layer_config = BuildDepthwiseConvolutionLayerWithPaddingNotSet(
      name, parameter_array_info, output_array_info, activation_function,
      kernel_y_dim, kernel_x_dim, y_stride, x_stride, depth_multiplier,
      input_layer_name);
  layer_config.set_pad(pad);
  return layer_config;
}

config::Layer BuildDepthwiseConvolutionLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function,
    const config::PaddingAmount& pad, int kernel_y_dim, int kernel_x_dim,
    int y_stride, int x_stride, int depth_multiplier,
    const string& input_layer_name) {
  config::Layer layer_config = BuildDepthwiseConvolutionLayerWithPaddingNotSet(
      name, parameter_array_info, output_array_info, activation_function,
      kernel_y_dim, kernel_x_dim, y_stride, x_stride, depth_multiplier,
      input_layer_name);
  layer_config.set_pad(config::CUSTOM);
  *layer_config.mutable_padding_amount() = pad;
  return layer_config;
}

config::Layer BuildFullyConnectedLayer(
    const string& name, const config::ArrayInfo& parameter_array_info,
    const config::ArrayInfo& output_array_info,
    config::Layer::ActivationFunction activation_function, int z_out_dim,
    const string& input_layer_name) {
  config::Layer layer_config;
  config::FullyConnectedLayer* config = layer_config.mutable_fully_connected();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  layer_config.set_activation_function(activation_function);
  *layer_config.mutable_parameter_array_info() = parameter_array_info;
  *layer_config.mutable_output_array() = output_array_info;
  config->set_z_out_dim(z_out_dim);

  return layer_config;
}

config::Layer BuildInputLayer(const string& name,
                              const config::ArrayInfo& array_info,
                              int y_in_size, int x_in_size, int z_in_size) {
  config::Layer layer_config;
  layer_config.set_name(name);
  *layer_config.mutable_output_array() = array_info;

  config::InputLayer* config = layer_config.mutable_input();
  config->set_y_in_dim(y_in_size);
  config->set_x_in_dim(x_in_size);
  config->set_z_in_dim(z_in_size);

  config->set_z_out_dim(z_in_size);

  return layer_config;
}

config::Layer BuildInputLayer(const config::Layer& config, int z_out_size) {
  CHECK(config.has_input());
  config::Layer layer_config = config;
  layer_config.mutable_input()->set_z_out_dim(z_out_size);
  return layer_config;
}

config::Layer BuildInputShardLayer(const string& name,
                                   const config::Layer& original_input_layer,
                                   int y_shard_begin, int y_shard_dim) {
  CHECK(original_input_layer.has_input());
  config::Layer layer_config;
  layer_config.set_name(name);
  *layer_config.mutable_output_array() = original_input_layer.output_array();

  config::InputShardLayer* config = layer_config.mutable_input_shard();
  config->set_input_layer_name(original_input_layer.name());
  config->set_sharded_y_in_begin(y_shard_begin);
  config->set_sharded_y_in_dim(y_shard_dim);

  config->set_y_in_dim(original_input_layer.input().y_in_dim());
  config->set_x_in_dim(original_input_layer.input().x_in_dim());
  config->set_z_in_dim(original_input_layer.input().z_in_dim());

  config->set_z_out_dim(original_input_layer.input().z_out_dim());
  return layer_config;
}

config::Layer BuildInputShardLayer(const config::Layer& config, int z_out_dim) {
  CHECK(config.has_input_shard());
  config::Layer layer_config = config;
  layer_config.mutable_input_shard()->set_z_out_dim(z_out_dim);
  return layer_config;
}

config::Layer BuildConstantLayer(const string& name,
                                 const config::ArrayInfo& array_info,
                                 int y_in_size, int x_in_size, int z_in_size) {
  config::Layer layer_config;
  layer_config.set_name(name);
  *layer_config.mutable_parameter_array_info() = array_info;
  *layer_config.mutable_output_array() = array_info;

  auto* config = layer_config.mutable_constant();
  config->set_y_in_dim(y_in_size);
  config->set_x_in_dim(x_in_size);
  config->set_z_in_dim(z_in_size);

  config->set_z_out_dim(z_in_size);

  return layer_config;
}

config::Layer BuildOutputLayer(const string& name, int z_out_dim,
                               const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  auto* config = layer_config.mutable_output();
  config->set_z_out_dim(z_out_dim);
  return layer_config;
}

config::Layer BuildOutputShardLayer(const string& name,
                                    const config::Layer& original_output_config,
                                    int y_original_dim, int y_shard_begin,
                                    int y_shard_dim,
                                    const string& input_layer_name) {
  CHECK(original_output_config.has_output());
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  auto* config = layer_config.mutable_output_shard();
  config->set_output_layer_name(original_output_config.name());
  config->set_sharded_y_out_begin(y_shard_begin);
  config->set_sharded_y_out_dim(y_shard_dim);
  config->set_y_out_dim(y_original_dim);
  config->set_z_out_dim(original_output_config.output().z_out_dim());
  return layer_config;
}

config::Layer BuildSpillLayer(const string& name,
                              config::SpillLocation spill_location,
                              const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config::SpillLayer* config = layer_config.mutable_spill();
  CHECK_NE(spill_location, config::SpillLocation::INVALID);
  config->set_spill_location(spill_location);

  return layer_config;
}

config::Layer BuildFetchLayer(const string& name,
                              config::SpillLocation spill_location,
                              const string& input_layer_name) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config::FetchLayer* config = layer_config.mutable_fetch();
  CHECK_NE(spill_location, config::SpillLocation::INVALID);
  config->set_spill_location(spill_location);

  return layer_config;
}

config::Layer BuildImageInterpolationLayer(
    const string& name, const config::ArrayInfo& output_info,
    config::ImageInterpolationLayer::Algorithm algorithm,
    config::ImageInterpolationLayer::StrideMethod stride_method, int y_size,
    int x_size, const string& input_layer_name) {
  config::Layer layer_config;
  config::ImageInterpolationLayer* config =
      layer_config.mutable_image_interpolation();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config->set_algorithm(algorithm);
  config->set_stride_method(stride_method);
  config->set_y_size(y_size);
  config->set_x_size(x_size);

  *layer_config.mutable_output_array() = output_info;

  return layer_config;
}

config::Layer BuildImageInterpolationLayer(
    const string& name, const config::ArrayInfo& output_info,
    config::ImageInterpolationLayer::Algorithm algorithm,
    const config::ImageInterpolationLayer::StartOffsetAndStride& y,
    const config::ImageInterpolationLayer::StartOffsetAndStride& x, int y_size,
    int x_size, const string& input_layer_name) {
  config::Layer layer_config;
  config::ImageInterpolationLayer* config =
      layer_config.mutable_image_interpolation();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config->set_algorithm(algorithm);
  config->set_stride_method(config::ImageInterpolationLayer::CUSTOM);
  *config->mutable_y() = y;
  *config->mutable_x() = x;
  config->set_y_size(y_size);
  config->set_x_size(x_size);

  *layer_config.mutable_output_array() = output_info;

  return layer_config;
}

config::Layer BuildBiasedNormalizationLayer(
    const string& name, const config::ArrayInfo& output_info, float alpha,
    const string& input_layer_name) {
  config::Layer layer_config;
  *layer_config.mutable_output_array() = output_info;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);
  layer_config.set_activation_function(config::Layer::RECIPROCAL_SQRT);

  config::NormalizationLayer* config = layer_config.mutable_normalization();
  config::NormalizationLayer::BiasedNormalization* biased_normalization =
      config->mutable_normalization_type()->mutable_biased_normalization();
  biased_normalization->set_alpha(alpha);

  return layer_config;
}

config::Layer BuildL2NormalizationLayer(const string& name,
                                        const config::ArrayInfo& array_info,
                                        const string& input_layer_name) {
  config::Layer layer_config;
  *layer_config.mutable_output_array() = array_info;
  config::NormalizationLayer* config = layer_config.mutable_normalization();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);
  layer_config.set_activation_function(config::Layer::RECIPROCAL_SQRT);

  config::NormalizationLayer::L2Normalization* l2_normalization =
      config->mutable_normalization_type()->mutable_l2_normalization();
  l2_normalization->clear_epsilon();

  return layer_config;
}

config::Layer BuildPoolingLayer(
    const string& name, const config::ArrayInfo& array_info,
    config::PoolingLayer::Pool pool,
    config::Layer::ActivationFunction activation_function, config::Pad pad,
    int kernel_y_dim, int kernel_x_dim, int y_stride, int x_stride,
    const string& input_layer_name) {
  if (pool == config::PoolingLayer::AVERAGE) {
    CHECK_EQ(activation_function, config::Layer::LINEAR)
        << "Average pooling requires activation function to be LINEAR";
  }

  config::Layer layer_config;
  *layer_config.mutable_output_array() = array_info;
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  layer_config.set_kernel_y_dim(kernel_y_dim);
  layer_config.set_kernel_x_dim(kernel_x_dim);
  layer_config.set_y_stride(y_stride);
  layer_config.set_x_stride(x_stride);
  layer_config.set_pad(pad);
  layer_config.set_activation_function(activation_function);

  config::PoolingLayer* config = layer_config.mutable_pooling();
  config->set_pool(pool);
  return layer_config;
}

config::Layer BuildPoolingLayer(const string& name,
                                const config::ArrayInfo& array_info,
                                config::PoolingLayer::Pool pool,
                                config::Pad pad, int kernel_y_dim,
                                int kernel_x_dim, int y_stride, int x_stride,
                                const string& input_layer_name) {
  return BuildPoolingLayer(
      name, array_info, pool, GetDefaultActivationFunction(pool), pad,
      kernel_y_dim, kernel_x_dim, y_stride, x_stride, input_layer_name);
}

config::Layer BuildPoolingLayer(const string& name,
                                const config::ArrayInfo& array_info,
                                config::PoolingLayer::Pool pool,
                                const config::PaddingAmount& pad,
                                int kernel_y_dim, int kernel_x_dim,
                                int y_stride, int x_stride,
                                const string& input_layer_name) {
  config::Layer layer_config;
  *layer_config.mutable_output_array() = array_info;
  config::PoolingLayer* config = layer_config.mutable_pooling();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  layer_config.set_kernel_y_dim(kernel_y_dim);
  layer_config.set_kernel_x_dim(kernel_x_dim);
  layer_config.set_y_stride(y_stride);
  layer_config.set_x_stride(x_stride);
  layer_config.set_pad(config::CUSTOM);
  *layer_config.mutable_padding_amount() = pad;
  layer_config.set_activation_function(GetDefaultActivationFunction(pool));
  config->set_pool(pool);

  return layer_config;
}

config::Layer BuildIdentityLayer(const string& name,
                                 const config::ArrayInfo& array_info,
                                 const string& input_layer_name) {
  // Current implementation uses 1x1 average pooling to implement identity
  // layer, and we can replace it with other implementation fine.
  return BuildPoolingLayer(name, array_info,
                           model::config::PoolingLayer::AVERAGE,
                           model::config::Pad::SAME,
                           /*kernel_y_dim=*/1, /*kernel_x_dim=*/1,
                           /*y_stride=*/1,
                           /*x_stride=*/1, input_layer_name);
}

config::Layer BuildReshapeLayer(const string& name, int y_out_dim,
                                int x_out_dim, int z_out_dim,
                                const string& input_layer_name) {
  config::Layer layer_config;
  config::ReshapeLayer* config = layer_config.mutable_reshape();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config->set_y_out_dim(y_out_dim);
  config->set_x_out_dim(x_out_dim);
  config->set_z_out_dim(z_out_dim);

  return layer_config;
}

config::Layer BuildSliceLayer(const string& name, config::SliceLayer::Mode mode,
                              int in_begin, int in_size,
                              const string& input_layer_name) {
  config::Layer layer_config;
  config::SliceLayer* config = layer_config.mutable_slice();
  layer_config.set_name(name);
  SetConfigInputLayers({input_layer_name}, &layer_config);

  config->set_mode(mode);
  config->set_in_begin(in_begin);
  config->set_in_size(in_size);

  return layer_config;
}

config::Layer BuildVectorLayer(
    const string& name, const config::ArrayInfo& array_info,
    config::VectorLayer::Op op,
    config::Layer::ActivationFunction activation_function,
    const std::vector<string>& input_layer_names) {
  config::Layer layer_config;
  config::VectorLayer* config = layer_config.mutable_vector();
  layer_config.set_name(name);
  SetConfigInputLayers(input_layer_names, &layer_config);

  layer_config.set_activation_function(activation_function);
  *layer_config.mutable_output_array() = array_info;
  config->set_op(op);

  if (op == config::VectorLayer::MINIMUM ||
      op == config::VectorLayer::MAXIMUM) {
    CHECK_EQ(activation_function, config::Layer::NONE)
        << "VectorMinimum or VectorMaximum don't support activation function.";
    CHECK(array_info.data_type() == config::FIXED_POINT8 ||
          array_info.data_type() == config::FIXED_POINT16)
        << "Data type not supported yet.";
  }
  return layer_config;
}

config::Layer BuildZeroPaddingLayer(
    const string& name, config::ZeroPaddingLayer::Dimension dimension,
    int pre_padding, int post_padding, const string& input_layer_name) {
  config::Layer layer_config;
  config::ZeroPaddingLayer* config = layer_config.mutable_zero_padding();
  layer_config.set_name(name);
  layer_config.add_input_layers(input_layer_name);
  config->set_dimension(dimension);
  config->set_pre_padding(pre_padding);
  config->set_post_padding(post_padding);
  return layer_config;
}

config::Layer BuildInterleaveLayer(
    const string& name, config::InterleaveLayer::Mode mode,
    const std::vector<string>& input_layer_names) {
  config::Layer layer_config;
  layer_config.set_name(name);
  SetConfigInputLayers(input_layer_names, &layer_config);

  config::InterleaveLayer* config = layer_config.mutable_interleave();
  config->set_mode(mode);
  return layer_config;
}

config::Layer BuildSpaceToDepthLayer(const string& name, int block_size,
                                     const string& input_layer_name) {
  config::Layer layer_config;
  config::SpaceToDepthLayer* config = layer_config.mutable_space_to_depth();
  layer_config.set_name(name);
  config->set_block_size(block_size);
  layer_config.add_input_layers(input_layer_name);
  return layer_config;
}

config::Layer BuildDepthToSpaceLayer(const string& name, int block_size,
                                     const string& input_layer_name) {
  config::Layer layer_config;
  config::DepthToSpaceLayer* config = layer_config.mutable_depth_to_space();
  layer_config.set_name(name);
  config->set_block_size(block_size);
  layer_config.add_input_layers(input_layer_name);
  return layer_config;
}

config::Layer BuildSpaceToBatchLayer(
    const string& name, int block_size,
    const config::PaddingAmount& padding_amount,
    const string& input_layer_name) {
  config::Layer layer_config;
  config::SpaceToBatchLayer* config = layer_config.mutable_space_to_batch();
  layer_config.set_name(name);
  config->set_block_size(block_size);
  *config->mutable_padding_amount() = padding_amount;
  layer_config.add_input_layers(input_layer_name);
  return layer_config;
}

config::Layer BuildBatchToSpaceLayer(const string& name, int block_size,
                                     const config::PaddingAmount& crop_amount,
                                     const string& input_layer_name) {
  config::Layer layer_config;
  config::BatchToSpaceLayer* config = layer_config.mutable_batch_to_space();
  layer_config.set_name(name);
  config->set_block_size(block_size);
  *config->mutable_crop_amount() = crop_amount;
  layer_config.add_input_layers(input_layer_name);
  return layer_config;
}

config::ArrayInfo ArrayType(config::DataType data_type) {
  config::ArrayInfo ai;
  ai.set_data_type(data_type);
  return ai;
}

config::ArrayInfo ArrayTypeRange(config::DataType data_type, double minimum,
                                 double maximum) {
  config::ArrayInfo ai = ArrayType(data_type);
  auto* range = ai.add_real_range();
  range->set_minimum(minimum);
  range->set_maximum(maximum);
  return ai;
}

config::ArrayInfo ArrayTypeRanges(config::DataType data_type,
                                  const std::vector<MinMax<double>>& minmax) {
  config::ArrayInfo ai = ArrayType(data_type);
  for (const auto& value : minmax) {
    auto* range = ai.add_real_range();
    range->set_minimum(value.min);
    range->set_maximum(value.max);
  }
  return ai;
}

// TODO(b/38385078): DO NOT use this for output activations.
config::ArrayInfo FixedPointArrayInfo(config::DataType data_type, double scale,
                                      int zero_point) {
  CHECK(data_type == config::FIXED_POINT8 || data_type == config::FIXED_POINT16)
      << "Only fixed point types can be supported";

  config::ArrayInfo ai;
  ai.set_data_type(data_type);
  auto* quantization_parameters = ai.add_quantization_parameters();
  quantization_parameters->set_scale(scale);
  quantization_parameters->set_zero_point(zero_point);

  // TODO(b/77856311) delete below once the bug is resolved.
  //
  // The real-range is currently required by Layer and associated code
  // http://shortn/_AbvX4Vqz5z
  // So also populate range synthesized from the quantization parameters above.
  auto* real_range = ai.add_real_range();
  real_range->set_minimum((0 - zero_point) * scale);

  const int fixed_point_max =
      (data_type == config::FIXED_POINT8)
          ? std::numeric_limits<uint8>::max()
          // TODO(b/38385078): this is incorrect for output activations.
          : std::numeric_limits<uint16>::max();
  real_range->set_maximum((fixed_point_max - zero_point) * scale);
  return ai;
}

}  // namespace model
}  // namespace darwinn
}  // namespace platforms
