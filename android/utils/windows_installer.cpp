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
#include <string>

#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/utils/windows_installer.h"

namespace android {

using base::String;
using base::StringAppendFormat;


namespace utils {

String GetWindowsError(DWORD error_code) {
    String result;

    LPSTR error_string = nullptr;

    DWORD format_result = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        nullptr,
        error_code,
        0,
        (LPSTR) &error_string,
        2,
        nullptr);
    if (error_string) {
        result.assign(error_string);
        ::LocalFree(error_string);
    }
    else {
        StringAppendFormat(&result,
                           "Error Code: %li (%li)",
                           error_code, format_result);
    }
    return result;
}

int32_t WindowsInstaller::GetVersion(const char* productDisplayName) {
    DWORD    cSubKeys=0;               // number of subkeys

    HKEY hProductsKey = 0;
    const char* registry_path =
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
        "Installer\\UserData\\S-1-5-18\\Products";
    LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_path, 0,
                               KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hProductsKey);
    if (result != ERROR_SUCCESS) {
        String error_string = GetWindowsError(result);
        printf("RegOpenKeyEx failed %li %s\n", result, error_string.c_str());
        return -1;
    }

    result = RegQueryInfoKeyW(hProductsKey, NULL, NULL, NULL, &cSubKeys, NULL,
                              NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS) {
        String error_string = GetWindowsError(result);
        printf("RegQueryInfoKeyW failed %li %s\n", result, error_string.c_str());
        return -1;
    }

    for (DWORD i=0; i<cSubKeys; i++)
    {
        const size_t MAX_KEY_LENGTH = 256;
        char product_guid[MAX_KEY_LENGTH];   // buffer for subkey name
        DWORD product_guid_len = MAX_KEY_LENGTH;
        result = RegEnumKeyExA(hProductsKey, i, product_guid, &product_guid_len,
                               NULL, NULL, NULL, NULL);
        if (result != ERROR_SUCCESS) {
            // this shouldn't happen
            continue;
        }

        std::string install_properties(registry_path);
        install_properties += "\\";
        install_properties += product_guid;
        install_properties += "\\InstallProperties";

        HKEY hInstallPropertiesKey = 0;
        LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, install_properties.c_str(),
                                   0, KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                                   &hInstallPropertiesKey);
        if (result != ERROR_SUCCESS) {
            // this shouldn't happen
            continue;
        }

        char display_name[4096];
        DWORD display_name_len = sizeof(display_name);
        const int RRF_SUBKEY_WOW6464KEY = 0x00010000;
        result = RegGetValue(hInstallPropertiesKey, NULL, "DisplayName",
                             RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, NULL,
                             display_name, &display_name_len);
        if (result != ERROR_SUCCESS) {
            // name might be too big to fit in buffer.
            // If so, it's not what we're looking for
            RegCloseKey(hInstallPropertiesKey);
            continue;
        }

        if (strstr(display_name, productDisplayName)) {
            // We've found the entry for productDisplayName
            DWORD version;
            DWORD version_len = sizeof(version);
            result = RegGetValue(hInstallPropertiesKey, NULL, "Version",
                                 RRF_RT_DWORD | RRF_SUBKEY_WOW6464KEY, NULL,
                                 &version, &version_len);
            if (result == ERROR_SUCCESS) {
                RegCloseKey(hInstallPropertiesKey);
                return (int32_t)version;
            }
            else {
                RegCloseKey(hInstallPropertiesKey);
                // this shouldn't happen
                return -1;
            }
        }
        RegCloseKey(hInstallPropertiesKey);
    }

    // productDisplayName not found in any of the subkeys, it's not installed
    return 0;
}

}  // namespace utils
}  // namespace android

#endif // _WIN32



