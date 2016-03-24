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

#pragma once

#include <string>
#include <vector>

namespace android {

// Convenience class used to build a space-based parameter list incrementally.
// Usage example:
//   1/ Create new instance.
//   2/ Add individual parameters with add(), add2(), addIf() or add2If()
//   3/ Retrieve final result as a single string with toString() or
//      toCString().
//   4/ Alternatively, retrieve the list's size with size(), and individual
//      parameters as a char* array with array().
class ParameterList {
public:
    // Default constructor.
    ParameterList() = default;

    // Return the number of items in the list.
    size_t size() const;

    // Return a char* array of size() items that points to the content of
    // individual items.
    char** array() const;

    // Access |n-th| items in the list. WARNING: Does not check bounds.
    const std::string& operator[](size_t n) const {
        return mParams[n];
    }

    // Convert list into a std::string. This joins all parameters with a
    // single space.
    std::string toString() const;

    // A variant of toString() that returns instead a heap-allocated C string
    // that must be free-d by the caller.
    char* toCStringCopy() const;

    // Add new parameter |param| to the list.
    void add(const std::string& param);

    // Note: this will also handle const char* parameters automatically.
    void add(std::string&& param);

    // Add two new parameters |param1| and |param2| to the list.
    void add2(const char* param1, const char* param2);

    // Add parameter |param| to the list iff |flag| is true.
    void addIf(const char* param, bool flag);

    // Add parameters |param1| and |param2| iff |param2| is not nullptr.
    void add2If(const char* param1, const char* param2);

private:
    std::vector<std::string> mParams;
    mutable std::vector<char *> mArray;
};

}  // namespace android
