#include "third_party/darwinn/driver/single_queue_dma_scheduler.h"

#include <iterator>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/aligned_allocator.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/dma_info_extractor.h"
#include "third_party/darwinn/driver/memory/mock_address_space.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/driver/test_util.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

class MockDmaInfoExtractor : public DmaInfoExtractor {
 public:
  MockDmaInfoExtractor() : DmaInfoExtractor(ExtractorType::kDmaHints) {}
  ~MockDmaInfoExtractor() override = default;

  MOCK_CONST_METHOD2(
      ExtractDmaInfos,
      std::list<DmaInfo>(const platforms::darwinn::driver::ExecutableReference&,
                         const DeviceBufferMapper&));
};

class SingleEndpointDmaSchedulerTest : public ::testing::Test {
 protected:
  SingleEndpointDmaSchedulerTest() {
    constexpr int kAlignmentBytes = 0x4000;
    allocator_ = gtl::MakeUnique<AlignedAllocator>(kAlignmentBytes);
    ON_CALL(address_space_, MapMemory(_, _, _))
        .WillByDefault(Return(DeviceBuffer()));
    ON_CALL(address_space_, UnmapMemory(_))
        .WillByDefault(Return(util::Status()));
  }

  // Returns a dummy executable reference.
  const driver::ExecutableReference* GetExecutableReference() {
    // We need executable to have some instructions.
    const std::vector<int> instruction_bytes = {1200};
    testutil::ExecutableGenerator generator;
    generator.AddInstructionBitstreams(instruction_bytes);
    generator.Finish();

    const auto* api_executable_reference =
        executable_registry_.RegisterSerialized(generator.PackageString())
            .ValueOrDie();
    const auto* package_reference =
        static_cast<const driver::PackageReference*>(api_executable_reference);
    return package_reference->MainExecutableReference();
  }

  // Returns a dummy device buffer mapper.
  std::unique_ptr<DeviceBufferMapper> GetDeviceBufferMapper() {
    return gtl::MakeUnique<DeviceBufferMapper>(&address_space_);
  }

  // Creates and returns a request.
  std::shared_ptr<Request> MakeRequest(Request::Done done) {
    auto request = std::make_shared<Request>(
        id_++, GetExecutableReference(), allocator_.get(),
        GetDeviceBufferMapper(), &extractor_, /*alignment_bytes=*/1,
        std::move(done));
    CHECK_OK(request->Validate());
    CHECK_OK(request->Prepare());
    return request;
  }

  // Test helpers.
  // We need to create allocator earlier than executable registry so that it
  // will be destroyed later.  This is needed since the instruction buffers
  // inside executable reference depends on allocator.
  // In the real world case this won't be a problem since the allocator is
  // owned by the driver which won't be destroyed before executable reference.
  std::unique_ptr<AlignedAllocator> allocator_;
  PackageRegistry executable_registry_;
  MockAddressSpace address_space_;
  MockDmaInfoExtractor extractor_;

  int id_{0};
};

TEST_F(SingleEndpointDmaSchedulerTest, MultipleOpenClose) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());
  ASSERT_OK(scheduler.Close());
  ASSERT_OK(scheduler.Open());
  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, OpenFail) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());
  EXPECT_EQ(scheduler.Open().CanonicalCode(), util::error::FAILED_PRECONDITION);
}

TEST_F(SingleEndpointDmaSchedulerTest, CloseFail) {
  SingleQueueDmaScheduler scheduler;
  EXPECT_EQ(scheduler.Close().CanonicalCode(),
            util::error::FAILED_PRECONDITION);
}

TEST_F(SingleEndpointDmaSchedulerTest, WrongDmaComplete) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());
  DmaInfo dma(0, DmaDescriptorType::kInstruction, DeviceBuffer(0, 1234));
  EXPECT_EQ(scheduler.NotifyDmaCompletion(&dma).CanonicalCode(),
            util::error::FAILED_PRECONDITION);

  dma.MarkCompleted();
  EXPECT_EQ(scheduler.NotifyDmaCompletion(&dma).CanonicalCode(),
            util::error::FAILED_PRECONDITION);
}

TEST_F(SingleEndpointDmaSchedulerTest, WrongRequestComplete) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());
  EXPECT_EQ(scheduler.NotifyRequestCompletion().CanonicalCode(),
            util::error::FAILED_PRECONDITION);
}

