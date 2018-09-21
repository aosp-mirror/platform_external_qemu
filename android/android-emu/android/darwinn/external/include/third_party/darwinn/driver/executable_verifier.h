#ifndef THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_VERIFIER_H_
#define THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_VERIFIER_H_

#include <fstream>
#include <string>

#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/openssl.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

// ExecutableVerifier is a class to verify executable packages using digital
// signatures.
class ExecutableVerifier {
 public:
  virtual ~ExecutableVerifier() = default;

  // Verifies the executable package provided its buffer.
  virtual util::Status VerifySignature(const void* package_buffer) const = 0;
};

// EcdsaExecutableVerifier is a class for verifying executable packages using
// ECDSA digital signatures.
class EcdsaExecutableVerifier : public ExecutableVerifier {
 public:
  // Makes a EcdsaExecutableVerifier provided a file path to the public key.
  static util::StatusOr<std::unique_ptr<EcdsaExecutableVerifier>> Make(
      const std::string& public_key);

  // Makes a EcdsaExecutableVerifier provided a file path to the public key.
  static util::StatusOr<std::unique_ptr<EcdsaExecutableVerifier>> MakeFromFile(
      const std::string& public_key_path);

  ~EcdsaExecutableVerifier() override = default;

  // Verifies the executable package provided its buffer.
  util::Status VerifySignature(const void* package_buffer) const override;

 private:
  // Clients should use the Make methods for constructing this class.
  EcdsaExecutableVerifier() = default;

  // The public key to be used for verifying package signatures.
  bssl::UniquePtr<EC_KEY> public_key_;
};

// A noop implementation of ExecutableVerifier that errors out on all calls.
class NoopExecutableVerifier : public ExecutableVerifier {
 public:
  NoopExecutableVerifier() = default;
  ~NoopExecutableVerifier() override = default;
  util::Status VerifySignature(const void* package_buffer) const override;
};

// Makes an ExecutableVerifier provided a public key. If the key is empty a noop
// verifier will be returned that errors on Verify.
util::StatusOr<std::unique_ptr<ExecutableVerifier>> MakeExecutableVerifier(
    const std::string& public_key);

// Makes an ExecutableVerifier provided a file path to the public key. If the
// path is empty a noop verifier will be returned that errors on Verify.
util::StatusOr<std::unique_ptr<ExecutableVerifier>>
MakeExecutableVerifierFromFile(const std::string& public_key_path);

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_EXECUTABLE_VERIFIER_H_
