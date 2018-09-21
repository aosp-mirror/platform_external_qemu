#ifndef THIRD_PARTY_DARWINN_DRIVER_MOCK_REQUEST_H_
#define THIRD_PARTY_DARWINN_DRIVER_MOCK_REQUEST_H_

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/chip.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {

// Mock Request.
class MockRequest : public api::Request {
 public:
  MockRequest(const ExecutableReference* executable)
      : executable_reference_(executable) {}
  ~MockRequest() override = default;

  MOCK_METHOD2(AddInput, util::Status(const std::string&, const Buffer&));
  MOCK_METHOD2(AddOutput, util::Status(const std::string&, Buffer));
  MOCK_CONST_METHOD0(id, int());
  MOCK_CONST_METHOD0(GetNumBatches, util::StatusOr<int>());
  MOCK_CONST_METHOD2(InputBuffer, const Buffer&(const std::string&, int));
  MOCK_CONST_METHOD2(OutputBuffer, Buffer(const std::string&, int));

  const ExecutableReference* executable_reference() {
    return executable_reference_;
  }

 private:
  const ExecutableReference* executable_reference_;
};

// Creates a mock request with all default mocks setup.
std::shared_ptr<api::Request> MakeRequest(
    const ExecutableReference* executable) {
  auto request = std::make_shared<testing::NiceMock<MockRequest>>(executable);
  ON_CALL(*request, AddInput(testing::_, testing::_))
      .WillByDefault(testing::Return(util::Status()));
  ON_CALL(*request, AddOutput(testing::_, testing::_))
      .WillByDefault(testing::Return(util::Status()));
  ON_CALL(*request, GetNumBatches()).WillByDefault(testing::Return(0));
  return request;
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_MOCK_REQUEST_H_
