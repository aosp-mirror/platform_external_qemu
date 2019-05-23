// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/FunctionView.h"

#include <gtest/gtest.h>

#include <functional>
#include <string>

namespace android {
namespace base {

TEST(FunctionView, Ctor) {
    // Making sure it can be constructed in all meaningful ways

    using EmptyFunc = FunctionView<void()>;

    EmptyFunc f0;
    EXPECT_FALSE(f0);

    EmptyFunc f1([]{});
    EXPECT_TRUE(f1);

    struct Functor { void operator()() const {} };
    EmptyFunc f2(Functor{});
    EXPECT_TRUE(f2);
    Functor fctr;
    EmptyFunc f3(fctr);
    EXPECT_TRUE(f3);

    EmptyFunc f4(std::function<void()>([f1, f2, f3]{}));
    EXPECT_TRUE(f4);

    const std::function<void()> func = []{};
    EmptyFunc f5(func);

    static_assert(sizeof(f1) <= 2 * sizeof(void*), "Too big FunctionView");
    static_assert(sizeof(f2) <= 2 * sizeof(void*), "Too big FunctionView");
    static_assert(sizeof(f3) <= 2 * sizeof(void*), "Too big FunctionView");
    static_assert(sizeof(f4) <= 2 * sizeof(void*), "Too big FunctionView");
    static_assert(sizeof(f5) <= 2 * sizeof(void*), "Too big FunctionView");
}

TEST(FunctionView, Call) {
    FunctionView<int(int)> view = [](int i) { return i + 1; };
    EXPECT_EQ(1, view(0));
    EXPECT_EQ(-1, view(-2));

    FunctionView<std::string(std::string)> fs = [](const std::string& s) { return s + "1"; };
    EXPECT_STREQ("s1", fs("s").c_str());
    EXPECT_STREQ("ssss1", fs("ssss").c_str());

    std::string base;
    auto lambda = [&base]() { return base + "1"; };
    FunctionView<std::string()> fs2 = lambda;
    base = "one";
    EXPECT_STREQ("one1", fs2().c_str());
    base = "forty two";
    EXPECT_STREQ("forty two1", fs2().c_str());
}

TEST(FunctionView, CopyAndAssign) {
    FunctionView<int(int)> view = [](int i) { return i + 1; };
    EXPECT_EQ(1, view(0));
    view = [](int i) { return i - 1; };
    EXPECT_EQ(0, view(1));

    FunctionView<int(int)> view2 = view;
    EXPECT_EQ(view(10), view2(10));

    decltype (view2) view3;
    auto view4 = view3;
    EXPECT_FALSE(view3);
    EXPECT_FALSE(view4);

    EXPECT_TRUE(view2);
    view2 = view3;
    EXPECT_FALSE(view2);
}

}  // namespace base
}  // namespace android
