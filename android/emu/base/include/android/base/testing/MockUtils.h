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

// Helpers for using replaceable pointer or std::function-style mocks, allowing
// tests to replace a value until the end of the scope.
//
// std::function example:
//
// std::function<void()> mock = []() { default };
// {
//     auto handle = replaceMock(&mock, []() { do thing });
//     mock();  // do thing
// }
// mock();  // default
//
// Pointer example:
//
// static MyPointer* sMock = nullptr;
// {
//     MockMyPointer currentMock;
//     auto handle = replaceMock(sMock, &currentMock);
//     assert(sMock == &currentMock);
// }
// assert(sMock == nullptr);

template <typename T>
class MockReplacementHandle {
    DISALLOW_COPY_AND_ASSIGN(MockReplacementHandle);

public:
    MockReplacementHandle() {}

    MockReplacementHandle(T* location, T original)
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
    T* mLocation = nullptr;
    T mOriginal;
};

template <typename T,
          typename U,
          typename = android::base::enable_if<
                  std::is_constructible<T, U>>>
MockReplacementHandle<T> replaceMock(T* location, U newValue) {
    T original = std::move(*location);
    *location = std::move(newValue);
    return MockReplacementHandle<T>(location, std::move(original));
}
