// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/base/memory/QSort.h"

#include <gtest/gtest.h>

#include <stdint.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

struct ByteQSortTraits {
    static int compare(const uint8_t* a, const uint8_t* b) {
        return *a - *b;
    }
    static void swap(uint8_t* a, uint8_t* b) {
        uint8_t tmp = *a;
        *a = *b;
        *b = tmp;
    }
};

struct Int32ReverseQSortTraits {
    static int compare(const int32_t* a, const int32_t* b) {
        return (int)(*b - *a);
    }
    static void swap(int32_t* a, int32_t* b) {
        int32_t tmp = *a;
        *a = *b;
        *b = tmp;
    }
};

class Foo {
public:
    Foo() : mX(0) {}
    Foo(int x) : mX(x) {}
    void set(int x) { mX = x; }
    int x() const { return mX; }

    void swap(Foo* other) {
        int tmp = mX;
        mX = other->mX;
        other->mX = tmp;
    }

    struct Traits {
        static int compare(const Foo* a, const Foo* b) {
            return b->x() - a->x();
        }
        static void swap(Foo* a, Foo* b) {
            a->swap(b);
        }
    };
private:
    int mX;
};

TEST(QSort, SortBytes) {
    static const uint8_t kExpected[10] = {
        0, 1, 2, 4, 8, 10, 40, 128, 192, 255,
    };
    uint8_t tab[10] = {
        8, 40, 1, 192, 128, 10, 255, 2, 0, 4,
    };
    QSort<uint8_t, ByteQSortTraits>::sort(tab, ARRAYLEN(tab));
    for (size_t n = 0; n < ARRAYLEN(tab); ++n) {
        EXPECT_EQ(kExpected[n], tab[n]) << n;
    }
}

TEST(QSort, SortIntegersReversed) {
    static const int32_t kExpected[10] = {
    };
    int32_t tab[10] = {
    };
    QSort<int32_t, Int32ReverseQSortTraits>::sort(tab, ARRAYLEN(tab));
    for (size_t n = 0; n < ARRAYLEN(tab); ++n) {
        EXPECT_EQ(kExpected[n], tab[n]) << n;
    }
}

TEST(QSort, SortFooObjects) {
    static const int kExpected[11] = { 10, 8, 6, 4, 2, 0, -2, -4, -6, -8, -10 };
    Foo foos[11] = { -10, -8, -6, 4, 2, 0, -2, -4, 6, 8, 10 };
    QSort<Foo, Foo::Traits>::sort(foos, ARRAYLEN(foos));
    for (size_t n = 0; n < ARRAYLEN(foos); ++n) {
        EXPECT_EQ(kExpected[n], foos[n].x()) << n;
    }
}

}  // namespace base
}  // namespace android
