#include "third_party/darwinn/driver/driver.h"

#include <stdlib.h>
#include <chrono>  // NOLINT
#include <memory>
#include <string>
#include <thread>  // NOLINT

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/driver/device_buffer.h"
#include "third_party/darwinn/driver/device_buffer_mapper.h"
#include "third_party/darwinn/driver/mock_request.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/driver/request.h"
#include "third_party/darwinn/driver/test_util.h"
#include "third_party/darwinn/port/blocking_counter.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/statusor.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using std::placeholders::_1;
using std::placeholders::_2;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::Sequence;
using testutil::MakePackage;
using testutil::MakeTypedExecutable;

class TestDriver : public Driver {
 public:
  TestDriver()
      : Driver(api::Chip::kBeagle,
               gtl::MakeUnique<PackageRegistry>(api::Chip::kBeagle)) {}
  ~TestDriver() override = default;

  util::Status DoOpen(bool debug_mode) override { return util::Status(); }

  util::Status DoClose(bool in_error) override { return util::Status(); }

  util::Status DoCancelAndWaitRequests(bool in_error) { return util::Status(); }

  Buffer DoMakeBuffer(size_t size_bytes) const override { return Buffer(); }

  MOCK_METHOD2(DoMapBuffer,
               util::StatusOr<MappedDeviceBuffer>(const Buffer& buffer,
                                                  DmaDirection direction));

  MOCK_METHOD0(Unmap, util::Status());

  util::StatusOr<std::shared_ptr<api::Request>> DoCreateRequest(
      const ExecutableReference* executable) override {
    return MakeRequest(executable);
  }

  const ExecutableReference* DoGetExecutableReference(
      std::shared_ptr<api::Request> api_request) override {
    auto* request = static_cast<MockRequest*>(api_request.get());
    return request->executable_reference();
  }

  MOCK_METHOD2(DoSubmit,
               util::Status(std::shared_ptr<api::Request>, api::Request::Done));
};

class DriverTest : public ::testing::Test {
 public:
  DriverTest() = default;
  ~DriverTest() override = default;

  // Mock request done callback.
  MOCK_METHOD2(RequestDone, void(int, const util::Status&));
};

TEST_F(DriverTest, TestOpenClose) {
  TestDriver driver;

  ASSERT_OK(driver.Open());
  EXPECT_TRUE(driver.IsOpen());
  EXPECT_OK(driver.Close());
  EXPECT_FALSE(driver.IsOpen());
}

TEST_F(DriverTest, TestOpenCloseTwice) {
  TestDriver driver;
  ASSERT_OK(driver.Open());
  ASSERT_OK(driver.Close());

  ASSERT_OK(driver.Open());
  EXPECT_OK(driver.Close());
}

TEST_F(DriverTest, TestOpenTwiceOk) {
  TestDriver driver;

  ASSERT_OK(driver.Open());
  ASSERT_OK(driver.Open());
}

TEST_F(DriverTest, TestOpenTwiceCloseTwice) {
  TestDriver driver;

  ASSERT_OK(driver.Open());
  ASSERT_OK(driver.Open());
  ASSERT_OK(driver.Close());
  ASSERT_OK(driver.Close());
}

TEST_F(DriverTest, TestOpenOnceCloseTwice) {
  TestDriver driver;

  ASSERT_OK(driver.Open());
  ASSERT_OK(driver.Close());
  EXPECT_EQ(driver.Close().CanonicalCode(), util::error::FAILED_PRECONDITION);
}

TEST_F(DriverTest, TestCloseFail) {
  TestDriver driver;
  EXPECT_EQ(driver.Close().CanonicalCode(), util::error::FAILED_PRECONDITION);
}

TEST_F(DriverTest, TestMapAndUnMapParameter) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  testutil::ExecutableGenerator generator;
  generator.Finish();

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillOnce(testing::Return(testing::ByMove(MappedDeviceBuffer())));

  EXPECT_CALL(driver, Unmap()).WillOnce(testing::Return(util::Status()));

  // Register Executable.
  ASSERT_OK_AND_ASSIGN(
      const auto* executable_reference,
      driver.RegisterExecutableSerialized(generator.PackageString()));

  // Set mapped parameter unmapping callback.
  auto* package_ref = const_cast<PackageReference*>(
      static_cast<const PackageReference*>(executable_reference));
  for (auto* driver_executable_ref : package_ref->AllExecutableReferences()) {
    driver_executable_ref->SetMappedParameters(
        {DeviceBuffer(), [&driver](DeviceBuffer) { return driver.Unmap(); }});
  }

  ASSERT_OK(driver.UnregisterExecutable(executable_reference));
}

