// Tests given models with specified test vectors.

#include <memory>
#include <string>

#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/driver_options_generated.h"
#include "third_party/darwinn/driver/driver_factory.h"
#include "third_party/darwinn/driver/driver_helper.h"
#include "third_party/darwinn/port/gflags.h"

DEFINE_string(chip, "beagle", "Chip.");
DEFINE_string(device_type, "pci",
              "Specify the device type, possible options: pci, usb, reference");
DEFINE_bool(debug_mode, false, "Run chip in a debug mode.");
DEFINE_string(test_prefix, "", "Comma separated list of test vector prefixes.");
DEFINE_string(device_path,
              platforms::darwinn::driver::DriverFactory::kDefaultDevicePath,
              "Path to device under test.");
DEFINE_bool(enumerate_devices, false,
            "Print a list of available devices and exit.");
DEFINE_string(parameter_caching_test_prefix, "",
              "Comma separated list of test vector prefixes for "
              "parameter-caching executables.");
DEFINE_int32(num_runs, 1, "Number of runs. -1 to run indefinitely.");
DEFINE_int32(max_pending_requests, 1, "Maximum number of pending requests.");
DEFINE_int32(batches, -1,
             "Number of batches per run. -1 to use the hardware batch size.");

DEFINE_string(public_key_path, "",
              "Path to the public key used for verifying executable packages.");
DEFINE_bool(verify_signature, false,
            "If set, digital signatures in the provided executable packages "
            "will be verified.");

// Beagle USB-specific flags
DEFINE_string(usb_dfu_firmware, "", "USB DFU firmware path.");
DEFINE_int32(performance_expectation, 2,
             "Clock rate settings affecting performance: 0: Low, 1: Medium, "
             "2: High (default), 3: Max");
DEFINE_bool(usb_always_dfu, true,
            "If true, always performs DFU. Otherwise, only performs DFU if "
            "device is in DFU mode.");

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

// Reads an entire file and returns it as string provided a path.
util::StatusOr<std::string> ReadFile(const std::string& file_path) {
  std::ifstream ifs;
  ifs.open(file_path, std::ifstream::in);
  if (!ifs.is_open()) {
    return util::NotFoundError(
        StringPrintf("Cannot open file: %s.", file_path.c_str()));
  }

  std::string data((std::istreambuf_iterator<char>(ifs)),
                   std::istreambuf_iterator<char>());
  ifs.close();

  return data;
}

}  // namespace

