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

#include "android/base/files/ScopedFileHandle.h"
#include "android/base/StringFormat.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"

#include <winioctl.h>
#include <string.h>

namespace android {
namespace base {

// This declaration if from <ntifs.h> which is not provided by Mingw.
struct REPARSE_DATA_BUFFER {
  ULONG  ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG  Flags;
      WCHAR  PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR  PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  };
};

// static
std::string Win32Utils::quoteCommandLine(StringView commandLine) {
    // If |commandLine| doesn't contain any problematic character, just return
    // it as-is.
    size_t n = strcspn(commandLine.c_str(), " \t\v\n\"");
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

// Recursive internal function that tries to follow the target of |path|.
// If |path| is a regular file or directory, return true and sets |*result|
// to a copy of |path|. If |path| is a symbolic link or mount point, extract
// the target and call itself recursively. If |path| is invalid, return false.
static bool followLinkUnicode(const Win32UnicodeString& path,
                              Win32UnicodeString* result) {
    WIN32_FILE_ATTRIBUTE_DATA fileData = {};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileData)) {
        // Probably an invalid path or access violation.
        return false;
    }
    if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        // Cannot be a symbolic link, since it doesn't have a reparse point.
        *result = path;
        return true;
    }
    // Extract target path now, the trick is to use FILE_READ_EA to avoid
    // requiring admin privileges to open the file. See:
    // http://blog.kalmbach-software.de/2008/02/28/howto-correctly-read-reparse-data-in-vista/
    // NOTE: FILE_FLAG_BACKUP_SEMANTICS is needed to open a HANDLE to a
    // directory.
    ScopedFileHandle fileHandle(CreateFileW(path.c_str(),
                                            FILE_READ_EA,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_EXISTING,
                                            FILE_FLAG_BACKUP_SEMANTICS |
                                            FILE_FLAG_OPEN_REPARSE_POINT,
                                            NULL));
    if (!fileHandle.valid()) {
        return false;
    }

    std::string reparseData(MAXIMUM_REPARSE_DATA_BUFFER_SIZE, '\0');
    DWORD reparseDataLen = 0;
    if (!DeviceIoControl(fileHandle.get(),
                         FSCTL_GET_REPARSE_POINT,
                         NULL,
                         0,
                         &reparseData[0],
                         reparseData.size(),
                         &reparseDataLen,
                         NULL)) {
        return false;
    }
    fileHandle.close();

    auto data = reinterpret_cast<REPARSE_DATA_BUFFER*>(&reparseData[0]);
    Win32UnicodeString target;
    if (data->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
        auto symlink = &data->SymbolicLinkReparseBuffer;
        size_t wlen = symlink->SubstituteNameLength / sizeof(WCHAR);
        target.resize(wlen);
        ::memmove(target.data(),
                  &symlink->PathBuffer[symlink->SubstituteNameOffset],
                  wlen * sizeof(WCHAR));
    } else if (data->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
        auto mountpoint = &data->MountPointReparseBuffer;
        size_t wlen = mountpoint->SubstituteNameLength / sizeof(WCHAR);
        target.resize(wlen);
        ::memmove(target.data(),
                  &mountpoint->PathBuffer[mountpoint->SubstituteNameOffset],
                  wlen * sizeof(WCHAR));
    } else {
        // Invalid tag. Should not happen.
        return false;
    }

    // Recursive call.
    return followLinkUnicode(target, result);
}

// static
std::string followLink(StringView path) {
    Win32UnicodeString result;
    if (followLinkUnicode(Win32UnicodeString(path), &result)) {
        return result.toString();
    } else {
        return std::string();
    }
}

}  // namespace base
}  // namespace android