TEST_F(DriverTest, TestSubmitMany) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  testutil::ExecutableGenerator generator;
  generator.Finish();

  // Parameter mapping should only happen once.
  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillOnce(testing::Return(testing::ByMove(MappedDeviceBuffer())));

  // Register Executable.
  ASSERT_OK_AND_ASSIGN(
      const auto* executable_reference,
      driver.RegisterExecutableSerialized(generator.PackageString()));

  // Create Requests.
  ASSERT_OK_AND_ASSIGN(auto request1,
                       driver.CreateRequest(executable_reference));
  ASSERT_OK_AND_ASSIGN(auto request2,
                       driver.CreateRequest(executable_reference));
  ASSERT_OK_AND_ASSIGN(auto request3,
                       driver.CreateRequest(executable_reference));

  // Count completions.
  BlockingCounter counter(3);

  // Complete requests in a detached thread.
  auto detached_execute_request =
      [&counter](int id, std::shared_ptr<api::Request> request,
                 api::Request::Done done) {
        std::thread t([id, &counter, done] {
          std::this_thread::sleep_for(std::chrono::seconds(2));
          done(id, util::Status());
          counter.DecrementCount();
        });
        t.detach();

        return util::Status();
      };

  // Submits will eventually execute detached_execute_request with different
  // id each time.
  EXPECT_CALL(driver, DoSubmit(_, _))
      .WillOnce(Invoke(std::bind(detached_execute_request, 0, _1, _2)))
      .WillOnce(Invoke(std::bind(detached_execute_request, 1, _1, _2)))
      .WillOnce(Invoke(std::bind(detached_execute_request, 3, _1, _2)));

  // Verify that done callbacks are executed correctly.
  EXPECT_CALL(*this, RequestDone(0, util::Status()));
  EXPECT_CALL(*this, RequestDone(1, util::Status()));
  EXPECT_CALL(*this, RequestDone(3, util::Status()));

  // Submit thrice.
  EXPECT_OK(driver.Submit(request1,
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(request2,
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(request3,
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));

  counter.Wait();
}

TEST_F(DriverTest, TestExecuteMany) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  testutil::ExecutableGenerator generator;
  generator.Finish();

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillOnce(testing::Return(testing::ByMove(MappedDeviceBuffer())));

  // Register Executable.
  ASSERT_OK_AND_ASSIGN(
      const auto* executable_reference,
      driver.RegisterExecutableSerialized(generator.PackageString()));

  // Create Requests.
  ASSERT_OK_AND_ASSIGN(auto request1,
                       driver.CreateRequest(executable_reference));
  ASSERT_OK_AND_ASSIGN(auto request2,
                       driver.CreateRequest(executable_reference));
  ASSERT_OK_AND_ASSIGN(auto request3,
                       driver.CreateRequest(executable_reference));

  // Complete requests in a detached thread.
  auto detached_execute_request = [](int id,
                                     std::shared_ptr<api::Request> request,
                                     api::Request::Done done) {
    std::thread t([id, done] {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      done(id, util::Status());
    });
    t.detach();
    return util::Status();
  };

  {
    Sequence s;
    EXPECT_CALL(driver, DoSubmit(request2, _))
        .InSequence(s)
        .WillOnce(Invoke(std::bind(detached_execute_request, 1, _1, _2)));
    EXPECT_CALL(driver, DoSubmit(request1, _))
        .InSequence(s)
        .WillOnce(Invoke(std::bind(detached_execute_request, 0, _1, _2)));
    EXPECT_CALL(driver, DoSubmit(request3, _))
        .InSequence(s)
        .WillOnce(Invoke(std::bind(detached_execute_request, 3, _1, _2)));
  }

  // Submit thrice.
  EXPECT_OK(driver.Execute({request2, request1, request3}));
}

