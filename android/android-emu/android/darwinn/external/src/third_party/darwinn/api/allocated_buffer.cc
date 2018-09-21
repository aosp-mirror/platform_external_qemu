#include "third_party/darwinn/api/allocated_buffer.h"

#include "third_party/darwinn/port/logging.h"

namespace platforms {
namespace darwinn {

AllocatedBuffer::AllocatedBuffer(unsigned char* ptr, size_t size_bytes,
                                 FreeCallback free_callback)
    : ptr_(ptr),
      size_bytes_(size_bytes),
      free_callback_(std::move(free_callback)) {
  CHECK(ptr != nullptr);
}

AllocatedBuffer::~AllocatedBuffer() { free_callback_(ptr_); }

}  // namespace darwinn
}  // namespace platforms
