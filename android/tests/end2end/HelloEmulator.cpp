#include <gtest/gtest.h>

TEST(HelloEmulator, Basic) {
    fprintf(stderr, "%s: Hi\n", __func__);
}