TEST_F(SingleEndpointDmaSchedulerTest, RequestCompleteWithPendingDmasFail) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::list<DmaInfo> dmas = {
      DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(0, 1234)),
      DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(2000, 888)),
      DmaInfo(2, DmaDescriptorType::kInputActivation,
              DeviceBuffer(4000, 4000))};
  EXPECT_CALL(extractor_, ExtractDmaInfos(_, _)).WillOnce(Return(dmas));

  EXPECT_OK(scheduler.Submit(MakeRequest([](int, const util::Status&) {})));

  auto* next_dma = scheduler.GetNextDma();
  ASSERT_NE(next_dma, nullptr);
  EXPECT_EQ(dmas.front().type(), next_dma->type());
  EXPECT_EQ(dmas.front().buffer(), next_dma->buffer());
  EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));

  EXPECT_EQ(scheduler.NotifyRequestCompletion().CanonicalCode(),
            util::error::FAILED_PRECONDITION);

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest,
       OutOfOrderRequestCompleteWithActiveDmas) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::list<DmaInfo> dmas = {
      DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(0, 1234))};
  EXPECT_CALL(extractor_, ExtractDmaInfos(_, _)).WillOnce(Return(dmas));

  EXPECT_OK(scheduler.Submit(MakeRequest([](int, const util::Status&) {})));

  auto* next_dma = scheduler.GetNextDma();
  ASSERT_NE(next_dma, nullptr);
  EXPECT_EQ(dmas.front().type(), next_dma->type());
  EXPECT_EQ(dmas.front().buffer(), next_dma->buffer());

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, SameDmas) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::list<DmaInfo> dmas = {
      DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(10000, 777)),
      DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(0, 1234)),
      DmaInfo(2, DmaDescriptorType::kInputActivation,
              DeviceBuffer(4000, 4000))};
  bool done = false;
  EXPECT_CALL(extractor_, ExtractDmaInfos(_, _)).WillOnce(Return(dmas));

  EXPECT_OK(
      scheduler.Submit(MakeRequest([&done](int, const util::Status& status) {
        EXPECT_OK(status);
        done = true;
      })));

  for (const auto& info : dmas) {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), info.type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), info.buffer());
    EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
    EXPECT_FALSE(done);
  }

  auto* next_dma = scheduler.GetNextDma();
  EXPECT_EQ(next_dma, nullptr);
  EXPECT_FALSE(done);

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done);

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, TwoRequests) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::vector<std::list<DmaInfo>> dmas = {
      {DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(10000, 777)),
       DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(0, 1234)),
       DmaInfo(2, DmaDescriptorType::kInputActivation,
               DeviceBuffer(4000, 4000))},

      {DmaInfo(0, DmaDescriptorType::kInstruction,
               DeviceBuffer(0x10dead, 1727)),
       DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(0x20beef, 3141)),
       DmaInfo(2, DmaDescriptorType::kInputActivation,
               DeviceBuffer(0x30dead, 5926))},
  };

  {
    InSequence s;
    for (const auto& curr_dmas : dmas) {
      EXPECT_CALL(extractor_, ExtractDmaInfos(_, _))
          .WillOnce(Return(curr_dmas))
          .RetiresOnSaturation();
    }
  }

  std::vector<bool> done(dmas.size(), false);
  for (int i = 0; i < dmas.size(); ++i) {
    EXPECT_OK(scheduler.Submit(
        MakeRequest([&done, i](int, const util::Status& status) {
          EXPECT_OK(status);
          done[i] = true;
        })));
  }

  std::vector<DmaInfo*> completed_dmas;
  for (const auto& curr_dmas : dmas) {
    for (const auto& info : curr_dmas) {
      auto* next_dma = scheduler.GetNextDma();
      ASSERT_NE(next_dma, nullptr);
      EXPECT_EQ(next_dma->type(), info.type());
      EXPECT_TRUE(next_dma->IsActive());
      EXPECT_EQ(next_dma->buffer(), info.buffer());
      for (const auto& curr_done : done) {
        EXPECT_FALSE(curr_done);
      }
      completed_dmas.push_back(next_dma);
    }
  }

  // No DMAs.
  for (int i = 0; i < 5; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
    for (const auto& curr_done : done) {
      EXPECT_FALSE(curr_done);
    }
  }

  for (auto* dma : completed_dmas) {
    EXPECT_OK(scheduler.NotifyDmaCompletion(dma));
  }

  for (int i = 0; i < dmas.size(); ++i) {
    EXPECT_OK(scheduler.NotifyRequestCompletion());
  }

  for (const auto& curr_done : done) {
    EXPECT_TRUE(curr_done);
  }

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, LocalFence) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::list<DmaInfo> dmas = {
      DmaInfo(0, DmaDescriptorType::kParameter, DeviceBuffer(0, 1234)),
      DmaInfo(1, DmaDescriptorType::kLocalFence),
      DmaInfo(2, DmaDescriptorType::kInputActivation,
              DeviceBuffer(4000, 4000))};
  bool done = false;
  EXPECT_CALL(extractor_, ExtractDmaInfos(_, _)).WillOnce(Return(dmas));

  EXPECT_OK(
      scheduler.Submit(MakeRequest([&done](int, const util::Status& status) {
        EXPECT_OK(status);
        done = true;
      })));

  std::vector<DmaInfo*> completed_dmas;
  {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), dmas.front().type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), dmas.front().buffer());
    completed_dmas.push_back(next_dma);
    EXPECT_FALSE(done);
  }

  // Local fence will not return any DMAs.
  for (int i = 0; i < 10; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
    EXPECT_FALSE(done);
  }

  // Local fence will be removed.
  EXPECT_OK(scheduler.NotifyDmaCompletion(completed_dmas[0]));

  {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), dmas.back().type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), dmas.back().buffer());
    EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
    EXPECT_FALSE(done);
  }

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done);

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, GlobalFence) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::vector<std::list<DmaInfo>> dmas = {
      {DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(10000, 777)),
       DmaInfo(1, DmaDescriptorType::kGlobalFence)},

      {DmaInfo(0, DmaDescriptorType::kInstruction,
               DeviceBuffer(0x10dead, 1727)),
       DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(0x20beef, 3141)),
       DmaInfo(2, DmaDescriptorType::kInputActivation,
               DeviceBuffer(0x30dead, 5926))},
  };

  {
    InSequence s;
    for (const auto& curr_dmas : dmas) {
      EXPECT_CALL(extractor_, ExtractDmaInfos(_, _))
          .WillOnce(Return(curr_dmas))
          .RetiresOnSaturation();
    }
  }

  std::vector<bool> done(dmas.size(), false);
  for (int i = 0; i < dmas.size(); ++i) {
    EXPECT_OK(scheduler.Submit(
        MakeRequest([&done, i](int, const util::Status& status) {
          EXPECT_OK(status);
          done[i] = true;
        })));
  }

  DmaInfo* completed_dma = nullptr;
  {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), dmas[0].front().type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), dmas[0].front().buffer());
    completed_dma = next_dma;
    for (const auto& curr_done : done) {
      EXPECT_FALSE(curr_done);
    }
  }

  // Global fence will not return any DMAs.
  for (int i = 0; i < 7; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
    for (const auto& curr_done : done) {
      EXPECT_FALSE(curr_done);
    }
  }

  EXPECT_OK(scheduler.NotifyDmaCompletion(completed_dma));

  // Global fence will not return any DMAs.
  for (int i = 0; i < 2; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
    for (const auto& curr_done : done) {
      EXPECT_FALSE(curr_done);
    }
  }

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done[0]);
  EXPECT_FALSE(done[1]);

  for (const auto& info : dmas[1]) {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), info.type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), info.buffer());
    EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
  }
  EXPECT_TRUE(done[0]);
  EXPECT_FALSE(done[1]);

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done[0]);
  EXPECT_TRUE(done[1]);

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, IntermediateGlobalFence) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::vector<std::list<DmaInfo>> dmas = {
      {DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(0x0, 1727)),
       DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(0xdead, 3141)),
       DmaInfo(2, DmaDescriptorType::kInputActivation,
               DeviceBuffer(0xbeef, 5926))},

      {DmaInfo(0, DmaDescriptorType::kInstruction,
               DeviceBuffer(0x10000000, 777)),
       DmaInfo(1, DmaDescriptorType::kGlobalFence)},

      {DmaInfo(0, DmaDescriptorType::kInstruction,
               DeviceBuffer(0x10dead, 1727)),
       DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(0x20beef, 3141)),
       DmaInfo(2, DmaDescriptorType::kInputActivation,
               DeviceBuffer(0x30dead, 5926))},
  };

  {
    InSequence s;
    for (const auto& curr_dmas : dmas) {
      EXPECT_CALL(extractor_, ExtractDmaInfos(_, _))
          .WillOnce(Return(curr_dmas))
          .RetiresOnSaturation();
    }
  }

  std::vector<bool> done(dmas.size(), false);
  for (int i = 0; i < dmas.size(); ++i) {
    EXPECT_OK(scheduler.Submit(
        MakeRequest([&done, i](int, const util::Status& status) {
          EXPECT_OK(status);
          done[i] = true;
        })));
  }

  {
    std::queue<DmaInfo> expected_dmas;
    bool met_global_fence = false;
    for (const auto& curr_dmas : dmas) {
      for (const auto& dma : curr_dmas) {
        if (dma.type() == DmaDescriptorType::kGlobalFence) {
          met_global_fence = true;
          break;
        }
        expected_dmas.push(dma);
      }

      if (met_global_fence) {
        break;
      }
    }

    auto* next_dma = scheduler.GetNextDma();
    while (next_dma != nullptr) {
      const auto& expected = expected_dmas.front();
      EXPECT_EQ(next_dma->type(), expected.type());
      EXPECT_TRUE(next_dma->IsActive());
      EXPECT_EQ(next_dma->buffer(), expected.buffer());
      EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
      expected_dmas.pop();

      for (const auto& curr_done : done) {
        EXPECT_FALSE(curr_done);
      }
      next_dma = scheduler.GetNextDma();
    }
  }

  // Global fence will not return any DMAs.
  for (int i = 0; i < 7; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
    for (const auto& curr_done : done) {
      EXPECT_FALSE(curr_done);
    }
  }

  // First Request is not for global fence hence cannot clear.
  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done[0]);
  EXPECT_FALSE(done[1]);
  EXPECT_FALSE(done[2]);

  // Global fence will not return any DMAs.
  for (int i = 0; i < 2; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
  }

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done[0]);
  EXPECT_TRUE(done[1]);
  EXPECT_FALSE(done[2]);

  for (const auto& info : dmas[2]) {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), info.type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), info.buffer());
    EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
  }

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done[0]);
  EXPECT_TRUE(done[1]);
  EXPECT_TRUE(done[2]);

  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, CancelPendingRequests) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::list<DmaInfo> dmas = {
      DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(0, 1234)),
      DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(2000, 888)),
      DmaInfo(2, DmaDescriptorType::kInputActivation,
              DeviceBuffer(4000, 4000))};
  EXPECT_CALL(extractor_, ExtractDmaInfos(_, _)).WillOnce(Return(dmas));

  bool cancelled = false;
  EXPECT_OK(scheduler.Submit(
      MakeRequest([&cancelled](int, const util::Status& status) {
        EXPECT_EQ(status.CanonicalCode(), util::error::CANCELLED);
        cancelled = true;
      })));
  EXPECT_OK(scheduler.CancelPendingRequests());
  EXPECT_TRUE(cancelled);

  for (int i = 0; i < 10; ++i) {
    auto* next_dma = scheduler.GetNextDma();
    EXPECT_EQ(next_dma, nullptr);
  }

  EXPECT_EQ(scheduler.NotifyRequestCompletion().CanonicalCode(),
            util::error::FAILED_PRECONDITION);
  EXPECT_OK(scheduler.Close());
}

