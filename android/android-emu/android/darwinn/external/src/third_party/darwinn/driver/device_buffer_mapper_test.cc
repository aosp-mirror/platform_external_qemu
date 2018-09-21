#include "third_party/darwinn/driver/device_buffer_mapper.h"

#include <memory>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/memory/address_space.h"
#include "third_party/darwinn/driver/memory/mock_address_space.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::_;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Property;
using ::testing::Return;

// Returns a Buffer to test.
Buffer NextBuffer() {
  constexpr uint64 kBaseSizeBytes = 1000;
  static uint64 byte_address = 10000;

  void *host_ptr = reinterpret_cast<void *>(byte_address);
  byte_address += 10000;
  return Buffer(host_ptr, kBaseSizeBytes);
}

// Returns a device address to test.
uint64 GetTestDeviceAddress(const Buffer &buffer) {
  return (11ULL << 37) | reinterpret_cast<uint64>(buffer.ptr());
}

class DeviceBufferMapperTest : public ::testing::Test {
 protected:
  DeviceBufferMapperTest() {
    address_space_ = gtl::MakeUnique<MockAddressSpace>();
    mapper_ = gtl::MakeUnique<DeviceBufferMapper>(address_space_.get());
  }

  ~DeviceBufferMapperTest() override = default;

  // Test helpers.
  std::unique_ptr<MockAddressSpace> address_space_;

  // Test instance.
  std::unique_ptr<DeviceBufferMapper> mapper_;
};

TEST_F(DeviceBufferMapperTest, EmptyUnmap) { EXPECT_OK(mapper_->UnmapAll()); }

TEST_F(DeviceBufferMapperTest, MapUnmapAll) {
  Buffer::NamedMap input;
  Buffer::NamedMap output;
  std::vector<Buffer> instruction;

  input["input"].push_back(NextBuffer());
  output["output"].push_back(NextBuffer());
  instruction.push_back(NextBuffer());
  const auto param_buffer = NextBuffer();

  const auto input_device_address = GetTestDeviceAddress(input["input"][0]);
  const auto output_device_address = GetTestDeviceAddress(output["output"][0]);
  const auto instruction_device_address = GetTestDeviceAddress(instruction[0]);

  EXPECT_CALL(*address_space_,
              MapMemory(Eq(input["input"][0]), Eq(DmaDirection::kToDevice), _))
      .WillOnce(Invoke([input_device_address](
                           const Buffer &buffer, DmaDirection direction,
                           MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
        return DeviceBuffer(input_device_address, buffer.size_bytes());
      }));
  EXPECT_CALL(*address_space_, MapMemory(Eq(output["output"][0]),
                                         Eq(DmaDirection::kFromDevice), _))
      .WillOnce(Invoke([output_device_address](
                           const Buffer &buffer, DmaDirection direction,
                           MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
        return DeviceBuffer(output_device_address, buffer.size_bytes());
      }));
  EXPECT_CALL(*address_space_,
              MapMemory(Eq(instruction[0]), Eq(DmaDirection::kToDevice), _))
      .WillOnce(Invoke([instruction_device_address](
                           const Buffer &buffer, DmaDirection direction,
                           MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
        return DeviceBuffer(instruction_device_address, buffer.size_bytes());
      }));

  EXPECT_CALL(
      *address_space_,
      UnmapMemory(AllOf(
          Property(&DeviceBuffer::device_address, input_device_address),
          Property(&DeviceBuffer::size_bytes, input["input"][0].size_bytes()))))
      .WillOnce(Return(util::Status()));
  EXPECT_CALL(*address_space_,
              UnmapMemory(AllOf(Property(&DeviceBuffer::device_address,
                                         output_device_address),
                                Property(&DeviceBuffer::size_bytes,
                                         output["output"][0].size_bytes()))))
      .WillOnce(Return(util::Status()));
  EXPECT_CALL(
      *address_space_,
      UnmapMemory(AllOf(
          Property(&DeviceBuffer::device_address, instruction_device_address),
          Property(&DeviceBuffer::size_bytes, instruction[0].size_bytes()))))
      .WillOnce(Return(util::Status()));

  EXPECT_OK(mapper_->MapInputs(input));
  EXPECT_OK(mapper_->MapOutputs(output));
  EXPECT_OK(mapper_->MapInstructions(instruction));
  EXPECT_OK(mapper_->UnmapAll());
}

TEST_F(DeviceBufferMapperTest, MapPartialUnmapAll) {
  Buffer::NamedMap input;
  Buffer::NamedMap output;

  input["input"].push_back(NextBuffer());
  output["output"].push_back(NextBuffer());

  const auto input_device_address = GetTestDeviceAddress(input["input"][0]);
  const auto output_device_address = GetTestDeviceAddress(output["output"][0]);

  EXPECT_CALL(*address_space_,
              MapMemory(Eq(input["input"][0]), Eq(DmaDirection::kToDevice), _))
      .WillOnce(Invoke([input_device_address](
                           const Buffer &buffer, DmaDirection direction,
                           MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
        return DeviceBuffer(input_device_address, buffer.size_bytes());
      }));
  EXPECT_CALL(*address_space_, MapMemory(Eq(output["output"][0]),
                                         Eq(DmaDirection::kFromDevice), _))
      .WillOnce(Invoke([output_device_address](
                           const Buffer &buffer, DmaDirection direction,
                           MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
        return DeviceBuffer(output_device_address, buffer.size_bytes());
      }));

  EXPECT_CALL(
      *address_space_,
      UnmapMemory(AllOf(
          Property(&DeviceBuffer::device_address, input_device_address),
          Property(&DeviceBuffer::size_bytes, input["input"][0].size_bytes()))))
      .WillOnce(Return(util::Status()));
  EXPECT_CALL(*address_space_,
              UnmapMemory(AllOf(Property(&DeviceBuffer::device_address,
                                         output_device_address),
                                Property(&DeviceBuffer::size_bytes,
                                         output["output"][0].size_bytes()))))
      .WillOnce(Return(util::Status()));

  EXPECT_OK(mapper_->MapInputs(input));
  EXPECT_OK(mapper_->MapOutputs(output));
  EXPECT_OK(mapper_->UnmapAll());
}

