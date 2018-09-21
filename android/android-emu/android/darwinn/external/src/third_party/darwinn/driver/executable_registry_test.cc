// Tests for executable_registry.cc

#include "third_party/darwinn/driver/executable_registry.h"

#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <memory>
#include <string>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/aligned_allocator.h"
#include "third_party/darwinn/driver/executable_verifier.h"
#include "third_party/darwinn/driver/test_util.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::Eq;
using testutil::MakePackage;
using testutil::MakeTypedExecutable;

// Creates and returns a package for testing provided a serialized multi-
// executable and signature.
std::string TestPackage(const std::vector<uint8>& serialized_multi_executable,
                        const std::vector<uint8>& signature) {
  flatbuffers::FlatBufferBuilder package_builder;
  auto package =
      CreatePackage(package_builder,
                    /*version=*/1,
                    /*serialized_multi_executable=*/
                    package_builder.CreateVector(serialized_multi_executable),
                    /*signature=*/package_builder.CreateVector(signature),
                    /*keypair_version=*/0);
  package_builder.Finish(package, api::kHeadPackageIdentifier);
  return std::string(
      reinterpret_cast<const char*>(package_builder.GetBufferPointer()),
      package_builder.GetSize());
}

// Creates a MultiExecutable that's almost empty for test cases.
std::vector<uint8> TestMultiExecutable() {
  flatbuffers::FlatBufferBuilder executable_builder;
  auto executable = CreateExecutable(executable_builder,
                                     /*version=*/1,
                                     /*name=*/0,
                                     /*serialized_model=*/0,
                                     /*batch_size=*/1);
  executable_builder.Finish(executable);
  std::string executable_serialized(
      reinterpret_cast<const char*>(executable_builder.GetBufferPointer()),
      executable_builder.GetSize());

  flatbuffers::FlatBufferBuilder multi_executable_builder;
  std::vector<flatbuffers::Offset<flatbuffers::String>> executables;
  executables.push_back(
      multi_executable_builder.CreateString(executable_serialized));
  auto multi_executable =
      CreateMultiExecutable(multi_executable_builder,
                            multi_executable_builder.CreateVector(executables));
  multi_executable_builder.Finish(multi_executable);

  std::string multi_executable_serialized(
      reinterpret_cast<char*>(multi_executable_builder.GetBufferPointer()),
      multi_executable_builder.GetSize());
  return std::vector<uint8>(multi_executable_serialized.begin(),
                            multi_executable_serialized.end());
}

// Signs a provided multi executable with a test private key and returns the
// signature.
std::vector<uint8> TestSign(
    const std::vector<uint8>& multi_executable_serialized) {
  // Load the test privaye key.
  std::ifstream private_key_ifs;
  private_key_ifs.open("third_party/darwinn/driver/test_data/private_key.pem",
                       std::ifstream::in);
  EXPECT_TRUE(private_key_ifs.is_open());
  std::string private_key_string(
      (std::istreambuf_iterator<char>(private_key_ifs)),
      std::istreambuf_iterator<char>());
  private_key_ifs.close();

  bssl::UniquePtr<BIO> bio(
      BIO_new_mem_buf(private_key_string.c_str(), private_key_string.length()));
  bssl::UniquePtr<EC_KEY> private_key(nullptr);
  private_key.reset(
      PEM_read_bio_ECPrivateKey(bio.get(), nullptr, nullptr, nullptr));
  EXPECT_NE(private_key, nullptr);

  // Create a digest of the serialized MultiExecutable.
  unsigned int digest_size;
  uint8 digest[EVP_MAX_MD_SIZE];
  EXPECT_EQ(EVP_Digest(multi_executable_serialized.data(),
                       multi_executable_serialized.size(), digest, &digest_size,
                       EVP_sha256(), NULL),
            1);

  // Sign the MultiExecutable with the private key.
  std::vector<uint8> signature;
  // We need to pre-allocate memory to signature vector for ECDSA_sign to fill
  // it up. After signature is set, we will resize the vector to the signature
  // length.
  signature.resize(ECDSA_size(private_key.get()));
  unsigned int signature_length;
  ERR_print_errors_fp(stderr);
  EXPECT_EQ(ECDSA_sign(/*type=*/0, digest, static_cast<size_t>(digest_size),
                       signature.data(), &signature_length, private_key.get()),
            1);
  signature.resize(signature_length);

  return signature;
}

