// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Optional.h"


#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace android {
namespace base {

TEST(Optional, ConstructFromValue) {
    {
        Optional<int> o;
        EXPECT_FALSE(o);
    }
    {
        Optional<int> o = kNullopt;
        EXPECT_FALSE(o);
    }
    {
        Optional<int> o(1);
        EXPECT_TRUE(o);
        EXPECT_EQ(1, *o);
    }
    {
        Optional<int> o = 1;
        EXPECT_TRUE(o);
        EXPECT_EQ(1, *o);
    }
    {
        Optional<int> o { 1 };
        EXPECT_TRUE(o);
        EXPECT_EQ(1, *o);
    }
    {
        short val = 10;
        Optional<int> o = val;
        EXPECT_TRUE(o);
        EXPECT_EQ(10, *o);
    }
    {
        Optional<std::vector<int>> o(kInplace, 10);
        EXPECT_TRUE(o);
        EXPECT_EQ((std::vector<int>(10)), *o);
    }
    {
        Optional<std::vector<int>> o(kInplace, {1,2,3,4});
        EXPECT_TRUE(o);
        EXPECT_EQ((std::vector<int>{1,2,3,4}), *o);
    }
}

TEST(Optional, ConstructFromOptional) {
    {
        Optional<int> o = Optional<int>();
        EXPECT_FALSE(o);
    }
    {
        Optional<short> o2;
        Optional<int> o(o2);
        EXPECT_FALSE(o);
    }
    {
        Optional<short> o2 = 42;
        Optional<int> o(o2);
        EXPECT_TRUE(o);
        EXPECT_EQ(42, *o);
    }
    {
        Optional<int> o(Optional<int>(1));
        EXPECT_TRUE(o);
        EXPECT_EQ(1, *o);
    }
    {
        Optional<int> o2 = 2;
        Optional<int> o = o2;
        EXPECT_TRUE(o);
        EXPECT_EQ(2, *o);
    }
    {
        Optional<std::vector<int>> o2 = std::vector<int>{20, 30, 40};
        Optional<std::vector<int>> o = o2;
        EXPECT_TRUE(o);
        EXPECT_EQ((std::vector<int>{20, 30, 40}), *o);
    }
}

TEST(Optional, Assign) {
    {
        Optional<int> o;
        o = 1;
        EXPECT_TRUE(o);
        EXPECT_EQ(1, *o);

        o = 2;
        EXPECT_TRUE(o);
        EXPECT_EQ(2, *o);

        o = kNullopt;
        EXPECT_FALSE(o);

        o = Optional<int>(10);
        EXPECT_TRUE(o);
        EXPECT_EQ(10, *o);

        Optional<int> o2;
        o = o2;
        EXPECT_FALSE(o);

        o = 2u;
        EXPECT_TRUE(o);
        EXPECT_EQ(2, *o);

        o = Optional<short>();
        EXPECT_FALSE(o);

        o = Optional<short>(20);
        EXPECT_TRUE(o);
        EXPECT_EQ(20, *o);

        Optional<short> o3(200);
        o = o3;
        EXPECT_TRUE(o);
        EXPECT_EQ(200, *o);
    }
}

TEST(Optional, MakeOptional) {
    {
        auto o = makeOptional(1);
        static_assert(std::is_same<decltype(o), Optional<int>>::value,
                      "Bad type deduction in makeOptional()");
        EXPECT_TRUE(o);
        EXPECT_EQ(1, *o);
    }
    {
        auto o = makeOptional(std::vector<char>{'1', '2'});
        static_assert(std::is_same<decltype(o), Optional<std::vector<char>>>::value,
                      "Bad type deduction in makeOptional()");
        EXPECT_TRUE(o);
        EXPECT_EQ((std::vector<char>{'1', '2'}), *o);
    }
    {
        auto o = makeOptional("String");
        static_assert(std::is_same<decltype(o), Optional<const char*>>::value,
                      "Bad type deduction in makeOptional()");
        EXPECT_TRUE(o);
        EXPECT_STREQ("String", *o);
    }
}

TEST(Optional, Move) {
    auto o = makeOptional(std::unique_ptr<int>(new int(10)));
    {
        decltype(o) o2 = std::move(o);
        EXPECT_TRUE(o);
        EXPECT_TRUE(o2);
        EXPECT_FALSE(bool(*o));
        EXPECT_TRUE(bool(*o2));
        EXPECT_EQ(10, **o2);

        decltype(o) o3;
        o3 = std::move(o2);
        EXPECT_TRUE(o2);
        EXPECT_TRUE(o3);
        EXPECT_FALSE(bool(*o2));
        EXPECT_TRUE(bool(*o3));
        EXPECT_EQ(10, **o3);

        o3 = std::move(o2);
        EXPECT_TRUE(o2);
        EXPECT_TRUE(o3);
        EXPECT_FALSE(bool(*o2));
        EXPECT_FALSE(bool(*o3));
    }

    {
        decltype(o) o1;
        decltype(o) o2 = std::move(o1);
        EXPECT_FALSE(o1);
        EXPECT_FALSE(o2);

        o2 = std::move(o1);
        EXPECT_FALSE(o1);
        EXPECT_FALSE(o2);

        decltype(o) o3 {kInplace, new int(20)};
        o3 = std::move(o1);
        EXPECT_FALSE(o1);
        EXPECT_FALSE(o3);
    }
}

TEST(Optional, Value) {
    auto o = makeOptional(1);
    EXPECT_EQ(1, o.value());
    EXPECT_EQ(1, o.valueOr(2));

    o = kNullopt;
    EXPECT_EQ(2, o.valueOr(2));
}

TEST(Optional, Clear) {
    auto o = makeOptional(1);
    o.clear();
    EXPECT_FALSE(o);

    o.clear();
    EXPECT_FALSE(o);
}

TEST(Optional, Emplace) {
    auto o = makeOptional(std::vector<int>{1,2,3,4});
    o.emplace(3, 1);
    EXPECT_TRUE(o);
    EXPECT_EQ((std::vector<int>{1,1,1}), *o);
    EXPECT_EQ(3, o->capacity());

    o.clear();
    o.emplace({1,2});
    EXPECT_EQ((std::vector<int>{1,2}), *o);
    EXPECT_EQ(2, o->capacity());
}

TEST(Optional, Reset) {
    auto o = makeOptional(std::vector<int>{1,2,3,4});
    o.reset(std::vector<int>{4,3});
    EXPECT_TRUE(o);
    EXPECT_EQ((std::vector<int>{4,3}), *o);
    EXPECT_EQ(2, o->capacity());

    o.clear();
    o.reset(std::vector<int>{1});
    EXPECT_EQ((std::vector<int>{1}), *o);
    EXPECT_EQ(1, o->capacity());
}

TEST(Optional, CompareEqual) {
    EXPECT_TRUE(makeOptional(1) == makeOptional(1));
    EXPECT_TRUE(makeOptional(1) == 1);
    EXPECT_TRUE(1 == makeOptional(1));
    EXPECT_FALSE(makeOptional(1) == makeOptional(2));
    EXPECT_FALSE(makeOptional(2) == 1);
    EXPECT_FALSE(2 == makeOptional(1));
    EXPECT_TRUE(makeOptional(1) != makeOptional(2));
    EXPECT_TRUE(makeOptional(1) != 2);
    EXPECT_TRUE(1 != makeOptional(2));

    EXPECT_FALSE(makeOptional(1) == kNullopt);
    EXPECT_FALSE(makeOptional(1) == Optional<int>());
    EXPECT_FALSE(kNullopt == makeOptional(1));
    EXPECT_FALSE(Optional<int>() == makeOptional(1));
    EXPECT_TRUE(makeOptional(1) != kNullopt);
    EXPECT_TRUE(makeOptional(1) != Optional<int>());
    EXPECT_TRUE(kNullopt != makeOptional(1));
    EXPECT_TRUE(Optional<int>() != makeOptional(1));

    EXPECT_TRUE(kNullopt == Optional<int>());
    EXPECT_TRUE(kNullopt == Optional<char*>());
    EXPECT_FALSE(kNullopt != Optional<int>());
    EXPECT_FALSE(kNullopt != Optional<char*>());
    EXPECT_TRUE(Optional<int>() == Optional<int>());
    EXPECT_FALSE(Optional<int>() != Optional<int>());
}

TEST(Optional, CompareLess) {
    EXPECT_TRUE(makeOptional(1) < makeOptional(2));
    EXPECT_TRUE(1 < makeOptional(2));
    EXPECT_TRUE(makeOptional(1) < 2);

    EXPECT_FALSE(makeOptional(1) < makeOptional(1));
    EXPECT_FALSE(1 < makeOptional(1));
    EXPECT_FALSE(makeOptional(1) < 1);
    EXPECT_FALSE(makeOptional(2) < makeOptional(1));
    EXPECT_FALSE(2 < makeOptional(1));
    EXPECT_FALSE(makeOptional(2) < 1);

    EXPECT_TRUE(kNullopt < makeOptional(2));
    EXPECT_TRUE(Optional<int>() < makeOptional(2));
    EXPECT_TRUE(Optional<int>() < 2);
    EXPECT_FALSE(makeOptional(1) < kNullopt);
    EXPECT_FALSE(makeOptional(1) < Optional<int>());
    EXPECT_FALSE(1 < Optional<int>());

    EXPECT_FALSE(kNullopt < Optional<int>());
    EXPECT_FALSE(Optional<int>() < kNullopt);
}

TEST(Optional, Destruction) {
    // create a reference counting class to check if we delete everything
    // we've created
    struct Track {
        Track(int& val) : mVal(val) {
            ++mVal.get();
        }
        Track(const Track& other) : mVal(other.mVal) {
            ++mVal.get();
        }
        Track(Track&& other) : mVal(other.mVal) {
            ++mVal.get();
        }
        Track& operator=(const Track& other) {
            --mVal.get();
            mVal = other.mVal;
            ++mVal.get();
            return *this;
        }
        Track& operator=(Track&& other) {
            --mVal.get();
            mVal = other.mVal;
            ++mVal.get();
            return *this;
        }

        ~Track() {
            --mVal.get();
        }

        std::reference_wrapper<int> mVal;
    };

    int counter = 0;
    {
        auto o = makeOptional(Track(counter));
        EXPECT_EQ(1, counter);
    }
    EXPECT_EQ(0, counter);

    {
        auto o = makeOptional(Track(counter));
        EXPECT_EQ(1, counter);
        o.clear();
        EXPECT_EQ(0, counter);
    }
    EXPECT_EQ(0, counter);

    {
        auto o = makeOptional(Track(counter));
        EXPECT_EQ(1, counter);
        int counter2 = 0;
        o.emplace(counter2);
        EXPECT_EQ(0, counter);
        EXPECT_EQ(1, counter2);
        o = Track(counter);
        EXPECT_EQ(1, counter);
        EXPECT_EQ(0, counter2);

        auto o2 = o;
        EXPECT_EQ(2, counter);
        EXPECT_EQ(0, counter2);
    }
    EXPECT_EQ(0, counter);

    {
        auto o = makeOptional(Track(counter));
        auto o2 = std::move(o);
        EXPECT_EQ(2, counter);
        o = o2;
        EXPECT_EQ(2, counter);
    }
    EXPECT_EQ(0, counter);
}

}  // namespace base
}  // namespace android
