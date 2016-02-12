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

#include "android/base/Compiler.h"
#include "android/base/files/FilePath.h"
#include "android/base/system/SystemResult.h"

namespace android {
namespace base {

using FileHandle = void*;

// The intention of this class it to provide a very low level basic abstraction
// for file and file system operations that are platform independent.
//
// The class is designed so that it can be replaced during unit tests without
// having to inject anything into other classes. This allows more feature-rich
// classes to be built on top of this and still remain easily unit testable.
class FileSystem {
public:
    enum OpenMode {
        ModeRead = 1 << 0,
        ModeWrite = 1 << 1,
        ModeTruncate = 1 << 2,
        ModeAppend = 1 << 3,
    };

    enum SeekDir {
        SeekSet,
        SeekCur,
        SeekEnd,
    };

    static const FileSystem& get();

    FileSystem() = default;
    virtual ~FileSystem() = default;

    // Copy a file from |source| to |destination|
    virtual SystemResult copyFile(const FilePath& destination,
                                  const FilePath& source) const = 0;

    // Move a file from |source| to |destination|
    virtual SystemResult moveFile(const FilePath& destination,
                                  const FilePath& source) const = 0;

    // Open a file at location |path| and place a handle to the file in
    // |handle|. The file is opened using |mode| which can be a bitwise OR
    // combination of values from the OpenMode enum. The method will return
    // InvalidMode if the combination of modes is not supported.
    virtual SystemResult openFile(const FilePath& path,
                                  FileHandle* handle,
                                  uint32_t mode) const = 0;

    // Close a file represented by |handle| that was previously opened by
    // openFile. Closing an already closed file is undefined behavior.
    virtual SystemResult closeFile(FileHandle handle) const = 0;

    // Read |bufferSize| bytes from file |handle| into |buffer|. If |bytesRead|
    // is not null the value it points to will be the number of bytes read after
    // the method returns.
    virtual SystemResult readFile(FileHandle handle,
                                  void* buffer,
                                  size_t bufferSize,
                                  size_t* bytesRead) const = 0;

    // Write |bufferSize| bytes from |buffer| into file represented by |handle|.
    // If |bytesWritten| is not null the value it points to will be the number
    // of bytes written after the method returns.
    virtual SystemResult writeFile(FileHandle handle,
                                   const void* buffer,
                                   size_t bufferSize,
                                   size_t* bytesWritten) const = 0;

    // Adjust the current read and write position in the file represented by
    // |handle| to match |offset| and |seekDir|.
    virtual SystemResult seekFile(FileHandle handle,
                                  ssize_t offset,
                                  SeekDir seekDir) const = 0;
private:
    DISALLOW_COPY_AND_ASSIGN(FileSystem);
};

}  // namespace base
}  // namespace android
