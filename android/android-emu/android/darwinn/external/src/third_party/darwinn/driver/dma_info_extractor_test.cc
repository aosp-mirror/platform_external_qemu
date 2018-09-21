#include "third_party/darwinn/driver/dma_info_extractor.h"

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/memory/mock_address_space.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/driver/test_util.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;

class DmaInfoExtractorTest : public ::testing::Test {
 protected:
  DmaInfoExtractorTest() {
    address_space_ = gtl::MakeUnique<MockAddressSpace>();
    mapper_ = gtl::MakeUnique<DeviceBufferMapper>(address_space_.get());
  }

  // Test helpers.
  std::unique_ptr<MockAddressSpace> address_space_;
  std::unique_ptr<DeviceBufferMapper> mapper_;
};

TEST_F(DmaInfoExtractorTest, SingleChunk) {
  const std::vector<uint64> device_addresses = {10000};
  const std::vector<int> instruction_bytes = {1724};

  testutil::ExecutableGenerator generator;
  generator.AddInstructionBitstreams(instruction_bytes);
  generator.Finish();
  auto executable = generator.Executable();

  PackageRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(auto api_executable_reference,
                       registry.RegisterSerialized(generator.PackageString()));

  const auto* package_reference =
      dynamic_cast<const PackageReference*>(api_executable_reference);
  ASSERT_NE(package_reference, nullptr);
  const auto* executable_reference =
      package_reference->MainExecutableReference();
  ASSERT_NE(executable_reference, nullptr);

  DmaInfoExtractor extractor(DmaInfoExtractor::ExtractorType::kInstructionDma);

  {
    InSequence s;
    for (const int device_address : device_addresses) {
      EXPECT_CALL(*address_space_, MapMemory(_, _, _))
          .WillOnce(
              Invoke([device_address](
                         const Buffer& buffer, DmaDirection,
                         MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
                return DeviceBuffer(device_address, buffer.size_bytes());
              }))
          .RetiresOnSaturation();
    }
  }

  std::vector<Buffer> instructions;
  for (const auto& chunk : *executable->instruction_bitstreams()) {
    instructions.push_back(
        Buffer(chunk->bitstream()->data(), chunk->bitstream()->Length()));
  }
  EXPECT_OK(mapper_->MapInstructions(instructions));

  const auto dmas = extractor.ExtractDmaInfos(*executable_reference, *mapper_);
  EXPECT_EQ(instruction_bytes.size(), dmas.size());
  int i = 0;
  for (const auto& dma : dmas) {
    EXPECT_EQ(dma.type(), DmaDescriptorType::kInstruction);
    EXPECT_EQ(dma.buffer().device_address(), device_addresses[i]);
    EXPECT_EQ(dma.buffer().size_bytes(), instruction_bytes[i]);
    ++i;
  }
}

TEST_F(DmaInfoExtractorTest, MultipleChunks) {
  const std::vector<uint64> device_addresses = {10000, 20000, 30000, 40000};
  const std::vector<int> instruction_bytes = {2048, 4000, 1011, 333};

  testutil::ExecutableGenerator generator;
  generator.AddInstructionBitstreams(instruction_bytes);
  generator.ExecBuilder()->add_version(1);
  generator.Finish();
  auto executable = generator.Executable();

  PackageRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(auto api_executable_reference,
                       registry.RegisterSerialized(generator.PackageString()));

  const auto* package_reference =
      dynamic_cast<const PackageReference*>(api_executable_reference);
  ASSERT_NE(package_reference, nullptr);
  const auto* executable_reference =
      package_reference->MainExecutableReference();
  ASSERT_NE(executable_reference, nullptr);

  DmaInfoExtractor extractor(DmaInfoExtractor::ExtractorType::kInstructionDma);

  {
    InSequence s;
    for (const int device_address : device_addresses) {
      EXPECT_CALL(*address_space_, MapMemory(_, _, _))
          .WillOnce(
              Invoke([device_address](
                         const Buffer& buffer, DmaDirection,
                         MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
                return DeviceBuffer(device_address, buffer.size_bytes());
              }))
          .RetiresOnSaturation();
    }
  }

  std::vector<Buffer> instructions;
  for (const auto& chunk : *executable->instruction_bitstreams()) {
    instructions.push_back(
        Buffer(chunk->bitstream()->data(), chunk->bitstream()->Length()));
  }
  EXPECT_OK(mapper_->MapInstructions(instructions));

  const auto dmas = extractor.ExtractDmaInfos(*executable_reference, *mapper_);
  EXPECT_EQ(instruction_bytes.size(), dmas.size());
  int i = 0;
  for (const auto& dma : dmas) {
    EXPECT_EQ(dma.type(), DmaDescriptorType::kInstruction);
    EXPECT_EQ(dma.buffer().device_address(), device_addresses[i]);
    EXPECT_EQ(dma.buffer().size_bytes(), instruction_bytes[i]);
    ++i;
  }
}

