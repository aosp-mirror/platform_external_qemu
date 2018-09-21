#include "third_party/darwinn/driver/dma_chunker.h"

#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

TEST(DmaChunkerTest, CompleteTransferFullCommit) {
  constexpr int kBufferSize = 20000;

  DmaChunker chunker(DmaChunker::HardwareProcessing::kCommitted,
                     DeviceBuffer(0, kBufferSize));

  EXPECT_TRUE(chunker.HasNextChunk());
  EXPECT_FALSE(chunker.IsCompleted());

  auto chunk = chunker.GetNextChunk();
  EXPECT_EQ(chunk.device_address(), 0);
  EXPECT_EQ(chunk.size_bytes(), kBufferSize);
  EXPECT_FALSE(chunker.IsCompleted());
  EXPECT_FALSE(chunker.HasNextChunk());

  chunker.NotifyTransfer(kBufferSize);
  EXPECT_TRUE(chunker.IsCompleted());
}

TEST(DmaChunkerTest, CompleteTransferFullBestEffort) {
  constexpr int kChunkSize = 10;
  constexpr int kIterations = 2000;
  constexpr int kBufferSize = 20000;

  DmaChunker chunker(DmaChunker::HardwareProcessing::kBestEffort,
                     DeviceBuffer(0, kBufferSize));

  int iterations = 0;
  while (chunker.HasNextChunk()) {
    EXPECT_FALSE(chunker.IsCompleted());

    auto chunk = chunker.GetNextChunk();
    const int offset = kChunkSize * iterations;
    EXPECT_EQ(chunk.device_address(), offset);
    EXPECT_EQ(chunk.size_bytes(), kBufferSize - offset);

    chunker.NotifyTransfer(kChunkSize);
    ++iterations;
  }

  EXPECT_EQ(iterations, kIterations);
  EXPECT_TRUE(chunker.IsCompleted());
}

TEST(DmaChunkerTest, IsActiveInCommitted) {
  constexpr int kChunkSize = 10;
  constexpr int kIterations = 2000;
  constexpr int kBufferSize = kChunkSize * kIterations;

  DmaChunker chunker(DmaChunker::HardwareProcessing::kCommitted,
                     DeviceBuffer(0, kBufferSize));

  int iterations = 0;
  EXPECT_FALSE(chunker.IsActive());
  while (chunker.HasNextChunk()) {
    EXPECT_FALSE(chunker.IsCompleted());

    auto chunk = chunker.GetNextChunk(kChunkSize);
    EXPECT_EQ(chunk.device_address(), kChunkSize * iterations);
    EXPECT_EQ(chunk.size_bytes(), kChunkSize);

    EXPECT_TRUE(chunker.IsActive());
    ++iterations;
  }

  EXPECT_EQ(iterations, kIterations);
  for (int i = 0; i < kIterations; ++i) {
    EXPECT_TRUE(chunker.IsActive());
    chunker.NotifyTransfer(kChunkSize);
  }

  EXPECT_FALSE(chunker.IsActive());
  EXPECT_TRUE(chunker.IsCompleted());
}

TEST(DmaChunkerTest, IsActiveInBestEffort) {
  constexpr uint64 kStartAddress = 2048;
  constexpr int kChunkSize = 10;
  constexpr int kIterations = 2000;
  constexpr int kBufferSize = 20000;

  DmaChunker chunker(DmaChunker::HardwareProcessing::kBestEffort,
                     DeviceBuffer(kStartAddress, kBufferSize));

  int iterations = 0;
  while (chunker.HasNextChunk()) {
    EXPECT_FALSE(chunker.IsActive());
    EXPECT_FALSE(chunker.IsCompleted());

    auto chunk = chunker.GetNextChunk();
    EXPECT_TRUE(chunker.IsActive());
    const int offset = kChunkSize * iterations;
    EXPECT_EQ(chunk.device_address(), kStartAddress + offset);
    EXPECT_EQ(chunk.size_bytes(), kBufferSize - offset);

    chunker.NotifyTransfer(kChunkSize);
    EXPECT_FALSE(chunker.IsActive());
    ++iterations;
  }

  EXPECT_EQ(iterations, kIterations);
  EXPECT_FALSE(chunker.IsActive());
  EXPECT_TRUE(chunker.IsCompleted());
}

class DmaChunkerParamTest
    : public ::testing::TestWithParam<DmaChunker::HardwareProcessing> {};

TEST_P(DmaChunkerParamTest, ZeroBuffer) {
  auto param = GetParam();
  DmaChunker chunker(param, DeviceBuffer());
  EXPECT_FALSE(chunker.HasNextChunk());
}

TEST_P(DmaChunkerParamTest, CompleteTransferInChunks) {
  constexpr int kChunkSize = 10;
  constexpr int kIterations = 2000;
  constexpr int kBufferSize = kChunkSize * kIterations;

  auto param = GetParam();
  DmaChunker chunker(param, DeviceBuffer(0, kBufferSize));

  int iterations = 0;
  while (chunker.HasNextChunk()) {
    EXPECT_FALSE(chunker.IsCompleted());

    auto chunk = chunker.GetNextChunk(kChunkSize);
    EXPECT_EQ(chunk.device_address(), kChunkSize * iterations);
    EXPECT_EQ(chunk.size_bytes(), kChunkSize);

    chunker.NotifyTransfer(kChunkSize);
    ++iterations;
  }

  EXPECT_EQ(iterations, kIterations);
  EXPECT_TRUE(chunker.IsCompleted());
}

TEST_P(DmaChunkerParamTest, CompleteTransferInChunksWithStartAddress) {
  constexpr uint64 kStartAddress = 2048;
  constexpr int kChunkSize = 10;
  constexpr int kIterations = 2000;
  constexpr int kBufferSize = kChunkSize * kIterations;

  auto param = GetParam();
  DmaChunker chunker(param, DeviceBuffer(kStartAddress, kBufferSize));

  int iterations = 0;
  while (chunker.HasNextChunk()) {
    EXPECT_FALSE(chunker.IsCompleted());

    auto chunk = chunker.GetNextChunk(kChunkSize);
    EXPECT_EQ(chunk.device_address(), kStartAddress + kChunkSize * iterations);
    EXPECT_EQ(chunk.size_bytes(), kChunkSize);

    chunker.NotifyTransfer(kChunkSize);
    ++iterations;
  }

  EXPECT_EQ(iterations, kIterations);
  EXPECT_TRUE(chunker.IsCompleted());
}

INSTANTIATE_TEST_CASE_P(
    DmaChunkerParamTest, DmaChunkerParamTest,
    ::testing::Values(DmaChunker::HardwareProcessing::kCommitted,
                      DmaChunker::HardwareProcessing::kBestEffort));

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
