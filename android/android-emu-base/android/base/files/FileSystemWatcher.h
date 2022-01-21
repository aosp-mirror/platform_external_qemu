// Copyright 2022` The Android Open Source Project
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

#include <stddef.h>       // for size_t
#include <functional>     // for function
#include <memory>         // for unique_ptr
#include <string>         // for std::string
#include <unordered_set>  // for unordered_set

namespace android {
namespace base {

// Listens to the file system change notifications and raises events when a
// directory, or file in a directory, changes.
//
// Note: Each observer consumes one thread.
// Note: It is very expensive on mac os, so do not observe large directory
// structures with this.
class FileSystemWatcher {
public:
    // On one day we will have std::filesystem everywhere..
    using Path = std::string;

    enum class WatcherChangeType {
        Created,  // The creation of a file or folder.
        Deleted,  // The deletion of a file or folder.
        Changed,  // The change of a file or folder. The types of changes
                  // include: changes to size, attributes, security
                  // settings, last write, and last access time.
    };

    // Change type, and file that was created, deleted or changed.
    using FileSystemWatcherCallback =
            std::function<void(WatcherChangeType, const Path&)>;

    FileSystemWatcher(FileSystemWatcherCallback callback)
        : mChangeCallback(callback) {}
    virtual ~FileSystemWatcher() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;

    // Watches for changes in the given directory.
    // Returns nullptr if path is not a directory.
    static std::unique_ptr<FileSystemWatcher> getFileSystemWatcher(
            Path path,
            FileSystemWatcherCallback onChangeCallback);

    FileSystemWatcherCallback mChangeCallback;
};
}  // namespace base
}  // namespace android