using DmaHintExtractorTest = DmaInfoExtractorTest;

TEST_F(DmaHintExtractorTest, GlobalFence) {
  const std::vector<int> instruction_bytes = {};
  testutil::ExecutableGenerator generator;
  generator.AddInstructionBitstreams(instruction_bytes);

  auto builder = generator.Builder();
  std::vector<flatbuffers::Offset<darwinn::DmaHint>> hints;
  auto hints_vector = builder->CreateVector(hints);
  DmaHintsBuilder dma_hints_builder(*builder);
  dma_hints_builder.add_fully_deterministic(false);
  dma_hints_builder.add_hints(hints_vector);
  auto dma_hints = dma_hints_builder.Finish();
  generator.ExecBuilder()->add_dma_hints(dma_hints);

  generator.Finish();

  PackageRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(auto api_executable_reference,
                       registry.RegisterSerialized(generator.PackageString()));

  const auto* package_reference =
      dynamic_cast<const PackageReference*>(api_executable_reference);
  ASSERT_NE(package_reference, nullptr);
  const auto* executable_reference =
      package_reference->MainExecutableReference();
  ASSERT_NE(executable_reference, nullptr);

  DmaInfoExtractor extractor(DmaInfoExtractor::ExtractorType::kDmaHints);

  const auto dmas = extractor.ExtractDmaInfos(*executable_reference, *mapper_);
  EXPECT_EQ(dmas.size(), 1);

  const auto& dma = dmas.front();
  EXPECT_EQ(dma.type(), DmaDescriptorType::kGlobalFence);
  EXPECT_FALSE(dma.buffer().IsValid());
}

