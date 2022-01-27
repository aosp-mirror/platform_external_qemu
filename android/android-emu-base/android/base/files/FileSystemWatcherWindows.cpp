// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/files/FileSystemWatcher.h"  // for FileSystemWatcher:...

#include <algorithm>      // for set_difference
#include <atomic>         // for atomic_bool
#include <functional>     // for function
#include <string>         // for basic_string
#include <thread>         // for thread
#include <unordered_set>  // for unordered_set<>::i...
#include <utility>        // for move
#include <vector>         // for vector

#include "android/base/system/System.h"  // for System
#include "android/base/system/Win32UnicodeString.h"
#include "android/utils/debug.h"  // for derror


#include "android/base/files/PathUtils.h"
#include "android/base/synchronization/Event.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                          \
    printf("ReadDirectoryChangesWin32: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif


using android::base::pj;
namespace android {
namespace base {

// A Very basic file system change detector.
class ReadDirectoryChangesWin32 : public FileSystemWatcher {
public:
    ReadDirectoryChangesWin32(Path path,
                              FileSystemWatcherCallback onChangeCallback)
        : FileSystemWatcher(onChangeCallback), mPath(path) {}

    ~ReadDirectoryChangesWin32() { stop(); }

    bool start() override {
        bool expected = false;
        if (!mRunning.compare_exchange_strong(expected, true)) {
            return false;
        }
        std::thread watcher([this] { watchForChanges(); });
        mWatcherThread = std::move(watcher);
        mStarted.wait();
        return mDirHandle != INVALID_HANDLE_VALUE;
    }

    void stop() override {
        bool expected = true;
        if (mRunning.compare_exchange_strong(expected, false)) {
            CancelIoEx(mDirHandle, NULL);
            mWatcherThread.join();
        }
    }

private:

    bool watchForChanges() {
        const Win32UnicodeString szDirectory(mPath.c_str());
        mDirHandle = CreateFileW(
                szDirectory.c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

        mStarted.signal();
        if (mDirHandle == INVALID_HANDLE_VALUE) {
            return false;
        }

        while (mRunning) {
            DWORD dwBytesReturned = 0;
            BYTE buffer[4096];
            if (ReadDirectoryChangesW(mDirHandle, buffer, sizeof(buffer), TRUE,
                                      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                              FILE_NOTIFY_CHANGE_ATTRIBUTES,
                                      &dwBytesReturned, NULL, NULL) == 0) {
                CloseHandle(mDirHandle);
                mDirHandle = INVALID_HANDLE_VALUE;
                return false;
            }
            for (FILE_NOTIFY_INFORMATION* info =
                         reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
                 mRunning && !info->NextEntryOffset; info->NextEntryOffset) {
                Win32UnicodeString filename(info->FileName);
                Path changed = pj(mPath, filename.toString());
                DD("Action: %d", info->Action);
                switch (info->Action) {
                    case FILE_ACTION_ADDED:
                        mChangeCallback(WatcherChangeType::Created, changed);
                        break;
                    case FILE_ACTION_MODIFIED:
                        mChangeCallback(WatcherChangeType::Changed, changed);
                        break;
                    case FILE_ACTION_REMOVED:
                        mChangeCallback(WatcherChangeType::Deleted, changed);
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        mChangeCallback(WatcherChangeType::Created, changed);
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        mChangeCallback(WatcherChangeType::Deleted, changed);
                        break;
                    default:
                        break;
                }
            }
        }

        CloseHandle(mDirHandle);
        mDirHandle = INVALID_HANDLE_VALUE;
        return true;
    }

    Path mPath;
    HANDLE mDirHandle;
    std::atomic_bool mRunning{false};
    std::thread mWatcherThread;
    Event mStarted;
};

std::unique_ptr<FileSystemWatcher> FileSystemWatcher::getFileSystemWatcher(
        Path path,
        FileSystemWatcherCallback onChangeCallback) {
    if (!System::get()->pathIsDir(path)) {
        return nullptr;
    }
    return std::make_unique<ReadDirectoryChangesWin32>(path, onChangeCallback);
};
}  // namespace base
}  // namespace android
