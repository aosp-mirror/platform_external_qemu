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

#pragma once

#include "android/base/Compiler.h"

#include <inttypes.h>

#include <string>
#include <unordered_map>

namespace android {
namespace base {

class IniFile {
public:
    typedef int64_t DiskSize;
    typedef std::unordered_map<std::string, std::string> MapType;

    // Note that the constructor _does not_ read data from the backing file.
    // Call |Read| to read the data.
    IniFile(const std::string& backingFilePath)
        : mBackingFilePath(backingFilePath) {}
    // When created without a backing file, all |read|/|write*| operations will
    // fail unless |setBackingFile| is called to point to a valid file path.
    IniFile() = default;

    // Set a new backing file. This does not read data from the file. Call
    // |read|
    // to refresh data from the new backing file.
    void setBackingFile(const std::string& filePath);

    // Reads data into IniFile from the the backing file, overwriting any
    // existing data.
    bool read();

    // Write the current IniFile to the backing file.
    bool write() const;
    // Write the current IniFile to backing file. Discard any keys that have
    // empty values.
    bool writeDiscardingEmpty() const;

    // Gets the number of (key,value) pairs in the file.
    int size() const;
    // Check if a certain key exists in the file.
    bool hasKey(const std::string& key) const;

    // ///////////////////// Value Getters
    // //////////////////////////////////////
    // The IniFile has no knowledge about the type of the values.
    // |defaultValue| is returned if the key doesn't exist or the value is badly
    // formatted for the requested type.
    std::string getString(const std::string& key,
                          const std::string& defaultValue) const;
    int getInt(const std::string& key, int defaultValue) const;
    int64_t getInt64(const std::string& key, int64_t defaultValue) const;
    double getDouble(const std::string& key, double defaultValue) const;
    // The serialized format for a bool acceepts the following values:
    //     True: "1", "yes", "YES".
    //     False: "0", "no", "NO".
    bool getBool(const std::string& key, bool defaultValue) const;
    // Parses a string as disk size. The serialized format is [0-9]+[kKmMgG].
    // The
    // suffixes correspond to KiB, MiB and GiB multipliers.
    // Note: We consider 1K = 1024, not 1000.
    DiskSize getDiskSize(const std::string& key, DiskSize defaultValue) const;

    // ///////////////////// Value Setters
    // //////////////////////////////////////
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setInt64(const std::string& key, int64_t value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);
    void setDiskSize(const std::string& key, DiskSize value);

    // //////////////////// Iterators
    // ///////////////////////////////////////////
    // You can iterate through (string, string) pairs for the key-values stored
    // in this IniFile.
    typedef MapType::iterator iterator;
    typedef MapType::const_iterator const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

protected:
    // Helper functions.
    bool isFileValid(const std::string& line);
    bool writeCommon(bool discardEmpty) const;

private:
    MapType mData;
    std::string mBackingFilePath;

    DISALLOW_COPY_AND_ASSIGN(IniFile);
};

}  // namespace base
}  // namespace android
