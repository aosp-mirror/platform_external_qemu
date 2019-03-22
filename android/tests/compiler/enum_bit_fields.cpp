extern "C" {
#include "enum_bit_fields.h"
}
#include "gtest/gtest.h"

// This test makes sure that the definitions in tcg are correct.
// Note, calling gtest from C doesn't work well, and using qemu from C++ doesn't
// work well either, so we have the test part that interacts with qemu in a .c file.
TEST(CompilerTest, LargeEnumInBitThing) {
     EXPECT_TRUE(test_enum_equal());
}
