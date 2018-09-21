#include "third_party/darwinn/driver/interrupt/wire_interrupt_handler.h"

#include <condition_variable>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <string>

#include "third_party/darwinn/driver/interrupt/interrupt_handler.h"
#include "third_party/darwinn/driver/registers/mock_registers.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::Eq;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

const config::WireCsrOffsets kWireCsrOffsets = {
    0x1000,  // wire_int_pending_bit_array
};

// Convenience Notification class.
class Notification {
 public:
  void Wait() LOCKS_EXCLUDED(mutex_) {
    StdCondMutexLock lock(&mutex_);
    while (!completed_) {
      cv_.wait(lock);
    }
  }

  void Notify() LOCKS_EXCLUDED(mutex_) {
    StdMutexLock lock(&mutex_);
    completed_ = true;
    cv_.notify_all();
  }

 private:
  std::condition_variable cv_;
  std::mutex mutex_;
  bool completed_ GUARDED_BY(mutex_){false};
};

// Base Test
class WireInterruptHandlerTest : public ::testing::Test {
 public:
  WireInterruptHandlerTest() {
    registers_ = gtl::MakeUnique<NiceMock<MockRegisters>>();
    wire_interrupt_handler_ = gtl::MakeUnique<PollingWireInterruptHandler>(
        registers_.get(), kWireCsrOffsets, [this]() { Sleep(); });
  }

  ~WireInterruptHandlerTest() override = default;

  void SetUp() override {
    // Mock defaults.
    ON_CALL(*this, Sleep()).WillByDefault(Invoke([]() {}));
    ON_CALL(*registers_, Read(_)).WillByDefault(Return(0));
  }

  MOCK_METHOD0(Sleep, void());
  MOCK_METHOD1(InterruptHandler, void(int));

  std::unique_ptr<MockRegisters> registers_;
  std::unique_ptr<WireInterruptHandler> wire_interrupt_handler_;
};

TEST_F(WireInterruptHandlerTest, TestOpenClose) {
  ASSERT_OK(wire_interrupt_handler_->Open());
  EXPECT_OK(wire_interrupt_handler_->Close());
}

TEST_F(WireInterruptHandlerTest, TestOpenCloseTwice) {
  ASSERT_OK(wire_interrupt_handler_->Open());
  ASSERT_OK(wire_interrupt_handler_->Close());

  ASSERT_OK(wire_interrupt_handler_->Open());
  EXPECT_OK(wire_interrupt_handler_->Close());
}

TEST_F(WireInterruptHandlerTest, TestAllInterrupt) {
  Notification notification;

  EXPECT_CALL(*this, Sleep()).WillOnce(Invoke([&notification]() {
    notification.Wait();
  }));

  ASSERT_OK(wire_interrupt_handler_->Open());

  // Raise all interrupts.
  EXPECT_CALL(*registers_, Read(kWireCsrOffsets.wire_int_pending_bit_array))
      .WillOnce(Return(0x10f1))
      .WillOnce(Return(0));

  EXPECT_CALL(*registers_, Read(kWireCsrOffsets.wire_int_mask_array))
      .WillOnce(Return(0));

  // Register all interrupts.
  ASSERT_OK(wire_interrupt_handler_->Register(
      DW_INTERRUPT_INSTR_QUEUE,
      std::bind(&WireInterruptHandlerTest::InterruptHandler, this,
                DW_INTERRUPT_INSTR_QUEUE)));
  ASSERT_OK(wire_interrupt_handler_->Register(
      DW_INTERRUPT_SC_HOST_0,
      std::bind(&WireInterruptHandlerTest::InterruptHandler, this,
                DW_INTERRUPT_SC_HOST_0)));
  ASSERT_OK(wire_interrupt_handler_->Register(
      DW_INTERRUPT_SC_HOST_1,
      std::bind(&WireInterruptHandlerTest::InterruptHandler, this,
                DW_INTERRUPT_SC_HOST_1)));
  ASSERT_OK(wire_interrupt_handler_->Register(
      DW_INTERRUPT_SC_HOST_2,
      std::bind(&WireInterruptHandlerTest::InterruptHandler, this,
                DW_INTERRUPT_SC_HOST_2)));
  ASSERT_OK(wire_interrupt_handler_->Register(
      DW_INTERRUPT_SC_HOST_3,
      std::bind(&WireInterruptHandlerTest::InterruptHandler, this,
                DW_INTERRUPT_SC_HOST_3)));
  ASSERT_OK(wire_interrupt_handler_->Register(
      DW_INTERRUPT_FATAL_ERR,
      std::bind(&WireInterruptHandlerTest::InterruptHandler, this,
                DW_INTERRUPT_FATAL_ERR)));

  // All interrupt handlers should be invoked exactly once.
  EXPECT_CALL(*this, InterruptHandler(DW_INTERRUPT_SC_HOST_0)).Times(1);
  EXPECT_CALL(*this, InterruptHandler(DW_INTERRUPT_SC_HOST_1)).Times(1);
  EXPECT_CALL(*this, InterruptHandler(DW_INTERRUPT_SC_HOST_2)).Times(1);
  EXPECT_CALL(*this, InterruptHandler(DW_INTERRUPT_SC_HOST_3)).Times(1);
  EXPECT_CALL(*this, InterruptHandler(DW_INTERRUPT_INSTR_QUEUE)).Times(1);
  EXPECT_CALL(*this, InterruptHandler(DW_INTERRUPT_FATAL_ERR)).Times(1);

  notification.Notify();

  ASSERT_OK(wire_interrupt_handler_->Close());
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