TEST_F(DriverTest, TestUnregister) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  testutil::ExecutableGenerator generator;
  generator.Finish();

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillOnce(testing::Return(testing::ByMove(MappedDeviceBuffer())));

  // Register Executable.
  ASSERT_OK_AND_ASSIGN(
      const auto* executable_reference,
      driver.RegisterExecutableSerialized(generator.PackageString()));

  ASSERT_OK(driver.UnregisterExecutable(executable_reference));

  // Unregister the same executable twice should fail gracefully.
  EXPECT_EQ(driver.UnregisterExecutable(executable_reference).CanonicalCode(),
            util::error::NOT_FOUND);
}

util::StatusOr<std::vector<std::shared_ptr<api::Request>>> CreateRequestVector(
    TestDriver& driver, const api::PackageReference* executable_ref,
    int count) {
  std::vector<std::shared_ptr<api::Request>> requests;
  for (int i = 0; i < count; ++i) {
    ASSIGN_OR_RETURN(auto request, driver.CreateRequest(executable_ref));
    requests.push_back(std::move(request));
  }
  return requests;
}

TEST_F(DriverTest, TestParameterCachingManySubmits) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillRepeatedly(testing::Invoke(
          [](const Buffer&, DmaDirection) { return MappedDeviceBuffer(); }));

  // A compiled model with parameter-caching.
  std::vector<std::string> cached_executables;
  cached_executables.push_back(
      MakeTypedExecutable(ExecutableType_EXECUTION_ONLY, 1));
  cached_executables.push_back(
      MakeTypedExecutable(ExecutableType_PARAMETER_CACHING, 1));
  auto cached_package = MakePackage(cached_executables);
  ASSERT_OK_AND_ASSIGN(const auto* cached_ref,
                       driver.RegisterExecutableSerialized(cached_package));

  // A compiled stand-alone model which is co-compiled with the one above.
  std::vector<std::string> cocompiled_executables;
  cocompiled_executables.push_back(
      MakeTypedExecutable(ExecutableType_STAND_ALONE, 1));
  auto cocompiled_package = MakePackage(cocompiled_executables);
  ASSERT_OK_AND_ASSIGN(const auto* cocompiled_ref,
                       driver.RegisterExecutableSerialized(cocompiled_package));

  // Another parameter-caching compiled model which was compiled at a different
  // time (different token).
  std::vector<std::string> cached_executables_2;
  cached_executables_2.push_back(
      MakeTypedExecutable(ExecutableType_EXECUTION_ONLY, 2));
  cached_executables_2.push_back(
      MakeTypedExecutable(ExecutableType_PARAMETER_CACHING, 2));
  auto cached_package_2 = MakePackage(cached_executables_2);
  ASSERT_OK_AND_ASSIGN(const auto* cached_ref_2,
                       driver.RegisterExecutableSerialized(cached_package_2));

  // Create Requests.
  constexpr int kNumFirstCachedRequests = 4;
  constexpr int kNumCoCompiledRequests = 2;
  constexpr int kNumSecondCachedRequests = 4;
  ASSERT_OK_AND_ASSIGN(
      auto cached_requests,
      CreateRequestVector(driver, cached_ref, kNumFirstCachedRequests));
  ASSERT_OK_AND_ASSIGN(
      auto cocompiled_requests,
      CreateRequestVector(driver, cocompiled_ref, kNumCoCompiledRequests));
  ASSERT_OK_AND_ASSIGN(
      auto cached_requests_2,
      CreateRequestVector(driver, cached_ref_2, kNumSecondCachedRequests));

  // Number of actual requests + parameter-caching requests needed.
  constexpr int kNumFirstParameterCachingRequests = 2;
  constexpr int kNumSecondParameterCachingRequests = 3;
  BlockingCounter counter(kNumFirstParameterCachingRequests +
                          kNumFirstCachedRequests + kNumCoCompiledRequests +
                          kNumSecondParameterCachingRequests +
                          kNumSecondCachedRequests);

  // Complete requests in a detached thread.
  auto detached_req = [&counter](int id, std::shared_ptr<api::Request> request,
                                 api::Request::Done done) {
    std::thread t([id, &counter, done] {
      done(id, util::Status());
      counter.DecrementCount();
    });
    t.detach();

    return util::Status();
  };

  // Submits will eventually execute detached_req with different
  // id each time.
  Sequence s;
  EXPECT_CALL(driver, DoSubmit(_, _))
      .InSequence(s)
      .WillOnce(Invoke(std::bind(detached_req, 0, _1, _2)))    // req0 PC
      .WillOnce(Invoke(std::bind(detached_req, 1, _1, _2)))    // req0 Inf
      .WillOnce(Invoke(std::bind(detached_req, 2, _1, _2)))    // req1 Inf
      .WillOnce(Invoke(std::bind(detached_req, 3, _1, _2)))    // req2 Inf
      .WillOnce(Invoke(std::bind(detached_req, 4, _1, _2)))    // req3 Inf
      .WillOnce(Invoke(std::bind(detached_req, 5, _1, _2)))    // req4 PC
      .WillOnce(Invoke(std::bind(detached_req, 6, _1, _2)))    // req4 Inf
      .WillOnce(Invoke(std::bind(detached_req, 7, _1, _2)))    // req5 Inf
      .WillOnce(Invoke(std::bind(detached_req, 8, _1, _2)))    // req6 Inf
      .WillOnce(Invoke(std::bind(detached_req, 9, _1, _2)))    // req7 PC
      .WillOnce(Invoke(std::bind(detached_req, 10, _1, _2)))   // req7 Inf
      .WillOnce(Invoke(std::bind(detached_req, 11, _1, _2)))   // req8 PC
      .WillOnce(Invoke(std::bind(detached_req, 12, _1, _2)))   // req8 Inf
      .WillOnce(Invoke(std::bind(detached_req, 13, _1, _2)))   // req9 PC
      .WillOnce(Invoke(std::bind(detached_req, 14, _1, _2)));  // req9 Inf

  // Verify that done callbacks are executed correctly.
  EXPECT_CALL(*this, RequestDone(1, util::Status()));
  EXPECT_CALL(*this, RequestDone(2, util::Status()));
  EXPECT_CALL(*this, RequestDone(3, util::Status()));
  EXPECT_CALL(*this, RequestDone(4, util::Status()));
  EXPECT_CALL(*this, RequestDone(6, util::Status()));
  EXPECT_CALL(*this, RequestDone(7, util::Status()));
  EXPECT_CALL(*this, RequestDone(8, util::Status()));
  EXPECT_CALL(*this, RequestDone(10, util::Status()));
  EXPECT_CALL(*this, RequestDone(12, util::Status()));
  EXPECT_CALL(*this, RequestDone(14, util::Status()));

  // Submit requests.
  EXPECT_OK(driver.Submit(cached_requests[0],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests[1],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cocompiled_requests[0],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests[2],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests_2[0],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests_2[1],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cocompiled_requests[1],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests_2[2],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests[3],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));
  EXPECT_OK(driver.Submit(cached_requests_2[3],
                          std::bind(&DriverTest::RequestDone, this, _1, _2)));

  counter.Wait();
}