util::Status run_main() {
  std::vector<TestVector> test_vectors;
  std::istringstream test_prefix_stream(FLAGS_test_prefix);
  std::string test_prefix;
  while (std::getline(test_prefix_stream, test_prefix, ',')) {
    test_vectors.push_back(TestVector(test_prefix));
  }

  std::vector<TestVector> parameter_caching_test_vectors;
  std::istringstream parameter_caching_test_prefix_stream(
      FLAGS_parameter_caching_test_prefix);
  std::string parameter_caching_test_prefix;
  while (std::getline(parameter_caching_test_prefix_stream,
                      parameter_caching_test_prefix, ',')) {
    parameter_caching_test_vectors.push_back(
        TestVector(parameter_caching_test_prefix));
  }

  auto factory = DriverFactory::GetOrCreate();

  if (FLAGS_enumerate_devices) {
    for (auto device : factory->Enumerate()) {
      LOG(INFO) << device.path << "\t" << GetChipName(device.chip) << "\t"
                << GetTypeName(device.type);
    }
    return util::Status();  // OK
  }

  QCHECK(!FLAGS_chip.empty()) << "Chip name not specified.";
  QCHECK(!test_prefix.empty()) << "Test vectors not specified.";


  // Construct the required device.
  api::Device device;

  // Construct options flat buffer object.
  flatbuffers::FlatBufferBuilder builder;
  auto usb_options = api::CreateDriverUsbOptions(
      builder, builder.CreateString(FLAGS_usb_dfu_firmware),
      FLAGS_usb_always_dfu);

  api::PerformanceExpectation performance_expectation;
  switch (FLAGS_performance_expectation) {
    case 0:
      performance_expectation = api::PerformanceExpectation_Low;
      break;
    case 1:
      performance_expectation = api::PerformanceExpectation_Medium;
      break;
    case 2:
      performance_expectation = api::PerformanceExpectation_High;
      break;
    case 3:
      performance_expectation = api::PerformanceExpectation_Max;
      break;
    default:
      LOG(FATAL) << "Unknown performance expectation setting: "
                 << FLAGS_performance_expectation;
  }

  std::string public_key;
  if (!FLAGS_public_key_path.empty()) {
    ASSIGN_OR_RETURN(public_key, ReadFile(FLAGS_public_key_path));
  }

  auto options_offset =
      CreateDriverOptions(builder,
                          /*version=*/1,
                          /*usb=*/usb_options,
                          /*verbosity=*/0,
                          /*performance_expectation=*/performance_expectation,
                          /*public_key=*/builder.CreateString(public_key));
  builder.Finish(options_offset);

  const api::Chip chip = api::GetChipByName(FLAGS_chip);
  if (FLAGS_device_type == "usb") {
    device.type = api::Device::Type::USB;
    device.chip = chip;
    device.path = FLAGS_device_path;
  } else if (FLAGS_device_type == "reference") {
    device.type = api::Device::Type::REFERENCE;
    device.chip = chip;
    device.path = DriverFactory::kDefaultDevicePath;
  } else if (FLAGS_device_type == "pci") {
    device.type = api::Device::Type::PCI;
    device.chip = chip;
    device.path = FLAGS_device_path;
  } else {
    LOG(FATAL) << "Unknown --device_type " << FLAGS_device_type;
  }

  ASSIGN_OR_RETURN(auto backing_driver,
                   factory->CreateDriver(
                       device, api::Driver::Options(builder.GetBufferPointer(),
                                                    builder.GetBufferPointer() +
                                                        builder.GetSize())));
  auto driver = gtl::MakeUnique<DriverHelper>(std::move(backing_driver),
                                              FLAGS_max_pending_requests);

  RETURN_IF_ERROR(driver->Open(FLAGS_debug_mode));

  // Register models.
  for (auto& test_vector : parameter_caching_test_vectors) {
    ASSIGN_OR_RETURN(
        auto executable_ref,
        // The parameter-caching executables are run only once, in the
        // beginning.
        driver->RegisterExecutableFile(test_vector.executable_file_name()));
    LOG(INFO) << "Registered executable : "
              << test_vector.executable_file_name();
    if (FLAGS_verify_signature) {
      RETURN_IF_ERROR(executable_ref->VerifySignature());
      LOG(INFO) << "Package signature verified.";
    }
    test_vector.set_executable_reference(executable_ref);
  }
  for (auto& test_vector : test_vectors) {
    ASSIGN_OR_RETURN(
        auto executable_ref,
        driver->RegisterExecutableFile(test_vector.executable_file_name()));
    LOG(INFO) << "Registered executable : "
              << test_vector.executable_file_name();
    if (FLAGS_verify_signature) {
      RETURN_IF_ERROR(executable_ref->VerifySignature());
      LOG(INFO) << "Package signature verified.";
    }
    test_vector.set_executable_reference(executable_ref);
  }

  driver->SetFatalErrorCallback([](const util::Status& status) {
    LOG(WARNING) << "Fatal Error Detected. " << status.ToString();
    CHECK_OK(status);
    // TODO(b/70535438): Close gracefully, cancelling pending requests.
  });

  // Submit parameter-caching requests first.
  for (const auto& test_vector : parameter_caching_test_vectors) {
    RETURN_IF_ERROR(driver->Submit(test_vector, 1));
  }

  // Submit requests.
  if (FLAGS_num_runs == -1) {
    for (;;) {
      for (const auto& test_vector : test_vectors) {
        RETURN_IF_ERROR(driver->Submit(test_vector, FLAGS_batches));
      }
    }
  } else {
    for (int i = 0; i < FLAGS_num_runs; ++i) {
      for (const auto& test_vector : test_vectors) {
        RETURN_IF_ERROR(driver->Submit(test_vector, FLAGS_batches));
      }
    }
  }

  RETURN_IF_ERROR(driver->Close());
  LOG(INFO) << "Done.";

  return util::Status();  // OK.
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

int main(int argc, char** argv) {
  ParseFlags(argv[0], &argc, &argv, true);
  QCHECK_OK(platforms::darwinn::driver::run_main());
  return 0;
}
