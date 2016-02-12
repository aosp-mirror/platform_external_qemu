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

#include "SystemResult.h"

#ifdef _WIN32
#include "android/base/system/Win32Utils.h"

#include <winerror.h>
#endif  // _WIN32

#include <string.h>

namespace android {
namespace base {

SystemResult SystemResult::kSuccess =
        SystemResult(SystemResult::Type::Errno, 0);

SystemResult::SystemResult(Type type, ResultT result)
    : mType(type)
    , mResult(result) {
}

bool SystemResult::success() const {
    // This is currently true for all supported result types
    return mResult == 0;
}

String SystemResult::message() const {
    switch (mType) {
    case Type::Errno:
        return strerror(mResult);
#ifdef _WIN32
    case Type::WinSystemCode:
        return Win32Utils::getErrorString(mResult);
    case Type::WinHResult:
        return Win32Utils::getErrorString(HRESULT_CODE(mResult));
#endif  // _WIN32
    }

    return String();
}

SystemResult SystemResult::fromErrno(int error) {
    return SystemResult(Type::Errno, error);
}

#ifdef _WIN32
SystemResult SystemResult::fromWinSystem(DWORD error) {
    return SystemResult(Type::WinSystemCode, error);
}

SystemResult SystemResult::fromHResult(HRESULT error) {
    return SystemResult(Type::WinHResult, error);
}
#endif

}  // namespace base
}  // namespace android