// Base Test
class ExecutableRegistryTest : public ::testing::Test {
 public:
  ~ExecutableRegistryTest() override = default;
};

TEST_F(ExecutableRegistryTest, TestRegister) {
  testutil::ExecutableGenerator one;
  one.SetBatchSize(1);
  one.Finish();

  testutil::ExecutableGenerator two;
  two.SetBatchSize(2);
  two.Finish();

  ExecutableRegistry registry(api::Chip::kBeagle);

  auto result_1 = registry.RegisterSerialized(one.PackageString());
  auto result_2 = registry.RegisterSerialized(two.PackageString());

  auto executable_ref_1 =
      static_cast<const PackageReference*>(result_1.ValueOrDie());
  auto executable_ref_2 =
      static_cast<const PackageReference*>(result_2.ValueOrDie());

  EXPECT_EQ(executable_ref_1->BatchSize(), one.Executable()->batch_size());

  EXPECT_EQ(executable_ref_2->BatchSize(), two.Executable()->batch_size());

  EXPECT_EQ(registry.GetRegistrySize(), 2);
}

TEST_F(ExecutableRegistryTest, TestDifferentChip) {
  testutil::ExecutableGenerator generator;
  auto executable_chip = generator.Builder()->CreateString("noronha");
  generator.ExecBuilder()->add_chip(executable_chip);
  generator.Finish();

  ExecutableRegistry registry(api::Chip::kBeagle);

  auto result = registry.RegisterSerialized(generator.PackageString());

  EXPECT_EQ(result.status().CanonicalCode(), util::error::INVALID_ARGUMENT);
  EXPECT_EQ(registry.GetRegistrySize(), 0);
}

TEST_F(ExecutableRegistryTest, TestSameChip) {
  testutil::ExecutableGenerator generator;
  auto executable_chip = generator.Builder()->CreateString("beagle");
  generator.ExecBuilder()->add_chip(executable_chip);
  generator.Finish();

  ExecutableRegistry registry(api::Chip::kBeagle);

  auto result = registry.RegisterSerialized(generator.PackageString());

  EXPECT_TRUE(result.ok());
  EXPECT_EQ(registry.GetRegistrySize(), 1);
}

TEST_F(ExecutableRegistryTest, TestChipNotSpecifiedInExecutable) {
  testutil::ExecutableGenerator generator;
  generator.Finish();

  ExecutableRegistry registry(api::Chip::kBeagle);

  auto result = registry.RegisterSerialized(generator.PackageString());

  EXPECT_TRUE(result.ok());
  EXPECT_EQ(registry.GetRegistrySize(), 1);
}

TEST_F(ExecutableRegistryTest, TestChipNotSpecifiedInRegistry) {
  testutil::ExecutableGenerator generator;
  auto executable_chip = generator.Builder()->CreateString("beagle");
  generator.ExecBuilder()->add_chip(executable_chip);
  generator.Finish();

  ExecutableRegistry registry;

  auto result = registry.RegisterSerialized(generator.PackageString());

  EXPECT_TRUE(result.ok());
  EXPECT_EQ(registry.GetRegistrySize(), 1);
}

TEST_F(ExecutableRegistryTest, TestUnregister) {
  testutil::ExecutableGenerator generator;
  auto executable_chip = generator.Builder()->CreateString("beagle");
  generator.ExecBuilder()->add_chip(executable_chip);
  generator.Finish();

  ExecutableRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(auto result_1,
                       registry.RegisterSerialized(generator.PackageString()));

  // First unregistration should work.
  EXPECT_EQ(registry.GetRegistrySize(), 1);
  EXPECT_OK(registry.Unregister(result_1));
  EXPECT_EQ(registry.GetRegistrySize(), 0);

  // Trying to unregister the same executable reference should fail.
  auto result_2 = registry.Unregister(result_1);
  EXPECT_EQ(result_2.CanonicalCode(), util::error::NOT_FOUND);
  EXPECT_EQ(registry.GetRegistrySize(), 0);
}

TEST_F(ExecutableRegistryTest, TestSignedPackage) {
  auto multi_executable = TestMultiExecutable();
  auto signature = TestSign(multi_executable);
  auto package = TestPackage(multi_executable, signature);

  ASSERT_OK_AND_ASSIGN(
      auto verifier,
      MakeExecutableVerifierFromFile(
          "third_party/darwinn/driver/test_data/public_key.pem"));
  ExecutableRegistry registry(api::Chip::kBeagle, std::move(verifier));

  ASSERT_OK_AND_ASSIGN(const auto* reference,
                       registry.RegisterSerialized(package));
  EXPECT_OK(reference->VerifySignature());
}

