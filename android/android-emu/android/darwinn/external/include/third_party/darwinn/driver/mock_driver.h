#ifndef THIRD_PARTY_DARWINN_DRIVER_MOCK_DRIVER_H_
#define THIRD_PARTY_DARWINN_DRIVER_MOCK_DRIVER_H_

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/mock_request.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Mock Driver.
class MockDriver : public api::Driver {
 public:
  MockDriver() = default;
  ~MockDriver() override = default;

  MOCK_CONST_METHOD0(IsOpen, bool());
  MOCK_CONST_METHOD0(IsError, bool());

  MOCK_METHOD1(
      RegisterExecutableFile,
      util::StatusOr<const api::PackageReference*>(const std::string&));
  MOCK_METHOD1(
      RegisterExecutableSerialized,
      util::StatusOr<const api::PackageReference*>(const std::string&));
  MOCK_METHOD2(RegisterExecutableSerialized,
               util::StatusOr<const api::PackageReference*>(const char*,
                                                            size_t));
  MOCK_METHOD1(UnregisterExecutable,
               util::Status(const platforms::darwinn::api::PackageReference*));

  MOCK_METHOD1(Open, util::Status(bool));
  MOCK_METHOD0(Close, util::Status());
  MOCK_CONST_METHOD1(MakeBuffer, Buffer(size_t));
  MOCK_METHOD1(CreateRequest,
               util::StatusOr<std::shared_ptr<api::Request>>(
                   const platforms::darwinn::api::PackageReference*));

  MOCK_METHOD2(Submit, util::Status(api::Request*, api::Request::Done));
  util::Status Submit(std::shared_ptr<api::Request> request,
                      api::Request::Done request_done) override {
    return Submit(request.get(), request_done);
  }

  MOCK_METHOD1(Cancel, util::Status(api::Request*));
  util::Status Cancel(std::shared_ptr<api::Request> request) override {
    return Cancel(request.get());
  }

  MOCK_METHOD1(Execute, util::Status(std::shared_ptr<api::Request>));
  MOCK_METHOD1(
      Execute,
      util::Status(const std::vector<std::shared_ptr<api::Request>>& ));

  MOCK_METHOD0(CancelAllRequests, util::Status());
  MOCK_METHOD1(SetFatalErrorCallback, void(FatalErrorCallback));
  MOCK_METHOD1(SetThermalWarningCallback, void(ThermalWarningCallback));
};

// Creates a mock driver with all default mocks setup.
std::unique_ptr<MockDriver> MakeDriver() {
  auto driver = gtl::MakeUnique<testing::NiceMock<MockDriver>>();

  ON_CALL(*driver, Open(testing::_))
      .WillByDefault(testing::Return(util::Status()));
  ON_CALL(*driver, Close()).WillByDefault(testing::Return(util::Status()));
  ON_CALL(*driver, MakeBuffer(testing::_))
      .WillByDefault(testing::Return(Buffer()));

  auto create_request = [](const api::PackageReference* reference)
      -> util::StatusOr<std::shared_ptr<api::Request>> {
    return MakeRequest(nullptr);
  };
  ON_CALL(*driver, CreateRequest(testing::_))
      .WillByDefault(testing::Invoke(create_request));

  auto submit = [](api::Request* request,
                   api::Request::Done done) -> util::Status {
    done(request->id(), util::Status());
    return util::Status();
  };
  ON_CALL(*driver, Submit(testing::_, testing::_))
      .WillByDefault(testing::Invoke(submit));

  return std::move(driver);
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MOCK_DRIVER_H_
