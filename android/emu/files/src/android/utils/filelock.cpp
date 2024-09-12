/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/utils/filelock.h"

#include <fcntl.h>
#include <cassert>
#include <cerrno>
#include "aemu/base/logging/Log.h"
#include "android/base/system/System.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/file_io.h"
#include "android/utils/lock.h"
#include "android/utils/path.h"

#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#ifdef _WIN32
#include "aemu/base/files/ScopedFileHandle.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "aemu/base/system/Win32UnicodeString.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
using android::base::ScopedFileHandle;
#else
#include <sys/types.h>
#include <unistd.h>
#include <csignal>
#endif

using android::base::System;

// Set to 1 to enable debug traces here.
#if 0
#define D(...) dprint(__VA_ARGS__), fflush(stdout)
#else
#define D(...) ((void)0)
#endif

/** FILE LOCKS SUPPORT
 **
 ** A FileLock is useful to prevent several emulator instances from using
 ** the same writable file (e.g. the userdata.img disk images).
 **
 ** Create a FileLock object with filelock_create(), this function should
 ** return NULL only if the corresponding file path could not be locked.
 **
 ** All file locks are automatically released and destroyed when the program
 ** exits. The filelock_lock() function can also detect stale file locks
 ** that can linger when the emulator crashes unexpectedly, and will happily
 ** clean them for you
 **
 ** Here's how it works, three files are used:
 **     file  - the data file accessed by the emulator
 **     lock  - a lock file  (file + '.lock')
 **     temp  - a temporary file made unique with mkstemp
 **
 ** When locking:
 **      create 'temp' and store our pid in it
 **      attemp to link 'lock' to 'temp'
 **         if the link succeeds, we obtain the lock
 **      unlink 'temp'
 **
 ** When unlocking:
 **      unlink 'lock'
 **
 **/

/* Thread safety:
 *     _all_filelocks_tl: Hold this when accessing |_all_filelocks|.
 *                        This is locked at exit, so hold this for very, *very*
 *                        short.
 */
static CLock* all_filelocks_tl;
/* This is set to true during exit.
 * All accesses of _all_filelocks should strictly follow this scheme:
 *   android_lock_acquire(_all_filelocks_tl);
 *   if (!_is_exiting) {
 *     // Safe to access _all_filelocks now.
 *   }
 *   android_lock_release(_all_filelocks_tl);
 */
static bool is_exiting = false;

struct FileLock {
    const char* file;
    const char* lock;
    char* temp;
#ifdef _WIN32
    HANDLE lock_handle;
#endif
    int locked;
    FileLock* next;
};

/* used to cleanup all locks at emulator exit */
static FileLock* all_filelocks;

#define LOCK_NAME ".lock"
#define TEMP_NAME ".tmp-XXXXXX"
#define WIN_PIDFILE_NAME "pid"

void filelock_init() {
    all_filelocks_tl = android_lock_new();
}

#ifdef _WIN32
template <class Func>
static bool retry(Func func, int tries = 4, int timeoutMs = 100) {
    for (;;) {
        if (func()) {
            return true;
        }
        if (--tries == 0) {
            return false;
        }
        System::get()->sleepMs(timeoutMs);
    }
}

static bool delete_file(const android::base::Win32UnicodeString& name) {
    return retry([&name]() {
        // Make sure the file isn't marked readonly, or deletion may fail.
        SetFileAttributesW(name.c_str(), FILE_ATTRIBUTE_NORMAL);
        return ::DeleteFileW(name.c_str()) != 0 ||
               ::GetLastError() == ERROR_FILE_NOT_FOUND;
    });
}

static bool delete_dir(const android::base::Win32UnicodeString& name) {
    return retry([&name]() {
        return ::RemoveDirectoryW(name.c_str()) != 0 ||
               ::GetLastError() == ERROR_FILE_NOT_FOUND;
    });
}

