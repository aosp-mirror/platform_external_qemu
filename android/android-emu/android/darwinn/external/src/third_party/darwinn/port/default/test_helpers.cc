#include "third_party/darwinn/port/default/test_helpers.h"

#include <regex>  // NOLINT

namespace platforms {
namespace darwinn {
namespace testing {

AssertionResult::AssertionResult(const AssertionResult& other)
    : success_(other.success_),
      message_(other.message_ != nullptr ? new std::string(*other.message_)
                                         : static_cast<std::string*>(nullptr)) {
}

// Returns the assertion's negation. Used with EXPECT/ASSERT_FALSE.
AssertionResult AssertionResult::operator!() const {
  AssertionResult negation(!success_);
  if (message_ != nullptr) negation << *message_;
  return negation;
}

AssertionResult& AssertionResult::operator=(const AssertionResult& ar) {
  success_ = ar.success_;
  message_.reset(ar.message_ != nullptr ? new std::string(*ar.message_)
                                        : nullptr);
  return *this;
}

AssertionResult AssertionFailure() { return AssertionResult(false); }

AssertionResult AssertionSuccess() { return AssertionResult(true); }

std::function<bool(const std::string& ece)> ContainsRegex(
    const std::string& regex_string) {
  return [regex_string](const std::string& to_test) {
    std::regex regex(regex_string);
    if (std::regex_search(to_test, regex)) {
      return true;
    } else {
      LOG(ERROR) << "Expected to find " << regex_string << " in " << to_test;
      return false;
    }
  };
}

std::function<bool(const std::string&)> HasSubstr(const std::string& part) {
  return [part](const std::string& whole) {
    return (whole.find(part) != std::string::npos);
  };
}

}  // namespace testing
}  // namespace darwinn
}  // namespace platforms
