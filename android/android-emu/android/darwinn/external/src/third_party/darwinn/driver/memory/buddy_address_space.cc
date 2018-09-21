#include "third_party/darwinn/driver/memory/buddy_address_space.h"

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/driver/memory/address_utilities.h"
#include "third_party/darwinn/driver/memory/mmio_address_space.h"
#include "third_party/darwinn/driver/memory/mmu_mapper.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/math_util.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"
#include "third_party/darwinn/port/logging.h"


namespace platforms {
namespace darwinn {
namespace driver {

namespace {

// Helper method to get number of pages required for given size. Because actual
// number of pages may differ depending on the starting host address, the
// returned number of pages is conservative.
int GetConservativeNumberPages(size_t size_bytes) {
  return MathUtil::CeilOfRatio<size_t>(size_bytes, kHostPageSize) + 1;
}

// Based on:
// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
uint64 RoundUpToNextPowerOfTwo(uint64 x) {
  x--;
  x |= x >> 1;   // handle  2 bit numbers
  x |= x >> 2;   // handle  4 bit numbers
  x |= x >> 4;   // handle  8 bit numbers
  x |= x >> 8;   // handle 16 bit numbers
  x |= x >> 16;  // handle 32 bit numbers
  x |= x >> 32;  // handle 64 bit numbers
  x++;

  return x;
}

// Returns the bin index given an order. The unit of allocation is a host page,
// so the smallest bin (bin 0) is for anything that is <= host page size.
int GetBinFromOrder(int order) {
  CHECK_GE(order, kHostPageShiftBits);
  return order - kHostPageShiftBits;
}

// Returns the order for a given bin. For example, bin 2 is of 4 times the host
// page size. On x86 it is 2^(12+2).
int GetOrderFromBin(int bin) {
  return bin + kHostPageShiftBits;
}

// For a given allocation request size, returns the index to the bin (i.e. for
// indexing block_ ) that size belongs to. This based on:
// https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightModLookup
// Rationale:
// pow(2, i) for 0 <= i < 32 have distinct modulo by 37. We use that property to
// perform fast lookup.
int FindBin(uint64 size) {
  const uint64 nearest_power_of_two = RoundUpToNextPowerOfTwo(size);
  // The trick below only works up to 2^31.
  CHECK_LE(nearest_power_of_two, 1ULL << 31);
  static constexpr int
      kMod37BitPosition[] =  // map a bit value mod 37 to its position
      {32, 0,  1,  26, 2,  23, 27, 0,  3, 16, 24, 30, 28, 11, 0,  13, 4,  7, 17,
       0,  25, 22, 31, 15, 29, 10, 12, 6, 0,  21, 14, 9,  5,  20, 8,  19, 18};
  const int num_zero = kMod37BitPosition[nearest_power_of_two % 37];
  return std::max(GetBinFromOrder(num_zero), 0);
}


}  // namespace

BuddyAddressSpace::BuddyAddressSpace(uint64 device_virtual_address_start,
                                     uint64 device_virtual_address_size_bytes,
                                     MmuMapper* mmu_mapper)
    : MmioAddressSpace(device_virtual_address_start,
                       device_virtual_address_size_bytes, mmu_mapper),
      // TODO(ijsung): allocate just enough higher order bins.
      blocks_(GetBinFromOrder(64)),
      allocated_(blocks_) {
  uint64 offset = 0;
  // Initialize all bins. In the worst case we'd miss up to kHostPageSize - 1
  // bytes.
  for (int i = 63; i >= kHostPageShiftBits; --i) {
    const uint64 mask = (1ULL << i);
    if (device_virtual_address_size_bytes & mask) {
      blocks_[GetBinFromOrder(i)].insert(offset);
      offset += mask;
    }
  }
}

util::StatusOr<DeviceBuffer> BuddyAddressSpace::MapMemory(
    const Buffer& buffer, DmaDirection direction,
    MappingTypeHint mapping_type) {
  const void* ptr = buffer.IsPtrType() ? buffer.ptr() : nullptr;
  if (!ptr && buffer.IsPtrType()) {
    return util::InvalidArgumentError(
        "Cannot map an invalid host-memory-backed Buffer.");
  }

  const size_t size_bytes = buffer.size_bytes();
  if (size_bytes == 0) {
    return util::InvalidArgumentError("Cannot map 0 bytes.");
  }

  auto num_requested_pages = GetNumberPages(ptr, size_bytes);

  const int desirable_bin = FindBin(num_requested_pages * kHostPageSize);

  StdMutexLock lock(&mutex_);
  int nearest_bin = desirable_bin;

  // Find the nearest bin that has at least something left.
  while (nearest_bin < blocks_.size() && blocks_[nearest_bin].empty()) {
    ++nearest_bin;
  }

  if (nearest_bin >= blocks_.size()) {
    return util::ResourceExhaustedError(
        StringPrintf("Can't allocate for %zu bytes.", size_bytes));
  }

  // Try mapping first before we breaking up higher order blocks.
  auto the_block = blocks_[nearest_bin].cbegin();
  const uint64 device_offset = *the_block;
  const uint64 device_va = device_offset + device_virtual_address_start();

  RETURN_IF_ERROR(Map(buffer, device_va, direction));

  blocks_[nearest_bin].erase(the_block);
  allocated_[desirable_bin].insert(device_offset);
  // If nearest bin != desirable bin, insert blocks produced by splitting higher
  // order ones
  for (int i = nearest_bin - 1; i >= desirable_bin; --i) {
    uint64 offset = device_offset + (1ULL << GetOrderFromBin(i));
    blocks_[i].insert(offset);
  }
  return DeviceBuffer(device_va + GetPageOffset(ptr), size_bytes);
}

util::Status BuddyAddressSpace::UnmapMemory(DeviceBuffer buffer) {
  StdMutexLock lock(&mutex_);
  const uint64 device_address = buffer.device_address();
  const size_t size_bytes = buffer.size_bytes();

  auto num_pages = GetNumberPages(device_address, size_bytes);
  int bin = FindBin(num_pages * kHostPageSize);
  const uint64 device_aligned_va = GetPageAddress(device_address);
  uint64 device_offset = device_aligned_va - device_virtual_address_start();

  auto allocated_iterator = allocated_[bin].find(device_offset);
  if (allocated_iterator == allocated_[bin].end()) {
    return util::InvalidArgumentError(
        StringPrintf("Never allocated device address 0x%016llx.",
                     static_cast<unsigned long long>(  // NOLINT(runtime/int)
                         device_address)));
  }

  RETURN_IF_ERROR(Unmap(device_aligned_va, num_pages));

  allocated_[bin].erase(allocated_iterator);

  for (; bin < blocks_.size(); ++bin) {
    // Find nearby block ("buddy") if any.
    const uint64 buddy_offset =
        device_offset ^ (1ULL << GetOrderFromBin(bin));

    auto buddy_iterator = blocks_[bin].find(buddy_offset);
    if (buddy_iterator != blocks_[bin].end()) {
      // Merging with the buddy at buddy_offset.
      blocks_[bin].erase(buddy_iterator);
      device_offset &= buddy_offset;
    } else {
      // We are done - can't coalesce more. Insert the block to the current bin.
      blocks_[bin].insert(device_offset);
      break;
    }
  }

  return util::Status();  // OK.
}

size_t BuddyAddressSpace::NumFreeBlocksByOrder(int order) const {
  StdMutexLock lock(&mutex_);
  const int bin = GetBinFromOrder(order);
  CHECK_LT(bin, blocks_.size());
  return blocks_[bin].size();
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