TEST_F(DmaHintExtractorTest, SimpleOffsetByte) {
  const std::string input_name("input");
  // Test buffer.
  constexpr uint64 kInputDeviceAddress = 10000;
  const Buffer input(reinterpret_cast<uint8*>(0xdeadbeef), 99999);

  auto dma_hints = gtl::MakeUnique<DmaHintsT>();
  dma_hints->fully_deterministic = true;

  // Multiple parameter hints at different offset/bytes.
  std::vector<DeviceBuffer> hint_sequence;
  {
    auto meta = gtl::MakeUnique<MetaT>();
    meta->desc = Description_BASE_ADDRESS_INPUT_ACTIVATION;
    meta->batch = 0;
    meta->name = input_name;
    constexpr int kOffsetBytes = 0;
    constexpr int kSizeBytes = 2017;

    auto* desc_hint = new DmaDescriptorHintT();
    desc_hint->offset_in_bytes = kOffsetBytes;
    desc_hint->size_in_bytes = kSizeBytes;
    desc_hint->meta = std::move(meta);

    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.value = reinterpret_cast<void*>(desc_hint);
    hint->any_hint.type = AnyHint_DmaDescriptorHint;

    dma_hints->hints.push_back(std::move(hint));
    hint_sequence.push_back(
        DeviceBuffer(kInputDeviceAddress + kOffsetBytes, kSizeBytes));
  }
  {
    auto meta = gtl::MakeUnique<MetaT>();
    meta->desc = Description_BASE_ADDRESS_INPUT_ACTIVATION;
    meta->batch = 0;
    meta->name = input_name;

    constexpr int kOffsetBytes = 2017;
    constexpr int kSizeBytes = 8192;

    auto* desc_hint = new DmaDescriptorHintT();
    desc_hint->offset_in_bytes = kOffsetBytes;
    desc_hint->size_in_bytes = kSizeBytes;
    desc_hint->meta = std::move(meta);

    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.value = reinterpret_cast<void*>(desc_hint);
    hint->any_hint.type = AnyHint_DmaDescriptorHint;

    dma_hints->hints.push_back(std::move(hint));
    hint_sequence.push_back(
        DeviceBuffer(kInputDeviceAddress + kOffsetBytes, kSizeBytes));
  }
  {
    auto meta = gtl::MakeUnique<MetaT>();
    meta->desc = Description_BASE_ADDRESS_INPUT_ACTIVATION;
    meta->batch = 0;
    meta->name = input_name;

    constexpr int kOffsetBytes = 41414;
    constexpr int kSizeBytes = 31313;

    auto* desc_hint = new DmaDescriptorHintT();
    desc_hint->offset_in_bytes = kOffsetBytes;
    desc_hint->size_in_bytes = kSizeBytes;
    desc_hint->meta = std::move(meta);

    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.value = reinterpret_cast<void*>(desc_hint);
    hint->any_hint.type = AnyHint_DmaDescriptorHint;

    dma_hints->hints.push_back(std::move(hint));
    hint_sequence.push_back(
        DeviceBuffer(kInputDeviceAddress + kOffsetBytes, kSizeBytes));
  }

  DmaInfoExtractor extractor(DmaInfoExtractor::ExtractorType::kDmaHints);

  EXPECT_CALL(*address_space_, MapMemory(_, _, _))
      .WillOnce(Invoke([](const Buffer& buffer, DmaDirection,
                          MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
        return DeviceBuffer(kInputDeviceAddress, buffer.size_bytes());
      }));
  Buffer::NamedMap inputs;
  inputs[input_name].push_back(input);
  EXPECT_OK(mapper_->MapInputs(inputs));

  ExecutableT executable;
  executable.batch_size = 1;
  executable.dma_hints = std::move(dma_hints);
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(darwinn::Executable::Pack(builder, &executable));
  const string serialized(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize());
  const string packaged =
      testutil::ExecutableGenerator::PackageString(serialized);
  PackageRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(auto api_executable_reference,
                       registry.RegisterSerialized(packaged));

  const auto* package_reference =
      dynamic_cast<const PackageReference*>(api_executable_reference);
  ASSERT_NE(package_reference, nullptr);
  const auto* executable_reference =
      package_reference->MainExecutableReference();
  ASSERT_NE(executable_reference, nullptr);

  const auto dmas = extractor.ExtractDmaInfos(*executable_reference, *mapper_);
  EXPECT_EQ(dmas.size(), hint_sequence.size());
  int i = 0;
  for (const auto& dma : dmas) {
    const auto& device_buffer = hint_sequence[i];
    EXPECT_EQ(dma.type(), DmaDescriptorType::kInputActivation);
    EXPECT_EQ(dma.buffer().device_address(), device_buffer.device_address());
    EXPECT_EQ(dma.buffer().size_bytes(), device_buffer.size_bytes());
    ++i;
  }
}