TEST_F(ExecutableRegistryTest, TestUnsignedPackage) {
  auto multi_executable = TestMultiExecutable();
  auto package = TestPackage(multi_executable, std::vector<uint8>());

  ASSERT_OK_AND_ASSIGN(
      auto verifier,
      MakeExecutableVerifierFromFile(
          "third_party/darwinn/driver/test_data/public_key.pem"));
  ExecutableRegistry registry(api::Chip::kBeagle, std::move(verifier));

  ASSERT_OK_AND_ASSIGN(const auto* reference,
                       registry.RegisterSerialized(package));
  EXPECT_EQ(reference->VerifySignature().CanonicalCode(),
            util::error::PERMISSION_DENIED);
}

TEST_F(ExecutableRegistryTest, TestMissignedPackage) {
  auto multi_executable = TestMultiExecutable();
  std::string signature_string = "foo";
  std::vector<uint8> signature(signature_string.begin(),
                               signature_string.end());
  auto package = TestPackage(multi_executable, signature);

  ASSERT_OK_AND_ASSIGN(
      auto verifier,
      MakeExecutableVerifierFromFile(
          "third_party/darwinn/driver/test_data/public_key.pem"));
  ExecutableRegistry registry(api::Chip::kBeagle, std::move(verifier));

  ASSERT_OK_AND_ASSIGN(const auto* reference,
                       registry.RegisterSerialized(package));
  EXPECT_EQ(reference->VerifySignature().CanonicalCode(),
            util::error::PERMISSION_DENIED);
}

TEST_F(ExecutableRegistryTest, TestNoPublicKey) {
  auto multi_executable = TestMultiExecutable();
  auto signature = TestSign(multi_executable);
  auto package = TestPackage(multi_executable, signature);

  ASSERT_OK_AND_ASSIGN(auto verifier, MakeExecutableVerifier(""));
  ExecutableRegistry registry(api::Chip::kBeagle, std::move(verifier));

  ASSERT_OK_AND_ASSIGN(const auto* reference,
                       registry.RegisterSerialized(package));
  EXPECT_EQ(reference->VerifySignature().CanonicalCode(),
            util::error::FAILED_PRECONDITION);
}

TEST_F(ExecutableRegistryTest, TestNoVerifier) {
  auto multi_executable = TestMultiExecutable();
  auto signature = TestSign(multi_executable);
  auto package = TestPackage(multi_executable, signature);
  ExecutableRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(const auto* reference,
                       registry.RegisterSerialized(package));
  EXPECT_EQ(reference->VerifySignature().CanonicalCode(),
            util::error::FAILED_PRECONDITION);
}

TEST_F(ExecutableRegistryTest, TestRegisterParameterCachingAndInference) {
  std::vector<std::string> executables;
  executables.push_back(MakeTypedExecutable(ExecutableType_EXECUTION_ONLY, 1));
  executables.push_back(
      MakeTypedExecutable(ExecutableType_PARAMETER_CACHING, 1));
  auto package = MakePackage(executables);

  ExecutableRegistry registry(api::Chip::kBeagle);
  ASSERT_OK_AND_ASSIGN(auto api_ref, registry.RegisterSerialized(package));
  const auto* package_ref = dynamic_cast<const PackageReference*>(api_ref);
  EXPECT_NE(package_ref, nullptr);

  EXPECT_EQ(package_ref->AllExecutableReferences().size(), 2);
  EXPECT_TRUE(package_ref->ParameterCachingEnabled());
  EXPECT_EQ(package_ref->ParameterCachingExecutableReference()
                ->ParameterCachingToken(),
            1);
  EXPECT_EQ(
      package_ref->InferenceExecutableReference()->ParameterCachingToken(), 1);
  EXPECT_EQ(package_ref->InferenceExecutableReference(),
            package_ref->MainExecutableReference());
}

TEST_F(ExecutableRegistryTest, TestRegisterStandAlone) {
  std::vector<std::string> executables;
  executables.push_back(MakeTypedExecutable(ExecutableType_STAND_ALONE, 1));
  auto package = MakePackage(executables);

  ExecutableRegistry registry(api::Chip::kBeagle);
  ASSERT_OK_AND_ASSIGN(auto api_ref, registry.RegisterSerialized(package));
  const auto* package_ref = dynamic_cast<const PackageReference*>(api_ref);
  EXPECT_NE(package_ref, nullptr);

  EXPECT_EQ(package_ref->AllExecutableReferences().size(), 1);
  EXPECT_EQ(package_ref->ParameterCachingEnabled(), false);
  EXPECT_EQ(package_ref->MainExecutableReference()->ParameterCachingToken(), 1);
}

