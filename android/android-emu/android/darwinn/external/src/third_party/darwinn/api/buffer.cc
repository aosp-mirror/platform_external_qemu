#include "third_party/darwinn/api/buffer.h"

#include <stddef.h>

#include "third_party/darwinn/api/allocated_buffer.h"
#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {

Buffer::Buffer(unsigned char* buffer, size_t size_bytes, bool on_chip_cached)
    : type_(Type::kWrapped),
      size_bytes_(size_bytes),
      ptr_(buffer),
      on_chip_cached_(on_chip_cached) {}

Buffer::Buffer(const unsigned char* buffer, size_t size_bytes,
               bool on_chip_cached)
    : Buffer(const_cast<unsigned char*>(buffer), size_bytes, on_chip_cached) {}

Buffer::Buffer(void* buffer, size_t size_bytes)
    : Buffer(reinterpret_cast<unsigned char*>(buffer), size_bytes) {}

Buffer::Buffer(const void* buffer, size_t size_bytes)
    : Buffer(const_cast<void*>(buffer), size_bytes) {}

Buffer::Buffer(int fd, size_t size_bytes)
    : type_(Type::kFileDescriptor),
      size_bytes_(size_bytes),
      file_descriptor_(fd) {}

Buffer::Buffer(std::shared_ptr<AllocatedBuffer> allocated_buffer)
    : type_(Type::kAllocated),
      size_bytes_(allocated_buffer->size_bytes()),
      ptr_(allocated_buffer->ptr()),
      allocated_buffer_(std::move(allocated_buffer)) {}

bool Buffer::operator==(const Buffer& rhs) const {
  return type_ == rhs.type_ && size_bytes_ == rhs.size_bytes_ &&
         ptr_ == rhs.ptr_ && allocated_buffer_ == rhs.allocated_buffer_;
}

bool Buffer::operator!=(const Buffer& rhs) const { return !(*this == rhs); }

Buffer::Buffer(Buffer&& other)
    : type_(other.type_),
      size_bytes_(other.size_bytes_),
      ptr_(other.ptr_),
      allocated_buffer_(std::move(other.allocated_buffer_)),
      file_descriptor_(other.file_descriptor_),
      on_chip_cached_(other.on_chip_cached_) {
  // Explicitly clear out other.
  other.type_ = Type::kInvalid;
  other.ptr_ = 0;
  other.size_bytes_ = 0;
  other.file_descriptor_ = -1;
  other.on_chip_cached_ = false;
  // other.allocated_buffer handled in move above.
}

Buffer& Buffer::operator=(Buffer&& other) {
  if (this != &other) {
    type_ = other.type_;
    size_bytes_ = other.size_bytes_;
    ptr_ = other.ptr_;
    allocated_buffer_ = std::move(other.allocated_buffer_);
    on_chip_cached_ = other.on_chip_cached_;

    // Explicitly clear out other.
    other.type_ = Type::kInvalid;
    other.ptr_ = 0;
    other.size_bytes_ = 0;
    other.on_chip_cached_ = false;
    // other.allocated_buffer handled in move above.
  }
  return *this;
}

const unsigned char* Buffer::ptr() const {
  // FD-type Buffers need to be mapped before use.
  CHECK_NE(type_, Type::kFileDescriptor);
  return ptr_;
}

unsigned char* Buffer::ptr() {
  // FD-type Buffers need to be mapped before use.
  CHECK_NE(type_, Type::kFileDescriptor);
  return ptr_;
}

int Buffer::fd() const {
  // Only valid with Type == kFileDescriptor.
  CHECK_EQ(type_, Type::kFileDescriptor);
  return file_descriptor_;
}

std::ostream& operator<<(std::ostream& stream, const Buffer::Type& type) {
  switch (type) {
    case Buffer::Type::kInvalid:
      return (stream << "kInvalid");
    case Buffer::Type::kWrapped:
      return (stream << "kWrapped");
    case Buffer::Type::kAllocated:
      return (stream << "kAllocated");
    case Buffer::Type::kFileDescriptor:
      return (stream << "kFileDescriptor");
    default:
      return (stream << "Unknown Buffer::Type: " << static_cast<int>(type));
  }
}
}  // namespace darwinn
}  // namespace platforms
