// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/StringFormat.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"

namespace android {
namespace base {

// static
std::string Win32Utils::quoteCommandLine(StringView commandLine) {
    assert(commandLine.isNullTerminated());

    // If |commandLine| doesn't contain any problematic character, just return
    // it as-is.
    size_t n = strcspn(commandLine.data(), " \t\v\n\"");
    if (commandLine[n] == '\0') {
        return commandLine;
    }

    // Otherwise, we need to quote some of the characters.
    std::string out("\"");

    n = 0;
    while (commandLine[n]) {
        size_t num_backslashes = 0;
        while (commandLine[n] == '\\') {
            n++;
            num_backslashes++;
        }

        if (!commandLine[n]) {
            // End of string, if there are backslashes, double them.
            for (; num_backslashes > 0; num_backslashes--)
                out += "\\\\";
            break;
        }

        if (commandLine[n] == '"') {
            // Escape all backslashes as well as the quote that follows them.
            for (; num_backslashes > 0; num_backslashes--)
                out += "\\\\";
            out += "\\\"";
        } else {
            for (; num_backslashes > 0; num_backslashes--)
                out += '\\';
            out += commandLine[n];
        }
        n++;
    }

    // Add final quote.
    out += '"';
    return out;
}

// static
std::string Win32Utils::getErrorString(DWORD error_code) {
    std::string result;

    LPWSTR error_string = nullptr;

    DWORD format_result = FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, error_code, 0, (LPWSTR)&error_string, 2, nullptr);
    if (error_string) {
        result = Win32UnicodeString::convertToUtf8(error_string);
        ::LocalFree(error_string);
    } else {
        StringAppendFormat(&result,
                           "Error Code: %li (FormatMessage result: %li)",
                           error_code, format_result);
    }
    return result;
}

// static
Optional<_OSVERSIONINFOEXW> Win32Utils::getWindowsVersion() {
    HMODULE hLib;
    OSVERSIONINFOEXW ver;
    ver.dwOSVersionInfoSize = sizeof(ver);

    typedef DWORD (WINAPI *RtlGetVersion_t) (OSVERSIONINFOEXW*);

    hLib = LoadLibraryW(L"Ntdll.dll");
    if (!hLib) {
        return {};
    }

    auto f_RtlGetVersion =
            (RtlGetVersion_t)GetProcAddress(hLib, "RtlGetVersion");
    if (!f_RtlGetVersion || f_RtlGetVersion(&ver)) {
        // RtlGetVersion returns non-zero values in case of errors.
        ver = {};
    }

    FreeLibrary(hLib);
    return ver;
}

ServiceStatus Win32Utils::getServiceStatus(const char *srcName) {
    SC_HANDLE hsrc, hscm;
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded, ec;
    int result;

    hscm = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hscm) {
        return SVC_ERROR_OPENSCMANAGER;
    }

    hsrc = OpenService(hscm, srcName, SERVICE_QUERY_STATUS);
    if (!hsrc) {
        ec = GetLastError();
        CloseServiceHandle(hscm);
        if (ec == ERROR_SERVICE_DOES_NOT_EXIST)
            return SVC_NOT_FOUND;
        return SVC_ERROR_OPENSERVICE;
    }

    result = QueryServiceStatusEx(hsrc, SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssStatus, sizeof(ssStatus), &dwBytesNeeded);

    CloseServiceHandle(hsrc);
    CloseServiceHandle(hscm);

    if (result == 0) {
        return SVC_ERROR_QUERYSERVICE;
    }

    return static_cast<ServiceStatus>(ssStatus.dwCurrentState);
}


}  // namespace base
}  // namespace android