TEST_F(DriverTest, TestParameterCachingExecute) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillRepeatedly(testing::Invoke(
          [](const Buffer&, DmaDirection) { return MappedDeviceBuffer(); }));

  // A compiled model with parameter-caching.
  std::vector<std::string> cached_executables;
  cached_executables.push_back(
      MakeTypedExecutable(ExecutableType_EXECUTION_ONLY, 1));
  cached_executables.push_back(
      MakeTypedExecutable(ExecutableType_PARAMETER_CACHING, 1));
  auto cached_package = MakePackage(cached_executables);
  ASSERT_OK_AND_ASSIGN(const auto* cached_ref,
                       driver.RegisterExecutableSerialized(cached_package));

  // Create Requests.
  constexpr int kNumFirstCachedRequests = 4;
  ASSERT_OK_AND_ASSIGN(
      auto cached_requests,
      CreateRequestVector(driver, cached_ref, kNumFirstCachedRequests));

  // Complete requests in a detached thread.
  auto detached_req = [](int id, std::shared_ptr<api::Request> request,
                         api::Request::Done done) {
    std::thread t([id, done] { done(id, util::Status()); });
    t.detach();

    return util::Status();
  };

  // Submits will eventually execute detached_req with different
  // id each time.
  Sequence s;
  EXPECT_CALL(driver, DoSubmit(_, _))
      .InSequence(s)
      .WillOnce(Invoke(std::bind(detached_req, 0, _1, _2)))   // PC
      .WillOnce(Invoke(std::bind(detached_req, 1, _1, _2)))   // Inf
      .WillOnce(Invoke(std::bind(detached_req, 2, _1, _2)))   // Inf
      .WillOnce(Invoke(std::bind(detached_req, 3, _1, _2)))   // Inf
      .WillOnce(Invoke(std::bind(detached_req, 4, _1, _2)));  // Inf

  // Execute requests.
  EXPECT_OK(driver.Execute(cached_requests));
}

