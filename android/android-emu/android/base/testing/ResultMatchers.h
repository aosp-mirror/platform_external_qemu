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

    return arg.ok().value() == value;
}

MATCHER_P(IsErr, err, "Err(" + ::testing::PrintToString(err) + ")") {
    if (!arg.err()) {
        return false;
    }

    return arg.err().value() == err;
}

}  // namespace base
}  // namespace android
