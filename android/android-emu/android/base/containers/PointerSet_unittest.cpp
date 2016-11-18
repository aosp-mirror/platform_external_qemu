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

#include "android/base/containers/PointerSet.h"

#include <gtest/gtest.h>

#include <stdlib.h>

namespace android {
namespace base {

namespace {

class Foo {
public:
    Foo() : mValue(0) {}
    explicit Foo(int value) : mValue(value) {}

    int value() const { return mValue; }

private:
    int mValue;
};

class ScopedFooSet : public PointerSet<Foo> {
public:
    ~ScopedFooSet() {
        Iterator iter(this);
        while (iter.hasNext()) {
            delete iter.next();
        }
    }
};

}  // namespace

TEST(PointerSet, init) {
    PointerSet<Foo> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(0U, set.size());
}

TEST(PointerSet, empty) {
    ScopedFooSet set;

    EXPECT_TRUE(set.empty());

    Foo* foo0 = new Foo(0);
    set.add(foo0);

    EXPECT_FALSE(set.empty());

    set.remove(foo0);
    EXPECT_TRUE(set.empty());

    delete foo0;
}

TEST(PointerSet, add) {
    ScopedFooSet set;
    const size_t kCount = 100U;

    for (size_t n = 0; n < kCount; ++n) {
        set.add(new Foo(n));
        EXPECT_EQ(n + 1U, set.size());
    }

    bool flags[kCount];
    for (size_t n = 0; n < kCount; ++n) {
        flags[n] = false;
    }

    Foo** array = set.toArray();
    ASSERT_TRUE(array);
    for (size_t n = 0; n < kCount; ++n) {
        Foo* foo = array[n];
        ASSERT_TRUE(foo);
        EXPECT_FALSE(flags[foo->value()]);
        flags[foo->value()] = true;
    }
    ::free(array);
}

TEST(PointerSet, contains) {
    ScopedFooSet set;
    const size_t kCount = 100U;
    Foo* foos[kCount];

    for (size_t n = 0; n < kCount; ++n) {
        foos[n] = new Foo(n);
        set.add(foos[n]);
    }

    EXPECT_EQ(kCount, set.size());

    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_TRUE(set.contains(foos[n])) << "Item #" << n;
    }
}

TEST(PointerSet, remove) {
    ScopedFooSet set;
    const size_t kCount = 100U;
    Foo* foos[kCount];

    for (size_t n = 0; n < kCount; ++n) {
        foos[n] = new Foo(n);
        set.add(foos[n]);
    }

    EXPECT_EQ(kCount, set.size());

    // Remove odd items from the set only.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        Foo* item = foos[n * 2U + 1U];
        EXPECT_TRUE(set.remove(item));
        EXPECT_EQ(kCount - n - 1, set.size());
    }

    // Try to remove them again, check that this doesn't change the set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        Foo* item = foos[n * 2U + 1U];
        EXPECT_FALSE(set.remove(item));
        EXPECT_EQ(kCount / 2U, set.size());
    }

    // Check that even items remain in the set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        EXPECT_TRUE(set.contains(foos[n * 2U])) << "Item #" << n;
    }

    // Delete off items which are no longer in the scoped set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        delete foos[n * 2U + 1U];
    }
}

TEST(PointerSet, clear) {
    ScopedFooSet set;
    const size_t kCount = 100U;
    Foo* foos[kCount];

    for (size_t n = 0; n < kCount; ++n) {
        foos[n] = new Foo(n);
        set.add(foos[n]);
    }

    EXPECT_EQ(kCount, set.size());

    set.clear();

    EXPECT_TRUE(set.empty());
    EXPECT_EQ(0U, set.size());

    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_FALSE(set.contains(foos[n])) << "Item #" << n;
    }

    for (size_t n = 0; n < kCount; ++n) {
        delete foos[n];
    }
}

TEST(PointerSet, Iterator) {
    ScopedFooSet set;
    const size_t kCount = 100U;
    Foo* foos[kCount];

    for (size_t n = 0; n < kCount; ++n) {
        foos[n] = new Foo(n);
        set.add(foos[n]);
    }

    EXPECT_EQ(kCount, set.size());

    bool flags[kCount];
    for (size_t n = 0; n < kCount; ++n) {
        flags[n] = false;
    }

    PointerSet<Foo>::Iterator iter(&set);
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_TRUE(iter.hasNext());
        Foo* foo = iter.next();
        EXPECT_TRUE(foo);
        ASSERT_LT(foo->value(), kCount);
        EXPECT_FALSE(flags[foo->value()]) << "Item #" << foo->value();
        flags[foo->value()] = true;
    }

    EXPECT_FALSE(iter.next());
}

}  // namespace base
}  // namespace android