/* returns 0 on success, -1 on failure */
static int filelock_lock(FileLock* lock, int timeout) {
    lock->lock_handle = nullptr;
    const auto unicodeDir = android::base::Win32UnicodeString(lock->lock);
    const auto unicodeName = android::base::Win32UnicodeString(lock->temp);
    static constexpr size_t pidCharsMax = 11;

    HANDLE lockHandle = INVALID_HANDLE_VALUE;
    bool createFileResult = false;
    int sleep_duration_ms = std::min(200, timeout);
    for (;;) {
        bool slept = false;
        if (!::CreateDirectoryW(unicodeDir.c_str(), nullptr) &&
            ::GetLastError() != ERROR_ALREADY_EXISTS) {
            derror("Unexpected error while creating: %s (error: %d)",
                   unicodeDir.toString(), ::GetLastError());
            continue;
        }

        // Open/create a lock file with a flags combination like:
        //  - open for writing only
        //  - allow only one process to open file for writing (our process)
        // Together, this guarantees that the file can only be opened when
        // our process is alive and keeps a handle to it.
        // As soon as our process ends or we close/delete it - other lock
        // can be acquired.
        // Note: FILE_FLAG_DELETE_ON_CLOSE doesn't work well here, as
        //  everyone else would need to open the file in FILE_SHARE_DELETE
        //  mode, and both Android Studio and cmd.exe don't use it. Fix
        //  at least the Android Studio part before trying to add this flag
        //  instead of the manual deletion code.
        lockHandle = ::CreateFileW(
                unicodeName.c_str(),
                GENERIC_WRITE,    // open only for writing
                FILE_SHARE_READ,  // allow others to read the file, but not
                                  // to write to it or not to delete it
                nullptr,          // no special security attributes
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                nullptr);  // no template file
        DWORD lastErr = GetLastError();
        if (lockHandle == INVALID_HANDLE_VALUE &&
            (lastErr == ERROR_ACCESS_DENIED ||
             lastErr == ERROR_SHARING_VIOLATION)) {
            ScopedFileHandle getpidHandle(::CreateFileW(
                    unicodeName.c_str(),
                    GENERIC_READ,  // open only for reading
                    FILE_SHARE_READ |
                            FILE_SHARE_WRITE,  // allow others to r+w the file
                                               // (we only read, but the write
                                               // flag needs to be there to fit
                                               // with the original process, or
                                               // this call fails)
                    nullptr,                   // default security
                    OPEN_EXISTING,             // existing file only
                    FILE_ATTRIBUTE_NORMAL,     // normal file
                    nullptr));                 // no template file

            if (getpidHandle.valid()) {
                // Read the pid of the locking process.
                char buf[pidCharsMax + 1] = {};
                DWORD bytesRead;
                if (::ReadFile(getpidHandle.get(), buf, pidCharsMax, &bytesRead,
                               nullptr)) {
                    DWORD lockingPid;
                    if (sscanf(buf, "%lu", &lockingPid) == 1) {
                        // Try waiting for the specified timeout for
                        // the locking process to exit. If that doesn't work,
                        // bail.
                        System::get()->waitForProcessExit(lockingPid,
                                                          sleep_duration_ms);
                        slept = true;
                    }
                }
                // Sometimes a previous file gets readonly attribute even when
                // its parent process has exited; that prevents us from opening
                // it for writing.
                SetFileAttributesW(unicodeName.c_str(), FILE_ATTRIBUTE_NORMAL);
            }
        }
        if (lockHandle != INVALID_HANDLE_VALUE) {
            createFileResult = true;
            break;
        }
        if (timeout == 0) {
            break;
        }
        if (sleep_duration_ms > 0) {
            if (!slept) {
                System::get()->sleepMs(sleep_duration_ms);
            }
            timeout -= sleep_duration_ms;
        }
        sleep_duration_ms = std::min(sleep_duration_ms, timeout);
    }

    if (!createFileResult) {
        assert(lockHandle == INVALID_HANDLE_VALUE);
        return -1;
    }

    assert(lockHandle != INVALID_HANDLE_VALUE);

    // Make sure we kill the file/directory on any failure.
    auto fileDeleter = android::base::makeCustomScopedPtr(
            lockHandle, [&unicodeName, &unicodeDir](HANDLE h) {
                ::CloseHandle(h);
                delete_file(unicodeName);
                delete_dir(unicodeDir);
            });

    // We're good. Now write down the process ID as Android Studio relies on it.
    const DWORD pid = ::GetCurrentProcessId();
    char pidBuf[pidCharsMax + 1] = {};
    const int len = snprintf(pidBuf, sizeof(pidBuf), "%lu", pid);
    assert(len > 0 && len < (int)sizeof(pidBuf));
    DWORD bytesWritten;
    if (!::WriteFile(lockHandle, pidBuf, static_cast<DWORD>(len), &bytesWritten,
                     nullptr) ||
        bytesWritten != static_cast<DWORD>(len)) {
        D("Failed to write the current PID into the lock file");
        return -1;
    }
    lock->locked = 1;
    lock->lock_handle = fileDeleter.release();
    return 0;  // we're all good
}
#else
/* returns 0 on success, -1 on failure */
static auto filelock_lock(FileLock* lock, int timeout) -> int {
    int ret;
    int temp_fd = -1;
    int lock_fd = -1;
    int rc;
    FILE* f = nullptr;
    char pid[8];
    struct stat st_temp;
    int sleep_duration_ms = 0;

    temp_fd = mkstemp(lock->temp);

    if (temp_fd < 0) {
        D("Cannot create locking temp file '%s'", lock->temp);
        goto Fail;
    }

    snprintf(pid, sizeof pid, "%d", getpid());
    ret = HANDLE_EINTR(write(temp_fd, pid, strlen(pid) + 1));
    if (ret < 0) {
        D("Cannot write to locking temp file '%s'", lock->temp);
        goto Fail;
    }
    close(temp_fd);
    temp_fd = -1;

    rc = HANDLE_EINTR(lstat(lock->temp, &st_temp));
    if (rc < 0) {
        D("Can't properly stat our locking temp file '%s'", lock->temp);
        goto Fail;
    }

    /* now attempt to link the temp file to the lock file */
    for (;;) {
        rc = HANDLE_EINTR(link(lock->temp, lock->lock));
        if (rc == 0) {
            /* The link() operation suceeded */
            lock->locked = 1;
            rc = HANDLE_EINTR(android_unlink(lock->temp));
            return 0;
        }

        struct stat st_lock;
        rc = HANDLE_EINTR(lstat(lock->lock, &st_lock));
        if (rc != 0) {
            // Try again after sleeping a little.
            continue;
        }

        if (S_ISDIR(st_lock.st_mode)) {
            char* win_pid;
            int win_pid_len;
            // The .lock file is a directory. This can only happen
            // when the AVD was previously used by a Win32 emulator
            // instance running under Wine on the same machine.
            dwarning("Stale Win32 lock file detected: %s", lock->lock);

            /* Try deleting the pid file dropped in windows.
             * Ignore error -- try blowing away the directory anyway.
             */
            win_pid_len = strlen(lock->lock) + 1 + sizeof(WIN_PIDFILE_NAME);
            win_pid = static_cast<char*>(malloc(win_pid_len));
            snprintf(win_pid, win_pid_len, "%s" PATH_SEP WIN_PIDFILE_NAME,
                     lock->lock);
            HANDLE_EINTR(android_unlink(win_pid));
            free(win_pid);

            rc = HANDLE_EINTR(android_rmdir(lock->lock));
            if (rc != 0) {
                D("Removing stale Win32 lockfile '%s' failed (%s)", lock->lock,
                  strerror(errno));
            }

            goto Fail;
        }
        /* if we get there, it means that the link() call failed */
        /* check the lockfile to see if it is stale              */
        using Freshness = enum {
            FRESHNESS_UNKNOWN = 0,
            FRESHNESS_FRESH,
            FRESHNESS_STALE,
        };

        Freshness freshness = FRESHNESS_UNKNOWN;

        struct stat st;
        time_t now;
        rc = HANDLE_EINTR(time(&now));
        st.st_mtime = now - 120;

        int lockpid = 0;
        int lockfd = HANDLE_EINTR(open(lock->lock, O_RDONLY));
        if (lockfd >= 0) {
            char buf[16];
            int len = HANDLE_EINTR(read(lockfd, buf, sizeof(buf) - 1U));
            if (len < 0) {
                len = 0;
            }
            buf[len] = 0;
            lockpid = atoi(buf);

            rc = HANDLE_EINTR(fstat(lockfd, &st));
            if (rc == 0) {
                now = st.st_atime;
            }
            IGNORE_EINTR(close(lockfd));
        }
        /* if there is a PID, check that it is still alive */
        bool slept = false;
        if (lockpid > 0) {
            rc = HANDLE_EINTR(kill(lockpid, 0));
            if (rc == 0 || errno == EPERM) {
                freshness = FRESHNESS_FRESH;
                // Try checking the locking process to exit.
                // If that doesn't work, bail.
                // If we waited until the process exited, the process
                // is not fresh anymore.
                if (System::get()->waitForProcessExit(lockpid,
                                                      sleep_duration_ms) ==
                    System::WaitExitResult::Exited) {
                    freshness = FRESHNESS_STALE;
                }
                slept = true;
            } else if (rc < 0 && errno == ESRCH) {
                freshness = FRESHNESS_STALE;
            }
        }
        if (freshness == FRESHNESS_UNKNOWN) {
            /* no pid, stale if the file is older than 1 minute */
            freshness = (now >= st.st_mtime + 60) ? FRESHNESS_STALE
                                                  : FRESHNESS_FRESH;
        }

        if (freshness == FRESHNESS_STALE) {
            D("Removing stale lockfile '%s'", lock->lock);
            rc = HANDLE_EINTR(android_unlink(lock->lock));
        }

        const int kSleepDurationMsIncrement = 50;
        if (timeout == 0) {
            break;
        }
        if (sleep_duration_ms > 0) {
            timeout -= sleep_duration_ms;
            if (!slept) {
                System::get()->sleepMs(sleep_duration_ms);
            }
        }
        sleep_duration_ms = std::min(
                timeout, sleep_duration_ms + kSleepDurationMsIncrement);
    }
    D("file '%s' is already in use by another process", lock->file);

Fail:
    if (f != nullptr) {
        fclose(f);
    }

    if (temp_fd >= 0) {
        IGNORE_EINTR(close(temp_fd));
    }

    if (lock_fd >= 0) {
        IGNORE_EINTR(close(lock_fd));
    }

    HANDLE_EINTR(android_unlink(lock->temp));
    return -1;
}
#endif

