#ifndef THIRD_PARTY_DARWINN_DRIVER_TEST_VECTOR_H_
#define THIRD_PARTY_DARWINN_DRIVER_TEST_VECTOR_H_

#include <fstream>
#include <string>

#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Encapsulates a test vector for a model.
class TestVector {
 public:
  explicit TestVector(const std::string& prefix)
      : TestVector(
            prefix, prefix + "-executable.bin", prefix + "-instructions.txt",
            prefix + "-instructions-debug.txt", prefix + "-test-input.bin",
            prefix + "-expected-output.bin", prefix + "-generated-output.bin") {
  }

  TestVector(const std::string& name, const std::string& executable_file_name,
             const std::string& decoded_instructions_file_name,
             const std::string& debug_instructions_file_name,
             const std::string& input_file_name,
             const std::string& expected_output_file_name,
             const std::string& output_file_name)
      : name_(name),
        executable_file_name_(executable_file_name),
        decoded_instructions_file_name_(decoded_instructions_file_name),
        debug_instructions_file_name_(debug_instructions_file_name),
        input_file_name_(input_file_name),
        expected_output_file_name_(expected_output_file_name),
        output_file_name_(output_file_name) {
    LoadInputAndExpectedOutputData();
  }

  // Returns test name
  std::string name() const { return name_; }

  // Returns the executable file name.
  std::string executable_file_name() const { return executable_file_name_; }

  // Returns the decoded-instructions file name.
  std::string decoded_instructions_file_name() const {
    return decoded_instructions_file_name_;
  }

  // Returns the debug-instructions file name.
  std::string debug_instructions_file_name() const {
    return debug_instructions_file_name_;
  }

  // Returns the test input file name.
  std::string input_file_name() const { return input_file_name_; }

  // Returns the expected output file name.
  std::string expected_output_file_name() const {
    return expected_output_file_name_;
  }

  // Returns the output file name.
  std::string output_file_name() const { return output_file_name_; }

  // Sets the executable reference associated with this test vector.
  void set_executable_reference(
      const api::PackageReference* executable_reference) {
    CHECK(executable_reference != nullptr);
    executable_reference_ = executable_reference;
  }

  // Returns the executable reference associated with this test vector.
  const api::PackageReference* executable_reference() const {
    CHECK(executable_reference_ != nullptr);
    return executable_reference_;
  }

  const std::string& GetInput() const { return input_; }

  const std::string& GetExpectedOutput() const { return expected_output_; }

 private:
  // Reads a file to a string.
  static std::string ReadToString(const std::string& file_name) {
    std::ifstream ifs(file_name);
    auto content = std::string(std::istreambuf_iterator<char>(ifs),
                               std::istreambuf_iterator<char>());
    return content;
  }

  // Loads input and expected output data into internal cache.
  void LoadInputAndExpectedOutputData() {
    input_ = ReadToString(input_file_name_);
    expected_output_ = ReadToString(expected_output_file_name_);
  }

  // Test name
  const std::string name_;

  // Input / Output Filenames.
  const std::string executable_file_name_;
  const std::string decoded_instructions_file_name_;
  const std::string debug_instructions_file_name_;
  const std::string input_file_name_;
  const std::string expected_output_file_name_;
  const std::string output_file_name_;

  // Executable reference.
  const api::PackageReference* executable_reference_{nullptr};

  // Cached input and expected output data in byte string.
  std::string expected_output_;
  std::string input_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_TEST_VECTOR_H_
