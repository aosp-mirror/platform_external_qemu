// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/IniFile.h"

#include "android/base/Log.h"
#include "android/base/system/System.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <iomanip>
#include <istream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>

#include <assert.h>
#include <string.h>

namespace android {
namespace base {

using std::ifstream;
using std::ios_base;
using std::pair;
using std::string;
using std::to_string;
using std::vector;

IniFile::IniFile(const char* data, int size) {
    readFromMemory(StringView(data, size));
}

void IniFile::setBackingFile(StringView filePath) {
    // We have no idea what the new backing file contains.
    mDirty = true;
    mBackingFilePath = filePath;
}

static bool isSpaceChar(unsigned uc) {
    return uc == ' ' || uc == '\r' || uc == '\t';
}

static bool isValueChar(unsigned uc) {
    return uc != '\r' && uc != '\n';
}

static bool isKeyStartChar(unsigned uc) {
    static const unsigned smallRange = 'z' - 'a' + 1;
    static const unsigned capitalRange = 'Z' - 'A' + 1;
    return (uc - 'a' < smallRange) || (uc - 'A' < capitalRange) || (uc == '_');
}

static bool isKeyChar(unsigned uc) {
    static const unsigned numRange = '9' - '0' + 1;
    return isKeyStartChar(uc) || (uc - '0' < numRange) || (uc == '.') ||
           (uc == '-');
}

template <typename CIterator, typename Pred>
static CIterator eat(CIterator citer, CIterator cend, Pred pred) {
    while (citer != cend && pred(*citer)) {
        ++citer;
    }
    return citer;
}

void IniFile::parseStream(std::istream* in, bool keepComments) {
    string line;
    int lineno = 0;
    // This is the line number we'd print at if the IniFile were immediately
    // written back. Unlike |line|, this will not be incremented for invalid
    // lines, since they're completely dropped.
    int outputLineno = 0;
    while (std::getline(*in, line)) {
        ++lineno;
        ++outputLineno;

        const string& cline = line;
        auto citer = std::begin(cline);
        auto cend = std::end(cline);
        citer = eat(citer, cend, isSpaceChar);

        // Handle empty lines, comments.
        if (citer == cend) {
            VLOG(avd_config) << "Line " << lineno << ": Skipped empty line.";
            if (keepComments) {
                mComments.emplace_back(outputLineno, std::move(line));
            }
            continue;
        }
        if (*citer == '#' || *citer == ';') {
            VLOG(avd_config) << "Line " << lineno << ": Skipped comment line.";
            if (keepComments) {
                mComments.emplace_back(outputLineno, std::move(line));
            }
            continue;
        }

        // Extract and validate key.
        const auto keyStartIter = citer;
        if (!isKeyStartChar(*citer)) {
            VLOG(avd_config) << "Line " << lineno
                             << ": Key does not start with a valid character."
                             << " Skipped line.";
            --outputLineno;
            continue;
        }
        ++citer;
        citer = eat(citer, cend, isKeyChar);
        auto key = string(keyStartIter, citer);

        // Gobble the = sign.
        citer = eat(citer, cend, isSpaceChar);
        if (citer == cend || *citer != '=') {
            VLOG(avd_config) << "Line " << lineno
                             << ": Missing expected assignment operator (=)."
                             << " Skipped line.";
            --outputLineno;
            continue;
        }
        ++citer;

        // Extract the value.
        citer = eat(citer, cend, isSpaceChar);
        const auto valueStartIter = citer;
        citer = eat(citer, cend, isValueChar);
        auto value = string(valueStartIter, citer);
        // Remove trailing space.
        auto trailingSpaceIter = eat(value.rbegin(), value.rend(), isSpaceChar);
        value.erase(trailingSpaceIter.base(), value.end());

        // Ensure there's no invalid remainder.
        citer = eat(citer, cend, isSpaceChar);
        if (citer != cend) {
            VLOG(avd_config) << "Line " << lineno
                             << ": Contains invalid character in the value."
                             << " Skipped line.";
            --outputLineno;
            continue;
        }

        // Everything parsed.
        auto insertRes = mData.emplace(std::move(key), std::string());
        insertRes.first->second = std::move(value);
        if (insertRes.second) {
            mOrderList.push_back(&*insertRes.first);
        }
    }
}

bool IniFile::read(bool keepComments) {
    mDirty = false;
    mData.clear();
    mOrderList.clear();
    mComments.clear();

    if (mBackingFilePath.empty()) {
        LOG(WARNING) << "Read called without a backing file!";
        return false;
    }

    ifstream inFile(mBackingFilePath, ios_base::in | ios_base::ate);
    if (!inFile) {
        VLOG(avd_config) << "Failed to process .ini file " << mBackingFilePath
                         << " for reading.";
        return false;
    }

    // avoid reading a very large file that was passed by mistake
    // this threshold is quite liberal.
    static const auto kMaxIniFileSize = ifstream::pos_type(655360);
    static const auto kInvlidPos = ifstream::pos_type(-1);
    const ifstream::pos_type endPos = inFile.tellg();
    inFile.seekg(0, ios_base::beg);
    const ifstream::pos_type begPos = inFile.tellg();
    if (begPos == kInvlidPos || endPos == kInvlidPos ||
        endPos - begPos > kMaxIniFileSize) {
        LOG(WARNING) << ".ini File " << mBackingFilePath << " too large ("
                     << (endPos - begPos) << " bytes)";
        return false;
    }

    parseStream(&inFile, keepComments);
    return true;
}

bool IniFile::readFromMemory(StringView data)
{
    mDirty = false;
    mData.clear();
    mOrderList.clear();
    mComments.clear();

    // Create a streambuf that's able to do a single pass over an array only.
    class OnePassIBuf : public std::streambuf {
    public:
        OnePassIBuf(StringView data) {
            setg(const_cast<char*>(data.data()), const_cast<char*>(data.data()),
                 const_cast<char*>(data.data()) + data.size());
        }
    };
    OnePassIBuf ibuf(data);
    std::istream in(&ibuf);
    if (!in) {
        LOG(WARNING) << "Failed to process input data for reading.";
        return false;
    }

    parseStream(&in, true);
    return true;
}

bool IniFile::writeCommon(bool discardEmpty) {
    if (mBackingFilePath.empty()) {
        LOG(WARNING) << "Write called without a backing file!";
        return false;
    }

    std::ofstream outFile(mBackingFilePath,
                          std::ios_base::out | std::ios_base::trunc);
    if (!outFile) {
        LOG(WARNING) << "Failed to open .ini file " << mBackingFilePath
                     << " for writing.";
        return false;
    }

    int lineno = 0;
    auto commentIter = std::begin(mComments);
    for (const auto& pair : mOrderList) {
        ++lineno;

        // Write comments
        for (;
             commentIter != std::end(mComments) && lineno >= commentIter->first;
             ++commentIter, ++lineno) {
            outFile << commentIter->second << "\n";
        }


        const string& value = pair->second;
        if (discardEmpty && value.empty()) {
            continue;
        }
        const string& key = pair->first;
        outFile << key << " = " << value << '\n';
    }

    mDirty = false;
    return true;
}

bool IniFile::write() {
    return writeCommon(false);
}

bool IniFile::writeDiscardingEmpty() {
    return writeCommon(true);
}

bool IniFile::writeIfChanged() {
    return !mDirty || writeCommon(false);
}

int IniFile::size() const {
    return static_cast<int>(mData.size());
}

bool IniFile::hasKey(StringView key) const {
    return mData.find(key) != std::end(mData);
}

std::string IniFile::makeValidKey(StringView str) {
    std::ostringstream res;
    res << std::hex << std::uppercase;
    res << '_'; // mark all keys passed through this function with a leading
                // underscore
    for (char c : str) {
        if (isKeyChar(c)) {
            res << c;
        } else {
            res << '.' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    return res.str();
}

string IniFile::makeValidValue(StringView str) {
    std::ostringstream res;
    for (const char* ch = str.begin(); ch != str.end() && *ch != '\0'; ch++) {
        if (*ch == '%')
            res << *ch;
        res << *ch;
    }
    return res.str();
}

// Substitute environment variables inside a string.
// Substitution is based on %ENVIRONMENT_VARIABLE%
// Double %% is treated as an escape.
// If a string is not terminated (i.e a starting %, but
// no ending %) the whole string will be returned.
//
// For example:
// "%PAGER%" => "less"
// "%PAG%%ER%" => "" ("PAG%ER" is not a valid name)
// "%FOO" => "%FOO" (Not terminated)
// "%%HI%%" => "%HI%" (Escaped)
// Note that: %%%USER%%% is parsed as %(USER)% and not %(USER%)%
static string envSubst(const StringView fix) {
    size_t len = fix.size();

    string res;
    string var;
    string* curr = &res;
    for (unsigned int i = 0; i < len; i++) {
        char ch = fix[i];

        // A normal character will be added to the current string
        if (ch != '%') {
            curr->push_back(ch);
            continue;
        }

        // Let's see if we are closing
        if (curr == &var) {
            string env = System::get()->envGet(var);
            if (env.empty()) {
                LOG(WARNING)
                        << "Environment variable " << var << " is not set";
            }
            res.append(env);
            var.clear();
            curr = &res;
            continue;
        }

        // Check if we have a case of escapism..
        // Note that len-1 >= 0
        char next = (i < len - 1) ? fix[i + 1] : '\0';

        if (next == '%') {
            // Escaped, let's skip it.
            curr->push_back(ch);
            i++;
        } else {
            // We open and start a env variable name.
            curr = &var;
        }
    }

    // Return full string for unterminated piece.
    if (curr == &var) {
        res.push_back('%');
        res.append(var);
    }

    return res;
}

string IniFile::getString(const string& key, StringView defaultValue) const {
    auto citer = mData.find(key);
    return envSubst((citer == std::end(mData)) ? defaultValue : StringView(citer->second));
}

int IniFile::getInt(const string& key, int defaultValue) const {
    if (mData.find(key) == std::end(mData)) {
        return defaultValue;
    }

    auto value = getString(key, "");
    char* end;
    errno = 0;
    const int result = strtol(value.c_str(), &end, 10);
    if (errno || *end != 0) {
        VLOG(avd_config) << "Malformed int value " << value << " for key "
                         << key;
        return defaultValue;
    }
    return result;
}

int64_t IniFile::getInt64(const string& key, int64_t defaultValue) const {
    if (mData.find(key) == std::end(mData)) {
        return defaultValue;
    }

    auto value = getString(key, "");
    char* end;
    errno = 0;
    const int64_t result = strtoll(value.c_str(), &end, 10);
    if (errno || *end != 0) {
        VLOG(avd_config) << "Malformed int64 value " << value << " for key "
                         << key;
        return defaultValue;
    }
    return result;
}

double IniFile::getDouble(const string& key, double defaultValue) const {
    if (mData.find(key) == std::end(mData)) {
        return defaultValue;
    }

    auto value = getString(key, "");
    char* end;
    errno = 0;
    const double result = strtod(value.c_str(), &end);
    if (errno || *end != 0) {
        VLOG(avd_config) << "Malformed double value " << value << " for key "
                         << key;
        return defaultValue;
    }
    return result;
}

static bool isBoolTrue(StringView value) {
    const char* cstr = value.data();
    const size_t size = value.size();

    return strncasecmp("yes", cstr, size) == 0 ||
           strncasecmp("true", cstr, size) == 0 ||
           strncasecmp("1", cstr, size) == 0;
}

static bool isBoolFalse(StringView value) {
    const char* cstr = value.data();
    const size_t size = value.size();
    return strncasecmp("no", cstr, size) == 0 ||
           strncasecmp("false", cstr, size) == 0 ||
           strncasecmp("0", cstr, size) == 0;
}

bool IniFile::getBool(const string& key, bool defaultValue) const {
    if (mData.find(key) == std::end(mData)) {
        return defaultValue;
    }

    const string& value = getString(key, "");
    if (isBoolTrue(value)) {
        return true;
    } else if (isBoolFalse(value)) {
        return false;
    } else {
        VLOG(avd_config) << "Malformed bool value " << value << " for key "
                         << key;
        return defaultValue;
    }
}

bool IniFile::getBool(const string& key, StringView defaultValue) const {
    return getBool(key, isBoolTrue(defaultValue));
}

// If not nullptr, |*outMalformed| is set to true if |valueStr| is malformed.
static IniFile::DiskSize parseDiskSize(StringView valueStr,
                                       IniFile::DiskSize defaultValue,
                                       bool* outMalformed) {
    if(outMalformed) {
        *outMalformed = false;
    }

    char* end;
    errno = 0;
    IniFile::DiskSize result = strtoll(c_str(valueStr), &end, 10);
    bool malformed = (errno != 0);
    if (!malformed) {
        switch (*end) {
            case 0:
                break;
            case 'k':
            case 'K':
                result *= 1024ULL;
                break;
            case 'm':
            case 'M':
                result *= 1024 * 1024ULL;
                break;
            case 'g':
            case 'G':
                result *= 1024 * 1024 * 1024ULL;
                break;
            default:
                malformed = true;
        }
    }

    if (malformed) {
        if(outMalformed) {
            *outMalformed = true;
        }
        return defaultValue;
    }
    return result;
}

IniFile::DiskSize IniFile::getDiskSize(const string& key,
                                       IniFile::DiskSize defaultValue) const {
    if (!hasKey(key)) {
        return defaultValue;
    }
    bool malformed = false;
    auto value = getString(key, "");
    IniFile::DiskSize result = parseDiskSize(value, defaultValue, &malformed);

    LOG_IF(VERBOSE, malformed)
            << "Malformed DiskSize value " << value << " for key " << key;
    return result;
}

IniFile::DiskSize IniFile::getDiskSize(const string& key,
                                       StringView defaultValue) const {
    return getDiskSize(key, parseDiskSize(defaultValue, 0, nullptr));
}

void IniFile::updateData(const string& key, string&& value) {
    mDirty = true;
    // note: may not move here, as it's currently unspecified if failed
    //  insertion moves from |value| or not. Move it separately instead.
    auto result = mData.emplace(key, std::string());
    result.first->second = std::move(value);
    if (result.second) {
        // New element was created.
        mOrderList.push_back(&*result.first);
    }
}

void IniFile::setString(const string& key, StringView value) {
    updateData(key, value);
}

void IniFile::setInt(const string& key, int value) {
    updateData(key, to_string(value));
}

void IniFile::setInt64(const string& key, int64_t value) {
    // long long is at least 64 bit in C++0x.
    updateData(key, to_string(static_cast<long long>(value)));
}

void IniFile::setDouble(const string& key, double value) {
    updateData(key, to_string(value));
}

void IniFile::setBool(const string& key, bool value) {
    updateData(key, value ? StringView("true") : "false");
}

void IniFile::setDiskSize(const string& key, DiskSize value) {
    static const DiskSize kKilo = 1024;
    static const DiskSize kMega = 1024 * kKilo;
    static const DiskSize kGiga = 1024 * kMega;

    char suffix = 0;
    if (value >= kGiga && !(value % kGiga)) {
        value /= kGiga;
        suffix = 'g';
    } else if (value >= kMega && !(value % kMega)) {
        value /= kMega;
        suffix = 'm';
    } else if (value >= kKilo && !(value % kKilo)) {
        value /= kKilo;
        suffix = 'k';
    }

    auto valueStr = to_string(value);
    if (suffix) {
        valueStr += suffix;
    }
    updateData(key, std::move(valueStr));
}

}  // namespace base
}  // namespace android