TEST_F(ExecutableRegistryTest, TestRegisterInferenceOnly) {
  std::vector<std::string> executables;
  executables.push_back(MakeTypedExecutable(ExecutableType_EXECUTION_ONLY, 1));
  auto package = MakePackage(executables);

  ExecutableRegistry registry(api::Chip::kBeagle);
  ASSERT_OK_AND_ASSIGN(auto api_ref, registry.RegisterSerialized(package));
  const auto* package_ref = dynamic_cast<const PackageReference*>(api_ref);
  EXPECT_NE(package_ref, nullptr);

  EXPECT_EQ(package_ref->AllExecutableReferences().size(), 1);
  EXPECT_EQ(package_ref->ParameterCachingEnabled(), false);
  EXPECT_EQ(package_ref->MainExecutableReference()->ParameterCachingToken(), 1);
}

// TODO(b/113115673) Update this test once this feature is implemented.
TEST_F(ExecutableRegistryTest, TestRegister3Executables) {
  std::vector<std::string> executables;
  executables.push_back(MakeTypedExecutable(ExecutableType_STAND_ALONE, 2));
  executables.push_back(
      MakeTypedExecutable(ExecutableType_PARAMETER_CACHING, 1));
  executables.push_back(MakeTypedExecutable(ExecutableType_EXECUTION_ONLY, 1));
  auto package = MakePackage(executables);

  ExecutableRegistry registry(api::Chip::kBeagle);
  ASSERT_OK_AND_ASSIGN(auto api_ref, registry.RegisterSerialized(package));
  const auto* package_ref = dynamic_cast<const PackageReference*>(api_ref);
  EXPECT_NE(package_ref, nullptr);

  EXPECT_EQ(package_ref->AllExecutableReferences().size(), 1);
  EXPECT_EQ(package_ref->ParameterCachingEnabled(), false);
  EXPECT_EQ(package_ref->MainExecutableReference()->ParameterCachingToken(), 2);
}

TEST_F(ExecutableRegistryTest, TestRegisterSameTypeExecutables) {
  std::vector<std::string> executables;
  executables.push_back(MakeTypedExecutable(ExecutableType_STAND_ALONE, 2));
  executables.push_back(MakeTypedExecutable(ExecutableType_STAND_ALONE, 1));
  auto package = MakePackage(executables);

  ExecutableRegistry registry(api::Chip::kBeagle);
  EXPECT_EQ(registry.RegisterSerialized(package).status().CanonicalCode(),
            util::error::INVALID_ARGUMENT);
}

TEST_F(ExecutableRegistryTest, TestRegisterIncompatibleExecutablesTypes) {
  std::vector<std::string> executables;
  executables.push_back(MakeTypedExecutable(ExecutableType_STAND_ALONE, 2));
  executables.push_back(
      MakeTypedExecutable(ExecutableType_PARAMETER_CACHING, 2));
  auto package = MakePackage(executables);

  ExecutableRegistry registry(api::Chip::kBeagle);
  EXPECT_EQ(registry.RegisterSerialized(package).status().CanonicalCode(),
            util::error::INVALID_ARGUMENT);
}

class ExecutableReferenceTest : public ::testing::Test {
 protected:
  ExecutableReferenceTest() {
    const std::vector<int> instruction_bytes = {1200};
    testutil::ExecutableGenerator generator;
    generator.AddInstructionBitstreams(instruction_bytes);
    generator.Finish();

    // Creates a dummy executable reference
    const auto* api_executable_reference =
        executable_registry_.RegisterSerialized(generator.PackageString())
            .ValueOrDie();
    const auto* package_reference =
        static_cast<const PackageReference*>(api_executable_reference);
    executable_reference_ = const_cast<ExecutableReference*>(
        package_reference->MainExecutableReference());

    constexpr int kAlignmentBytes = 0x4000;
    allocator_ = gtl::MakeUnique<AlignedAllocator>(kAlignmentBytes);
  }

  std::unique_ptr<AlignedAllocator> allocator_;
  driver::ExecutableReference* executable_reference_;
  ExecutableRegistry executable_registry_;
};

