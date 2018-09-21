#include "third_party/darwinn/api/buffer.h"

#include <stddef.h>
#include <stdlib.h>
#include <memory>
#include <string>

#include "third_party/darwinn/api/allocated_buffer.h"
#include "third_party/darwinn/driver/allocator.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::Return;

class MockAllocator : public Allocator {
 public:
  MOCK_METHOD1(Allocate, void*(size_t));
  MOCK_METHOD1(Free, void(void*));
};

class BufferTest : public ::testing::Test {
 protected:
  BufferTest() = default;
  ~BufferTest() override = default;

  std::unique_ptr<AllocatedBuffer> MakeAllocatedBuffer(size_t size_bytes) {
    auto free_cb = [this](void* ptr) { allocator_.Free(ptr); };
    return gtl::MakeUnique<AllocatedBuffer>(
        static_cast<uint8*>(allocator_.Allocate(size_bytes)), size_bytes,
        std::move(free_cb));
  }

  MockAllocator allocator_;
};

TEST_F(BufferTest, WrappedBufferTest) {
  uint8 temp = 128;
  const Buffer buffer(&temp, sizeof(temp));

  EXPECT_TRUE(buffer.IsValid());
  EXPECT_EQ(buffer.ptr(), &temp);
  EXPECT_EQ(buffer.size_bytes(), sizeof(temp));

  // Following is a compiler error since buffer is const.
  // uint8* b = buffer.ptr();
}

TEST_F(BufferTest, WrappedBufferCopyConstructorTest) {
  uint8 temp = 128;
  Buffer buffer(&temp, sizeof(temp));

  Buffer other_buffer(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), &temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));
}

TEST_F(BufferTest, WrappedBufferCopyAssignmentTest) {
  uint8 temp = 128;
  Buffer buffer(&temp, sizeof(temp));

  Buffer other_buffer = buffer;
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), &temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));
}

TEST_F(BufferTest, WrappedBufferMoveConstructorTest) {
  uint8 temp = 128;
  Buffer buffer(&temp, sizeof(temp));

  Buffer other_buffer(std::move(buffer));
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), &temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));

  EXPECT_FALSE(buffer.IsValid());
  EXPECT_EQ(buffer.ptr(), nullptr);
  EXPECT_EQ(buffer.size_bytes(), 0);
}

TEST_F(BufferTest, WrappedBufferMoveAssignmentTest) {
  uint8 temp = 128;
  Buffer buffer(&temp, sizeof(temp));

  Buffer other_buffer = std::move(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), &temp);
  EXPECT_EQ(other_buffer.size_bytes(), sizeof(temp));

  EXPECT_FALSE(buffer.IsValid());
  EXPECT_EQ(buffer.ptr(), nullptr);
  EXPECT_EQ(buffer.size_bytes(), 0);
}

TEST_F(BufferTest, Equality) {
  uint8 temp1 = 128;
  uint8 temp2 = 23;

  Buffer bufferA1(&temp1, sizeof(temp1));
  Buffer bufferA2(&temp1, sizeof(temp1));
  EXPECT_EQ(bufferA1, bufferA2);

  Buffer bufferA3(&temp1, sizeof(temp1) + 2);
  EXPECT_NE(bufferA1, bufferA3);
  Buffer bufferA4(&temp2, sizeof(temp1));
  EXPECT_NE(bufferA1, bufferA4);
}

TEST_F(BufferTest, AllocatedBufferTest) {
  void* buf_ptr = reinterpret_cast<void*>(0xDEADBEEF);
  const size_t buf_size = 1024;

  EXPECT_CALL(allocator_, Allocate(buf_size)).WillOnce(Return(buf_ptr));
  EXPECT_CALL(allocator_, Free(buf_ptr)).Times(1);

  std::shared_ptr<AllocatedBuffer> allocated_buffer =
      MakeAllocatedBuffer(buf_size);
  Buffer buffer(allocated_buffer);

  EXPECT_EQ(buffer.ptr(), reinterpret_cast<uint8*>(buf_ptr));
  EXPECT_EQ(buffer.size_bytes(), buf_size);

  EXPECT_EQ(allocated_buffer.use_count(), 2);
}

TEST_F(BufferTest, AllocatedBufferCopyConstructorTest) {
  void* buf_ptr = reinterpret_cast<void*>(0xDEADBEEF);
  const size_t buf_size = 1024;

  EXPECT_CALL(allocator_, Allocate(buf_size)).WillOnce(Return(buf_ptr));
  EXPECT_CALL(allocator_, Free(buf_ptr)).Times(1);

  std::shared_ptr<AllocatedBuffer> allocated_buffer =
      MakeAllocatedBuffer(buf_size);
  Buffer buffer(allocated_buffer);

  Buffer other_buffer(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), reinterpret_cast<uint8*>(buf_ptr));
  EXPECT_EQ(other_buffer.size_bytes(), buf_size);
  EXPECT_EQ(buffer, other_buffer);

  EXPECT_EQ(allocated_buffer.use_count(), 3);
}

TEST_F(BufferTest, AllocatedBufferCopyAssignmentTest) {
  void* buf_ptr = reinterpret_cast<void*>(0xDEADBEEF);
  const size_t buf_size = 1024;

  EXPECT_CALL(allocator_, Allocate(buf_size)).WillOnce(Return(buf_ptr));
  EXPECT_CALL(allocator_, Free(buf_ptr)).Times(1);

  std::shared_ptr<AllocatedBuffer> allocated_buffer =
      MakeAllocatedBuffer(buf_size);
  Buffer buffer(allocated_buffer);

  Buffer other_buffer = buffer;
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), reinterpret_cast<uint8*>(buf_ptr));
  EXPECT_EQ(other_buffer.size_bytes(), buf_size);
  EXPECT_EQ(buffer, other_buffer);

  EXPECT_EQ(allocated_buffer.use_count(), 3);
}

