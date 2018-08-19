// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Result.h"
#include "android/base/Compiler.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

using android::base::Err;
using android::base::Ok;
using android::base::Result;

using testing::Eq;
using testing::Optional;
enum class MyError { Failed1, Failed2 };

TEST(Result, VoidEnum) {
    {
        Result<void, MyError> result = Ok();
        EXPECT_TRUE(result.isOk());
        EXPECT_FALSE(result.isErr());
        EXPECT_FALSE(result.err().hasValue());
    }

    {
        Result<void, MyError> result = Err(MyError::Failed1);
        EXPECT_FALSE(result.isOk());
        EXPECT_TRUE(result.isErr());
        EXPECT_THAT(result.err(), Optional(Eq(MyError::Failed1)));
    }
}

TEST(Result, ReturnValue) {
    {
        Result<uint32_t, MyError> result = Ok(1);
        EXPECT_TRUE(result.isOk());
        EXPECT_FALSE(result.isErr());
        EXPECT_THAT(result.ok(), Optional(Eq(1)));
    }

    {
        Result<uint32_t, MyError> result = Err(MyError::Failed2);
        EXPECT_FALSE(result.isOk());
        EXPECT_TRUE(result.isErr());
        EXPECT_THAT(result.err(), Optional(Eq(MyError::Failed2)));
    }
}

TEST(Result, SameType) {
    Result<uint32_t, uint32_t> success = Ok(123);
    EXPECT_THAT(success.ok(), Optional(Eq(123)));
    EXPECT_FALSE(success.err().hasValue());
    EXPECT_EQ(success.unwrap(), 123);
    EXPECT_DEATH(success.unwrapErr(), "unwrapErr");

    Result<uint32_t, uint32_t> fail = Err(456u);
    EXPECT_THAT(fail.err(), Optional(Eq(456u)));
    EXPECT_FALSE(fail.ok().hasValue());
    EXPECT_DEATH(fail.unwrap(), "unwrap");
    EXPECT_EQ(fail.unwrapErr(), 456u);
}

TEST(Result, ComplexError) {
    struct ComplexError {
        ComplexError(MyError code, std::string message = "")
            : mCode(code), mMessage(message) {}

        const MyError mCode;
        const std::string mMessage;
    };

    {
        Result<void, ComplexError> result = Ok();
        EXPECT_TRUE(result.isOk());
    }

    {
        Result<void, ComplexError> result =
                Err(ComplexError(MyError::Failed1, "message"));
        EXPECT_TRUE(result.isErr());
        EXPECT_EQ(result.err().value().mCode, MyError::Failed1);
        EXPECT_EQ(result.err().value().mMessage, "message");
    }
}

TEST(Result, NonCopyable) {
    struct NonCopyable {
        DISALLOW_COPY_AND_ASSIGN(NonCopyable);

    public:
        NonCopyable(uint32_t value) : mValue(value) {}

        NonCopyable(NonCopyable&& other) : mValue(std::move(other.mValue)) {}

        const uint32_t mValue;
    };

    auto testFn = []() -> Result<void, NonCopyable> {
        return Err(NonCopyable(123));
    };

    Result<void, NonCopyable> result = testFn();
    ASSERT_TRUE(result.err().hasValue());
    EXPECT_EQ(result.err().value().mValue, 123);
}
