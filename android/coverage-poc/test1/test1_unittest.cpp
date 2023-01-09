#include "gtest/gtest.h"

#include "test1_lib.h"

TEST(Test1, PrintFunc) {
    std::string p = test1_print();
    ASSERT_EQ(p, "called test1_print");
}
