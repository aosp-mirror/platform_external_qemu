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

#include <assert.h>

#include <fstream>
#include <set>
#include <string>

namespace android {
namespace base {

using std::ifstream;
using std::set;
using std::string;
using std::to_string;

void IniFile::setBackingFile(const string& filePath) {
    mBackingFilePath = filePath;
}

bool IniFile::isFileValid(const string& filePath) {
    FILE* fp = fopen(filePath.c_str(), "rt");
    if (!fp) {
        LOG(VERBOSE) << "Could not open .ini file: " << filePath;
        return false;
    }

// avoid reading a very large file that was passed by mistake
// this threshold is quite liberal.
#define MAX_INI_FILE_SIZE 655360
    fseek(fp, 0, SEEK_END);
    int const size = ftell(fp);
    fclose(fp);

    if (size < 0 || size > MAX_INI_FILE_SIZE) {
        LOG(WARNING) << ".ini File " << filePath << " too large (%ld bytes)"
                     << size;
        return false;
    }
    return true;
}

static inline bool isSpaceChar(unsigned uc) {
    return uc == ' ' || uc == '\r' || uc == '\t';
}

static inline bool isValueChar(unsigned uc) {
    return !isSpaceChar(uc);
}

static inline bool isKeyStartChar(unsigned uc) {
    return (uc - 'a' < 26) || (uc - 'A' < 26) || (uc == '_');
}

static inline bool isKeyChar(unsigned uc) {
    return isKeyStartChar(uc) || (uc - '0' < 10) || (uc == '.') || (uc == '-');
}

static inline string::const_iterator eatSpace(string::const_iterator citer,
                                              string::const_iterator cend) {
    for (; citer != cend; ++citer) {
        if (!isSpaceChar(*citer)) {
            return citer;
        }
    }
    return citer;
}

bool IniFile::read() {
    mData.clear();

    if (mBackingFilePath.empty()) {
        LOG(WARNING) << "Read called without a backing file!";
        return false;
    }

    ifstream inFile(mBackingFilePath);
    if (!isFileValid(mBackingFilePath) || !inFile) {
        LOG(WARNING) << "Failed to process .ini file " << mBackingFilePath
                     << " for reading.";
        return false;
    }

    LOG(VERBOSE) << "Parsing .ini file: " << mBackingFilePath;

    string line;
    int lineno = 0;
    while (std::getline(inFile, line)) {
        ++lineno;

        const string& cline = line;
        auto citer = std::begin(cline);
        auto cend = std::end(cline);
        citer = eatSpace(citer, cend);

        // Handle empty lines, comments.
        if (citer == cend) {
            LOG(VERBOSE) << "Line " << lineno << ": Skipped empty line.";
            continue;
        }
        if (*citer == '#' || *citer == ';') {
            LOG(VERBOSE) << "Line " << lineno << ": Skipped comment line.";
            continue;
        }

        // Extract and validate key.
        string key;
        if (!isKeyStartChar(*citer)) {
            LOG(VERBOSE) << "Line " << lineno
                         << ": Key does not start with a valid character."
                         << " Skipped line.";
            continue;
        }
        key += *citer;
        ++citer;
        for (; citer != cend && isKeyChar(*citer); ++citer) {
            key += *citer;
        }

        // Gobble the = sign.
        citer = eatSpace(citer, cend);
        if (citer == cend || *citer != '=') {
            LOG(VERBOSE) << "Line " << lineno
                         << ": Missing expected assignment operator (=)."
                         << " Skipped line.";
            continue;
        }
        ++citer;

        // Extract the value.
        string value;
        citer = eatSpace(citer, cend);
        for (; citer != cend && isValueChar(*citer); ++citer) {
            value += *citer;
        }

        // Ensure there's no invalid remainder.
        citer = eatSpace(citer, cend);
        if (citer != cend) {
            LOG(VERBOSE) << "Line " << lineno
                         << ": Contains invalid character in the value."
                         << " Skipped line.";
            continue;
        }

        // Everything parsed.
        mData[key] = value;
    }
    return true;
}

bool IniFile::writeCommon(bool discardEmpty) const {
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

    for (const auto& keyvalue : mData) {
        const string& key = keyvalue.first;
        const string& value = keyvalue.second;
        if (discardEmpty && value.empty()) {
            continue;
        }
        outFile << key << " = " << value << std::endl;
    }
    return true;
}

bool IniFile::write() const {
    return writeCommon(false);
}

bool IniFile::writeDiscardingEmpty() const {
    return writeCommon(true);
}

int IniFile::size() const {
    return mData.size();
}

bool IniFile::hasKey(const string& key) const {
    return mData.find(key) != std::end(mData);
}

string IniFile::getString(const string& key, const string& defaultValue) const {
    return hasKey(key) ? mData.at(key) : defaultValue;
}

int IniFile::getInt(const string& key, int defaultValue) const {
    if (!hasKey(key)) {
        return defaultValue;
    }
    char* end;
    errno = 0;
    const int result = strtol(mData.at(key).c_str(), &end, 10);
    if (errno || *end != 0) {
        LOG(VERBOSE) << "Malformed int value " << mData.at(key) << " for key "
                     << key;
        return defaultValue;
    }
    return result;
}

int64_t IniFile::getInt64(const string& key, int64_t defaultValue) const {
    if (!hasKey(key)) {
        return defaultValue;
    }
    char* end;
    errno = 0;
    const int64_t result = strtoll(mData.at(key).c_str(), &end, 10);
    if (errno || *end != 0) {
        LOG(VERBOSE) << "Malformed int64 value " << mData.at(key) << " for key "
                     << key;
        return defaultValue;
    }
    return result;
}

double IniFile::getDouble(const string& key, double defaultValue) const {
    if (!hasKey(key)) {
        return defaultValue;
    }
    char* end;
    errno = 0;
    const double result = strtod(mData.at(key).c_str(), &end);
    if (errno || *end != 0) {
        LOG(VERBOSE) << "Malformed double value " << mData.at(key)
                     << " for key " << key;
        return defaultValue;
    }
    return result;
}

bool IniFile::getBool(const string& key, bool defaultValue) const {
    const static set<string> falseValues = {"0", "no", "NO", "false", "FALSE"};
    const static set<string> trueValues = {"1", "yes", "YES", "true", "TRUE"};
    if (!hasKey(key)) {
        return defaultValue;
    }
    const string& value = mData.at(key);
    if (falseValues.find(value) != std::end(falseValues)) {
        return false;
    } else if (trueValues.find(value) != std::end(trueValues)) {
        return true;
    } else {
        LOG(VERBOSE) << "Malformed bool value " << value << " for key " << key;
        return defaultValue;
    }
}

IniFile::DiskSize IniFile::getDiskSize(const string& key,
                                       IniFile::DiskSize defaultValue) const {
    if (!hasKey(key)) {
        return defaultValue;
    }
    char* end;
    errno = 0;
    IniFile::DiskSize result = strtoll(mData.at(key).c_str(), &end, 10);
    bool malformed = (errno != 0);
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

    if (malformed) {
        LOG(VERBOSE) << "Malformed DiskSize value " << mData.at(key)
                     << " for key " << key;
        return defaultValue;
    }
    return result;
}

void IniFile::setString(const string& key, const string& value) {
    mData[key] = value;
}

void IniFile::setInt(const string& key, int value) {
    mData[key] = to_string(value);
}

void IniFile::setInt64(const string& key, int64_t value) {
    // long long is at least 64 bit in C++0x.
    mData[key] = to_string(static_cast<long long>(value));
}

void IniFile::setDouble(const string& key, double value) {
    mData[key] = to_string(value);
}

void IniFile::setBool(const string& key, bool value) {
    mData[key] = value ? "true" : "false";
}

void IniFile::setDiskSize(const string& key, int64_t value) {
    static const int64_t kKilo = 1024;
    static const int64_t kMega = 1024 * kKilo;
    static const int64_t kGiga = 1024 * kMega;

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

    mData[key] = to_string(value);
    if (suffix) {
        mData[key] += suffix;
    }
}

IniFile::iterator IniFile::begin() {
    return std::begin(mData);
}
IniFile::iterator IniFile::end() {
    return std::end(mData);
}
IniFile::const_iterator IniFile::begin() const {
    return std::begin(mData);
}
IniFile::const_iterator IniFile::end() const {
    return std::end(mData);
}

}  // namespace base
}  // namespace android
