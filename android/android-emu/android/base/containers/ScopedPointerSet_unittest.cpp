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

#include "android/base/containers/ScopedPointerSet.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

class Foo {
public:
    Foo() : mValue(0) { mCreates++; }
    explicit Foo(int value) : mValue(value) { mCreates++; }
    ~Foo() { mDeletes++; }

    int value() const { return mValue; }

    static void resetStats() {
        mDeletes = 0;
        mCreates = 0;
    }

    static size_t mDeletes;
    static size_t mCreates;

private:
    int mValue;
};

size_t Foo::mCreates = 0U;
size_t Foo::mDeletes = 0U;

typedef ScopedPointerSet<Foo> ScopedFooSet;

}  // namespace

TEST(ScopedPointerSet, init) {
    ScopedPointerSet<Foo> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(0U, set.size());
}

TEST(ScopedPointerSet, empty) {
    Foo::resetStats();

    ScopedFooSet set;

    EXPECT_TRUE(set.empty());
    EXPECT_EQ(0U, Foo::mCreates);
    EXPECT_EQ(0U, Foo::mDeletes);

    Foo* foo0 = new Foo(0);
    EXPECT_EQ(1U, Foo::mCreates);
    EXPECT_EQ(0U, Foo::mDeletes);

    set.add(foo0);
    EXPECT_EQ(1U, Foo::mCreates);
    EXPECT_EQ(0U, Foo::mDeletes);

    EXPECT_FALSE(set.empty());

    set.remove(foo0);
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(1U, Foo::mCreates);
    EXPECT_EQ(1U, Foo::mDeletes);
}

TEST(ScopedPointerSet, add) {
    ScopedFooSet set;
    const size_t kCount = 100U;

    Foo::resetStats();

    for (size_t n = 0; n < kCount; ++n) {
        set.add(new Foo(n));
        EXPECT_EQ(n + 1U, set.size());
        EXPECT_EQ(n + 1U, Foo::mCreates);
        EXPECT_EQ(0U, Foo::mDeletes);
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

TEST(ScopedPointerSet, contains) {
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

TEST(ScopedPointerSet, remove) {
    ScopedFooSet set;
    const size_t kCount = 100U;
    Foo* foos[kCount];

    Foo::resetStats();

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

    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(kCount / 2U, Foo::mDeletes);

    // Try to remove them again, check that this doesn't change the set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        Foo* item = foos[n * 2U + 1U];
        EXPECT_FALSE(set.remove(item));
        EXPECT_EQ(kCount / 2U, set.size());
    }

    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(kCount / 2U, Foo::mDeletes);


    // Check that even items remain in the set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        EXPECT_TRUE(set.contains(foos[n * 2U])) << "Item #" << n;
    }
}

TEST(ScopedPointerSet, pick) {
    ScopedFooSet set;
    const size_t kCount = 100U;
    Foo* foos[kCount];

    Foo::resetStats();

    for (size_t n = 0; n < kCount; ++n) {
        foos[n] = new Foo(n);
        set.add(foos[n]);
    }

    EXPECT_EQ(kCount, set.size());

    // Remove odd items from the set only.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        Foo* item = foos[n * 2U + 1U];
        Foo* picked = set.pick(item);
        EXPECT_EQ(item, picked);
        EXPECT_FALSE(set.contains(item));
        EXPECT_EQ(kCount - n - 1, set.size());
    }

    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(0U, Foo::mDeletes);

    // Try to remove them again, check that this doesn't change the set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        Foo* item = foos[n * 2U + 1U];
        Foo* picked = set.pick(item);
        EXPECT_FALSE(picked);
        EXPECT_EQ(kCount / 2U, set.size());
    }

    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(0U, Foo::mDeletes);


    // Check that even items remain in the set.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        EXPECT_TRUE(set.contains(foos[n * 2U])) << "Item #" << n;
    }

    // Delete odd items.
    for (size_t n = 0; n < kCount / 2U; ++n) {
        delete foos[n * 2U + 1U];
    }
}

TEST(ScopedPointerSet, clear) {
    ScopedFooSet set;
    const size_t kCount = 100U;

    Foo::resetStats();

    for (size_t n = 0; n < kCount; ++n) {
        set.add(new Foo(n));
    }

    EXPECT_EQ(kCount, set.size());
    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(0U, Foo::mDeletes);

    set.clear();

    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(kCount, Foo::mDeletes);

    EXPECT_TRUE(set.empty());
    EXPECT_EQ(0U, set.size());
}

TEST(ScopedPointerSet, Iterator) {
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

TEST(ScopedPointerSet, destructorClearsItems) {
    const size_t kCount = 100U;
    Foo::resetStats();

    {
        ScopedFooSet set;

        for (size_t n = 0; n < kCount; ++n) {
            set.add(new Foo(n));
        }
    }  // destructor called here.

    EXPECT_EQ(kCount, Foo::mCreates);
    EXPECT_EQ(kCount, Foo::mDeletes);
}

}  // namespace base
}  // namespace android
