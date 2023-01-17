#include "gtest/gtest.h"

#include "test2_lib.h"

TEST(Test2, PrintFunc) {
    std::string p = test2_print();
    ASSERT_EQ(p, "called test2_print");
}
