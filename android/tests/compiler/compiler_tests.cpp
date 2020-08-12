extern "C" {
#include "compiler_tests.h"
#include <glib.h>
#include <glib/gprintf.h>
}
#include "gtest/gtest.h"
#include <inttypes.h>
#include <cstdint>
#include <string_view> // Make sure we can use C++17 std::string_view
#include <optional>

TEST(CompilerTest, stringview) {
    std::string_view hello("Hello");
    EXPECT_STREQ("Hello", hello.data());
}

TEST(CompilerTest, optional) {
    auto godzilla = std::optional<std::string>{"Godzilla"};
    EXPECT_TRUE(godzilla.has_value());
}

// This test makes sure that the definitions in tcg are correct.
// Note, calling gtest from C doesn't work well, and using qemu from C++ doesn't
// work well either, so we have the test part that interacts with qemu in a .c file.
TEST(CompilerTest, LargeEnumInBitThing) {
     EXPECT_TRUE(test_enum_equal());
}

TEST(CompilerTest, long_jump_stack_test) {
     // This test is to guarantee that the stack frame is set properly.
     // a broken stack frame will result in an exception/termination.
     EXPECT_TRUE(long_jump_stack_test());
}

TEST(CompilerTest, long_jump_double_call) {
     EXPECT_TRUE(long_jump_double_call());
}

TEST(CompilerTest, long_jump_ret_value) {
     EXPECT_TRUE(long_jump_ret_value());
}

TEST(CompilerTest, long_jump_preserve_int_params) {
     // This test is to guarantee that the parameters are still available
     // parameters (on windows) can be passed in as registers.
     // Note, the bitmask should help you identify which parameter is missing. bit 0/4/8/12 etc = param1, 1/5/9/13 = param2 etc.
     EXPECT_EQ(long_jump_preserve_int_params(PARAM1, PARAM2, PARAM3, PARAM4), PARAM1 + PARAM2 + PARAM3 + PARAM4);
}

TEST(CompilerTest, long_jump_preserve_float_params) {
     // This test is to guarantee that the parameters are still available
     // parameters (on windows) can be passed in as registers.
     // Note, the bitmask should help you identify which parameter is missing. bit 0/4/8/12 etc = param1, 1/5/9/13 = param2 etc.
    EXPECT_TRUE(long_jump_preserve_float_params(1.123, 2.123, 3.123, 4.123));
}


TEST(CompilerTest, setjmp_sets_fields) {
     EXPECT_TRUE(setjmp_sets_fields());
}

TEST(CompilerTest, g_strdup_printf_incorrect_b129781540) {
    int64_t val = 6442450944;
    char* str = g_strdup_printf("%" PRId64, val);
    EXPECT_STREQ("6442450944", str);
    free(str);
}
