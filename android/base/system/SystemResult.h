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

#pragma once

#include "android/base/String.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <memory>

namespace android {
namespace base {

class SystemResult {
public:
    enum class Type {
        Errno, // From errno
#ifdef _WIN32
        WinSystemCode, // Typically from GetLastError
        WinHResult, // From a HRESULT return value
#endif  // _WIN32
    };
    // Currently fits all types in the Type enum
    using ResultT = uint32_t;

    SystemResult(Type type, ResultT result);

    // Does the result indicate success?
    bool success() const;
    operator bool() const { return success(); }

    // Get a string describing the error if this is not a success
    android::base::String message() const;

    // Static convenience methods to quickly create results
    static SystemResult fromErrno(int error);
#ifdef _WIN32
    static SystemResult fromWinSystem(DWORD error);
    static SystemResult fromHResult(HRESULT error);
#endif

    static SystemResult kSuccess;
private:
    Type mType;
    ResultT mResult;
};

}  // namespace base
}  // namespace android
