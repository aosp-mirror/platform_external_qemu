// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_BASE_STRING_H
#define ANDROID_BASE_STRING_H

#include <stddef.h>

namespace android {
namespace base {

class StringView;

// Custom string implementation:
// - Not a template, hence much better compiler error messages.
// - Uses short-string-optimization on all platforms.
// - No iterators (for now).
class String {
public:
    // Empty constructor.
    String();

    // Regular constructor from a C string.
    String(const char* string);

    // Copy constructor.
    String(const String& other);

    // Constructor from a StringView.
    explicit String(const StringView& other);

    // Constructor from a sized C string.
    String(const char* str, size_t len);

    // Fill-constructor.
    String(size_t count, char fill);

    // Destructor.
    ~String();

    // Return string as a zero-terminated C string.
    const char* c_str() const { return mStr; }

    void* data() { return mStr; }

    const void* data() const { return mStr; }

    // Return size of string in bytes, not including terminating zero.
    size_t size() const { return mSize; }

    // Return true if the string is empty, i.e. its size is 0.
    // Note that a non-empty string can embed 0 bytes.
    bool empty() const { return mSize == 0; }

    // Return current capacity.
    size_t capacity() const {
        return (mStr == mStorage) ?
                static_cast<size_t>(kMinCapacity) : mCapacity;
    }

    // Clear the content of a given instance.
    void clear() { resize(0); }

    // Array indexing operators.
    char& operator[](size_t index) { return mStr[index]; }
    const char& operator[](size_t index) const { return mStr[index]; }

    // Assign new values to strings.
    String& assign(const char* str);
    String& assign(const char* str, size_t len);
    String& assign(const String& other);
    String& assign(const StringView& other);
    String& assign(size_t count, char fill);
    String& assign(char ch);

    // Assignment operators.
    String& operator=(const char* str) {
        return this->assign(str);
    }

    String& operator=(const String& other) {
        return this->assign(other);
    }

    String& operator=(const StringView& other) {
        return this->assign(other);
    }

    String& operator=(char ch) {
        return this->assign(ch);
    }

    // Append new values to current string.
    String& append(const char* str);
    String& append(const char* str, size_t len);
    String& append(const String& other);
    String& append(const StringView& other);
    String& append(char ch);

    // Addition-Assignment operators.
    String& operator+=(const char* str) {
        return this->append(str);
    }

    String& operator+=(const String& other) {
        return this->append(other);
    }

    String& operator+=(const StringView& other) {
        return this->append(other);
    }

    String& operator+=(char ch) {
        return this->append(ch);
    }

    // Resize to a new size.
    void resize(size_t newSize);

    // Reserve enough room for |newSize| characters, followed by a
    // terminating zero.
    void reserve(size_t newSize);

    // Compare to something, return -1, 0 or +1 based on byte values.
    int compare(const char* str, size_t len) const;
    int compare(const char* str) const;
    int compare(const String& other) const;
    int compare(const StringView& other) const;
    int compare(char ch) const;

    // Compare for equality.
    bool equals(const char* str, size_t len) const;
    bool equals(const char* str) const;
    bool equals(const String& other) const;
    bool equals(const StringView& other) const;
    bool equals(char ch) const;

    // Binary operators.
    bool operator==(const char* str) const {
        return equals(str);
    }

    bool operator==(const String& other) const {
        return equals(other);
    }

    bool operator==(const StringView& other) const {
        return equals(other);
    }

    bool operator==(char ch) const {
        return equals(ch);
    }

    bool operator!=(const char* str) const {
        return !operator==(str);
    }

    bool operator!=(const String& other) const {
        return !operator==(other);
    }

    bool operator!=(const StringView& other) const {
        return !operator==(other);
    }

    bool operator!=(char ch) const {
        return !operator==(ch);
    }

    // Swap the content of this string with another one.
    void swap(String* other);

protected:
    friend class StringVector;

    // Internal helper routine to be used when |count| strings objects
    // have been moved or copied from |fromStrings| to |toStrings|.
    // This adjusts any internal |mStr| pointer to |mStorage| to the
    // new appropriate region. Used by StringVector to implement
    // optimized insert/copy/remove operations.
    static void adjustMovedSlice(String* fromStrings,
                                 String* toStrings,
                                 size_t count);

    // Move |count| strings from |strings + from| to |strings + to|
    // in memory. Used by StringVector to implement insert/delete
    // operations.
    static void moveSlice(String* strings,
                          size_t from,
                          size_t to,
                          size_t count);

    // Finalize |count| String instances from |strings|.
    // Used by StringVector to implement resize/reserve operations.
    static void finalizeSlice(String* strings, size_t count);

    // Minimum capacity for the in-object storage array |mStorage|,
    // not including the terminating zero. With a value of 16,
    // each String instance is 24 bytes on a 32-bit system, and
    // 32-bytes on a 64-bit one.
    enum {
        kMinCapacity = 15
    };

    char* mStr;
    size_t mSize;
    union {
        size_t mCapacity;
        char mStorage[kMinCapacity + 1U];
    };
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_STRING_H
