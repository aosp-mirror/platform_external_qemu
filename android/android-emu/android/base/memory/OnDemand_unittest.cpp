// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/memory/OnDemand.h"

#include "android/base/threads/ThreadPool.h"

#include <gtest/gtest.h>

using android::base::makeAtomicOnDemand;
using android::base::makeOnDemand;
using android::base::MemberOnDemandT;
using android::base::OnDemand;

namespace {

struct State {
    int created;
    int destroyed;
    int moved;

    friend bool operator==(const State& l, const State& r) {
        return l.created == r.created && l.destroyed == r.destroyed &&
               l.moved == r.moved;
    }
};

static State sState = {};

class OnDemandTest : public ::testing::Test {
public:
    void SetUp() override { sState = {}; }

    void TearDown() override { sState = {}; }
};

struct Test0 {
    Test0() { ++sState.created; }
    ~Test0() { ++sState.destroyed; }
    Test0(Test0&& other) { ++sState.moved; }
};

struct Test1 : Test0 {
    int n;
    Test1(int n) : n(n) {}
};

}  // namespace

TEST_F(OnDemandTest, createDestroy0) {
    {
        auto t = makeOnDemand<Test0>();
        EXPECT_EQ((State{0, 0}), sState);
        t.get();
        EXPECT_EQ((State{1, 0}), sState);
        t.get();
        EXPECT_EQ((State{1, 0}), sState);
        t.clear();
        EXPECT_EQ((State{1, 1}), sState);
        t.get();
        EXPECT_EQ((State{2, 1}), sState);
    }
    EXPECT_EQ((State{2, 2}), sState);
}

TEST_F(OnDemandTest, createDestroy1) {
    {
        auto t = makeOnDemand<Test1>(10);
        EXPECT_EQ((State{0, 0}), sState);
        t.get();
        EXPECT_EQ((State{1, 0}), sState);
        EXPECT_EQ(10, t.get().n);
    }
    EXPECT_EQ((State{1, 1}), sState);
}

TEST_F(OnDemandTest, createDestroyFun) {
    {
        auto t = makeOnDemand<Test1>([] { return 120; });
        EXPECT_EQ((State{0, 0}), sState);
        t.get();
        EXPECT_EQ((State{1, 0}), sState);
        EXPECT_EQ(120, t.get().n);
    }
    EXPECT_EQ((State{1, 1}), sState);
}

TEST_F(OnDemandTest, createMember) {
    {
        MemberOnDemandT<int, int> val{[] { return std::make_tuple(19); }};
        EXPECT_EQ(*val, 19);
    }
    {
        MemberOnDemandT<int, int> val{[] { return 21; }};
        EXPECT_EQ(*val, 21);
    }
    {
        MemberOnDemandT<int, int> val{42};
        EXPECT_EQ(*val, 42);
    }

    {
        MemberOnDemandT<std::pair<int, int>, int, int> val{4, 2};
        EXPECT_EQ(val->first, 4);
        EXPECT_EQ(val->second, 2);
    }
}

TEST_F(OnDemandTest, makeMore) {
    struct Test4 : Test0 {
        int n;
        char c;
        std::string s;
        bool b;

        Test4(int n, char c, std::string s, bool b = true)
            : n(n), c(c), s(s), b(b) {}
    };

    {
        auto t = makeOnDemand<Test4>(1, 'c', std::string("str"), false);
        EXPECT_EQ((State{0, 0}), sState);
        t.get();
        EXPECT_EQ((State{1, 0}), sState);
        EXPECT_EQ(1, t.get().n);
        EXPECT_EQ('c', t.get().c);
        EXPECT_STREQ("str", t->s.c_str());
        EXPECT_FALSE(t->b);
    }
    EXPECT_EQ((State{1, 1}), sState);

    {
        auto t = makeOnDemand<Test4>(1, 'c', std::string("str"));
        EXPECT_EQ((State{1, 1}), sState);
        t.get();
        EXPECT_EQ((State{2, 1}), sState);
        EXPECT_EQ(1, t.get().n);
        EXPECT_EQ('c', t.get().c);
        EXPECT_STREQ("str", t->s.c_str());
        EXPECT_TRUE(t->b);
    }
    EXPECT_EQ((State{2, 2}), sState);
}

