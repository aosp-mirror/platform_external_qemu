#include "third_party/darwinn/port/default/status_macros.h"

#include "third_party/darwinn/port/default/errors.h"
#include "third_party/darwinn/port/default/status_test_util.h"
#include "third_party/darwinn/port/default/statusor.h"
#include "third_party/darwinn/port/default/test_helpers.h"

#include "testing/base/public/gmock.h"
#include "testing/base/public/gunit.h"

namespace platforms {
namespace darwinn {
namespace util {

Status RetCheckFail() {
  RET_CHECK(2 > 3);
  return Status::OK();
}

Status RetCheckFailWithExtraMessage() {
  RET_CHECK(2 > 3) << "extra message";
  return Status::OK();
}

Status RetCheckSuccess() {
  RET_CHECK(3 > 2);
  return Status::OK();
}

TEST(StatusMacros, RetCheckFailing) {
  Status status = RetCheckFail();
  EXPECT_EQ(status.code(), error::INTERNAL);
  EXPECT_MATCH(status.error_message(),
               testing::ContainsRegex("RET_CHECK failure.*2 > 3"));
}

TEST(StatusMacros, RetCheckFailingWithExtraMessage) {
  Status status = RetCheckFailWithExtraMessage();
  EXPECT_EQ(status.code(), error::INTERNAL);
  EXPECT_MATCH(status.error_message(),
               testing::ContainsRegex("RET_CHECK.*2 > 3 extra message"));
}

TEST(StatusMacros, RetCheckSucceeding) {
  Status status = RetCheckSuccess();
  EXPECT_OK(status);
}

StatusOr<int> CreateIntSuccessfully() { return 42; }

StatusOr<int> CreateIntUnsuccessfully() { return InternalError("foobar"); }

TEST(StatusMacros, AssignOrAssertOnOK) {
  ASSIGN_OR_ASSERT_OK(int result, CreateIntSuccessfully());
  EXPECT_EQ(42, result);
}

Status ReturnStatusOK() { return Status::OK(); }

Status ReturnStatusError() { return (InternalError("foobar")); }

using StatusReturningFunction = std::function<Status()>;

StatusOr<int> CallStatusReturningFunction(StatusReturningFunction func) {
  RETURN_IF_ERROR(func());
  return 42;
}

TEST(StatusMacros, ReturnIfErrorOnOK) {
  StatusOr<int> rc = CallStatusReturningFunction(ReturnStatusOK);
  EXPECT_IS_OK(rc);
  EXPECT_EQ(42, rc.ConsumeValueOrDie());
}

TEST(StatusMacros, ReturnIfErrorOnError) {
  StatusOr<int> rc = CallStatusReturningFunction(ReturnStatusError);
  EXPECT_FALSE(rc.ok());
  EXPECT_EQ(rc.status().code(), error::INTERNAL);
}

TEST(StatusMacros, AssignOrReturnSuccessufully) {
  Status status = []() {
    ASSIGN_OR_RETURN(int value, CreateIntSuccessfully());
    EXPECT_EQ(value, 42);
    return Status::OK();
  }();
  EXPECT_IS_OK(status);
}

TEST(StatusMacros, AssignOrReturnUnsuccessfully) {
  Status status = []() {
    ASSIGN_OR_RETURN(int value, CreateIntUnsuccessfully());
    (void)value;
    return Status::OK();
  }();
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(status.code(), error::INTERNAL);
}

}  // namespace util
}  // namespace darwinn
}  // namespace platforms
