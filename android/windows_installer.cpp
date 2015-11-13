/* Copyright (C) 2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#ifdef _WIN32

#include <windows.h>
#include <memory>
#include <string>

#include "android/base/files/ScopedRegKey.h"
#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/windows_installer.h"

// TODO: expose implementation in glib-mini-win32.c, update prebuilt and remove
static char*
utf16_to_utf8(const wchar_t* wstring, int wstring_len)
{
  int utf8_len = WideCharToMultiByte(CP_UTF8,          // CodePage
                                     0,                // dwFlags
                                     (LPWSTR) wstring, // lpWideCharStr
                                     wstring_len,      // cchWideChar
                                     NULL,             // lpMultiByteStr
                                     0,                // cbMultiByte
                                     NULL,             // lpDefaultChar
                                     NULL);            // lpUsedDefaultChar
  if (utf8_len == 0)
    return strdup("");

  char* result = (char*)malloc(utf8_len + 1);

  WideCharToMultiByte(CP_UTF8, 0, (LPWSTR) wstring, wstring_len,
                      result, utf8_len, NULL, NULL);
  result[utf8_len] = '\0';
  return result;
}

namespace android {

using base::String;
using base::StringAppendFormat;
using base::ScopedRegKey;

String GetWindowsError(DWORD error_code) {
    String result;

    LPSTR error_string = nullptr;

    DWORD format_result = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, error_code, 0, (LPSTR)&error_string, 2, nullptr);
    if (error_string) {
        result.assign(error_string);
        ::LocalFree(error_string);
    } else {
        StringAppendFormat(&result, "Error Code: %li (%li)", error_code,
                           format_result);
    }
    return result;
}

int32_t WindowsInstaller::getVersion(const char* productDisplayName) {
    DWORD cSubKeys = 0;  // number of subkeys
    REGSAM samDesired = KEY_READ;
    DWORD dwGetValueFlags = 0;

#if !_WIN64
    // 32 bit binary
    BOOL bIsWow64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &bIsWow64)) {
        if (bIsWow64) {
            // this is a 32 bit binary running on a 64 bit host, check the 64
            // bit registry
            samDesired |= KEY_WOW64_64KEY;
            const int RRF_SUBKEY_WOW6464KEY = 0x00010000;
            dwGetValueFlags |= RRF_SUBKEY_WOW6464KEY;
        }
    }
#endif

    HKEY hkey = 0;
    const char* registry_path =
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
            "Installer\\UserData\\S-1-5-18\\Products";
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_path, 0, samDesired,
                               &hkey);
    if (result != ERROR_SUCCESS) {
        String error_string = GetWindowsError(result);
        printf("RegOpenKeyEx failed %li %s\n", result, error_string.c_str());
        return -1;
    }
    ScopedRegKey hProductsKey(hkey);

    result = RegQueryInfoKeyW(hProductsKey.get(), NULL, NULL, NULL, &cSubKeys,
                              NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS) {
        String error_string = GetWindowsError(result);
        printf("RegQueryInfoKeyW failed %li %s\n", result,
               error_string.c_str());
        return -1;
    }

    for (DWORD i = 0; i < cSubKeys; i++) {
        const size_t kMaxKeyLength = 256;
        char product_guid[kMaxKeyLength];  // buffer for subkey name
        DWORD product_guid_len = kMaxKeyLength;
        result = RegEnumKeyExA(hProductsKey.get(), i, product_guid,
                               &product_guid_len, NULL, NULL, NULL, NULL);
        if (result != ERROR_SUCCESS) {
            String error_string = GetWindowsError(result);
            printf("RegEnumKeyExA failed %li %s\n", result,
                   error_string.c_str());
            continue;
        }

        std::string install_properties(registry_path);
        install_properties += "\\";
        install_properties += product_guid;
        install_properties += "\\InstallProperties";

        HKEY hkey = 0;
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, install_properties.c_str(), 0,
                              samDesired, &hkey);
        if (result != ERROR_SUCCESS) {
            // This is normal at least once
            continue;
        }
        ScopedRegKey hInstallPropertiesKey(hkey);

        DWORD display_name_size = 0;
        const WCHAR displayNameKey[] = L"DisplayName";
        result = RegGetValueW(hInstallPropertiesKey.get(), NULL, displayNameKey,
                              RRF_RT_REG_SZ | dwGetValueFlags, NULL, nullptr,
                              &display_name_size);
        if (result != ERROR_SUCCESS && ERROR_MORE_DATA != result) {
            String error_string = GetWindowsError(result);
            printf("RegGetValueW failed %li %s\n", result,
                   error_string.c_str());
            continue;
        }

        std::unique_ptr<WCHAR[]> display_name(new WCHAR[display_name_size / 2]);
        result = RegGetValueW(hInstallPropertiesKey.get(), NULL, displayNameKey,
                              RRF_RT_REG_SZ | dwGetValueFlags, NULL,
                              display_name.get(), &display_name_size);
        if (result != ERROR_SUCCESS) {
            String error_string = GetWindowsError(result);
            printf("RegGetValueW failed %li %s\n", result,
                   error_string.c_str());
            continue;
        }
        std::unique_ptr<const char[]> display_name8(
                utf16_to_utf8(display_name.get(), wcslen(display_name.get())));

        if (0 == strcmp(display_name8.get(), productDisplayName)) {
            // We've found the entry for productDisplayName
            DWORD version;
            DWORD version_len = sizeof(version);
            result = RegGetValue(hInstallPropertiesKey.get(), NULL, "Version",
                                 RRF_RT_DWORD | dwGetValueFlags, NULL, &version,
                                 &version_len);
            if (result == ERROR_SUCCESS) {
                return (int32_t)version;
            } else {
                // this shouldn't happen
                return -1;
            }
        }
    }

    // productDisplayName not found in any of the subkeys, it's not installed
    return 0;
}

}  // namespace android

#endif  // _WIN32