// Tests if the instruction buffers were able to be reused.
TEST_F(ExecutableReferenceTest, TestGetAndReturnOneInstructionBuffers) {
  // Creates new instruction buffers.
  std::unique_ptr<InstructionBuffers> instruction_buffers =
      executable_reference_->GetInstructionBuffers(allocator_.get());
  ASSERT_NE(instruction_buffers, nullptr);

  const InstructionBuffers* buffers1 = instruction_buffers.get();

  // Returns instruction buffers back to executable references.
  executable_reference_->ReturnInstructionBuffers(
      std::move(instruction_buffers));
  EXPECT_EQ(instruction_buffers, nullptr);

  // Reuses the returned instruction buffers.
  instruction_buffers =
      executable_reference_->GetInstructionBuffers(allocator_.get());
  ASSERT_NE(instruction_buffers, nullptr);

  const InstructionBuffers* buffers2 = instruction_buffers.get();
  EXPECT_EQ(buffers1, buffers2);
}

// Tests if multiple instruction buffers could be reused.
TEST_F(ExecutableReferenceTest, TestGetAndReturnTwoInstructionBuffers) {
  // Stores raw pointers to instruction buffers.
  std::vector<const InstructionBuffers*> allocated_buffers_vector;

  // Creates two instruction buffers
  std::unique_ptr<InstructionBuffers> instruction_buffers1 =
      executable_reference_->GetInstructionBuffers(allocator_.get());
  ASSERT_NE(instruction_buffers1, nullptr);

  const InstructionBuffers* buffers1 = instruction_buffers1.get();
  allocated_buffers_vector.push_back(buffers1);

  std::unique_ptr<InstructionBuffers> instruction_buffers2 =
      executable_reference_->GetInstructionBuffers(allocator_.get());
  ASSERT_NE(instruction_buffers2, nullptr);

  const InstructionBuffers* buffers2 = instruction_buffers2.get();
  allocated_buffers_vector.push_back(buffers2);

  // Returns both instruction buffers back to executable references.
  executable_reference_->ReturnInstructionBuffers(
      std::move(instruction_buffers2));
  EXPECT_EQ(instruction_buffers2, nullptr);

  executable_reference_->ReturnInstructionBuffers(
      std::move(instruction_buffers1));
  EXPECT_EQ(instruction_buffers1, nullptr);

  // Asks for two more instruction buffers. Expects to see same instruction
  // buffers returned before.
  std::unique_ptr<InstructionBuffers> instruction_buffers3 =
      executable_reference_->GetInstructionBuffers(allocator_.get());
  ASSERT_NE(instruction_buffers3, nullptr);
  auto iter =
      std::find(allocated_buffers_vector.begin(),
                allocated_buffers_vector.end(),
                instruction_buffers3.get());
  ASSERT_NE(iter, allocated_buffers_vector.end());

  std::unique_ptr<InstructionBuffers> instruction_buffers4 =
      executable_reference_->GetInstructionBuffers(allocator_.get());
  ASSERT_NE(instruction_buffers4, nullptr);
  iter = std::find(allocated_buffers_vector.begin(),
                   allocated_buffers_vector.end(),
                   instruction_buffers3.get());
  ASSERT_NE(iter, allocated_buffers_vector.end());
}

// Tests we can allocate multiple buffers.
TEST_F(ExecutableReferenceTest, TestGetMultipleBuffers) {
  // Placeholder for InstructionBuffers unique_ptrs.
  std::vector<std::unique_ptr<InstructionBuffers>> instruction_buffers_vector;
  // Stores raw pointers to instruction buffers.
  std::vector<const InstructionBuffers*> raw_buffers_vector;

  // Creates multiple instruction buffers and makes sure we didn't see them
  // before.
  for (int i = 0; i < 10; i++) {
    std::unique_ptr<InstructionBuffers> instruction_buffers =
      executable_reference_->GetInstructionBuffers(allocator_.get());
    ASSERT_NE(instruction_buffers, nullptr);

    // Verifies this is a new InstructionBuffers object.
    auto iter = std::find(raw_buffers_vector.begin(),
                          raw_buffers_vector.end(),
                          instruction_buffers.get());
    EXPECT_EQ(iter, raw_buffers_vector.end());

    const InstructionBuffers* buffers = instruction_buffers.get();
    raw_buffers_vector.push_back(buffers);

    instruction_buffers_vector.push_back(std::move(instruction_buffers));
  }
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