TEST_F(DmaHintExtractorTest, SimpleExecutable) {
  const std::string input_name("input");
  const std::string output_name("output");

  // Test buffers.
  constexpr uint64 kParameterDeviceAddress = 10000;
  const Buffer parameter(reinterpret_cast<uint8*>(0xdeadbeef), 4567);

  constexpr uint64 kInputDeviceAddress = 20000;
  Buffer::NamedMap input;
  input[input_name].push_back(
      Buffer(reinterpret_cast<uint8*>(0xaadeadbeef), 2345));

  constexpr uint64 kOutputDeviceAddress = 30000;
  Buffer::NamedMap output;
  output[output_name].push_back(
      Buffer(reinterpret_cast<uint8*>(0xeedeadbeef), 8765));

  const std::vector<uint64> instruction_device_addresses = {40000};
  const std::vector<int> instruction_bytes = {1234};

  ExecutableT executable;
  executable.batch_size = 1;
  for (const int num_bytes : instruction_bytes) {
    // Fill random instructions.
    std::string bitstream(num_bytes, 'a');
    auto instruction_bitstream = gtl::MakeUnique<InstructionBitstreamT>();
    instruction_bitstream->bitstream =
        std::vector<uint8>(bitstream.begin(), bitstream.end());
    executable.instruction_bitstreams.push_back(
        std::move(instruction_bitstream));
  }

  auto dma_hints = gtl::MakeUnique<DmaHintsT>();
  dma_hints->fully_deterministic = true;

  // Hint is instruction -> input -> parameter -> output -> interrupt.
  {
    auto* instruction_hint = new InstructionHintT();
    instruction_hint->instruction_chunk_index = 0;
    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.type = AnyHint_InstructionHint;
    hint->any_hint.value = reinterpret_cast<void*>(instruction_hint);
    dma_hints->hints.push_back(std::move(hint));
  }
  {
    auto meta = gtl::MakeUnique<MetaT>();
    meta->desc = Description_BASE_ADDRESS_INPUT_ACTIVATION;
    meta->batch = 0;
    meta->name = input_name;

    auto* desc_hint = new DmaDescriptorHintT();
    desc_hint->offset_in_bytes = 0;
    desc_hint->size_in_bytes = input[input_name][0].size_bytes();
    desc_hint->meta = std::move(meta);

    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.value = reinterpret_cast<void*>(desc_hint);
    hint->any_hint.type = AnyHint_DmaDescriptorHint;
    dma_hints->hints.push_back(std::move(hint));
  }
  {
    auto meta = gtl::MakeUnique<MetaT>();
    meta->desc = Description_BASE_ADDRESS_PARAMETER;

    auto* desc_hint = new DmaDescriptorHintT();
    desc_hint->offset_in_bytes = 0;
    desc_hint->size_in_bytes = parameter.size_bytes();
    desc_hint->meta = std::move(meta);

    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.value = reinterpret_cast<void*>(desc_hint);
    hint->any_hint.type = AnyHint_DmaDescriptorHint;
    dma_hints->hints.push_back(std::move(hint));
  }
  {
    auto meta = gtl::MakeUnique<MetaT>();
    meta->desc = Description_BASE_ADDRESS_OUTPUT_ACTIVATION;
    meta->batch = 0;
    meta->name = output_name;

    auto* desc_hint = new DmaDescriptorHintT();
    desc_hint->offset_in_bytes = 0;
    desc_hint->size_in_bytes = output[output_name][0].size_bytes();
    desc_hint->meta = std::move(meta);

    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.value = reinterpret_cast<void*>(desc_hint);
    hint->any_hint.type = AnyHint_DmaDescriptorHint;
    dma_hints->hints.push_back(std::move(hint));
  }
  {
    auto* interrupt_hint = new InterruptHintT();
    interrupt_hint->type = InterruptType_SCALAR_CORE_INT_0;
    auto hint = gtl::MakeUnique<DmaHintT>();
    hint->any_hint.type = AnyHint_InterruptHint;
    hint->any_hint.value = reinterpret_cast<void*>(interrupt_hint);
    dma_hints->hints.push_back(std::move(hint));
  }
  executable.dma_hints = std::move(dma_hints);

  std::vector<DeviceBuffer> hint_sequence;
  hint_sequence.push_back(
      DeviceBuffer(instruction_device_addresses[0], instruction_bytes[0]));
  hint_sequence.push_back(
      DeviceBuffer(kInputDeviceAddress, input[input_name][0].size_bytes()));
  hint_sequence.push_back(
      DeviceBuffer(kParameterDeviceAddress, parameter.size_bytes()));
  hint_sequence.push_back(
      DeviceBuffer(kOutputDeviceAddress, output[output_name][0].size_bytes()));

  DmaInfoExtractor extractor(DmaInfoExtractor::ExtractorType::kDmaHints);

  // Expects Map to occur in input -> output -> instructions.
  std::vector<DeviceBuffer> map_sequence;
  map_sequence.push_back(
      DeviceBuffer(kInputDeviceAddress, input[input_name][0].size_bytes()));
  map_sequence.push_back(
      DeviceBuffer(kOutputDeviceAddress, output[output_name][0].size_bytes()));
  map_sequence.push_back(
      DeviceBuffer(instruction_device_addresses[0], instruction_bytes[0]));
  {
    InSequence s;
    for (const auto& device_buffer : map_sequence) {
      EXPECT_CALL(*address_space_, MapMemory(_, _, _))
          .WillOnce(
              Invoke([&device_buffer](
                         const Buffer& buffer, DmaDirection,
                         MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
                EXPECT_EQ(device_buffer.size_bytes(), buffer.size_bytes());
                return device_buffer;
              }))
          .RetiresOnSaturation();
    }
  }

  // Map in input -> output -> instruction sequence.
  EXPECT_OK(mapper_->MapInputs(input));
  EXPECT_OK(mapper_->MapOutputs(output));

  std::vector<Buffer> instructions;
  for (const int num_bytes : instruction_bytes) {
    std::string bitstream(num_bytes, 'a');
    instructions.push_back(Buffer(bitstream.data(), bitstream.size()));
  }
  EXPECT_OK(mapper_->MapInstructions(instructions));

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(darwinn::Executable::Pack(builder, &executable));

  const string serialized(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize());
  const string packaged =
      testutil::ExecutableGenerator::PackageString(serialized);

  PackageRegistry registry(api::Chip::kBeagle);

  ASSERT_OK_AND_ASSIGN(auto api_package_reference,
                       registry.RegisterSerialized(packaged));

  const auto* package_reference =
      dynamic_cast<const driver::PackageReference*>(api_package_reference);
  ASSERT_NE(package_reference, nullptr);
  auto* executable_reference = const_cast<ExecutableReference*>(
      package_reference->MainExecutableReference());
  ASSERT_NE(executable_reference, nullptr);

  // Add mapping of parameter.
  executable_reference->SetMappedParameters(
      {DeviceBuffer(kParameterDeviceAddress, parameter.size_bytes()),
       [](DeviceBuffer) { return util::Status(); }});
  const auto dmas = extractor.ExtractDmaInfos(*executable_reference, *mapper_);
  EXPECT_EQ(dmas.size(), hint_sequence.size() + 1);
  for (int i = 0; i < hint_sequence.size(); ++i) {
    auto it = dmas.begin();
    std::advance(it, i);
    const auto& dma = *it;
    const auto& device_buffer = hint_sequence[i];
    EXPECT_EQ(dma.buffer().device_address(), device_buffer.device_address());
    EXPECT_EQ(dma.buffer().size_bytes(), device_buffer.size_bytes());
  }

  const std::vector<DmaDescriptorType> expected_types = {
      DmaDescriptorType::kInstruction, DmaDescriptorType::kInputActivation,
      DmaDescriptorType::kParameter, DmaDescriptorType::kOutputActivation};

  for (int i = 0; i < expected_types.size(); ++i) {
    auto it = dmas.begin();
    std::advance(it, i);
    EXPECT_EQ(it->type(), expected_types[i]);
  }

  EXPECT_EQ(dmas.back().type(), DmaDescriptorType::kScalarCoreInterrupt0);
  EXPECT_FALSE(dmas.back().buffer().IsValid());
  registry.UnregisterAll();
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
