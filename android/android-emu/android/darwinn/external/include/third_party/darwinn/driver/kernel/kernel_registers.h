#ifndef THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_REGISTERS_H_
#define THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_REGISTERS_H_

#include <mutex>  // NOLINT
#include <string>
#include <vector>

#include "third_party/darwinn/driver/registers/registers.h"
#include "third_party/darwinn/port/integral_types.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/thread_annotations.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Kernel implementation of the register interface.
class KernelRegisters : public Registers {
 public:
  struct MmapRegion {
    uint64 offset;
    uint64 size;
  };

  KernelRegisters(const std::string& device_path,
                  const std::vector<MmapRegion>& mmap_region, bool read_only);

  KernelRegisters(const std::string& device_path, uint64 mmap_offset,
                  uint64 mmap_size, bool read_only);

  ~KernelRegisters() override;

  // Overrides from registers.h
  util::Status Open() LOCKS_EXCLUDED(mutex_) override;
  util::Status Close() LOCKS_EXCLUDED(mutex_) override;
  util::Status Write(uint64 offset, uint64 value)
      LOCKS_EXCLUDED(mutex_) override;
  util::StatusOr<uint64> Read(uint64 offset) LOCKS_EXCLUDED(mutex_) override;
  util::Status Write32(uint64 offset, uint32 value)
      LOCKS_EXCLUDED(mutex_) override;
  util::StatusOr<uint32> Read32(uint64 offset) LOCKS_EXCLUDED(mutex_) override;

 private:
  struct MappedRegisterRegion {
    uint64 offset;
    uint64 size;
    uint64* registers;
  };

  // Maps CSR offset to virtual address.
  util::StatusOr<uint64*> GetMappedOffset(uint64 offset, int alignment) const
      EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Device path.
  const std::string device_path_;

  // mmap() region.
  std::vector<MappedRegisterRegion> mmap_region_ GUARDED_BY(mutex_);

  // true, if read only. false otherwise.
  const bool read_only_;

  // File descriptor of the opened device.
  int fd_ GUARDED_BY(mutex_){-1};

  // Mutex that guards fd_;
  mutable std::mutex mutex_;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_KERNEL_KERNEL_REGISTERS_H_
