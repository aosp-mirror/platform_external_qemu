// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/FileUtils.h"
#include "android/utils/eintr_wrapper.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#include <io.h>
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace android {

bool readFileIntoString(int fd, std::string* file_contents) {
#ifdef _MSC_VER
    if (fd < 0) return false; // msvc does not handle fd -1 very well.
#endif

    off_t size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        return false;
    }
    off_t err = lseek(fd, 0, SEEK_SET);
    if (err == (off_t)-1) {
        return false;
    }

    std::string buf((size_t)size, '\0');
    ssize_t result = HANDLE_EINTR(read(fd, &buf[0], size));
    if (result != size) {
        return false;
    }
    *file_contents = std::move(buf);
    return true;
}

bool writeStringToFile(int fd, const std::string& file_contents) {
#ifdef _MSC_VER
    // msvc does not handle fd -1 very well.
    if (fd < 0)  return false;

    ssize_t result = HANDLE_EINTR(
            _write(fd, file_contents.c_str(), file_contents.size()));
#else
    ssize_t result = HANDLE_EINTR(
            write(fd, file_contents.c_str(), file_contents.size()));
#endif
    if (result != (ssize_t)file_contents.size()) {
        return false;
    }
    return true;
}

base::Optional<std::string> readFileIntoString(base::StringView name) {
    std::ifstream is(base::c_str(name), std::ios_base::binary);
    if (!is) {
        return {};
    }

    std::ostringstream ss;
    ss << is.rdbuf();
    return ss.str();
}

bool setFileSize(int fd, int64_t size) {
#ifdef _WIN32
#ifdef _MSC_VER
    if (fd < 0) return false; // msvc does not handle fd -1 very well.
#endif
    return _chsize_s(fd, size) == 0;
#else
    return ftruncate(fd, size) == 0;
#endif
}

}  // namespace android
