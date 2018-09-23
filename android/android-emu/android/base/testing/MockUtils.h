// Copyright 2018 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/TypeTraits.h"

#include <memory>

// Helpers for using std::function-style mocks, allowing tests to replace the
// value of a std::function until it goes out of scope.
//
// Example:
// std::function<void()> mock = []() { default };
// {
//     auto handle = replaceMock(&mock, []() { do thing });
//     mock();  // do thing
// }
//
// mock();  // default

template <typename FunctionType>
class MockReplacementHandle {
    DISALLOW_COPY_AND_ASSIGN(MockReplacementHandle);

public:
    MockReplacementHandle() {}

    MockReplacementHandle(FunctionType* location, FunctionType original)
        : mLocation(location), mOriginal(std::move(original)) {}

    MockReplacementHandle(MockReplacementHandle&& other)
        : mLocation(std::move(other.mLocation)),
          mOriginal(std::move(other.mOriginal)) {
        other.mLocation = nullptr;
    }

    ~MockReplacementHandle() { reset(); }

    MockReplacementHandle& operator=(MockReplacementHandle&& other) {
        mLocation = std::move(other.mLocation);
        mOriginal = std::move(other.mOriginal);
        other.mLocation = nullptr;
        return *this;
    }

    void reset() {
        if (mLocation) {
            *mLocation = mOriginal;
            mLocation = nullptr;
        }
    }

private:
    FunctionType* mLocation = nullptr;
    FunctionType mOriginal;
};

template <typename FunctionType,
          typename U,
          typename = android::base::enable_if<
                  std::is_constructible<FunctionType, U>>>
MockReplacementHandle<FunctionType> replaceMock(FunctionType* location,
                                                U newValue) {
    FunctionType original = std::move(*location);
    *location = std::move(newValue);
    return MockReplacementHandle<FunctionType>(location, std::move(original));
}
