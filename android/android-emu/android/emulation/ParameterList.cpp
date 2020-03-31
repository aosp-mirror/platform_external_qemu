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

#include <algorithm>  // for count

#include "android/base/misc/StringUtils.h"  // for strDup

#ifdef _WIN32
#include "android/base/system/Win32Utils.h"
#endif

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

// Escapes individual parameter by surrounding it with " and escaping inner "
inline static std::string escapeCommandLine(const std::string& line) {
    size_t quoteCnt = std::count_if(line.begin(), line.end(), [](char x) {
        return x == '"' || x == '\\';
    });
    if (quoteCnt == 0)
        return '"' + line + '"';

    std::string escaped;
    escaped.reserve(quoteCnt + line.size() + 2);
    escaped += '"';
    for (int i = 0; i < line.size(); i++) {
        if (line[i] == '"' || line[i] == '\\') {
            escaped += '\\';
        }
        escaped += line[i];
    }
    escaped += '"';

    return escaped;
}

// Properly escapes individual parameters depending on the platform
inline static std::string quoteCommandLine(const std::string& line) {
#ifdef _WIN32
    return base::Win32Utils::quoteCommandLine(line);
#else
    return line.find(' ') == std::string::npos ? line : '\'' + line + '\'';
#endif
}

std::string ParameterList::toString(Format fmt) const {
    std::string result;
    for (size_t n = 0; n < mParams.size(); ++n) {
        if (n > 0) {
            result += ' ';
        }
        switch (fmt) {
            case Format::space:
                result += mParams[n];
                break;
            case Format::quoteWhenNeeded:
                result += quoteCommandLine(mParams[n]);
                break;
            case Format::alwaysQuoteAndEscape:
                result += escapeCommandLine(mParams[n]);
                break;
        }
    }
    return result;
}

char* ParameterList::toCStringCopy(Format fmt) const {
    return android::base::strDup(toString(fmt));
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
