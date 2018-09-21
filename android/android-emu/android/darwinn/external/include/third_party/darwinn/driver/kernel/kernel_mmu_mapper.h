#ifndef THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_MMU_MAPPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_MMU_MAPPER_H_

#include <sys/ioctl.h>
#include <mutex>  // NOLINT

#include "third_party/darwinn/driver/memory/dma_direction.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Kernel implementation of the MMU mapper interface.
class KernelMmuMapper : public MmuMapper {
 public:
  explicit KernelMmuMapper(const std::string &device_path);
  ~KernelMmuMapper() override = default;

  // Overrides from mmu_mapper.h
  util::Status Open(int num_simple_page_table_entries_requested) override;
  util::Status Close() override;

 protected:
  util::Status DoMap(const void *buffer, int num_pages,
                     uint64 device_virtual_address, DmaDirection direction,
                     bool on_chip_cached) override;
  util::Status DoUnmap(const void *buffer, int num_pages,
                       uint64 device_virtual_address) override;

  // Calls ioctl on the device file descriptor owned by this instance.
  // Forwards the parameters to the ioctl too; returns -1 on closed device and
  // the return value of ioctl otherwise.
  template <typename... Params>
  int DoIoctl(Params &&... params) {
    // TODO (ijsung): At this moment this mutex is there to guard uses of fd,
    // but if later we find there's a lot of concurrent threaded map/unmap
    // activity we could consider trying to allow them to run in parallel. If
    // this macro is to be called on ioctls that aren't necessarily
    // mutually-exclusive / can run in parallel (at the runtime level)
    // then may want to make appropriate locking the caller's responsibility.

    StdMutexLock lock(&mutex_);
    if (fd_ != -1) {
      return ioctl(fd_, std::forward<Params>(params)...);
    } else {
      VLOG(4) << "Invalid file descriptor.";
      return -1;
    }
  }

 private:
  // Device path.
  const std::string device_path_;

  // File descriptor of the opened device.
  int fd_ GUARDED_BY(mutex_){-1};

  // Mutex that guards fd_;
  mutable std::mutex mutex_;

  // Indicates whether the kernel driver supports GASKET_IOCTL_MAP_BUFFER_FLAGS.
  bool map_flags_supported_ GUARDED_BY(mutex_){true};
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_MMU_MAPPER_H_
