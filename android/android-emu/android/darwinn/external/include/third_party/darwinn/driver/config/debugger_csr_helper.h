#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_DEBUGGER_CSR_HELPER_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_DEBUGGER_CSR_HELPER_H_

#include "third_party/darwinn/driver/bitfield.h"
#include "third_party/darwinn/port/integral_types.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {
namespace registers {

// Interface to access fields for TTU breakpoints.
class TtuBreakpointInterface {
 public:
  virtual ~TtuBreakpointInterface() = default;

  // Access to aggregated value.
  virtual void set_raw(uint64 value) = 0;
  virtual uint64 raw() const = 0;

  // Accesses to fields.
  virtual void set_enable(uint64 value) = 0;
  virtual uint64 enable() const = 0;
  virtual void set_loop_depth(uint64 value) = 0;
  virtual uint64 loop_depth() const = 0;
  virtual void set_ttu_reference(uint64 value) = 0;
  virtual uint64 ttu_reference() const = 0;
};

// Implements CSR helper with given LOOP_DEPTH_BITS.
template <int LOOP_DEPTH_BITS>
class TtuBreakpoint : public TtuBreakpointInterface {
 public:
  TtuBreakpoint() : TtuBreakpoint(0ULL) {}
  explicit TtuBreakpoint(uint64 value) { reg_.raw_ = value; }
  ~TtuBreakpoint() = default;

  void set_raw(uint64 value) override { reg_.raw_ = value; }
  uint64 raw() const override { return reg_.raw_; }
  void set_enable(uint64 value) override { reg_.enable_ = value; }
  uint64 enable() const override { return reg_.enable_(); }
  void set_loop_depth(uint64 value) override { reg_.loop_depth_ = value; }
  uint64 loop_depth() const override { return reg_.loop_depth_(); }
  void set_ttu_reference(uint64 value) override { reg_.ttu_reference_ = value; }
  uint64 ttu_reference() const override { return reg_.ttu_reference_(); }

 private:
  union {
    uint64 raw_;
    platforms::darwinn::driver::Bitfield<0, 1> enable_;
    platforms::darwinn::driver::Bitfield<1, LOOP_DEPTH_BITS> loop_depth_;
    platforms::darwinn::driver::Bitfield<1 + LOOP_DEPTH_BITS, 1> ttu_reference_;
  } reg_;
};

// Interface to access fields for memory access CSR.
class MemoryAccessInterface {
 public:
  virtual ~MemoryAccessInterface() = default;

  // Accesses to aggregated value.
  virtual void set_raw(uint64 value) = 0;
  virtual uint64 raw() const = 0;

  // Sets the memory access mode.
  virtual void set_read_mode() = 0;
  virtual void set_write_mode() = 0;
  virtual uint64 mode() const = 0;

  // Sets the trigger to perform memory access.
  virtual void set_trigger() = 0;

  // Writes the memory. If row contains more than 64 bit, use partial mode to
  // fill up the row, then use full mode to write the value. Otherwise, just use
  // full mode.
  virtual void set_write_partial() = 0;
  virtual void set_write_full() = 0;

  // For scalar core, 0: scalar memory, 1: instruction memory, 2: activation, 3:
  // parameter.
  // For tiles, 0: wide memory, 1: narrow memory, 2: bias memory, 3: scales
  // memory.
  virtual void select_memory(uint64 value) = 0;
  virtual void set_row_address(uint64 address) = 0;
  // If row contains more than 64 bit, use index to specify which 64 bit.
  virtual void set_chunk_index(uint64 index) = 0;

  // Returns 1 if read/write is complete.
  virtual uint64 done() const = 0;
};

// Implementation for CSR access to read only memory.
class ReadOnlyMemoryAccess : public MemoryAccessInterface {
 public:
  ReadOnlyMemoryAccess() : ReadOnlyMemoryAccess(0ULL) {}
  explicit ReadOnlyMemoryAccess(uint64 value) { reg_.raw_ = value; }
  ~ReadOnlyMemoryAccess() override = default;

  // Accesses to aggregated value.
  void set_raw(uint64 value) override { reg_.raw_ = value; }
  uint64 raw() const override { return reg_.raw_; }

  // Sets the memory access mode.
  void set_read_mode() override { reg_.mode_ = 1; }
  void set_write_mode() override {}
  uint64 mode() const override { return reg_.mode_(); }

  // Sets the trigger to perform memory access.
  void set_trigger() override { reg_.trigger_ = 1; }

  // Writes the memory. Does nothing.
  void set_write_partial() override {}
  void set_write_full() override {}

