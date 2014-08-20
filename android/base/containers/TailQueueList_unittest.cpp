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

#include "android/base/containers/TailQueueList.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// Helper class used to hold an integer value, and which can be
class Foo {
public:
    Foo() : mValue(0) {}
    explicit Foo(int value) : mValue(value) {}

    int value() const { return mValue; }
    Foo* next() const { return mLink.next(); }

    TAIL_QUEUE_LIST_TRAITS(Traits, Foo, mLink);

private:
    TailQueueLink<Foo> mLink;
    int mValue;
};

typedef TailQueueList<Foo> FooList;

void clearFooList(FooList* list) {
    for (;;) {
        Foo* foo = list->popFront();
        if (!foo) {
            break;
        }
        delete foo;
    }
}

class ScopedFooList : public FooList {
public:
    ~ScopedFooList() {
        clearFooList((FooList*)this);
    }
};

}  // namespace

TEST(TailQueueList, init) {
    FooList list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(0U, list.size());
    EXPECT_FALSE(list.contains(NULL));
}

TEST(TailQueueList, front) {
    ScopedFooList list;
    const size_t kCount = 100;

    EXPECT_FALSE(list.front());

    for (size_t n = 0; n < kCount; ++n) {
        list.pushBack(new Foo(n));
        EXPECT_TRUE(list.front()) << "Item #" << n;
        EXPECT_EQ(0U, list.front()->value()) << "Item #" << n;
    }
}

TEST(TailQueueList, last) {
    ScopedFooList list;
    const size_t kCount = 100;

    EXPECT_FALSE(list.last());

    for (size_t n = 0; n < kCount; ++n) {
        list.pushBack(new Foo(n));
        EXPECT_TRUE(list.last()) << "Item #" << n;
        EXPECT_EQ(n, list.last()->value()) << "Item #" << n;
    }
}

TEST(TailQueueList, insertTail) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.pushBack(new Foo(n));
        EXPECT_EQ(n + 1, list.size()) << "Item #" << n;
    }

    EXPECT_EQ(kCount, list.size());

    size_t n = 0;
    Foo* foo = list.front();
    for (; n < kCount && foo; n++, foo = foo->next()) {
        EXPECT_EQ(n, foo->value()) << "Item #" << n;
    }

    EXPECT_EQ(kCount, n);
    EXPECT_FALSE(foo);
}

TEST(TailQueueList, insertHead) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertHead(new Foo(n));
        EXPECT_EQ(n + 1, list.size()) << "Item #" << n;
    }

    EXPECT_EQ(kCount, list.size());

    size_t n = 0;
    Foo* foo = list.front();
    for (; n < kCount && foo; n++, foo = foo->next()) {
        EXPECT_EQ(kCount - 1 - n, foo->value()) << "Item #" << n;
    }

    EXPECT_EQ(kCount, n);
    EXPECT_FALSE(foo);
}

TEST(TailQueueList, insertBefore) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
    }

    // Insert before head.
    list.insertBefore(list.front(), new Foo(kCount + 1U));
    EXPECT_EQ(kCount + 1U, list.size());

    Foo* foo = list.front();
    EXPECT_EQ(kCount + 1U, foo->value());

    // Insert before second.
    foo = foo->next();
    EXPECT_EQ(0U, foo->value()) << "Item #1";

    list.insertBefore(foo, new Foo(kCount + 2U));
    EXPECT_EQ(kCount + 2U, list.size());

    foo = list.front();
    EXPECT_EQ(kCount + 1U, foo->value());
    foo = foo->next();
    EXPECT_EQ(kCount + 2U, foo->value());
    foo = foo->next();

    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_EQ(n, foo->value()) << "Item #" << (n + 2u);
        foo = foo->next();
    }
}

TEST(TailQueueList, insertAfter) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
    }

    // Insert after head.
    list.insertAfter(list.front(), new Foo(kCount + 1U));
    EXPECT_EQ(kCount + 1U, list.size());

    Foo* foo = list.front();
    EXPECT_EQ(0U, foo->value());
    foo = foo->next();
    EXPECT_EQ(kCount + 1U, foo->value());
    foo = foo->next();

    for (size_t n = 1; n < kCount - 1U; ++n) {
        EXPECT_EQ(n, foo->value()) << "Item #" << (n + 2U);
        foo = foo->next();
    }

    EXPECT_EQ(kCount - 1U, foo->value()) << "Item #" << kCount - 1U;

    // Insert after last.
    list.insertAfter(foo, new Foo(kCount + 2U));
    foo = list.front();
    EXPECT_EQ(0U, foo->value());
    foo = foo->next();
    EXPECT_EQ(kCount + 1U, foo->value());
    foo = foo->next();

    for (size_t n = 1; n < kCount - 1U; ++n) {
        EXPECT_EQ(n, foo->value()) << "Item #" << (n + 1U);
        foo = foo->next();
    }

    EXPECT_EQ(kCount - 1U, foo->value());
    foo = foo->next();
    EXPECT_TRUE(foo);
    EXPECT_EQ(kCount + 2U, foo->value());
}

TEST(TailQueueList, removeLinearFromHead) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
    }

    // Remove all items in order, from the head.
    Foo* foo = list.front();
    for (size_t n = 0; n < kCount; ++n) {
        Foo* next = foo->next();
        EXPECT_EQ(n, foo->value());
        list.remove(foo);
        EXPECT_EQ(kCount - n - 1, list.size());
        EXPECT_EQ(next, list.front());
        foo = next;
    }
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(0U, list.size());
}

TEST(TailQueueList, removeLinearFromTail) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
    }

    // Remove all items in reverse order, from the tail.
    for (size_t pos = kCount; pos > 0; --pos) {
        // Get item at position |pos - 1|
        Foo* foo = list.front();
        for (size_t n = 1; n < pos; ++n) {
            foo = foo->next();
        }
        EXPECT_TRUE(foo);
        EXPECT_EQ(pos - 1U, foo->value());
        list.remove(foo);
        EXPECT_EQ(pos - 1U, list.size());
    }
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(0U, list.size());
}

TEST(TailQueueList, removeOddOnly) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
    }

    Foo* foo = list.front()->next();
    for (size_t n = 0; n < kCount / 2U; ++n) {
        Foo* next = (n < kCount / 2U - 1U) ? foo->next()->next() : NULL;
        EXPECT_EQ(n * 2U + 1U, foo->value());
        list.remove(foo);
        foo = next;
    }

    EXPECT_EQ(kCount / 2U, list.size());

    foo = list.front();
    for (size_t n = 0; n < kCount / 2; ++n) {
        EXPECT_TRUE(foo);
        EXPECT_EQ(n*2U, foo->value());
        foo = foo->next();
    }
}

TEST(TailQueueList, size) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
        EXPECT_EQ(n + 1U, list.size());
    }
}

TEST(TailQueueList, nth) {
    ScopedFooList list;
    const size_t kCount = 100;

    for (size_t n = 0; n < kCount; ++n) {
        list.insertTail(new Foo(n));
    }

    for (size_t n = 0; n < kCount; ++n) {
        Foo* foo = list.nth(n);
        EXPECT_TRUE(foo);
        EXPECT_EQ(n, foo->value());
    }

    EXPECT_FALSE(list.nth(kCount));
    EXPECT_FALSE(list.nth(kCount + 1U));
}

}  // namespace base
}  // namespace android
