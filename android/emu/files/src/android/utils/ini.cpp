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

#include "android/utils/ini.h"

#include <iterator>  // for next
#include <ostream>   // for basic_ostream::operator<<
#include <string>    // for allocator

#include "aemu/base/Log.h"               // for LOG, LogMessage

#include "aemu/base/misc/StringUtils.h"  // for strDup
#include "aemu/base/logging/LogSeverity.h"     // for EMULATOR_LOG_VERBOSE

#include "android/base/files/IniFile.h"     // for IniFile, IniFile::const_i...

using CIniFile = ::CIniFile;
using BaseIniFile = android::base::IniFile;
using android::base::strDup;

static auto asBaseIniFile(CIniFile* i) -> BaseIniFile* {
    return reinterpret_cast<BaseIniFile*>(i);
}

void iniFile_free(CIniFile* i) {
    delete asBaseIniFile(i);
}

auto iniFile_hasKey(CIniFile* i, const char* key) -> bool {
    return asBaseIniFile(i)->hasKey(key);
}

auto iniFile_getPairCount(CIniFile* i) -> int {
    return asBaseIniFile(i)->size();
}

auto iniFile_newEmpty(const char* /*filepath*/) -> CIniFile* {
    return reinterpret_cast<CIniFile*>(new BaseIniFile());
}

auto iniFile_newFromFile(const char* filepath) -> CIniFile* {
    auto* ini = new BaseIniFile(filepath);
    ini->read();
    return reinterpret_cast<CIniFile*>(ini);
}

// NB: Returns 0 on success.
auto iniFile_saveToFile(CIniFile* f, const char* filepath) -> int {
    asBaseIniFile(f)->setBackingFile(filepath ? filepath : "");
    return static_cast<int>(!asBaseIniFile(f)->write());
}

// NB: Returns 0 on success.
auto iniFile_saveToFileClean(CIniFile* f, const char* filepath) -> int {
    asBaseIniFile(f)->setBackingFile(filepath ? filepath : "");
    return static_cast<int>(!asBaseIniFile(f)->writeDiscardingEmpty());
}

auto iniFile_getEntry(CIniFile* f, int index, char** key, char** value) -> int {
    BaseIniFile* ini = asBaseIniFile(f);
    if (index >= ini->size()) {
        LOG(DEBUG) << "Index " << index
                     << "exceeds the number of ini file entries "
                     << ini->size();
        return -1;
    }

    const auto it = std::next(ini->begin(), index);
    *key = strDup(*it);
    *value = strDup(ini->getString(*it, ""));
    return 0;
}

auto iniFile_getString(CIniFile* f, const char* key, const char* defaultValue)
        -> char* {
    if (defaultValue != nullptr) {
        return strDup(asBaseIniFile(f)->getString(key, defaultValue));
    }
    // |defaultValue| of NULL is not handled well by getString.
    if (!asBaseIniFile(f)->hasKey(key)) {
        return nullptr;
    }
    return strDup(asBaseIniFile(f)->getString(key, ""));
}

auto iniFile_getInteger(CIniFile* f, const char* key, int defaultValue) -> int {
    return asBaseIniFile(f)->getInt(key, defaultValue);
}

auto iniFile_getDouble(CIniFile* f, const char* key, double defaultValue)
        -> double {
    return asBaseIniFile(f)->getDouble(key, defaultValue);
}

auto iniFile_getBoolean(CIniFile* f, const char* key, const char* defaultValue)
        -> int {
    return static_cast<int>(asBaseIniFile(f)->getBool(key, defaultValue));
}

auto iniFile_getDiskSize(CIniFile* f, const char* key, const char* defaultValue)
        -> int64_t {
    return asBaseIniFile(f)->getDiskSize(key, defaultValue);
}

auto iniFile_getInt64(CIniFile* f, const char* key, int64_t defaultValue)
        -> int64_t {
    return asBaseIniFile(f)->getInt64(key, defaultValue);
}

void iniFile_setValue(CIniFile* f, const char* key, const char* value) {
    // Silently protect us from blowing up.
    asBaseIniFile(f)->setString(key, (value == nullptr) ? "" : value);
}

void iniFile_setInteger(CIniFile* f, const char* key, int value) {
    asBaseIniFile(f)->setInt(key, value);
}

void iniFile_setInt64(CIniFile* f, const char* key, int64_t value) {
    asBaseIniFile(f)->setInt64(key, value);
}

void iniFile_setDouble(CIniFile* f, const char* key, double value) {
    asBaseIniFile(f)->setDouble(key, value);
}

void iniFile_setBoolean(CIniFile* f, const char* key, int value) {
    asBaseIniFile(f)->setBool(key, value != 0);
}

void iniFile_setDiskSize(CIniFile* f, const char* key, int64_t size) {
    asBaseIniFile(f)->setDiskSize(key, size);
}

void iniFile_setString(CIniFile* f, const char* key, const char* str) {
    asBaseIniFile(f)->setString(key, str);
}