TEST_F(DriverTest, TestBatching) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  // Create a regular executable with batch size 1.
  testutil::ExecutableGenerator generator;
  generator.SetInputLayers({"in1", "in2"});
  generator.SetOutputLayers({"out1", "out2"});
  generator.Finish();

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillRepeatedly(testing::Invoke(
          [](const Buffer&, DmaDirection) { return MappedDeviceBuffer(); }));

  // Register Executable.
  ASSERT_OK_AND_ASSIGN(
      const auto* executable_reference,
      driver.RegisterExecutableSerialized(generator.PackageString()));

  ASSERT_OK_AND_ASSIGN(auto request,
                       driver.CreateRequest(executable_reference));

  // Set up the mock request.
  auto mock_request = std::static_pointer_cast<MockRequest>(request);
  EXPECT_CALL(*mock_request, GetNumBatches()).WillRepeatedly(Return(2));
  Buffer input_buffer;
  EXPECT_CALL(*mock_request, InputBuffer(_, _))
      .WillRepeatedly(testing::ReturnRef(input_buffer));
  EXPECT_CALL(*mock_request, OutputBuffer(_, _))
      .WillRepeatedly(Return(Buffer()));

  // Complete requests in a detached thread.
  auto detached_req = [](int id, std::shared_ptr<api::Request> request,
                         api::Request::Done done) {
    std::thread t([id, done] { done(id, util::Status()); });
    t.detach();

    return util::Status();
  };

  // There should be 2 submits, one for each batch.
  Sequence s;
  EXPECT_CALL(driver, DoSubmit(_, _))
      .InSequence(s)
      .WillOnce(Invoke(std::bind(detached_req, 0, _1, _2)))
      .WillOnce(Invoke(std::bind(detached_req, 1, _1, _2)));

  EXPECT_OK(driver.Execute(request));
}

TEST_F(DriverTest, TestBatchingNonMultipleOfHardware) {
  TestDriver driver;
  ASSERT_OK(driver.Open());

  // Create a regular executable with batch size 4.
  testutil::ExecutableGenerator generator;
  generator.SetInputLayers({"in1", "in2"});
  generator.SetOutputLayers({"out1", "out2"});
  generator.SetBatchSize(4);
  generator.Finish();

  EXPECT_CALL(driver, DoMapBuffer(_, _))
      .WillRepeatedly(testing::Invoke(
          [](const Buffer&, DmaDirection) { return MappedDeviceBuffer(); }));

  // Register Executable.
  ASSERT_OK_AND_ASSIGN(
      const auto* executable_reference,
      driver.RegisterExecutableSerialized(generator.PackageString()));

  ASSERT_OK_AND_ASSIGN(auto request,
                       driver.CreateRequest(executable_reference));

  // Set up the mock request with 11 batches.
  auto mock_request = std::static_pointer_cast<MockRequest>(request);
  EXPECT_CALL(*mock_request, GetNumBatches()).WillRepeatedly(Return(11));
  Buffer input_buffer;
  EXPECT_CALL(*mock_request, InputBuffer(_, _))
      .WillRepeatedly(testing::ReturnRef(input_buffer));
  EXPECT_CALL(*mock_request, OutputBuffer(_, _))
      .WillRepeatedly(Return(Buffer()));

  // Complete requests in a detached thread.
  auto detached_req = [](int id, std::shared_ptr<api::Request> request,
                         api::Request::Done done) {
    std::thread t([id, done] { done(id, util::Status()); });
    t.detach();

    return util::Status();
  };

  // There should be 3 submits to the backend.
  Sequence s;
  EXPECT_CALL(driver, DoSubmit(_, _))
      .InSequence(s)
      .WillOnce(Invoke(std::bind(detached_req, 0, _1, _2)))
      .WillOnce(Invoke(std::bind(detached_req, 1, _1, _2)))
      .WillOnce(Invoke(std::bind(detached_req, 2, _1, _2)));

  EXPECT_OK(driver.Execute(request));
}

}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
