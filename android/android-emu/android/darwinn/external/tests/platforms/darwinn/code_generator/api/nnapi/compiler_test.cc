#include "platforms/darwinn/code_generator/api/nnapi/compiler.h"
#include "platforms/darwinn/code_generator/api/nnapi/embedded_compiler_test_vector.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/test_util.h"
#include "third_party/darwinn/port/logging.h"

#ifdef DARWINN_COMPILER_TEST_EXTERNAL
#include "platforms/darwinn/model/config/representation.pb.h"
#else
#include "platforms/darwinn/model/config/representation.proto.h"
#endif

#ifndef HAS_GLOBAL_STRING
using std::string;
#endif

using platforms::darwinn::Executable;
using platforms::darwinn::MultiExecutable;
using platforms::darwinn::Package;
using platforms::darwinn::testutil::MakePackage;
using platforms::darwinn::code_generator::api::nnapi::CompileModel;
using platforms::darwinn::code_generator::api::nnapi::DataArray;

// Returns a FileToc strucutre corresponding to the embedded binary blob with
// the given file name.
const FileToc* GetEmbeddedData(const string& embedded_file_name) {
  for (const FileToc* p = platforms::darwinn::code_generator::api::
           embedded_compiler_test_vector_create();
       p->name != nullptr; ++p) {
    if (p->name == embedded_file_name) {
      return p;
    }
  }
  // Embedded data not found.
  LOG(FATAL) << "Embedded data not found: " << embedded_file_name;
  return nullptr;
}

// Gets an executable package and returns a copy of it with all random data
// cleaned up so that we can safely compare packages.
string CleanupRandomData(const char* serialized_package) {
  const auto* package = flatbuffers::GetRoot<Package>(
      reinterpret_cast<const uint8_t*>(serialized_package));
  const auto* multi_executable = flatbuffers::GetRoot<MultiExecutable>(
      package->serialized_multi_executable()->data());
  std::vector<std::string> serialized_executables;

  for (const auto* executable_serialized :
       *multi_executable->serialized_executables()) {
    auto* executable = flatbuffers::GetRoot<Executable>(
        reinterpret_cast<const uint8_t*>(executable_serialized->c_str()));
    auto* executable_packer = executable->UnPack();
    executable_packer->parameter_caching_token = 0;

    flatbuffers::FlatBufferBuilder builder;
    builder.Finish(Executable::Pack(builder, executable_packer));
    string new_executable_serialized(
        reinterpret_cast<const char*>(builder.GetBufferPointer()),
        builder.GetSize());
    serialized_executables.push_back(new_executable_serialized);
  }

  return MakePackage(serialized_executables);
}

void CheckPackagesMatch(const DataArray& serialized_package,
                        const string& expected_package_embedded_file_name) {
  // Verify generated package.
  flatbuffers::Verifier package_verifier(
      reinterpret_cast<const uint8_t*>(serialized_package.data),
      serialized_package.size_in_bytes);
  CHECK(package_verifier.VerifyBuffer<Package>());

  // Verify expected package.
  const FileToc* embedded_serialized_expected_package =
      GetEmbeddedData(expected_package_embedded_file_name);
  flatbuffers::Verifier expected_package_verifier(
      reinterpret_cast<const uint8_t*>(
          embedded_serialized_expected_package->data),
      embedded_serialized_expected_package->size);
  CHECK(expected_package_verifier.VerifyBuffer<Package>());

  // Clear out values that we don't expect to match (e.g. random generated
  // tokens).

  // Compare the serialized versions.
  auto generated_string = CleanupRandomData(serialized_package.data);
  auto expected_string =
      CleanupRandomData(embedded_serialized_expected_package->data);
  CHECK_EQ(generated_string, expected_string);
}

int main() {
  // Deserialize and re-serialize model to test model proto serialization and
  // deserialization works correctly across google3 and Android.
  DataArray reserialized_model;
  const FileToc* embedded_serialized_model =
      GetEmbeddedData("mobilenet_quantized_224-model.bin");
  platforms::darwinn::model::config::Model model;
  CHECK(model.ParseFromArray(embedded_serialized_model->data,
                             embedded_serialized_model->size));

  const int data_size = model.ByteSize();
  char* data = new char[data_size];
  model.SerializeToArray(data, data_size);
  reserialized_model.data = data;
  reserialized_model.size_in_bytes = data_size;

  // Compile model.
  DataArray serialized_package;
  CompileModel(platforms::darwinn::api::Chip::kBeagle,
               /*enable_parameter_caching=*/true, reserialized_model,
               &serialized_package);
  CheckPackagesMatch(serialized_package,
                     "mobilenet_quantized_224-executable.bin");

  delete[] reserialized_model.data;
  delete[] serialized_package.data;
  return 0;
}
