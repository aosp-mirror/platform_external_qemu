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

#include "android/base/files/HostFileSystem.h"

#include "android/base/files/ScopedFd.h"
#include "android/base/memory/LazyInstance.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#ifdef _WIN32
#define O_RDWR _O_RDWR
#define O_CREAT _O_CREAT
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_APPEND _O_APPEND
#define O_TRUNC _O_TRUNC
#define O_CLOEXEC 0  // Does not exist on windows, 0 negates it in an OR

#define OPEN_FILE_MODE (_S_IREAD | _S_IWRITE)

#define OPEN _wopen
#define RENAME _wrename
#define FOPEN _wfopen
#define FDOPEN _wfdopen
#else
#define O_BINARY 0  // POSIX does not have this one, 0 negates it in an OR

#define OPEN_FILE_MODE (0666) // This will be modified by the umask

#define OPEN open
#define RENAME rename
#define FOPEN fopen
#define FDOPEN fdopen
#endif

namespace android {
namespace base {

LazyInstance<HostFileSystem> sHostFileSystem = LAZY_INSTANCE_INIT;
FileSystem* sFileSystemForTesting = nullptr;

const FileSystem& FileSystem::get() {
    if (sFileSystemForTesting) {
        return *sFileSystemForTesting;
    }
    return *sHostFileSystem.ptr();
}

SystemResult HostFileSystem::copyFile(const FilePath& destination,
                                      const FilePath& source) const {
    ScopedFd destFd(OPEN(destination.c_str(),
                         O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_CLOEXEC,
                         OPEN_FILE_MODE));
    if (destFd.get() == -1) {
        return SystemResult::fromErrno(errno);
    }
    ScopedFd sourceFd(OPEN(source.c_str(), O_RDONLY | O_CLOEXEC | O_BINARY));
    if (sourceFd.get() == -1) {
        return SystemResult::fromErrno(errno);
    }

    char buf[4096];
    for (ssize_t n; (n = read(sourceFd.get(), buf, sizeof(buf))) > 0; ) {
        if (n == -1) {
            return SystemResult::fromErrno(errno);
        }
        if (write(destFd.get(), buf, n) != n) {
            return SystemResult::fromErrno(errno);
        }
    }
    return SystemResult::kSuccess;
}

SystemResult HostFileSystem::moveFile(const FilePath& destination,
                                      const FilePath& source) const {
    int result = RENAME(source.c_str(), destination.c_str());
    if (result != 0) {
        return SystemResult::fromErrno(errno);
    }
    return SystemResult::kSuccess;
}

SystemResult HostFileSystem::openFile(const FilePath& path,
                                      FileHandle* handle,
                                      uint32_t mode) const {
    const FilePath::CharType* modeString = translateOpenModeString(mode);
    if (modeString == nullptr) {
        return SystemResult::fromErrno(EINVAL);
    }

    // We need to use open here first in order to support write only modes.
    // fopen will only create new files if the mode also truncates but we
    // want to support open-or-create without automatically truncating if
    // the file already exists
    int flags = translateOpenModeFlags(mode);
    if (flags == -1) {
        return SystemResult::fromErrno(EINVAL);
    }

    // The FILE* object will take ownership of the file descriptor so we don't
    // wrap it in a ScopedFd or attempt to close it.
    int fd = OPEN(path.c_str(), flags, OPEN_FILE_MODE);
    if (fd == -1) {
        return SystemResult::fromErrno(errno);
    }
    FILE* file = FDOPEN(fd, modeString);
    if (file == nullptr) {
        return SystemResult::fromErrno(errno);
    }
    *handle = file;
    return SystemResult::kSuccess;
}

SystemResult HostFileSystem::closeFile(FileHandle handle) const {
    if (handle == nullptr) {
        return SystemResult::fromErrno(EINVAL);
    }
    int result = fclose(static_cast<FILE*>(handle));
    if (result != 0) {
        return SystemResult::fromErrno(errno);
    }
    return SystemResult::kSuccess;
}

SystemResult HostFileSystem::readFile(FileHandle handle,
                                      void* buffer,
                                      size_t bufferSize,
                                      size_t* bytesRead) const {
    FILE* file = static_cast<FILE*>(handle);
    size_t result = fread(buffer, 1, bufferSize, file);
    if (bytesRead) {
        *bytesRead = result;
    }
    if (result != bufferSize) {
        if (feof(file)) {
            return SystemResult::kSuccess;
        } else if (ferror(file)) {
            return SystemResult::fromErrno(errno);
        }
        // Not really sure what to call this error, but something is wrong
        return SystemResult::fromErrno(EINVAL);
    }
    return SystemResult::kSuccess;
}

SystemResult HostFileSystem::writeFile(FileHandle handle,
                                       const void* buffer,
                                       size_t bufferSize,
                                       size_t* bytesWritten) const {
    FILE* file = static_cast<FILE*>(handle);
    size_t result = fwrite(buffer, 1, bufferSize, file);
    if (bytesWritten) {
        *bytesWritten = result;
    }
    if (result != bufferSize) {
        return SystemResult::fromErrno(errno);
    }
    return SystemResult::kSuccess;
}

SystemResult HostFileSystem::seekFile(FileHandle handle,
                                      ssize_t offset,
                                      SeekDir seekDir) const {
    // This has to match the order of the SeekDir enum and the corresponding
    // fseek values.
    static int whence[] = { SEEK_SET, SEEK_CUR, SEEK_END };

    if (seekDir < 0 || seekDir >= ARRAY_SIZE(whence)) {
        return SystemResult::fromErrno(EINVAL);
    }

    FILE* file = static_cast<FILE*>(handle);
    if (fseek(file, offset, whence[seekDir]) != 0) {
        return SystemResult::fromErrno(errno);
    }
    return SystemResult::kSuccess;
}
const FilePath::CharType*
HostFileSystem::translateOpenModeString(uint32_t mode) const {
    if (mode == ModeRead) {
        return PATH_LITERAL("rb");
    } else if (mode == ModeWrite) {
        return PATH_LITERAL("wb");
    } else if (mode == (ModeRead | ModeWrite)) {
        return PATH_LITERAL("r+b");
    } else if (mode == (ModeWrite | ModeTruncate)) {
        return PATH_LITERAL("wb");
    } else if (mode == (ModeRead | ModeWrite | ModeTruncate)) {
        return PATH_LITERAL("w+b");
    } else if (mode == (ModeWrite | ModeAppend)) {
        return PATH_LITERAL("ab");
    } else if (mode == (ModeRead | ModeWrite | ModeAppend)) {
        return PATH_LITERAL("a+b");
    }
    return nullptr;
}

#ifdef _WIN32
// This isn't exactly pretty since this should be in the window file, but
// this is all that's needed to get the code below to work on several platforms
// so I'd rather avoid code duplication and do this
#endif

int HostFileSystem::translateOpenModeFlags(uint32_t mode) const {
    int flags = O_CLOEXEC | O_BINARY;
    if ((mode & (ModeRead | ModeWrite)) == (ModeRead | ModeWrite)) {
        flags |= O_RDWR | O_CREAT;
    } else if (mode & ModeRead) {
        flags |= O_RDONLY;
    } else if (mode & ModeWrite) {
        flags |= O_WRONLY | O_CREAT;
    } else {
        // Need at least one of ModeRead and ModeWrite
        return -1;
    }

    if ((mode & (ModeAppend | ModeTruncate)) == (ModeAppend | ModeTruncate)) {
        // Make up your mind
        return -1;
    } else if (mode & ModeAppend) {
        if ((mode & ModeWrite) == 0) {
            // Only applicable if we're writing
            return -1;
        }
        flags |= O_APPEND;
    } else if (mode & ModeTruncate) {
        if ((mode & ModeWrite) == 0) {
            // Only applicable if we're writing
            return -1;
        }
        flags |= O_TRUNC;
    }
    return flags;
}

}  // namespace base
}  // namespace android
