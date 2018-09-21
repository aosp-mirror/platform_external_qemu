#ifndef THIRD_PARTY_DARWINN_API_ALLOCATED_BUFFER_H_
#define THIRD_PARTY_DARWINN_API_ALLOCATED_BUFFER_H_

#include <functional>

namespace platforms {
namespace darwinn {

// A type for buffer that holds (owns) allocated host memory. This class takes
// ownership of the buffer pointers passed into it, freeing them using the given
// Free function when destroyed.
class AllocatedBuffer {
 public:
  // A type for the callback executed to free the buffer.
  using FreeCallback = std::function<void(void*)>;

  AllocatedBuffer(unsigned char* ptr, size_t size_bytes,
                  FreeCallback free_callback);

  ~AllocatedBuffer();

  // Not copyable or movable
  AllocatedBuffer(const AllocatedBuffer&) = delete;
  AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;

  // Returns const buffer pointer.
  const unsigned char* ptr() const { return ptr_; }

  // Returns buffer pointer.
  unsigned char* ptr() { return ptr_; }

  // Size of this buffer in bytes.
  size_t size_bytes() const { return size_bytes_; }

 private:
  // Points to allocated buffer.
  unsigned char* ptr_;

  // Size of the buffer.
  size_t size_bytes_;

  // Callback executed to free the buffer.
  FreeCallback free_callback_;
};

}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_API_ALLOCATED_BUFFER_H_
