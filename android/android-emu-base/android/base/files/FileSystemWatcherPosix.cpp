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
#include <limits.h>       // for PATH_MAX
#include <stdint.h>       // for uint8_t
#include <sys/inotify.h>  // for inotify_event, ino...
#include <unistd.h>       // for close, read
#include <atomic>         // for atomic_bool
#include <functional>     // for function
#include <memory>         // for make_unique, uniqu...
#include <thread>         // for thread
#include <utility>        // for move

#include "android/base/files/FileSystemWatcher.h"  // for FileSystemWatcher:...
#include "android/base/files/PathUtils.h"
#include "android/base/synchronization/Event.h"  // for Event

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                       \
    printf("FileSystemWatcherPosix: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif


using android::base::pj;
namespace android {
namespace base {

class FileSystemWatcherPosix : public FileSystemWatcher {
public:
    FileSystemWatcherPosix(Path path,
                           FileSystemWatcherCallback onChangeCallback)
        : FileSystemWatcher(onChangeCallback), mPath(path) {}

    ~FileSystemWatcherPosix() { stop(); }

    bool start() override {
        bool expected = false;
        if (!mRunning.compare_exchange_strong(expected, true)) {
            return false;
        }
        std::thread watcher([this] { watchForChanges(); });
        mWatcherThread = std::move(watcher);
        mStarted.wait();
        return mNotifyFd != 0;
    }

    void stop() override {
        bool expected = true;
        if (mRunning.compare_exchange_strong(expected, false)) {
            DD("Closing watchers");
            close(mNotifyFd);
            mWatcherThread.join();
        }
    }

private:
    bool watchForChanges() {
        mNotifyFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (mNotifyFd < 1) {
            mStarted.signal();
            return false;
        }

        // Note, this will not work for filenames longer than 256 chars, this
        // does not include the path.
        constexpr int MAX_EVENTS = 4;
        constexpr int EVENT_SIZE = sizeof(struct inotify_event);
        constexpr int MAX_FILENAME = 256;
        constexpr int EVENT_BUFFER = MAX_EVENTS * (EVENT_SIZE + MAX_FILENAME);

        auto fd = inotify_add_watch(mNotifyFd, mPath.c_str(),
                                    IN_MODIFY | IN_CREATE | IN_DELETE);

        if (fd == -1) {
            close(mNotifyFd);
            mNotifyFd = 0;
            mStarted.signal();
            return false;
        }

        mStarted.signal();
        while (mRunning) {
            uint8_t buffer[EVENT_BUFFER];

            int length = read(mNotifyFd, buffer, sizeof(buffer));
            DD("Read %d bytes", length);

            struct inotify_event* event;
            for (int i = 0; i < length && mRunning;
                 i += EVENT_SIZE + event->len) {
                event = (struct inotify_event*)&buffer[i];
                DD("i: %d, event->len: %d", i, event->len);
                if (event->len) {
                    Path changed = pj(mPath, std::string(event->name));
                    DD("Changed: %s", changed.c_str());
                    if (event->mask & IN_CREATE) {
                        mChangeCallback(WatcherChangeType::Created, changed);
                    } else if (event->mask & IN_DELETE) {
                        mChangeCallback(WatcherChangeType::Deleted, changed);
                    } else if (event->mask & IN_MODIFY) {
                        mChangeCallback(WatcherChangeType::Changed, changed);
                    }
                }
            }
        }

        DD("Exit loop");
        return true;
    }

    Path mPath;
    std::atomic_bool mRunning{false};
    std::thread mWatcherThread;
    Event mStarted;
    int mNotifyFd;
};

std::unique_ptr<FileSystemWatcher> FileSystemWatcher::getFileSystemWatcher(
        Path path,
        FileSystemWatcherCallback onChangeCallback) {
    if (!System::get()->pathIsDir(path)) {
        return nullptr;
    }
    return std::make_unique<FileSystemWatcherPosix>(path, onChangeCallback);
};
}  // namespace base
}  // namespace android
