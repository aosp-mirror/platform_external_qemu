#ifndef THIRD_PARTY_DARWINN_DRIVER_NORONHA_NORONHA_MMU_MAPPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_NORONHA_NORONHA_MMU_MAPPER_H_

#include <unordered_map>

#include "third_party/darwinn/driver/kernel/kernel_mmu_mapper.h"
#include "third_party/darwinn/driver/noronha/oscar_ioctl.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Noronha implementation of the MMU mapper interface.
class NoronhaMmuMapper : public KernelMmuMapper {
 public:
  explicit NoronhaMmuMapper(const std::string &device_path)
      : KernelMmuMapper(device_path) {}
  ~NoronhaMmuMapper() override = default;

 protected:
  util::Status DoMap(const void *buffer, int num_pages,
                     uint64 device_virtual_address, DmaDirection direction,
                     bool on_chip_cached) override;
  util::Status DoUnmap(const void *buffer, int num_pages,
                       uint64 device_virtual_address) override;

 private:
  // Helper class that wraps ABDRAM buffer file descriptor.
  class AbdramBuffer {
   public:
    AbdramBuffer(NoronhaMmuMapper *mapper, int fd)
        : mmu_mapper_(mapper), fd_(fd) {
      CHECK_NE(mapper, nullptr);
    }
    AbdramBuffer() = default;

    // Movable, not copyable, as the a buffer FD should only be closed exactly
    // once. Not assignable only because we don't have that use-case for now.
    AbdramBuffer &operator=(const AbdramBuffer &) = delete;
    AbdramBuffer &operator=(AbdramBuffer &&other) = delete;
    AbdramBuffer(const AbdramBuffer &) = delete;
    AbdramBuffer(AbdramBuffer &&other);

    // Unmaps the wrapped buffer if mapped. Also deallocates and closes the
    // wrapped file descriptor per requirement from the kernel driver.
    ~AbdramBuffer();

    // Calls ioctl to map the wrapped AB DRAM buffer. Updates mapped_.
    // Returns the ioctl return value for OSCAR_IOCTL_ABC_MAP_BUFFER.
    int Map(uint64_t device_virtual_address);

    // Calls ioctl to DMA to the wrapped AB DRAM buffer.
    // Returns the ioctl return value for OSCAR_IOCTL_ABC_SYNC_BUFFER.
    int Dma(uint64_t host_address, int num_pages);

   private:
    // Calls ioctl on the device file descriptor owned by the MMU mapper.
    template <typename... Params>
    int Ioctl(Params &&... params) {
      return mmu_mapper_->DoIoctl(std::forward<Params>(params)...);
    }
    // Calls OSCAR_IOCTL_ABC_*_BUFFER with the wrapped file descriptor.
    // Returns the return value of the ioctl call.
    int Ioctl(int operation);

    NoronhaMmuMapper *mmu_mapper_ = nullptr;
    int fd_ = -1;
    bool mapped_ = false;
  };

  // AB DRAM file descriptors of the cached buffers.
  std::unordered_map<const void *, AbdramBuffer> cached_buffers_
      GUARDED_BY(mutex_);

  // Mutex that guards cached_buffers_;
  mutable std::mutex mutex_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_NORONHA_NORONHA_MMU_MAPPER_H_
