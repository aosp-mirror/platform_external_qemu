#include "third_party/darwinn/driver/memory/mmio_address_space.h"

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

util::StatusOr<const Buffer> MmioAddressSpace::Translate(
    const DeviceBuffer& buffer) const {
  StdMutexLock lock(&mutex_);
  ASSIGN_OR_RETURN(auto* host_address, mmu_mapper_->TranslateDeviceAddress(
                                           buffer.device_address()));
  return Buffer(host_address, buffer.size_bytes());
}

util::Status MmioAddressSpace::Map(const Buffer& buffer, uint64 device_address,
                                   DmaDirection direction) {
  CHECK(IsPageAligned(device_address));

  StdMutexLock lock(&mutex_);

  // If already mapped, fail.
  // TODO(jasonjk): Add a finer grained check, e.g., overlap, if necessary?
  if (mapped_.find(device_address) != mapped_.end()) {
    return util::InvalidArgumentError(
        "Trying to map a segment that is already mapped.");
  }

  RETURN_IF_ERROR(mmu_mapper_->Map(buffer, device_address, direction));

  // Track mapped segments.
  // Make a copy of Buffer since the given buffer may change later.
  auto insert_result = mapped_.insert(
      {device_address, buffer});
  CHECK(insert_result.second);

  // buffer.ptr() may or may not be vaild.
  // TODO(ijsung): print out buffer address if the buffer has valid ptr().
  VLOG(4) << StringPrintf(
      "MapMemory() page-aligned : device_address = 0x%016llx",
      static_cast<unsigned long long>(device_address));  // NOLINT(runtime/int)

  return util::Status();  // OK
}

util::Status MmioAddressSpace::Unmap(uint64 device_address,
                                     int num_released_pages) {
  // TODO(ijsung): verify num_released_pages if the Buffer is backed by host
  // memory.
  CHECK(IsPageAligned(device_address));

  StdMutexLock lock(&mutex_);

  auto find_result = mapped_.find(device_address);
  if (find_result == mapped_.end()) {
    return util::InvalidArgumentError(
        "Trying to ummap a segment that is not already mapped.");
  }

  // Need to pass the Buffer object as the MMU mapper might require the backing
  // file descriptor underneath.
  CHECK_OK(mmu_mapper_->Unmap(find_result->second, device_address));

  // buffer.ptr() may or may not be vaild.
  // TODO(ijsung): print out buffer address if the buffer has valid ptr().
  VLOG(4) << StringPrintf(
      "UnmapMemory() page-aligned : device_address = 0x%016llx, num_pages = %d",
      static_cast<unsigned long long>(device_address),  // NOLINT(runtime/int)
      num_released_pages);

  mapped_.erase(find_result);

  return util::Status();  // OK
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