TEST_F(OnDemandTest, clear0) {
    {
        auto t = makeOnDemand<Test0>();
        EXPECT_EQ((State{0, 0}), sState);
        t.clear();
        EXPECT_EQ((State{0, 0}), sState);
    }
    EXPECT_EQ((State{0, 0}), sState);

    {
        auto t = makeOnDemand<Test0>();
        EXPECT_EQ((State{0, 0}), sState);
        t.get();
        EXPECT_EQ((State{1, 0}), sState);
        t.clear();
        EXPECT_EQ((State{1, 1}), sState);
    }
    EXPECT_EQ((State{1, 1}), sState);
}

TEST_F(OnDemandTest, getPtr) {
    auto t = makeOnDemand<Test0>();
    EXPECT_EQ(&t.get(), t.ptr());
}

TEST_F(OnDemandTest, hasInstance) {
    auto t = makeOnDemand<Test0>();
    EXPECT_FALSE(t.hasInstance());
    t.get();
    EXPECT_TRUE(t.hasInstance());
    t.clear();
    EXPECT_FALSE(t.hasInstance());
}

TEST_F(OnDemandTest, explicitType) {
    struct T {
        MemberOnDemandT<Test1, int> var;
        T() : var([] { return std::make_tuple(10); }) {}
    };

    T t;
    EXPECT_FALSE(t.var.hasInstance());
    t.var.get();
    EXPECT_TRUE(t.var.hasInstance());
    EXPECT_EQ(10, t.var->n);
}

TEST_F(OnDemandTest, move) {
    {
        auto t = makeOnDemand<Test0>();
        auto t2 = std::move(t);
        EXPECT_EQ((State{0, 0, 0}), sState);
        t2.ptr();
        EXPECT_EQ((State{1, 0, 0}), sState);
        auto t3 = std::move(t2);
        EXPECT_EQ((State{1, 1, 1}), sState);
        EXPECT_TRUE(t3.hasInstance());
        EXPECT_FALSE(t2.hasInstance());
    }
    EXPECT_EQ((State{1, 2, 1}), sState);
}

//
// Now test how does OnDemand<> behave if constructed/destroyed from multiple
// threads.
//
using ThreadPool = android::base::ThreadPool<std::function<void()>>;
constexpr int kNumThreads = 200;

TEST_F(OnDemandTest, multiConstruct) {
    ThreadPool pool(kNumThreads, [](ThreadPool::Item&& f) { f(); });
    auto t = makeAtomicOnDemand<Test1>(100);
    Test1* values[kNumThreads] = {};
    for (int i = 0; i < kNumThreads; ++i) {
        pool.enqueue([&t, i, &values]() {
            values[i] = t.ptr();
            EXPECT_EQ(100, t.ptr()->n);
        });
    }
    pool.start();
    pool.done();
    pool.join();

    EXPECT_TRUE(t.hasInstance());
    EXPECT_EQ((State{1, 0}), sState);

    // Make sure all threads got the same pointer value.
    for (int i = 0; i < kNumThreads; ++i) {
        EXPECT_EQ(values[i], t.ptr());
    }
}

TEST_F(OnDemandTest, multiDestroy) {
    ThreadPool pool(kNumThreads, [](ThreadPool::Item&& f) { f(); });
    auto t = makeAtomicOnDemand<Test1>(100);
    t.get();
    EXPECT_EQ((State{1, 0}), sState);
    for (int i = 0; i < kNumThreads; ++i) {
        pool.enqueue([&t]() { t.clear(); });
    }
    pool.start();
    pool.done();
    pool.join();

    EXPECT_TRUE(!t.hasInstance());
    EXPECT_EQ((State{1, 1}), sState);
}

TEST_F(OnDemandTest, ifExists) {
    auto a = makeOnDemand<int>();
    a.ifExists([](int& i) { FAIL(); });
    *a = 0;
    a.ifExists([](int& i) { ++i; });
    EXPECT_EQ(a.get(), 1);
}