  // For scalar core, 0: scalar memory, 1: instruction memory, 2: activation, 3:
  // parameter.
  // For tiles, 0: wide memory, 1: narrow memory, 2: bias memory, 3: scales
  // memory.
  void select_memory(uint64 value) override { reg_.memory_select_ = value; }
  void set_row_address(uint64 address) override { reg_.address_ = address; }
  // If row contains more than 64 bit, use index to specify which 64 bit.
  void set_chunk_index(uint64 index) override {
    reg_.memory_data_index_ = index;
  }

  // Returns 1 if read/write is complete.
  uint64 done() const override { return reg_.done_(); }

 private:
  union {
    uint64 raw_;
    // 0: disable, 1: read mode, 2: write mode, 3: unused.
    platforms::darwinn::driver::Bitfield<0, 2> mode_;
    platforms::darwinn::driver::Bitfield<2, 1> trigger_;
    platforms::darwinn::driver::Bitfield<3, 1> done_;
    platforms::darwinn::driver::Bitfield<4, 2> memory_select_;
    platforms::darwinn::driver::Bitfield<16, 16> address_;
    platforms::darwinn::driver::Bitfield<32, 5> memory_data_index_;
  } reg_;
};

// Implementation for CSR access to read/write memory.
class MemoryAccess : public MemoryAccessInterface {
 public:
  MemoryAccess() : MemoryAccess(0ULL) {}
  explicit MemoryAccess(uint64 value) { reg_.raw_ = value; }
  ~MemoryAccess() override = default;

  // Accesses to aggregated value.
  void set_raw(uint64 value) override { reg_.raw_ = value; }
  uint64 raw() const override { return reg_.raw_; }

  // Sets the memory access mode.
  void set_read_mode() override { reg_.mode_ = 1; }
  void set_write_mode() override { reg_.mode_ = 2; }
  uint64 mode() const override { return reg_.mode_(); }

  // Sets the trigger to perform memory access.
  void set_trigger() override { reg_.trigger_ = 1; }

  // Writes the memory.
  void set_write_partial() override { reg_.write_partial_data_ = 1; }
  void set_write_full() override { reg_.write_partial_data_ = 0; }

  // For scalar core, 0: scalar memory, 1: instruction memory, 2: activation, 3:
  // parameter.
  // For tiles, 0: wide memory, 1: narrow memory, 2: bias memory, 3: scales
  // memory.
  void select_memory(uint64 value) override { reg_.memory_select_ = value; }
  void set_row_address(uint64 address) override { reg_.address_ = address; }
  // If row contains more than 64 bit, use index to specify which 64 bit.
  void set_chunk_index(uint64 index) override {
    reg_.memory_data_index_ = index;
  }

  // Returns 1 if read/write is complete.
  uint64 done() const override { return reg_.done_(); }

 private:
  union {
    uint64 raw_;
    // 0: disable, 1: read mode, 2: write mode, 3: unused.
    platforms::darwinn::driver::Bitfield<0, 2> mode_;
    platforms::darwinn::driver::Bitfield<2, 1> trigger_;
    platforms::darwinn::driver::Bitfield<3, 1> done_;
    platforms::darwinn::driver::Bitfield<4, 2> memory_select_;
    platforms::darwinn::driver::Bitfield<16, 16> address_;
    platforms::darwinn::driver::Bitfield<32, 3> memory_data_index_;
    platforms::darwinn::driver::Bitfield<35, 1> write_partial_data_;
  } reg_;
};

// Implements helper methods to extract trace buffer.
class TraceBuffer {
 public:
  TraceBuffer() : TraceBuffer(0ULL) {}
  explicit TraceBuffer(uint64 value) { reg_.raw_ = value; }
  ~TraceBuffer() = default;

  void set_raw(uint64 value) { reg_.raw_ = value; }
  uint64 raw() const { return reg_.raw_; }

  uint64 valid() const { return reg_.valid_(); }
  uint64 id() const { return reg_.id_(); }
  uint64 start_cycle() const { return reg_.start_cycle_(); }
  uint64 end_cycle() const { return reg_.end_cycle_(); }

 private:
  union {
    uint64 raw_;
    platforms::darwinn::driver::Bitfield<0, 1> valid_;
    platforms::darwinn::driver::Bitfield<1, 16> id_;
    platforms::darwinn::driver::Bitfield<17, 16> start_cycle_;
    platforms::darwinn::driver::Bitfield<33, 16> end_cycle_;
  } reg_;
};

}  // namespace registers
}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_DEBUGGER_CSR_HELPER_H_
