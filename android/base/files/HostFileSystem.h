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

#include "android/base/files/FileSystem.h"

namespace android {
namespace base {

class HostFileSystem : public FileSystem {
public:

    virtual SystemResult copyFile(const FilePath& destination,
                                  const FilePath& source) const override;

    virtual SystemResult moveFile(const FilePath& destination,
                                  const FilePath& source) const override;

    virtual SystemResult openFile(const FilePath& path,
                                  FileHandle* handle,
                                  uint32_t mode) const override;

    virtual SystemResult closeFile(FileHandle handle) const override;

    virtual SystemResult readFile(FileHandle handle,
                                  void* buffer,
                                  size_t bufferSize,
                                  size_t* bytesRead) const override;

    virtual SystemResult writeFile(FileHandle handle,
                                   const void* buffer,
                                   size_t bufferSize,
                                   size_t* bytesWritten) const override;

    virtual SystemResult seekFile(FileHandle handle,
                                  ssize_t offset,
                                  SeekDir seekDir) const override;
private:
    // Translate a bit field of OpenMode flags to a string suitable for fopen.
    // Returns nullptr if the combination of flags is not valid
    const FilePath::CharType* translateOpenModeString(uint32_t mode) const;
    // Translate a bit fiel of OpenMode flags to a set of flags suitable for
    // open. Returns -1 if the combination of flags it not valid
    int translateOpenModeFlags(uint32_t mode) const;
};

}  // namespace base
}  // namespace android
