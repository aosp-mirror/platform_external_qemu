#include "third_party/darwinn/driver/noronha/noronha_mmu_mapper.h"

#include <inttypes.h>  // for PRI*64
#include <unistd.h>

#include "third_party/darwinn/driver/hardware_structures.h"
#include "third_party/darwinn/port/cleanup.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

NoronhaMmuMapper::AbdramBuffer::AbdramBuffer(AbdramBuffer &&other) {
  mmu_mapper_ = other.mmu_mapper_;
  fd_ = other.fd_;
  mapped_ = other.mapped_;
  other.mmu_mapper_ = nullptr;
  other.fd_ = -1;
  other.mapped_ = false;
}

NoronhaMmuMapper::AbdramBuffer::~AbdramBuffer() {
  if (fd_ > 0) {
    if (mapped_) {
      mapped_ = false;
      if (Ioctl(OSCAR_IOCTL_ABC_UNMAP_BUFFER, fd_) < 0) {
        VLOG(4) << StringPrintf(
            "ioctl(OSCAR_IOCTL_ABC_UNMAP_BUFFER): could not unmap pages : %d "
            "(%s)",
            fd_, strerror(errno));
      }
    }
    if (Ioctl(OSCAR_IOCTL_ABC_DEALLOC_BUFFER, fd_) < 0) {
      VLOG(4) << StringPrintf(
          "ioctl(OSCAR_IOCTL_ABC_DEALLOC_BUFFER): could not deallocate AB DRAM "
          ": %d (%s)",
          fd_, strerror(errno));
    }
    close(fd_);
  }
  mmu_mapper_ = nullptr;
  fd_ = -1;
}

int NoronhaMmuMapper::AbdramBuffer::Map(uint64_t device_virtual_address) {
  // Performs AB DRAM FD mapping.
  oscar_abdram_map_ioctl buffer_to_map;
  buffer_to_map.page_table_index = 0;
  buffer_to_map.fd = fd_;
  buffer_to_map.device_address = device_virtual_address;
  if (!mapped_) {
    mapped_ = true;
    VLOG(4) << StringPrintf("%d FD -> %016" PRIx64, buffer_to_map.fd,
                            buffer_to_map.device_address);
    return Ioctl(OSCAR_IOCTL_ABC_MAP_BUFFER, &buffer_to_map);
  } else {
    VLOG(4) << StringPrintf(
        "Attempt to map a mapped %d file descriptor to %016" PRIx64, fd_,
        device_virtual_address);
    return -1;
  }
}

int NoronhaMmuMapper::AbdramBuffer::Dma(uint64_t host_address, int num_pages) {
  oscar_abdram_sync_ioctl sync_arg;
  sync_arg.fd = fd_;
  sync_arg.cmd = OSCAR_SYNC_TO_BUFFER;
  sync_arg.host_address = host_address;
  sync_arg.len = num_pages * kHostPageSize;
  VLOG(4) << StringPrintf("%016" PRIx64 "- > FD %d", host_address, fd_);
  return Ioctl(OSCAR_IOCTL_ABC_SYNC_BUFFER, &sync_arg);
}

util::Status NoronhaMmuMapper::DoMap(const void *buffer, int num_pages,
                                     uint64 device_virtual_address,
                                     DmaDirection direction,
                                     bool on_chip_cached) {
  StdMutexLock lock(&mutex_);

  auto fallback = [this, buffer, num_pages, device_virtual_address, direction,
                   on_chip_cached]() {
    return KernelMmuMapper::DoMap(buffer, num_pages, device_virtual_address,
                                  direction, on_chip_cached);
  };

  if (!on_chip_cached) return fallback();

  // TODO(ijsung): support other directions if necessary.
  if (direction != DmaDirection::kToDevice) return fallback();

  // Should there be any existing cached buffer mapping, unmap/deallocate/close
  // it.
  if (cached_buffers_.erase(buffer) > 0) {
    VLOG(4) << StringPrintf(
        "Trying to map a host VA %p that has already been mapped.", buffer);
  }

  struct oscar_abdram_alloc_ioctl alloc_ioctl_arg = {
      /*flags=*/0,
      /*size=*/static_cast<uint64_t>(num_pages * kHostPageSize),
  };

  const int dma_buf_fd =
      KernelMmuMapper::DoIoctl(OSCAR_IOCTL_ABC_ALLOC_BUFFER, &alloc_ioctl_arg);
  // It is not an error if we can't allocate AB DRAM; just fall back to the
  // KernelMmuMapper.
  if (dma_buf_fd < 0) {
    VLOG(4) << StringPrintf(
        "Unable to allocate AB DRAM for (%d pages); falling back to "
        "KernelMmuMapper#DoMap()",
        num_pages);
    return fallback();
  }

  AbdramBuffer abdram_buffer(this, dma_buf_fd);

  // Performs DMA from buffer to FD.
  // TODO(ijsung): Check DMA direction is to-device / buffer provides read data
  // to TPU.
  if (abdram_buffer.Dma(
          static_cast<uint64_t>(reinterpret_cast<uintptr_t>(buffer)),
          num_pages) < 0) {
    VLOG(4) << StringPrintf(
        "ioctl(OSCAR_IOCTL_ABC_SYNC_BUFFER): could not perform DMA to FD "
        "%d: %s. Falling back to normal mapper.",
        dma_buf_fd, strerror(errno));
    // Fallback to normal mapper. Note this destructs abdram_buffer and thus
    // deallocates the buffer and calls close(dma_buf_fd).
    return fallback();
  }

  // Performs AB DRAM FD mapping. Falls back if not successful.
  if (abdram_buffer.Map(device_virtual_address) < 0) {
    VLOG(4) << StringPrintf(
        "ioctl(OSCAR_IOCTL_ABC_MAP_BUFFER): could not map pages : %d (%s)",
        dma_buf_fd, strerror(errno));
    return fallback();
  }

  // At this point, the buffer should be ready. Move the wrapped buffer into the
  // cached_buffers so it won't be destroyed as we return.
  cached_buffers_.emplace(buffer, std::move(abdram_buffer));
  return util::Status();  // OK
}

util::Status NoronhaMmuMapper::DoUnmap(const void *buffer, int num_pages,
                                       uint64 device_virtual_address) {
  StdMutexLock lock(&mutex_);
  // TODO(ijsung): DMA output data from ab-dram to host buffer if DMA direction
  // indicates is needed.
  if (cached_buffers_.erase(buffer) > 0) {
    return util::Status();  // OK
  } else {
    return KernelMmuMapper::DoUnmap(buffer, num_pages, device_virtual_address);
  }
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
