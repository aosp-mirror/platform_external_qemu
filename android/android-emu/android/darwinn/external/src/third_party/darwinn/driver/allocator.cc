#include "third_party/darwinn/driver/allocator.h"

#include <functional>
#include <memory>

#include "third_party/darwinn/api/allocated_buffer.h"
#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/ptr_util.h"

namespace platforms {
namespace darwinn {
namespace driver {

Buffer Allocator::MakeBuffer(size_t size_bytes) {
  auto free_cb = [this](void* ptr) { Free(ptr); };

  uint8* ptr = static_cast<uint8*>(Allocate(size_bytes));
  return Buffer(
      gtl::MakeUnique<AllocatedBuffer>(ptr, size_bytes, std::move(free_cb)));
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
