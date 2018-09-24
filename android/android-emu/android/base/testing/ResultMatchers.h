// Copyright (C) 2018 The Android Open Source Project
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
#pragma once

#include "android/base/Result.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android {
namespace base {

template <typename T,
          typename E,
          typename = typename std::enable_if<!std::is_void<T>::value>::type>
void PrintTo(const Result<T, E>& param, std::ostream* os) {
    if (param.ok()) {
        *os << "Ok(" << ::testing::PrintToString(param.ok().value()) << ")";
    } else {
        *os << "Err(" << ::testing::PrintToString(param.err().value()) << ")";
    }
}

template <typename E>
void PrintTo(const Result<void, E>& param, std::ostream* os) {
    if (param.ok()) {
        *os << "Ok()";
    } else {
        *os << "Err(" << ::testing::PrintToString(param.err().value()) << ")";
    }
}

MATCHER(IsOk, "Ok()") {
    return arg.ok();
}

MATCHER_P(IsOk, value, "Ok(" + ::testing::PrintToString(value) + ")") {
    if (!arg.ok()) {
        return false;
    }

    return ::testing::Matches(value)(arg.ok().value());
}

MATCHER_P(IsErr, err, "Err(" + ::testing::PrintToString(err) + ")") {
    if (!arg.err()) {
        return false;
    }

    return ::testing::Matches(err)(arg.err().value());
}

}  // namespace base
}  // namespace android
