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

#include "android/base/system/Win32UnicodeString.h"

#include <algorithm>

#include <windows.h>

#include <string.h>

namespace android {
namespace base {

Win32UnicodeString::Win32UnicodeString() : mStr(nullptr), mSize(0u) {}

Win32UnicodeString::Win32UnicodeString(const char* str, size_t len)
    : mStr(nullptr), mSize(0u) {
    reset(str, len);
}

Win32UnicodeString::Win32UnicodeString(const String& str)
    : mStr(nullptr), mSize(0u) {
    reset(str.c_str(), str.size());
}

Win32UnicodeString::Win32UnicodeString(size_t size) : mStr(nullptr), mSize(0u) {
    resize(size);
}

Win32UnicodeString::Win32UnicodeString(const wchar_t* str)
    : mStr(nullptr), mSize(0u) {
    size_t len = str ? wcslen(str) : 0u;
    resize(len);
    ::memcpy(mStr, str, len * sizeof(wchar_t));
}

Win32UnicodeString::Win32UnicodeString(const Win32UnicodeString& other)
    : mStr(nullptr), mSize(0u) {
    resize(other.mSize);
    ::memcpy(mStr, other.mStr, other.mSize * sizeof(wchar_t));
}

Win32UnicodeString::~Win32UnicodeString() {
    delete[] mStr;
}

Win32UnicodeString& Win32UnicodeString::operator=(
        const Win32UnicodeString& other) {
    resize(other.mSize);
    ::memcpy(mStr, other.mStr, other.mSize * sizeof(wchar_t));
    return *this;
}

wchar_t* Win32UnicodeString::data() {
    if (!mStr) {
        // Ensure the function never returns NULL.
        resize(0u);
    }
    return mStr;
}

String Win32UnicodeString::toString() const {
    return convertToUtf8(mStr, mSize);
}

void Win32UnicodeString::reset(const char* str, size_t len) {
    if (mStr) {
        delete[] mStr;
    }
    int utf16Len = MultiByteToWideChar(CP_UTF8,  // CodePage
                                       0,        // dwFlags
                                       str,      // lpMultiByteStr
                                       len,      // cbMultiByte
                                       NULL,     // lpWideCharStr
                                       0);       // cchWideChar
    mStr = new wchar_t[utf16Len + 1u];
    mSize = static_cast<size_t>(utf16Len);
    MultiByteToWideChar(CP_UTF8, 0, str, len, mStr, utf16Len);
    mStr[mSize] = L'\0';
}

void Win32UnicodeString::reset(const String& str) {
    reset(str.c_str(), str.size());
}

void Win32UnicodeString::resize(size_t newSize) {
    wchar_t* oldStr = mStr;
    mStr = new wchar_t[newSize + 1u];
    size_t copySize = std::min(newSize, mSize);
    ::memcpy(mStr, oldStr, copySize * sizeof(wchar_t));
    mStr[copySize] = L'\0';
    mStr[newSize] = L'\0';
    mSize = newSize;
    delete[] oldStr;
}

void Win32UnicodeString::append(const wchar_t* str) {
    append(str, wcslen(str));
}

void Win32UnicodeString::append(const wchar_t* str, size_t len) {
    // NOTE: This method should be rarely used, so don't try to optimize
    // storage with larger capacity values and exponential increments.
    size_t oldSize = size();
    resize(oldSize + len);
    memmove(mStr + oldSize, str, len);
}

void Win32UnicodeString::append(const Win32UnicodeString& other) {
    append(other.c_str(), other.size());
}

wchar_t* Win32UnicodeString::release() {
    wchar_t* result = mStr;
    mStr = NULL;
    mSize = 0u;
    return result;
}

// static
String Win32UnicodeString::convertToUtf8(const wchar_t* str) {
    return convertToUtf8(str, wcslen(str));
}

// static
String Win32UnicodeString::convertToUtf8(const wchar_t* str, size_t len) {
    String result;
    int utf8Len = WideCharToMultiByte(CP_UTF8,  // CodePage
                                      0,        // dwFlags
                                      str,      // lpWideCharStr
                                      len,      // cchWideChar
                                      NULL,     // lpMultiByteStr
                                      0,        // cbMultiByte
                                      NULL,     // lpDefaultChar
                                      NULL);    // lpUsedDefaultChar
    if (utf8Len > 0) {
        result.resize(static_cast<size_t>(utf8Len));
        WideCharToMultiByte(CP_UTF8, 0, str, len, &result[0], utf8Len, NULL,
                            NULL);
    }
    return result;
}

}  // namespace base
}  // namespace android
