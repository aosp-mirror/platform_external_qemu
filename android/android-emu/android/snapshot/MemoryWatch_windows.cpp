// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/MemoryWatch.h"

#include "android/base/threads/FunctorThread.h"

#include <windows.h>

#include <cassert>
#include <utility>

namespace android {
namespace snapshot {

static MemoryAccessWatch* sWatch = nullptr;

class MemoryAccessWatch::Impl {
public:
    // TODO: implement the idle loader
    // base::FunctorThread mOnIdleThread;
    void* mExceptionHandler = nullptr;

    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;

    static LONG WINAPI handleException(EXCEPTION_POINTERS* exceptionInfo) {
        if (exceptionInfo->ExceptionRecord->ExceptionCode !=
            EXCEPTION_ACCESS_VIOLATION) {
            return (LONG)EXCEPTION_CONTINUE_SEARCH;
        }
        auto ptr =
                (void*)exceptionInfo->ExceptionRecord->ExceptionInformation[1];
        sWatch->mImpl->mAccessCallback(ptr);
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
    }
};

bool MemoryAccessWatch::isSupported() {
    return true;
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback)
    : mImpl(new Impl()) {
    mImpl->mAccessCallback = std::move(accessCallback);
    mImpl->mIdleCallback = std::move(idleCallback);
    sWatch = this;
    mImpl->mExceptionHandler =
            ::AddVectoredExceptionHandler(true, &Impl::handleException);
}

MemoryAccessWatch::~MemoryAccessWatch() {
    ::RemoveVectoredExceptionHandler(mImpl->mExceptionHandler);
    assert(sWatch == this);
    sWatch = nullptr;
}

bool MemoryAccessWatch::valid() const {
    return mImpl->mExceptionHandler != nullptr;
}

bool MemoryAccessWatch::registerMemoryRange(void* start, size_t length) {
    DWORD oldProtect;
    return ::VirtualProtect(start, length, PAGE_NOACCESS, &oldProtect) != 0;
}

void MemoryAccessWatch::doneRegistering() {}

bool MemoryAccessWatch::fillPage(void* ptr, size_t length, const void* data) {
    DWORD oldProtect;
    if (!::VirtualProtect(ptr, length, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    memcpy(ptr, data, length);
    return true;
}

}  // namespace snapshot
}  // namespace android
