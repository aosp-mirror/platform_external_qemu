// Copyright 2015 The Android Open Source Project
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

#pragma once

#include "aemu/base/Compiler.h"

#include <iosfwd>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <inttypes.h>

namespace android {
namespace base {

class IniFile {
    DISALLOW_COPY_AND_ASSIGN(IniFile);

public:
    using DiskSize = uint64_t;
    using MapType = std::unordered_map<std::string, std::string>;
    using ElementOrderList = std::vector<const MapType::value_type*>;

    // A custom iterator to return the keys in original order.
    class const_iterator : public ElementOrderList::const_iterator {
    public:
        using value_type = std::string;

        explicit const_iterator(ElementOrderList::const_iterator keyIterator)
            : ElementOrderList::const_iterator(keyIterator) {}

        const value_type& operator*() const {
            return ElementOrderList::const_iterator::operator*()->first;
        }
        const value_type* operator->() const {
            return &**this;
        }
    };

    // Note that the constructor _does not_ read data from the backing file.
    // Call |Read| to read the data.
    // When created without a backing file, all |read|/|write*| operations will
    // fail unless |setBackingFile| is called to point to a valid file path.
    explicit IniFile(std::string_view backingFilePath = {})
        : mBackingFilePath(backingFilePath) {}

    // This constructor reads the data from memory at |data| of |size| bytes.
    IniFile(const char* data, int size);

    // Set a new backing file. This does not read data from the file. Call
    // |read| to refresh data from the new backing file.
    void setBackingFile(std::string_view filePath);
    const std::string& getBackingFile() const { return mBackingFilePath; }

    // Reads data into IniFile from the the backing file, overwriting any
    // existing data.
    bool read(bool keepComments = true);
    // Same thing, but parses an already read file data.
    // Note: write operations fail unless there's a backing file set
    bool readFromMemory(std::string_view data);

    // Write the current IniFile to the backing file.
    bool write();
    // Write the current IniFile to backing file. Discard any keys that have
    // empty values.
    bool writeDiscardingEmpty();
    // An optimized write.
    // - Advantage: We don't write if there have been no updates since last
    // write.
    // - Disadvantage: Not safe if something else might be changing the ini
    //   file -- your view of the file is no longer consistent. Actually, this
    //   "bug" can be considered a "feature", if the ini file changed unbeknown
    //   to you, you're probably doing wrong in overwriting the changes without
    //   any update on your side.
    bool writeIfChanged();

    // Gets the number of (key,value) pairs in the file.
    int size() const;
    // Check if a certain key exists in the file.
    bool hasKey(std::string_view key) const;

    // Make sure the string can be used as a valid key/value
    static std::string makeValidKey(std::string_view str);
    static std::string makeValidValue(std::string_view str);

    // ///////////////////// Value Getters
    // //////////////////////////////////////
    // The IniFile has no knowledge about the type of the values.
    // |defaultValue| is returned if the key doesn't exist or the value is badly
    // formatted for the requested type.
    //
    // For some value types where the disk format is significantly more useful
    // for human-parsing, overloads are provided that accept default values as
    // strings to be parsed just like the backing ini file.
    // - This has the benefit that default values can be stored in a separate
    //   file in human friendly form, and used directly.
    // - The disadvantage is that behaviour is undefined if we fail to parse the
    //   default value.
    std::string getString(const std::string& key,
                          std::string_view defaultValue) const;
    int getInt(const std::string& key, int defaultValue) const;
    int64_t getInt64(const std::string& key, int64_t defaultValue) const;
    double getDouble(const std::string& key, double defaultValue) const;
    // The serialized format for a bool acceepts the following values:
    //     True: "1", "yes", "YES".
    //     False: "0", "no", "NO".
    bool getBool(const std::string& key, bool defaultValue) const;
    bool getBool(const std::string& key,
                 std::string_view defaultValueStr) const;
    bool getBool(const std::string& key, const char* defaultValue) const {
        return getBool(key, std::string_view(defaultValue));
    }
    // Parses a string as disk size. The serialized format is [0-9]+[kKmMgG].
    // The
    // suffixes correspond to KiB, MiB and GiB multipliers.
    // Note: We consider 1K = 1024, not 1000.
    DiskSize getDiskSize(const std::string& key, DiskSize defaultValue) const;
    DiskSize getDiskSize(const std::string& key,
                         std::string_view defaultValue) const;

    // ///////////////////// Value Setters
    // //////////////////////////////////////
    void setString(const std::string& key, std::string_view value);
    void setInt(const std::string& key, int value);
    void setInt64(const std::string& key, int64_t value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);
    void setDiskSize(const std::string& key, DiskSize value);

    // //////////////////// Iterators
    // ///////////////////////////////////////////
    // You can iterate through (string) keys in this IniFile, and then use the
    // correct |get*| function to obtain the corresponding value.
    // The order of keys is guaranteed to be an extension of the order in the
    // backing file:
    //    - For keys that exist in the backing file, order is maintained.
    //    - Rest of the keys are appended in the end, in the order they were
    //      first added.
    //  Only const_iterator is provided. Use |set*| functions to modify the
    //  IniFile.
    const_iterator begin() const { return const_iterator(std::begin(mOrderList)); }
    const_iterator end() const { return const_iterator(std::end(mOrderList)); }

protected:
    void parseStream(std::istream* inFile, bool keepComments);
    void updateData(const std::string& key, std::string&& value);
    bool writeCommon(bool discardEmpty);

private:
    bool writeCommonImpl(bool discardEmpty, const std::string& filepath);

    MapType mData;
    ElementOrderList mOrderList;
    std::vector<std::pair<int, std::string>> mComments;
    std::string mBackingFilePath;
    bool mDirty = true;
};

}  // namespace base
}  // namespace android