TEST_F(BufferTest, AllocatedBufferMoveConstructorTest) {
  void* buf_ptr = reinterpret_cast<void*>(0xDEADBEEF);
  const size_t buf_size = 1024;

  EXPECT_CALL(allocator_, Allocate(buf_size)).WillOnce(Return(buf_ptr));
  EXPECT_CALL(allocator_, Free(buf_ptr)).Times(1);

  std::shared_ptr<AllocatedBuffer> allocated_buffer =
      MakeAllocatedBuffer(buf_size);
  Buffer buffer(allocated_buffer);

  Buffer other_buffer(std::move(buffer));
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), reinterpret_cast<uint8*>(buf_ptr));
  EXPECT_EQ(other_buffer.size_bytes(), buf_size);

  EXPECT_FALSE(buffer.IsValid());
  EXPECT_EQ(buffer.ptr(), nullptr);
  EXPECT_EQ(buffer.size_bytes(), 0);

  EXPECT_EQ(allocated_buffer.use_count(), 2);
}

TEST_F(BufferTest, AllocatedBufferMoveAssignmentTest) {
  void* buf_ptr = reinterpret_cast<void*>(0xDEADBEEF);
  const size_t buf_size = 1024;

  EXPECT_CALL(allocator_, Allocate(buf_size)).WillOnce(Return(buf_ptr));
  EXPECT_CALL(allocator_, Free(buf_ptr)).Times(1);

  std::shared_ptr<AllocatedBuffer> allocated_buffer =
      MakeAllocatedBuffer(buf_size);
  Buffer buffer(allocated_buffer);

  Buffer other_buffer = std::move(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.ptr(), reinterpret_cast<uint8*>(buf_ptr));
  EXPECT_EQ(other_buffer.size_bytes(), buf_size);

  EXPECT_FALSE(buffer.IsValid());
  EXPECT_EQ(buffer.ptr(), nullptr);
  EXPECT_EQ(buffer.size_bytes(), 0);

  EXPECT_EQ(allocated_buffer.use_count(), 2);
}

TEST_F(BufferTest, AllocatedBufferEquality) {
  void* buf_ptr1 = reinterpret_cast<void*>(0xDEADBEEF);
  void* buf_ptr2 = reinterpret_cast<void*>(0xDEADFACE);
  const size_t buf_size1 = 1020;
  const size_t buf_size2 = 1024;

  EXPECT_CALL(allocator_, Allocate(buf_size1)).WillOnce(Return(buf_ptr1));
  EXPECT_CALL(allocator_, Allocate(buf_size2)).WillOnce(Return(buf_ptr2));
  EXPECT_CALL(allocator_, Free(buf_ptr1)).Times(1);
  EXPECT_CALL(allocator_, Free(buf_ptr2)).Times(1);

  Buffer bufferA1(MakeAllocatedBuffer(buf_size1));
  Buffer bufferA2(bufferA1);

  Buffer bufferB1(MakeAllocatedBuffer(buf_size2));

  EXPECT_EQ(bufferA1, bufferA2);
  EXPECT_NE(bufferA1, bufferB1);
}

TEST_F(BufferTest, FdBufferTest) {
  int fd = 42;
  size_t fd_sz = 4242;
  const Buffer buffer(fd, fd_sz);

  EXPECT_TRUE(buffer.IsValid());
  EXPECT_EQ(buffer.size_bytes(), fd_sz);
  EXPECT_EQ(buffer.fd(), fd);
}

TEST_F(BufferTest, FdBufferCopyConstructorTest) {
  int fd = 42;
  size_t fd_sz = 4242;
  const Buffer buffer(fd, fd_sz);

  Buffer other_buffer(buffer);
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.fd(), fd);
  EXPECT_EQ(other_buffer.size_bytes(), fd_sz);
}

TEST_F(BufferTest, FdBufferCopyAssignmentTest) {
  int fd = 42;
  size_t fd_sz = 4242;
  const Buffer buffer(fd, fd_sz);

  Buffer other_buffer = buffer;
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.fd(), fd);
  EXPECT_EQ(other_buffer.size_bytes(), fd_sz);
}

TEST_F(BufferTest, FdBufferMoveConstructorTest) {
  int fd = 42;
  size_t fd_sz = 4242;
  Buffer buffer(fd, fd_sz);

  Buffer other_buffer(std::move(buffer)); // NOLINT
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.fd(), fd);
  EXPECT_EQ(other_buffer.size_bytes(), fd_sz);

  EXPECT_FALSE(buffer.IsValid()); // NOLINT
  EXPECT_EQ(buffer.size_bytes(), 0); // NOLINT
}

TEST_F(BufferTest, FdBufferMoveAssignmentTest) {
  int fd = 42;
  size_t fd_sz = 4242;
  Buffer buffer(fd, fd_sz);

  Buffer other_buffer = std::move(buffer); // NOLINT
  EXPECT_TRUE(other_buffer.IsValid());
  EXPECT_EQ(other_buffer.fd(), fd);
  EXPECT_EQ(other_buffer.size_bytes(), fd_sz);

  EXPECT_FALSE(buffer.IsValid()); // NOLINT
  EXPECT_EQ(buffer.size_bytes(), 0); // NOLINT
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
