#include "third_party/darwinn/api/driver_options_helper.h"

#include "third_party/darwinn/api/driver_options_generated.h"

namespace platforms {
namespace darwinn {
namespace api {

// Returns driver options for default.
Driver::Options DriverOptionsHelper::Defaults() {
  flatbuffers::FlatBufferBuilder builder;
  auto options_offset = api::CreateDriverOptions(
      builder,
      /*version=*/1,
      /*usb=*/0,
      /*verbosity=*/0,
      /*performance_expectation=*/api::PerformanceExpectation_High,
      /*public_key=*/builder.CreateString(""));
  builder.Finish(options_offset);
  return api::Driver::Options(builder.GetBufferPointer(),
                              builder.GetBufferPointer() + builder.GetSize());
}

// Returns driver options for maximum performance.
Driver::Options DriverOptionsHelper::MaxPerformance() {
  flatbuffers::FlatBufferBuilder builder;
  auto options_offset = api::CreateDriverOptions(
      builder,
      /*version=*/1,
      /*usb=*/0,
      /*verbosity=*/0,
      /*performance_expectation=*/api::PerformanceExpectation_Max,
      /*public_key=*/builder.CreateString(""));
  builder.Finish(options_offset);
  return api::Driver::Options(builder.GetBufferPointer(),
                              builder.GetBufferPointer() + builder.GetSize());
}


}  // namespace api
}  // namespace darwinn
}  // namespace platforms
