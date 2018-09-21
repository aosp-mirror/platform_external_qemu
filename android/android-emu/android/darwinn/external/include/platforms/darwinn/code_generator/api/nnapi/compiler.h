#ifndef PLATFORMS_DARWINN_CODE_GENERATOR_API_NNAPI_COMPILER_H_
#define PLATFORMS_DARWINN_CODE_GENERATOR_API_NNAPI_COMPILER_H_

#include "third_party/darwinn/api/chip.h"

// The NNAPI compiler interface has a serialized DarwiNN Model config as input,
// and serialized DarwiNN packages as output.
//
// Conversion from NNAPI model to DarwiNN model is implemented in Android
// source code repository.
//
// Also, there are no C++ std::* components in this interface as the C++ libs
// don't match between google3 and Android.

namespace platforms {
namespace darwinn {
namespace code_generator {
namespace api {
namespace nnapi {

// Encapsulates a chunk of pre-allocated data, and its size.
struct DataArray {
  char* data;
  int size_in_bytes;
};

// Compiles a model. The input model is passed as a serialized Model proto, and
// the compiled package is returned back as a serialized package fbs. Returns
// whether the compilation was successful.
bool CompileModel(platforms::darwinn::api::Chip chip_type,
                  bool enable_parameter_caching,
                  const DataArray& serialized_model,
                  DataArray* serialized_package);

// Returns the compiler version as string. It returns an empty string if it is
// not a released binary.
const char* CompilerVersion();

}  // namespace nnapi
}  // namespace api
}  // namespace code_generator
}  // namespace darwinn
}  // namespace platforms

#endif  // PLATFORMS_DARWINN_CODE_GENERATOR_API_NNAPI_COMPILER_H_
