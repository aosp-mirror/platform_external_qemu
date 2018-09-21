#include "third_party/darwinn/driver/usb/usb_io_request.h"

#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

TEST(UsbUsbIoRequestConstructorTest, InterruptFromDevice) {
  constexpr int kRequestId = 10;
  constexpr auto kTag = UsbMlCommands::DescriptorTag::kInterrupt0;
  UsbIoRequest request(kRequestId, kTag);

  // The type must be scalar core interrupt.
  EXPECT_EQ(request.GetType(), UsbIoRequest::Type::kScHostInterrupt);
  EXPECT_EQ(request.GetTag(), kTag);

  // It is meant for representing interrupt actually sent from device.
  EXPECT_EQ(request.GetSourceAndMatchStatus(),
            UsbIoRequest::SourceAndMatchStatus::kSubmittedByDevice);
}

TEST(UsbUsbIoRequestConstructorTest, RequestFromDevice) {
  constexpr int kRequestId = 10;
  constexpr auto kTag = UsbMlCommands::DescriptorTag::kInstructions;
  constexpr auto kType = UsbIoRequest::Type::kBulkOut;
  DeviceBuffer invalid_buffer;
  UsbIoRequest request(kRequestId, kType, kTag, invalid_buffer);

  // The type must be scalar core interrupt.
  EXPECT_EQ(request.GetType(), kType) << "Type mismatch";
  EXPECT_EQ(request.GetTag(), kTag) << "Tag mismatch";

  // It is meant for representing interrupt actually sent from device.
  EXPECT_EQ(request.GetSourceAndMatchStatus(),
            UsbIoRequest::SourceAndMatchStatus::kSubmittedByDevice);
}

struct TypeCompatibilityCombo {
  DmaDescriptorType dma_type;
  UsbIoRequest::Type io_request_type;
  UsbMlCommands::DescriptorTag io_request_tag;
};

TypeCompatibilityCombo type_combo[] = {
    {DmaDescriptorType::kInstruction, UsbIoRequest::Type::kBulkOut,
     UsbMlCommands::DescriptorTag::kInstructions},
    {DmaDescriptorType::kInputActivation, UsbIoRequest::Type::kBulkOut,
     UsbMlCommands::DescriptorTag::kInputActivations},
    {DmaDescriptorType::kParameter, UsbIoRequest::Type::kBulkOut,
     UsbMlCommands::DescriptorTag::kParameters},
    {DmaDescriptorType::kOutputActivation, UsbIoRequest::Type::kBulkIn,
     UsbMlCommands::DescriptorTag::kOutputActivations},
    {DmaDescriptorType::kScalarCoreInterrupt0,
     UsbIoRequest::Type::kScHostInterrupt,
     UsbMlCommands::DescriptorTag::kInterrupt0},
    {DmaDescriptorType::kScalarCoreInterrupt1,
     UsbIoRequest::Type::kScHostInterrupt,
     UsbMlCommands::DescriptorTag::kInterrupt1},
    {DmaDescriptorType::kScalarCoreInterrupt2,
     UsbIoRequest::Type::kScHostInterrupt,
     UsbMlCommands::DescriptorTag::kInterrupt2},
    {DmaDescriptorType::kScalarCoreInterrupt3,
     UsbIoRequest::Type::kScHostInterrupt,
     UsbMlCommands::DescriptorTag::kInterrupt3}};

class UsbUsbIoRequestHintTypeTest
    : public ::testing::TestWithParam<TypeCompatibilityCombo> {};

TEST_P(UsbUsbIoRequestHintTypeTest, HintTypeCompatibility) {
  auto param = GetParam();
  constexpr int kRequestId = 10;

  DmaInfo dma_info(kRequestId, param.dma_type);
  UsbIoRequest request(&dma_info);

  // The type must be as specified in dma info.
  EXPECT_EQ(request.GetType(), param.io_request_type);

  // The tag must be as specified in dma info.
  EXPECT_EQ(request.GetTag(), param.io_request_tag);

  // It is meant for representing a hint from code generator.
  EXPECT_EQ(request.GetSourceAndMatchStatus(),
            UsbIoRequest::SourceAndMatchStatus::kHintNotYetMatched);
}

INSTANTIATE_TEST_CASE_P(UsbUsbIoRequestHintTypeTest,
                        UsbUsbIoRequestHintTypeTest,
                        ::testing::ValuesIn(type_combo));

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
