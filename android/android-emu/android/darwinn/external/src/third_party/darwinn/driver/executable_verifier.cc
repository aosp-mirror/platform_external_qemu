#include "third_party/darwinn/driver/executable_verifier.h"

#include <string>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::StatusOr<std::unique_ptr<EcdsaExecutableVerifier>>
EcdsaExecutableVerifier::Make(const std::string& public_key) {
  // Using new since the constructor is private.
  auto executable_verifier =
      std::unique_ptr<EcdsaExecutableVerifier>(new EcdsaExecutableVerifier());

  bssl::UniquePtr<BIO> bio(
      BIO_new_mem_buf(public_key.c_str(), public_key.length()));
  executable_verifier->public_key_.reset(
      PEM_read_bio_EC_PUBKEY(bio.get(), nullptr, nullptr, nullptr));
  if (!executable_verifier->public_key_) {
    ERR_clear_error();
    return util::FailedPreconditionError("Failed to load public key.");
  }

  return executable_verifier;
}

util::StatusOr<std::unique_ptr<EcdsaExecutableVerifier>>
EcdsaExecutableVerifier::MakeFromFile(const std::string& public_key_path) {
  std::ifstream public_key_ifs;
  public_key_ifs.open(public_key_path, std::ifstream::in);
  if (!public_key_ifs.is_open()) {
    return util::NotFoundError(
        StringPrintf("Cannot find public key at %s.", public_key_path.c_str()));
  }

  std::string public_key_string(
      (std::istreambuf_iterator<char>(public_key_ifs)),
      std::istreambuf_iterator<char>());
  public_key_ifs.close();

  return Make(public_key_string);
}

util::Status EcdsaExecutableVerifier::VerifySignature(
    const void* package_buffer) const {
  auto package = flatbuffers::GetRoot<Package>(package_buffer);

  // Create a digest of the serialized MultiExecutable.
  unsigned int digest_size;
  uint8 digest[EVP_MAX_MD_SIZE];
  if (EVP_Digest(reinterpret_cast<const char*>(
                     package->serialized_multi_executable()->data()),
                 package->serialized_multi_executable()->size(), digest,
                 &digest_size, EVP_sha256(), NULL) != 1) {
    ERR_clear_error();
    return util::FailedPreconditionError(
        "Failed to create MultiExecutable digest.");
  }

  // Verify the signature.
  auto verification_result =
      ECDSA_verify(/*type=*/0, digest, static_cast<size_t>(digest_size),
                   package->signature()->data(), package->signature()->size(),
                   public_key_.get());
  if (verification_result != /*success=*/1) {
    ERR_clear_error();
    return util::PermissionDeniedError("Signature verification failed.");
  }

  return util::Status();  // OK
}

util::Status NoopExecutableVerifier::VerifySignature(const void*) const {
  return util::FailedPreconditionError(
      "No verifier was created yet verification was requested.");
}

util::StatusOr<std::unique_ptr<ExecutableVerifier>> MakeExecutableVerifier(
    const std::string& public_key_path) {
  if (public_key_path.empty()) {
    return {gtl::MakeUnique<NoopExecutableVerifier>()};
  }

  return EcdsaExecutableVerifier::Make(public_key_path);
}

util::StatusOr<std::unique_ptr<ExecutableVerifier>>
MakeExecutableVerifierFromFile(const std::string& public_key_path) {
  if (public_key_path.empty()) {
    return {gtl::MakeUnique<NoopExecutableVerifier>()};
  }

  return EcdsaExecutableVerifier::MakeFromFile(public_key_path);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