TEST_F(SingleEndpointDmaSchedulerTest, ActiveRequestsNotCancelled) {
  SingleQueueDmaScheduler scheduler;
  ASSERT_OK(scheduler.Open());

  const std::list<DmaInfo> dmas = {
      DmaInfo(0, DmaDescriptorType::kInstruction, DeviceBuffer(0, 1234)),
      DmaInfo(1, DmaDescriptorType::kParameter, DeviceBuffer(2000, 888)),
      DmaInfo(2, DmaDescriptorType::kInputActivation,
              DeviceBuffer(4000, 4000))};
  EXPECT_CALL(extractor_, ExtractDmaInfos(_, _)).WillOnce(Return(dmas));

  bool done = false;
  EXPECT_OK(
      scheduler.Submit(MakeRequest([&done](int, const util::Status& status) {
        EXPECT_OK(status);
        done = true;
      })));

  {
    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), dmas.front().type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), dmas.front().buffer());
    EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
  }

  EXPECT_OK(scheduler.CancelPendingRequests());
  EXPECT_FALSE(done);

  for (int i = 1; i < dmas.size(); ++i) {
    auto it = dmas.begin();
    std::advance(it, i);
    const auto& dma = *it;

    auto* next_dma = scheduler.GetNextDma();
    ASSERT_NE(next_dma, nullptr);
    EXPECT_EQ(next_dma->type(), dma.type());
    EXPECT_TRUE(next_dma->IsActive());
    EXPECT_EQ(next_dma->buffer(), dma.buffer());
    EXPECT_OK(scheduler.NotifyDmaCompletion(next_dma));
  }

  EXPECT_OK(scheduler.NotifyRequestCompletion());
  EXPECT_TRUE(done);
  EXPECT_OK(scheduler.Close());
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
