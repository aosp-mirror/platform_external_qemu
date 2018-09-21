#include "third_party/darwinn/driver/memory/mmu_mapper.h"

#include "third_party/darwinn/driver/memory/address_utilities.h"
namespace platforms {
namespace darwinn {
namespace driver {

util::Status MmuMapper::Map(const Buffer &buffer, uint64 device_virtual_address,
                            DmaDirection direction) {
  // Buffers backed by file descriptors do not have valid ptr().
  const void *ptr = buffer.IsFileDescriptorType() ? nullptr : buffer.ptr();
  if (buffer.IsPtrType() && ptr == nullptr) {
    return util::InvalidArgumentError("Cannot map a Buffer of nullptr.");
  }

  const size_t size_bytes = buffer.size_bytes();
  if (size_bytes == 0) {
    return util::InvalidArgumentError("Cannot map 0 bytes.");
  }

  auto num_requested_pages = GetNumberPages(ptr, size_bytes);

  // Buffers backed by file descriptors are handled differently.
  if (buffer.IsFileDescriptorType()) {
    return DoMap(buffer.fd(), num_requested_pages, device_virtual_address,
                 direction);
  } else {
    auto aligned_buffer_addr = GetPageAddressForBuffer(ptr);
    return DoMap(aligned_buffer_addr, num_requested_pages,
                 device_virtual_address, direction, buffer.IsOnChipCached());
  }
}

util::Status MmuMapper::Unmap(const Buffer &buffer,
                              uint64 device_virtual_address) {
  // Buffers backed by file descriptors do not have valid ptr().
  const void *ptr = buffer.IsFileDescriptorType() ? nullptr : buffer.ptr();
  if (buffer.IsPtrType() && ptr == nullptr) {
    return util::InvalidArgumentError("Cannot unmap a Buffer of nullptr.");
  }

  const size_t size_bytes = buffer.size_bytes();
  if (size_bytes == 0) {
    return util::InvalidArgumentError("Cannot unmap 0 bytes.");
  }

  auto num_mapped_pages = GetNumberPages(ptr, size_bytes);

  // Buffers backed by file descriptors are handled differently.
  if (buffer.IsFileDescriptorType()) {
    return DoUnmap(buffer.fd(), num_mapped_pages, device_virtual_address);
  } else {
    auto aligned_buffer_addr = GetPageAddressForBuffer(ptr);
    return DoUnmap(aligned_buffer_addr, num_mapped_pages,
                   device_virtual_address);
  }
}


}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
