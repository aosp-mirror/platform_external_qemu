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
#include <string.h>

#include <string>

namespace android {
namespace base {

using std::ifstream;
using std::ios_base;
using std::pair;
using std::string;
using std::to_string;
using std::vector;

void IniFile::setBackingFile(const string& filePath) {
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

void IniFile::parseFile(ifstream* inFile) {
    string line;
    int lineno = 0;
    // This is the line number we'd print at if the IniFile were immediately
    // written back. Unlike |line|, this will not be incremented for invalid
    // lines, since they're completely dropped.
    int outputLineno = 0;
    while (std::getline(*inFile, line)) {
        ++lineno;
        ++outputLineno;

        const string& cline = line;
        auto citer = std::begin(cline);
        auto cend = std::end(cline);
        citer = eat(citer, cend, isSpaceChar);

        // Handle empty lines, comments.
        if (citer == cend) {
            LOG(VERBOSE) << "Line " << lineno << ": Skipped empty line.";
            mComments.push_back({outputLineno, line});
            continue;
        }
        if (*citer == '#' || *citer == ';') {
            LOG(VERBOSE) << "Line " << lineno << ": Skipped comment line.";
            mComments.push_back({outputLineno, line});
            continue;
        }

        // Extract and validate key.
        const auto keyStartIter = citer;
        if (!isKeyStartChar(*citer)) {
            LOG(VERBOSE) << "Line " << lineno
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
            LOG(VERBOSE) << "Line " << lineno
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
            LOG(VERBOSE) << "Line " << lineno
                         << ": Contains invalid character in the value."
                         << " Skipped line.";
            --outputLineno;
            continue;
        }

        // Everything parsed.
        mKeys.push_back(key);
        mData[key] = value;
    }
}

bool IniFile::read() {
    mDirty = false;
    mData.clear();
    mKeys.clear();
    mComments.clear();

    if (mBackingFilePath.empty()) {
        LOG(WARNING) << "Read called without a backing file!";
        return false;
    }

    ifstream inFile(mBackingFilePath, ios_base::in | ios_base::ate);
    if (!inFile) {
        LOG(WARNING) << "Failed to process .ini file " << mBackingFilePath
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

    parseFile(&inFile);
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
    vector<pair<int, string>>::const_iterator commentIter =
            std::begin(mComments);
    for (const auto& key : mKeys) {
        ++lineno;

        // Write comments
        for (;
             commentIter != std::end(mComments) && lineno >= commentIter->first;
             ++commentIter, ++lineno) {
            outFile << commentIter->second << "\n";
        }

        const string& value = mData.at(key);
        if (discardEmpty && value.empty()) {
            continue;
        }
        outFile << key << " = " << value << "\n";
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

bool IniFile::hasKey(const string& key) const {
    return mData.find(key) != std::end(mData);
}

string IniFile::getString(const string& key, const string& defaultValue) const {
    auto citer = mData.find(key);
    return (citer == std::end(mData)) ? defaultValue : citer->second;
}

int IniFile::getInt(const string& key, int defaultValue) const {
    auto citer = mData.find(key);
    if (citer == std::end(mData)) {
        return defaultValue;
    }

    char* end;
    errno = 0;
    const int result = strtol(citer->second.c_str(), &end, 10);
    if (errno || *end != 0) {
        LOG(VERBOSE) << "Malformed int value " << citer->second << " for key "
                     << key;
        return defaultValue;
    }
    return result;
}

int64_t IniFile::getInt64(const string& key, int64_t defaultValue) const {
    auto citer = mData.find(key);
    if (citer == std::end(mData)) {
        return defaultValue;
    }

    char* end;
    errno = 0;
    const int64_t result = strtoll(citer->second.c_str(), &end, 10);
    if (errno || *end != 0) {
        LOG(VERBOSE) << "Malformed int64 value " << citer->second << " for key "
                     << key;
        return defaultValue;
    }
    return result;
}

double IniFile::getDouble(const string& key, double defaultValue) const {
    auto citer = mData.find(key);
    if (citer == std::end(mData)) {
        return defaultValue;
    }

    char* end;
    errno = 0;
    const double result = strtod(citer->second.c_str(), &end);
    if (errno || *end != 0) {
        LOG(VERBOSE) << "Malformed double value " << citer->second
                     << " for key " << key;
        return defaultValue;
    }
    return result;
}

static bool isBoolTrue(const string& value) {
    const char* cstr = value.c_str();
    return strcasecmp("yes", cstr) == 0 || strcasecmp("true", cstr) == 0 ||
           strcasecmp("1", cstr) == 0;
}

static bool isBoolFalse(const string& value) {
    const char* cstr = value.c_str();
    return strcasecmp("no", cstr) == 0 || strcasecmp("false", cstr) == 0 ||
           strcasecmp("0", cstr) == 0;
}

bool IniFile::getBool(const string& key, bool defaultValue) const {
    auto citer = mData.find(key);
    if (citer == std::end(mData)) {
        return defaultValue;
    }

    const string& value = citer->second;
    if (isBoolTrue(value)) {
        return true;
    } else if (isBoolFalse(value)) {
        return false;
    } else {
        LOG(VERBOSE) << "Malformed bool value " << value << " for key " << key;
        return defaultValue;
    }
}

bool IniFile::getBoolStr(const string& key, const string& defaultValue) const {
    return getBool(key, isBoolTrue(defaultValue));
}

// If not nullptr, |*outMalformed| is set to true if |valueStr| is malformed.
static IniFile::DiskSize parseDiskSize(const string& valueStr,
                                              IniFile::DiskSize defaultValue,
                                              bool* outMalformed) {
    if(outMalformed) {
        *outMalformed = false;
    }

    char* end;
    errno = 0;
    IniFile::DiskSize result = strtoll(valueStr.c_str(), &end, 10);
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
    IniFile::DiskSize result = parseDiskSize(mData.at(key), defaultValue,
                                             &malformed);

    LOG_IF(VERBOSE, malformed) << "Malformed DiskSize value " << mData.at(key)
                               << " for key " << key;
    return result;
}

IniFile::DiskSize IniFile::getDiskSizeStr(const string& key,
                                          const string& defaultValueStr) const {
    return getDiskSize(key, parseDiskSize(defaultValueStr, 0, nullptr));
}

void IniFile::updateData(const string& key, const string& value) {
    mDirty = true;
    auto result = mData.emplace(key, value);
    if (result.second) {
       // New element was created.
       mKeys.push_back(key);
    } else {
        // emplace does not update an existing value.
        result.first->second = value;
    }
}

void IniFile::setString(const string& key, const string& value) {
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
    updateData(key, value ? "true" : "false");
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

    auto valueStr = to_string(value);
    if (suffix) {
        valueStr += suffix;
    }
    updateData(key, valueStr);
}

}  // namespace base
}  // namespace android