TEST_F(DeviceBufferMapperTest, MapInputUnmap) {
  constexpr int kBatch = 4;
  Buffer::NamedMap input;

  for (int i = 0; i < kBatch; ++i) {
    input["input"].push_back(NextBuffer());
  }

  std::vector<uint64> device_addresses;
  device_addresses.reserve(input["input"].size());
  for (const auto &buffer : input["input"]) {
    device_addresses.push_back(GetTestDeviceAddress(buffer));
  }

  for (int i = 0; i < kBatch; ++i) {
    EXPECT_CALL(*address_space_, MapMemory(Eq(input["input"][i]),
                                           Eq(DmaDirection::kToDevice), _))
        .WillOnce(Invoke([&device_addresses, i](
                             const Buffer &buffer, DmaDirection direction,
                             MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
          return DeviceBuffer(device_addresses[i], buffer.size_bytes());
        }));

    EXPECT_CALL(*address_space_,
                UnmapMemory(AllOf(Property(&DeviceBuffer::device_address,
                                           device_addresses[i]),
                                  Property(&DeviceBuffer::size_bytes,
                                           input["input"][i].size_bytes()))))
        .WillOnce(Return(util::Status()));
  }

  EXPECT_OK(mapper_->MapInputs(input));
  EXPECT_OK(mapper_->UnmapAll());
}

TEST_F(DeviceBufferMapperTest, MapOutputUnmap) {
  constexpr int kBatch = 2;
  Buffer::NamedMap output;

  for (int i = 0; i < kBatch; ++i) {
    output["output0"].push_back(NextBuffer());
    output["output1"].push_back(NextBuffer());
  }

  for (int i = 0; i < kBatch; ++i) {
    const auto &output0 = output["output0"][i];
    const auto &output1 = output["output1"][i];

    const auto device_address0 = GetTestDeviceAddress(output0);
    const auto device_address1 = GetTestDeviceAddress(output1);

    EXPECT_CALL(*address_space_,
                MapMemory(Eq(output0), Eq(DmaDirection::kFromDevice), _))
        .WillOnce(Invoke(
            [device_address0](const Buffer &buffer, DmaDirection direction,
                              MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
              return DeviceBuffer(device_address0, buffer.size_bytes());
            }));
    EXPECT_CALL(*address_space_,
                MapMemory(Eq(output1), Eq(DmaDirection::kFromDevice), _))
        .WillOnce(Invoke(
            [device_address1](const Buffer &buffer, DmaDirection direction,
                              MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
              return DeviceBuffer(device_address1, buffer.size_bytes());
            }));

    EXPECT_CALL(*address_space_,
                UnmapMemory(AllOf(
                    Property(&DeviceBuffer::device_address, device_address0),
                    Property(&DeviceBuffer::size_bytes, output0.size_bytes()))))
        .WillOnce(Return(util::Status()));
    EXPECT_CALL(*address_space_,
                UnmapMemory(AllOf(
                    Property(&DeviceBuffer::device_address, device_address1),
                    Property(&DeviceBuffer::size_bytes, output1.size_bytes()))))
        .WillOnce(Return(util::Status()));
  }

  EXPECT_OK(mapper_->MapOutputs(output));
  EXPECT_OK(mapper_->UnmapAll());
}

TEST_F(DeviceBufferMapperTest, MapInstructionUnmap) {
  constexpr int kNumChunk = 3;

  std::vector<Buffer> instructions;
  for (int i = 0; i < kNumChunk; ++i) {
    instructions.push_back(NextBuffer());
  }

  std::vector<uint64> device_addresses;
  device_addresses.reserve(instructions.size());
  for (const auto &buffer : instructions) {
    device_addresses.push_back(GetTestDeviceAddress(buffer));
  }

  for (int i = 0; i < kNumChunk; ++i) {
    EXPECT_CALL(*address_space_,
                MapMemory(Eq(instructions[i]), Eq(DmaDirection::kToDevice), _))
        .WillOnce(Invoke([&device_addresses, i](
                             const Buffer &buffer, DmaDirection direction,
                             MappingTypeHint) -> util::StatusOr<DeviceBuffer> {
          return DeviceBuffer(device_addresses[i], buffer.size_bytes());
        }));

    EXPECT_CALL(
        *address_space_,
        UnmapMemory(AllOf(
            Property(&DeviceBuffer::device_address, device_addresses[i]),
            Property(&DeviceBuffer::size_bytes, instructions[i].size_bytes()))))
        .WillOnce(Return(util::Status()));
  }

  EXPECT_OK(mapper_->MapInstructions(instructions));
  EXPECT_OK(mapper_->UnmapAll());
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
