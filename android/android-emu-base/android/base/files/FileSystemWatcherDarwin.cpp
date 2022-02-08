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
#include <errno.h>  // for errno
#include <fcntl.h>  // for fcntl, open, F_GET...
#include <inttypes.h>
#include <limits.h>     // for PATH_MAX
#include <stdio.h>      // for size_t
#include <string.h>     // for strerror
#include <sys/event.h>  // for kevent, kqueue
#include <sys/stat.h>
#include <unistd.h>  // for close
#include <atomic>    // for atomic_bool
#include <chrono>
#include <filesystem>  // for path, operator<
#include <functional>  // for function
#include <memory>
#include <string>         // for basic_string
#include <thread>         // for thread
#include <unordered_map>  // for unordered_map<>::i...
#include <utility>        // for move
#include <vector>         // for vector

#include <CoreServices/CoreServices.h>
#include "android/base/files/FileSystemWatcher.h"  // for FileSystemWatcher:...
#include "android/base/synchronization/Event.h"
#include "android/base/system/System.h"  // for System
#include "android/utils/debug.h"         // for derror

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                        \
    printf("FileSystemWatcherKQueue: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace base {

using Path = FileSystemWatcher::Path;
// struct PathHash {
//     std::size_t operator()(Path const& p) const noexcept {
//         return Path::hash_value(p);
//     }
// };

// Maps the path to the actual file stat.
using PathMap = std::unordered_map<Path, struct stat>;

static PathMap scanDir(Path scanPath) {
    PathMap paths;
    for (auto strPath : System::get()->scanDirEntries(scanPath.c_str(), true)) {
        struct stat fstat;
        stat(strPath.c_str(), &fstat);
        paths[strPath] = fstat;
        DD("Added %s (%d)", strPath.c_str(), fstat.st_size);
    }
    return paths;
}

bool statEqual(struct stat lhs, struct stat rhs) {
    return lhs.st_dev == rhs.st_dev && lhs.st_ino == rhs.st_ino &&
           lhs.st_mode == rhs.st_mode && lhs.st_nlink == rhs.st_nlink &&
           lhs.st_uid == rhs.st_uid && lhs.st_gid == rhs.st_gid &&
           lhs.st_rdev == rhs.st_rdev && lhs.st_size == rhs.st_size &&
           lhs.st_flags == rhs.st_flags &&
           lhs.st_mtimespec.tv_sec == rhs.st_mtimespec.tv_sec &&
           lhs.st_mtimespec.tv_nsec == rhs.st_mtimespec.tv_nsec;
}

// Scan's the given path, invoking the callback if the current scan state
// results in a mismatch.
static PathMap scanAndNotify(
        Path scanPath,
        const PathMap& oldState,
        FileSystemWatcher::FileSystemWatcherCallback callback) {
    // dir add/remove
    auto currentState = scanDir(scanPath);
    for (const auto& [name, state] : oldState) {
        if (currentState.count(name) == 0) {
            // name got deleted.
            callback(FileSystemWatcher::WatcherChangeType::Deleted, name);
        } else if (!statEqual(currentState[name], state)) {
            // name changed
            DD("Name change due to stat mismatch for %s", name.c_str());
            callback(FileSystemWatcher::WatcherChangeType::Changed, name);
        }
    }
    for (auto& [name, state] : currentState) {
        if (oldState.count(name) == 0) {
            // The creation of a file or folder.
            callback(FileSystemWatcher::WatcherChangeType::Created, name);
        }
    }

    return currentState;
}

// File system events based upon kernel queues
// https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/FSEvents_ProgGuide/KernelQueues/KernelQueues.html
class FileSystemWatcherKQueue : public FileSystemWatcher {
public:
    FileSystemWatcherKQueue(Path path,
                            FileSystemWatcherCallback onChangeCallback)
        : FileSystemWatcher(onChangeCallback),
          mPath(path),
          mDirectoryView(scanDir(path)) {}

    ~FileSystemWatcherKQueue() { stop(); }

    bool start() override {
        bool expected = false;
        if (!mRunning.compare_exchange_strong(expected, true)) {
            return false;
        }
        std::thread watcher([this] { watchForChanges(); });
        mWatcherThread = std::move(watcher);
        mStarted.wait();
        return mPipe[0] != -1;
    }

    void stop() override {
        bool expected = true;
        if (mRunning.compare_exchange_strong(expected, false)) {
            DD("Stopping thread.");
            write(mPipe[1], "x", 1);
            DD("Done");
            mWatcherThread.join();
        }
    }

private:
    bool watchForChanges() {
        constexpr int NUM_EVENT_SLOTS = 1;
        constexpr int NUM_EVENT_FDS = 2;
        int kq;
        int event_fd;
        struct kevent events_to_monitor[NUM_EVENT_FDS];
        struct kevent event_data[NUM_EVENT_SLOTS];
        void* user_data;
        unsigned int vnode_events;

        if ((kq = kqueue()) < 0) {
            derror("Unable to watch for changes, kqueue error: %s.",
                   strerror(errno));
            mStarted.signal();
            return false;
        }

        if (pipe(mPipe) != 0 || (fcntl(mPipe[0], F_SETFL, O_NONBLOCK) < 0)) {
            derror("Unable to open pipe: %s", strerror(errno));
            mPipe[1] = -1;
            mStarted.signal();
            return false;
        };

        DD("Pipes: %d <-> %d", mPipe[0], mPipe[1]);

        event_fd = open(mPath.c_str(), O_EVTONLY);
        if (event_fd <= 0) {
            derror("The file %s could not be opened for monitoring.  Error "
                   "was %s.",
                   mPath.c_str(), strerror(errno));
            close(mPipe[0]);
            close(mPipe[1]);
            mPipe[1] = -1;
            mStarted.signal();
            return false;
        }

        // Pipe is used to signal exit.
        EV_SET(events_to_monitor, mPipe[0], EVFILT_READ, EV_ADD, 0, 0, NULL);

        vnode_events = NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB |
                       NOTE_LINK | NOTE_RENAME | NOTE_REVOKE;
        EV_SET(events_to_monitor + 1, event_fd, EVFILT_VNODE,
               EV_ADD | EV_ENABLE | EV_ONESHOT, vnode_events, 0, nullptr);

        /* Handle events. */
        int num_files = 1;
        mStarted.signal();
        while (mRunning) {
            int event_count = kevent(kq, events_to_monitor, NUM_EVENT_FDS,
                                     event_data, num_files, 0);
            DD("Got %d events\n", event_count);
            if (mRunning) {
                if ((event_count < 0) || (event_data[0].flags == EV_ERROR)) {
                    /* An error occurred. */
                    derror("An error occurred (event count %d).  The error "
                           "was "
                           "%s.",
                           event_count, strerror(errno));
                    break;
                }
                if (event_count) {
                    DD("fd: %d, Event %" PRIdPTR
                       " occurred.  Filter %d, flags %d, filter flags %s, "
                       "filter data %" PRIdPTR ", path %s\n",
                       event_fd, event_data[0].ident, event_data[0].filter,
                       event_data[0].flags, flagstring(event_data[0].fflags),
                       event_data[0].data, (char*)event_data[0].udata);
                    // Okay, let's interpret the event
                    handleChange(event_fd, event_data);
                }
            }
        }

        close(event_fd);
        close(mPipe[0]);
        close(mPipe[1]);
        return true;
    }

    /* A simple routine to return a string for a set of flags. */
    char* flagstring(int flags) {
        static char ret[512];
        char* orr = "";

        ret[0] = '\0';  // clear the string.
        if (flags & NOTE_DELETE) {
            strcat(ret, orr);
            strcat(ret, "NOTE_DELETE");
            orr = "|";
        }
        if (flags & NOTE_WRITE) {
            strcat(ret, orr);
            strcat(ret, "NOTE_WRITE");
            orr = "|";
        }
        if (flags & NOTE_EXTEND) {
            strcat(ret, orr);
            strcat(ret, "NOTE_EXTEND");
            orr = "|";
        }
        if (flags & NOTE_ATTRIB) {
            strcat(ret, orr);
            strcat(ret, "NOTE_ATTRIB");
            orr = "|";
        }
        if (flags & NOTE_LINK) {
            strcat(ret, orr);
            strcat(ret, "NOTE_LINK");
            orr = "|";
        }
        if (flags & NOTE_RENAME) {
            strcat(ret, orr);
            strcat(ret, "NOTE_RENAME");
            orr = "|";
        }
        if (flags & NOTE_REVOKE) {
            strcat(ret, orr);
            strcat(ret, "NOTE_REVOKE");
            orr = "|";
        }
        return ret;
    }

    void handleChange(int fd, struct kevent* event_data) {
        int flags = event_data->fflags;
        if (flags & NOTE_DELETE) {
            mChangeCallback(WatcherChangeType::Deleted, mPath);
        } else if (flags & NOTE_ATTRIB) {
            mChangeCallback(WatcherChangeType::Changed, mPath);
        } else if (flags & NOTE_RENAME) {
            char newPath[PATH_MAX] = {0};
            fcntl(fd, F_GETPATH, newPath);
            mChangeCallback(WatcherChangeType::Deleted, mPath);
            mChangeCallback(WatcherChangeType::Created, newPath);
        } else {
            mDirectoryView = std::move(
                    scanAndNotify(mPath, mDirectoryView, mChangeCallback));
        }
    }

    Path mPath;
    PathMap mDirectoryView{};
    std::atomic_bool mRunning{false};
    std::thread mWatcherThread;
    Event mStarted;
    int mPipe[2];
};

// Filesystem watcher based on
// https://developer.apple.com/documentation/coreservices/file_system_events
// api.
class FileSystemWatcherFS : public FileSystemWatcher {
public:
    FileSystemWatcherFS(Path path, FileSystemWatcherCallback onChangeCallback)
        : FileSystemWatcher(onChangeCallback),
          mPath(path),
          mDirectoryView(scanDir(path)) {}

    ~FileSystemWatcherFS() { stop(); }

    bool start() override {
        bool expected = false;
        if (!mRunning.compare_exchange_strong(expected, true)) {
            return false;
        }
        std::thread watcher([this] { watchForChanges(); });
        mWatcherThread = std::move(watcher);
        mStarted.wait();
        return mCfRunLoop != nullptr;
    }

    void stop() override {
        bool expected = true;
        if (mRunning.compare_exchange_strong(expected, false)) {
            DD("Stopping loop.");
            CFRunLoopStop(mCfRunLoop);
            mWatcherThread.join();
        }
    }

private:
    static void watcherCb(ConstFSEventStreamRef,
                          void* clientCallBackInfo,
                          size_t numEvents,
                          void* eventPaths,
                          const FSEventStreamEventFlags eventFlags[],
                          const FSEventStreamEventId*) {
        // Sadly the event does not include the actual filepath, but only the
        // root path. so, best we can do is rescan the structure and see what
        // has changed.
        // See:
        // https://developer.apple.com/documentation/coreservices/fseventstreamcallback
        // for details.
        auto* watcher = static_cast<FileSystemWatcherFS*>(clientCallBackInfo);
        watcher->mDirectoryView =
                std::move(scanAndNotify(watcher->mPath, watcher->mDirectoryView,
                                        watcher->mChangeCallback));
    }

    bool watchForChanges() {
        mCfRunLoop = nullptr;
        auto dir = CFStringCreateWithCString(nullptr, mPath.c_str(),
                                             kCFStringEncodingUTF8);
        auto pathsToWatch =
                CFArrayCreate(nullptr, reinterpret_cast<const void**>(&dir), 1,
                              &kCFTypeArrayCallBacks);

        FSEventStreamContext streamCtx = {0, this, NULL, NULL, NULL};
        auto stream = FSEventStreamCreate(
                nullptr, &FileSystemWatcherFS::watcherCb, &streamCtx,
                pathsToWatch, kFSEventStreamEventIdSinceNow, 0,
                kFSEventStreamCreateFlagFileEvents |
                        kFSEventStreamCreateFlagNoDefer);

        if (!stream) {
            mStarted.signal();
            return false;
        }

        // Fire the event loop
        mCfRunLoop = CFRunLoopGetCurrent();
        FSEventStreamScheduleWithRunLoop(stream, mCfRunLoop,
                                         kCFRunLoopDefaultMode);
        FSEventStreamStart(stream);

        DD("Starting run loop.");
        mStarted.signal();
        CFRunLoopRun();

        DD("Done..");
        FSEventStreamStop(stream);
        FSEventStreamInvalidate(stream);
        FSEventStreamRelease(stream);

        return true;
    }

    Path mPath;
    std::atomic_bool mRunning{false};
    std::thread mWatcherThread;
    Event mStarted;
    PathMap mDirectoryView{};

    CFRunLoopRef mCfRunLoop;
};

std::unique_ptr<FileSystemWatcher> FileSystemWatcher::getFileSystemWatcher(
        Path path,
        FileSystemWatcherCallback onChangeCallback) {
    if (!System::get()->pathIsDir(path)) {
        return nullptr;
    }
    return std::make_unique<FileSystemWatcherFS>(path, onChangeCallback);
};
}  // namespace base
}  // namespace android