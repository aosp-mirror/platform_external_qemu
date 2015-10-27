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

#include "android/base/system/Win32Utils.h"

#include <algorithm>

#include <windows.h>
#include <string.h>

namespace android {
namespace base {

// static
String Win32Utils::quoteCommandLine(const char* commandLine) {
  // If |commandLine| doesn't contain any problematic character, just return
  // it as-is.
  size_t n = strcspn(commandLine, " \t\v\n\"");
  if (commandLine[n] == '\0') {
      return String(commandLine);
  }

  // Otherwise, we need to quote some of the characters.
  String out("\"");

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

Win32Utils::UnicodeString::UnicodeString() : mStr(nullptr), mSize(0u) {}

Win32Utils::UnicodeString::UnicodeString(const char* str, size_t len) : mStr(nullptr), mSize(0u) {
    reset(str, len);
}

Win32Utils::UnicodeString::UnicodeString(const String& str) : mStr(nullptr), mSize(0u) {
    reset(str.c_str(), str.size());
}

Win32Utils::UnicodeString::UnicodeString(size_t size) : mStr(nullptr), mSize(0u) {
    resize(size);
}

Win32Utils::UnicodeString::UnicodeString(const wchar_t* str) : mStr(nullptr), mSize(0u) {
    size_t len = str ? wcslen(str) : 0u;
    resize(len);
    ::memmove(mStr, str, len * sizeof(wchar_t));
    mSize = len;
}

Win32Utils::UnicodeString::~UnicodeString() {
    delete [] mStr;
}

String Win32Utils::UnicodeString::toString() const {
    String result;
    int utf8Len = WideCharToMultiByte(CP_UTF8,          // CodePage
                                      0,                // dwFlags
                                      mStr,             // lpWideCharStr
                                      mSize,            // cchWideChar
                                      NULL,             // lpMultiByteStr
                                      0,                // cbMultiByte
                                      NULL,             // lpDefaultChar
                                      NULL);            // lpUsedDefaultChar
    if (utf8Len > 0) {
        result.resize(static_cast<size_t>(utf8Len));
        WideCharToMultiByte(CP_UTF8, 0, mStr, mSize,
                            &result[0], utf8Len, NULL, NULL);
    }
    return result;
}

void Win32Utils::UnicodeString::reset(const char* str, size_t len) {
    if (mStr) {
        delete [] mStr;
    }
    int utf16Len = MultiByteToWideChar(CP_UTF8,         // CodePage
                                       0,               // dwFlags
                                       str,             // lpMultiByteStr
                                       len,             // cbMultiByte
                                       NULL,            // lpWideCharStr
                                       0);              // cchWideChar
    mStr = new wchar_t[utf16Len + 1u];
    mSize = static_cast<size_t>(utf16Len);
    MultiByteToWideChar(CP_UTF8, 0, str, len, mStr, utf16Len);
    mStr[mSize] = L'\0';
}

void Win32Utils::UnicodeString::reset(const String& str) {
    reset(str.c_str(), str.size());
}

void Win32Utils::UnicodeString::resize(size_t newSize) {
    wchar_t* oldStr = mStr;
    mStr = new wchar_t[newSize + 1u];
    size_t copySize = std::min(newSize, mSize);
    ::memmove(mStr, oldStr, copySize * sizeof(wchar_t));
    mStr[copySize] = L'\0';
    mStr[newSize] = L'\0';
    mSize = newSize;
    delete [] oldStr;
}

void Win32Utils::UnicodeString::append(const wchar_t* str) {
    append(str, wcslen(str));
}

void Win32Utils::UnicodeString::append(const wchar_t* str, size_t len) {
    size_t oldSize = size();
    resize(oldSize + len);
    memmove(mStr + oldSize, str, len);
}

void Win32Utils::UnicodeString::append(const UnicodeString& other) {
    append(other.c_str(), other.size());
}

wchar_t* Win32Utils::UnicodeString::release() {
    wchar_t* result = mStr;
    mStr = NULL;
    mSize = 0u;
    return result;
}

}  // namespace base
}  // namespace android