void filelock_release(FileLock* lock) {
    if (lock->locked != 0) {
#ifdef _WIN32
        ::CloseHandle(lock->lock_handle);
        lock->lock_handle = nullptr;
        delete_file(android::base::Win32UnicodeString(lock->temp));
        delete_dir(android::base::Win32UnicodeString(lock->lock));
#else
        android_unlink(const_cast<char*>(lock->lock));
#endif
        free(const_cast<char*>(lock->file));
        lock->file = lock->lock = lock->temp = nullptr;
        lock->locked = 0;
    }
}

static void filelock_atexit() {
    android_lock_acquire(all_filelocks_tl);
    if (!is_exiting) {
        for (FileLock* lock = all_filelocks; lock != nullptr;) {
            filelock_release(lock);
            FileLock* const prev = lock;
            lock = lock->next;
            delete prev;
        }
    }
    is_exiting = true;
    android_lock_release(all_filelocks_tl);
    // We leak |_all_filelocks_tl| here. We can never guarantee that another
    // thread isn't trying to use filelock concurrently, so we can not safely
    // clean up the mutex. See b.android.com/209635
}

auto filelock_create(const char* file) -> FileLock* {
    return filelock_create_timeout(file, 0);
}

/* create a file lock */
auto filelock_create_timeout(const char* file, int timeout) -> FileLock* {
    if (file == nullptr) {
        return nullptr;
    }

    int file_len = strlen(file);
    uint64_t lock_len = file_len + sizeof(LOCK_NAME);
#ifdef _WIN32
    int temp_len = lock_len + 1 + sizeof(WIN_PIDFILE_NAME);
#else
    uint64_t temp_len = file_len + sizeof(TEMP_NAME);
#endif
    const int paths_len = file_len + lock_len + temp_len + 3;

    auto* const lock = new FileLock();
    char* const paths = static_cast<char*>(malloc(paths_len));

    lock->file = paths;
    memcpy(const_cast<char*>(lock->file), file, file_len + 1);

    lock->lock = lock->file + file_len + 1;
    memcpy(const_cast<char*>(lock->lock), file, file_len + 1);
    strcat(const_cast<char*>(lock->lock), LOCK_NAME);

    lock->temp = const_cast<char*>(lock->lock) + lock_len + 1;
#ifdef _WIN32
    snprintf((char*)lock->temp, temp_len, "%s\\" WIN_PIDFILE_NAME, lock->lock);
#else
    snprintf(lock->temp, temp_len, "%s%s", lock->file, TEMP_NAME);
#endif
    lock->locked = 0;

    if (filelock_lock(lock, timeout) < 0) {
        free(paths);
        delete lock;
        return nullptr;
    }

    android_lock_acquire(all_filelocks_tl);
    if (is_exiting) {
        android_lock_release(all_filelocks_tl);
        filelock_release(lock);
        free(lock);
        return nullptr;
    }

    lock->next = all_filelocks;
    all_filelocks = lock;
    android_lock_release(all_filelocks_tl);

    if (lock->next == nullptr) {
        atexit(filelock_atexit);
    }

    return lock;
}
