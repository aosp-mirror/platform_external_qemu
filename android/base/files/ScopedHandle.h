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

#ifndef ANDROID_BASE_FILES_SCOPED_HANDLE_H
#define ANDROID_BASE_FILES_SCOPED_HANDLE_H

#if !defined(_WIN32) && !defined(_WIN64)
#error "Only compile this file when targetting Windows!"
#endif

#include "android/base/Compiler.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

namespace android {
namespace base {

// Helper class used to wrap a Win32 HANDLE that will be closed when
// the instance is destroyed, unless the release() method was called
// before that.
class ScopedHandle {
public:
    // Default destructor is used to wrap an invalid handle value.
    ScopedHandle() : handle_(INVALID_HANDLE_VALUE) {}

    // Constructor takes ownership of |handle|.
    explicit ScopedHandle(HANDLE handle) : handle_(handle) {}

    // Destructor calls close() method.
    ~ScopedHandle() { close(); }

    // Returns true iff the wrapped HANDLE value is valid.
    bool valid() const { return handle_ != INVALID_HANDLE_VALUE; }

    // Return current HANDLE value. Does _not_ transfer ownership.
    HANDLE get() const { return handle_; }

    // Return current HANDLE value, transferring ownership to the caller.
    HANDLE release() {
        HANDLE h = handle_;
        handle_ = INVALID_HANDLE_VALUE;
        return h;
    }

    // Close handle if it is not invalid.
    void close() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    // Swap the content of two ScopedHandle instances.
    void swap(ScopedHandle* other) {
        HANDLE handle = handle_;
        handle_ = other->handle_;
        other->handle_ = handle;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ScopedHandle);

    HANDLE handle_;
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_FILES_SCOPED_HANDLE_H
