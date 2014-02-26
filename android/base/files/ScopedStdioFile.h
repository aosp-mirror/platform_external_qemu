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

#ifndef ANDROID_BASE_SCOPED_STDIO_FILE_H
#define ANDROID_BASE_SCOPED_STDIO_FILE_H

#include <stdio.h>

namespace android {
namespace base {

// Helper class used to implement a scoped stdio FILE* pointer.
// I.e. guarantees that the file is closed on scope exit, unless
// the release() method is called. It is also possible to close
// the file explicitly with close().
class ScopedStdioFile {
public:
    // Default constructor, uses an empty file.
    ScopedStdioFile() : mFile(NULL) {}

    // Regular constructor, takes owneship of |file|.
    explicit ScopedStdioFile(FILE* file) : mFile(file) {}

    // Destructor always calls close().
    ~ScopedStdioFile() { close(); }

    // Returns FILE* pointer directly.
    FILE* get() const { return mFile; }

    // Release the FILE* handle and return it to the caller, which becomes
    // its owner. Returns NULL if the instance was already closed or empty.
    FILE* release() {
        FILE* file = mFile;
        mFile = NULL;
        return file;
    }

    // Swap two scoped FILE* pointers.
    void swap(ScopedStdioFile* other) {
        FILE* tmp = other->mFile;
        other->mFile = mFile;
        mFile = tmp;
    }

    // Explicit close of a scoped FILE*.
    void close() {
        if (mFile) {
            ::fclose(mFile);
            mFile = NULL;
        }
    }
private:
    FILE* mFile;
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_SCOPED_STDIO_FILE_H
