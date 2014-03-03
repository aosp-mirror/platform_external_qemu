// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/memory/ScopedPtr.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

class Foo {
public:
    Foo(int* countPtr) : mCountPtr(countPtr) {}

    void DoIncrement() {
        (*mCountPtr)++;
    }

    ~Foo() {
        DoIncrement();
    }
private:
    int* mCountPtr;
};

class ScopedPtrTest : public testing::Test {
public:
    ScopedPtrTest() : mDestroyCount(0) {}

    Foo* NewFoo() {
        return new Foo(&mDestroyCount);
    }

protected:
    int mDestroyCount;
};

TEST_F(ScopedPtrTest, EmptyConstructor) {
    ScopedPtr<Foo> foo;
    EXPECT_FALSE(foo.get());
}

TEST_F(ScopedPtrTest, SimpleConstructor) {
    ScopedPtr<Foo> foo(NewFoo());
    EXPECT_TRUE(foo.get());
}

TEST_F(ScopedPtrTest, DestroyOnScopeExit) {
    {
        ScopedPtr<Foo> foo(NewFoo());
        EXPECT_EQ(0, mDestroyCount);
    }
    EXPECT_EQ(1, mDestroyCount);
}

TEST_F(ScopedPtrTest, Release) {
    Foo* savedFoo = NULL;
    {
        ScopedPtr<Foo> foo(NewFoo());
        EXPECT_EQ(0, mDestroyCount);
        savedFoo = foo.release();
        EXPECT_EQ(0, mDestroyCount);
        EXPECT_FALSE(foo.get());
        EXPECT_TRUE(savedFoo);
    }
    EXPECT_EQ(0, mDestroyCount);
    delete savedFoo;
}

TEST_F(ScopedPtrTest, ArrowOperator) {
    ScopedPtr<Foo> foo(NewFoo());
    foo->DoIncrement();
    EXPECT_EQ(1, mDestroyCount);
}

TEST_F(ScopedPtrTest, StarOperator) {
    ScopedPtr<Foo> foo(NewFoo());
    (*foo).DoIncrement();
    EXPECT_EQ(1, mDestroyCount);
}

}  // namespace base
}  // namespace android