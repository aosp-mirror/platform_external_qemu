// Tests for executable_util.cc

// LINT.IfChange(body)

#include "third_party/darwinn/driver/executable_util.h"

#include <stdlib.h>
#include <memory>
#include <string>

#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/tests.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace {

using ::testing::Eq;
using ::testing::Pointwise;

// Convert a hex string to vector<uint8>.
void HexStringToBytes(const string& hex_string, std::vector<uint8>* bytes) {
  CHECK_EQ(hex_string.length() % 2, 0);
  bytes->clear();

  for (int i = 0; i < hex_string.length(); i += 2) {
    string temp = hex_string.substr(i, 2);
    uint8 byte =
        static_cast<uint8>(strtol(temp.c_str(), nullptr, 16));  // NOLINT
    bytes->push_back(byte);
  }
}

// Base Test
class ExecutableUtilTest : public ::testing::Test {
 protected:
  ExecutableUtilTest() {}
  ~ExecutableUtilTest() override {}
};

struct TestParam {
  string original;
  uint32 value_to_set;
  int offset;
  string expected;
};

// Tests ExecutableUtil::CopyUint32
class CopyUint32Test : public ExecutableUtilTest,
                       public ::testing::WithParamInterface<TestParam> {};

// Tests CopyUint32 by setting kTestValue at the parameterized offset.
TEST_P(CopyUint32Test, TestCopyUint32AtOffset) {
  std::vector<uint8> input, expected;
  HexStringToBytes(GetParam().original, &input);
  HexStringToBytes(GetParam().expected, &expected);

  // Set |kTestValue| at offset_bits.
  ExecutableUtil::CopyUint32(&input, GetParam().offset,
                             GetParam().value_to_set);

  EXPECT_THAT(input, Pointwise(Eq(), expected));
}

const TestParam kTestVectors[]{
    {"a55acc137fff", 0xdeadbeef, 0, "efbeadde7fff"},
    {"a55acc137fff", 0xdeadbeef, 1, "df7d5bbd7fff"},
    {"a55acc137fff", 0xdeadbeef, 2, "bdfbb67a7fff"},
    {"a55acc137fff", 0xdeadbeef, 3, "7df76df57eff"},
    {"a55acc137fff", 0xdeadbeef, 4, "f5eedbea7dff"},
    {"a55acc137fff", 0xdeadbeef, 5, "e5ddb7d57bff"},
    {"a55acc137fff", 0xdeadbeef, 6, "e5bb6fab77ff"},
    {"a55acc137fff", 0xdeadbeef, 7, "a577df566fff"},
    {"a55acc137fff", 0xdeadbeef, 8, "a5efbeaddeff"},
    {"a55acc137fff", 0xdeadbeef, 9, "a5de7d5bbdff"},
    {"a55acc137fff", 0xdeadbeef, 10, "a5befbb67aff"},
    {"a55acc137fff", 0xdeadbeef, 11, "a57af76df5fe"},
    {"a55acc137fff", 0xdeadbeef, 12, "a5faeedbeafd"},
    {"a55acc137fff", 0xdeadbeef, 13, "a5faddb7d5fb"},
    {"a55acc137fff", 0xdeadbeef, 14, "a5dabb6fabf7"},
    {"a55acc137fff", 0xdeadbeef, 15, "a5da77df56ef"},
    {"a55acc137fff", 0xdeadbeef, 16, "a55aefbeadde"},
};

INSTANTIATE_TEST_CASE_P(CopyUint32Test, CopyUint32Test,
                        ::testing::ValuesIn(kTestVectors));
}  // namespace
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

// LINT.ThenChange(executable_util_proto_test.cc:body)
