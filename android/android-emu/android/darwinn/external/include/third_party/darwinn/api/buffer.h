#ifndef THIRD_PARTY_DARWINN_API_BUFFER_H_
#define THIRD_PARTY_DARWINN_API_BUFFER_H_

#include <stddef.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "third_party/darwinn/api/allocated_buffer.h"

namespace platforms {
namespace darwinn {

// Abstracts a buffer. Movable and copyable.
// TODO(jnjoseph): Consider adding two different variants of this class
// for indicating Const and Mutable variants (like ArraySlice). For now,
// const Buffer, requires that contents of underlying buffer is const.
class Buffer {
 public:
  // Convenience structure for keeping track of named array of Buffers.
  using NamedMap = std::unordered_map<std::string, std::vector<Buffer>>;

  // Default constructor. Defaults to an invalid non-existent buffer.
  Buffer() = default;

  // Constructors for wrapping an existing host buffer.
  Buffer(void* buffer, size_t size_bytes);
  Buffer(const void* buffer, size_t size_bytes);

  // Constructors for wrapping an existing host buffer, and optionally hints
  // the runtime to cache it in on-chip memory outside the DarwiNN core.
  Buffer(unsigned char* buffer, size_t size_bytes, bool on_chip_cached = false);
  Buffer(const unsigned char* buffer, size_t size_bytes,
         bool on_chip_cached = false);

  // Constructors for wrapping an allocated buffer.
  Buffer(std::shared_ptr<AllocatedBuffer> allocated_buffer);

  // Constructor for wrapping an mmap-able file descriptor.
  Buffer(int fd, size_t size_bytes);

  // This type is copyable, with default implementations.
  Buffer(const Buffer&) = default;
  Buffer& operator=(const Buffer&) = default;

  // This type is movable.
  Buffer(Buffer&& other);
  Buffer& operator=(Buffer&& other);

  // Destructors.
  ~Buffer() = default;

  // Size of this buffer in bytes.
  size_t size_bytes() const { return size_bytes_; }

  // Returns true if buffer is valid.
  bool IsValid() const { return type_ != Type::kInvalid; }

  // Returns buffer pointer.
  const unsigned char* ptr() const;

  // Returns buffer pointer.
  unsigned char* ptr();

  // Returns true if the buffer is backed by some host memory, may or may not be
  // owned by this Buffer.
  bool IsPtrType() const {
    return type_ == Type::kWrapped || type_ == Type::kAllocated;
  }

  // Returns file descriptor.
  int fd() const;

  // Returns true if the buffer is backed by a file descriptor.
  bool IsFileDescriptorType() const { return type_ == Type::kFileDescriptor; }

  // Returns whether this Buffer may have copies in on-chip memory.
  bool IsOnChipCached() const { return on_chip_cached_; }

  // Equality operators.
  bool operator==(const Buffer& rhs) const;
  bool operator!=(const Buffer& rhs) const;

 private:
  // Type for the buffer.
  enum class Type {
    // Invalid.
    kInvalid = 0,

    // Wraps an existing host process addressable buffer.
    kWrapped = 1,

    // Wraps an allocated host process addressable buffer.
    kAllocated = 2,

    // Wraps an mmap-able file descriptor, possibly from ION.
    kFileDescriptor = 3,
  };

  // To allow Buffer::Type being tested in CHECK() et al.
  friend std::ostream& operator<< (std::ostream& stream, const Type& type);

  // Type for the buffer.
  Type type_{Type::kInvalid};

  // Size of the buffer.
  size_t size_bytes_{0};

  // Points to host buffer. Valid when type is kWrapped / kAllocated.
  unsigned char* ptr_{nullptr};

  // Points to allocated buffer. Valid when type is kAllocated.
  std::shared_ptr<AllocatedBuffer> allocated_buffer_;

  // File descriptor. Valid when type is kFileDescriptor.
  // Reset to -1 when moved.
  int file_descriptor_{-1};

  // Hints the kernel driver that this buffer should be mapped to on-chip memory
  // whenever possible.
  bool on_chip_cached_{false};
};

}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_BUFFER_H_
