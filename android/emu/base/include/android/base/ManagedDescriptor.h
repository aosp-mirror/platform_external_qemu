// Copyright 2022 The Android Open Source Project
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

#include <optional>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

namespace android {
namespace base {
namespace internal {
#ifdef _WIN32
struct PlatformTraitWin32 {
    using DescriptorType = HANDLE;
    void closeDescriptor(DescriptorType handle) { CloseHandle(handle); }
};
#elif defined(__APPLE__) || defined(__linux__)
struct PlatformTraitUnixLike {
    using DescriptorType = int;
    void closeDescriptor(DescriptorType handle) { close(handle); }
};
#endif
}  // namespace internal


template <class PlatformTrait>
using DescriptorTypeBase = typename PlatformTrait::DescriptorType;

// An RAII wrapper for fd on *nix or HANDLE on Windows. The interfaces are similar to std::unique.
template <class PlatformTrait>
class ManagedDescriptorBase {
   public:
    using DescriptorType = typename PlatformTrait::DescriptorType;
    // The platformTrait argument is only used for testing to inject. platformTrait must live longer
    // than the created ManagedDescriptorBase object.
    ManagedDescriptorBase(DescriptorType rawDescriptor,
                          PlatformTrait& platformTrait = sPlatformTrait)
        : mRawDescriptor(rawDescriptor), mPlatformTrait(platformTrait) {}
    ManagedDescriptorBase(PlatformTrait& platformTrait = sPlatformTrait)
        : mRawDescriptor(std::nullopt), mPlatformTrait(platformTrait) {}
    ~ManagedDescriptorBase() {
        if (!mRawDescriptor.has_value()) {
            return;
        }
        mPlatformTrait.closeDescriptor(*mRawDescriptor);
    }

    ManagedDescriptorBase(const ManagedDescriptorBase&) = delete;
    ManagedDescriptorBase& operator=(const ManagedDescriptorBase&) = delete;
    ManagedDescriptorBase(ManagedDescriptorBase&& that)
        : mRawDescriptor(std::nullopt), mPlatformTrait(that.mPlatformTrait) {
        *this = std::move(that);
    }
    ManagedDescriptorBase& operator=(ManagedDescriptorBase&& that) {
        // Assume this->mPlatformTrait and that.mPlatformTrait are the same.
        if (this == &that) {
            return *this;
        }

        ManagedDescriptorBase other(mPlatformTrait);
        other.mRawDescriptor = std::move(mRawDescriptor);

        mRawDescriptor = std::move(that.mRawDescriptor);
        that.mRawDescriptor = std::nullopt;
        return *this;
    }

    std::optional<DescriptorType> release() {
        std::optional<DescriptorType> res = mRawDescriptor;
        mRawDescriptor = std::nullopt;
        return res;
    }

    std::optional<DescriptorType> get() { return mRawDescriptor; }

   private:
    std::optional<DescriptorType> mRawDescriptor;
    PlatformTrait& mPlatformTrait;

    // The PlatformTraits singleton used as the default value of the platformTrait parameter in the
    // constructor.
    inline static PlatformTrait sPlatformTrait = {};
};

#ifdef _WIN32
using DescriptorType = DescriptorTypeBase<internal::PlatformTraitWin32>;
using ManagedDescriptor = ManagedDescriptorBase<internal::PlatformTraitWin32>;
#elif defined(__APPLE__) || defined(__linux__)
using DescriptorType = DescriptorTypeBase<internal::PlatformTraitUnixLike>;
using ManagedDescriptor = ManagedDescriptorBase<internal::PlatformTraitUnixLike>;
#endif

}  // namespace base
}  // namespace android
