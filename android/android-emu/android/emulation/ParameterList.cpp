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

#include "android/emulation/ParameterList.h"

#include "android/base/misc/StringUtils.h"
#include "android/base/system/Win32Utils.h"

namespace android {

ParameterList::ParameterList(int argc, char** argv)
    : mParams(argv, argv + argc) {}

ParameterList::ParameterList(std::initializer_list<std::string> lst)
    : mParams(lst) {}

size_t ParameterList::size() const {
    return mParams.size();
}

char** ParameterList::array() const {
    if (mArray.empty() && !mParams.empty()) {
        mArray.resize(mParams.size());
        for (size_t n = 0; n < mParams.size(); ++n) {
            mArray[n] = const_cast<char*>(&(mParams[n][0]));
        }
    }
    return &mArray[0];
}

// Properly escapes individual parameters depending on the platform
inline static std::string quoteCommandLine(const std::string& line) {
#ifdef _WIN32
  return base::Win32Utils::quoteCommandLine(line);
#else
  return line.find(' ') == std::string::npos ? line : '\'' + line + '\'';
#endif
}

std::string ParameterList::toString(bool quotes) const {
  std::string result;
  for (size_t n = 0; n < mParams.size(); ++n) {
    if (n > 0) {
      result += ' ';
    }
    result += quotes ? quoteCommandLine(mParams[n]) : mParams[n];
  }
  return result;
}

char* ParameterList::toCStringCopy(bool quotes) const {
    return android::base::strDup(toString(quotes));
}

void ParameterList::add(const std::string& param) {
    mParams.push_back(param);
}

void ParameterList::add(const ParameterList& other) {
    mParams.insert(mParams.end(), other.mParams.begin(), other.mParams.end());
}

void ParameterList::add(std::string&& param) {
    mParams.emplace_back(std::move(param));
}

void ParameterList::addFormatRaw(const char* format, ...) {
    va_list args;
    va_start(args, format);
    addFormatWithArgs(format, args);
    va_end(args);
}

void ParameterList::addFormatWithArgs(const char* format, va_list args) {
    add(android::base::StringFormatWithArgs(format, args));
}

void ParameterList::add2(const char* param1, const char* param2) {
    mParams.push_back(param1);
    mParams.push_back(param2);
}

void ParameterList::addIf(const char* param, bool flag) {
    if (flag) {
        add(param);
    }
}

void ParameterList::add2If(const char* param1, const char* param2) {
    if (param2) {
        add2(param1, param2);
    }
}

}  // namespace android
