// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/Compiler.h"
#include "android/base/CpuTime.h"
#include "android/base/EnumFlags.h"
#include "android/base/Optional.h"
#include "android/base/StringView.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

#include <limits.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#undef ERROR  // necessary to compile LOG(ERROR) statements
#else  // !_WIN32
#ifndef _MSC_VER
#include <unistd.h>
#endif
#endif        // !_WIN32


namespace android {
namespace base {

// Type of the current operating system
enum class OsType {
    Windows,
    Mac,
    Linux
};

std::string toString(OsType osType);

enum class RunOptions {
    // Can't use None here: X.h defines None to 0L.
    Empty = 0,

    // some pseudo flags to just state the default behavior
    DontWait = 0,
    HideAllOutput = 0,

    // Wait for the launched shell command to finish, and return true only if
    // the command was successful.
    WaitForCompletion = 1,
    // Attempt to terminate the launched process if it doesn't finish in time.
    // Note that terminating a mid-flight process can leave the whole system in
    // a weird state.
    // Only make sense with |WaitForCompletion|.
    TerminateOnTimeout = 2,

    // These flags and RunOptions::HideAllOutput are mutually exclusive.
    ShowOutput = 4,
    DumpOutputToFile = 8,

    Default = 0,  // don't wait, hide all output
};

// Interface class to the underlying operating system.
class System {
public:
    typedef int64_t  Duration;
    typedef uint64_t WallDuration;
    using FileSize = uint64_t;

    // Information about user, system and wall clock times for some process,
    // in milliseconds
    struct Times {
        Duration userMs;
        Duration systemMs;
        WallDuration wallClockMs;
    };

    enum { RunFailed = -1 };

    using RunOptions = android::base::RunOptions;

#ifdef _WIN32
    using Pid = DWORD;
    using ProcessExitCode = DWORD;
#else
    using Pid = pid_t;
    using ProcessExitCode = int;
#endif

public:
    // Call this function to get the instance
    static System* get();

    // Default constructor doesn't do anything.
    System() = default;

    // Default destructor is empty but virtual.
    virtual ~System() = default;

    // Return the host bitness as an integer, either 32 or 64.
    // Note that this is different from the program's bitness. I.e. if
    // a 32-bit program runs under a 64-bit host, getProgramBitness()
    // shall return 32, but getHostBitness() shall return 64.
    virtual int getHostBitness() const = 0;

    // Return the current OS type
    virtual OsType getOsType() const = 0;

    // Return the current OS product/version name.
    // Return error string in the format of "Error: [reason]"
    // if we are unable to get host OS information or error
    // occurs.
    virtual std::string getOsName() = 0;

    // The major os version that this code is running on in
    // the form of '[0-9]+\.[0-9]+'
    virtual std::string getMajorOsVersion() const = 0;

    // Check if we're running under Wine;
    // returns false for non-Windows builds
    virtual bool isRunningUnderWine() const = 0;

    // Get the current process ID
    virtual Pid getCurrentProcessId() const = 0;

    // Wait for some process with ID |pid| to exit,
    // with a maximum timeout |timeoutMs| in milliseconds.
    // Returns true if we succesfully found the process and waited it out,
    // or if there was no such process in the first place.
    // Returns false if something went wrong.
    enum class WaitExitResult {
        Exited = 0,
        Timeout = 1,
        Error = 2,
    };
    virtual WaitExitResult waitForProcessExit(int pid, Duration timeoutMs) const = 0;

    // Get the number of hardware CPU cores available. Hyperthreading cores are
    // counted as separate here.
    virtual int getCpuCoreCount() const = 0;

    // Gets memory statistics.
    struct MemUsage {
        uint64_t resident;
        uint64_t resident_max;
        uint64_t virt;
        uint64_t virt_max;
        uint64_t total_phys_memory;
        uint64_t avail_phys_memory;
        uint64_t total_page_file;
    };
    virtual MemUsage getMemUsage() const = 0;

    // Returns just the free RAM on the system. Useful in many cases.
    static int freeRamMb();

    // Measures whether or not the system is considered in a memory pressure
    // state, and returns true if so. Optionally, a freeRamMb output pointer
    // can be given so the caller can see how much RAM is actually free.
    static constexpr int kMemoryPressureLimitMb = 513;
    static bool isUnderMemoryPressure(int* freeRamMb = nullptr);

    static constexpr System::FileSize kDiskPressureLimitBytes = 2147483648ULL;
    static bool isUnderDiskPressure(StringView path, System::FileSize* freeDisk = nullptr);

    static System::FileSize getFilePageSizeForPath(StringView path);
    static System::FileSize getAlignedFileSize(System::FileSize align, System::FileSize size);

    // Return the program bitness as an integer, either 32 or 64.
#ifdef __x86_64__
    static const int kProgramBitness = 64;
#else
    static const int kProgramBitness = 32;
#endif

#ifdef _WIN32
    static const char kDirSeparator = '\\';
#else
    static const char kDirSeparator = '/';
#endif

    // The character used to separator directories in path-related
    // environment variables.
#ifdef _WIN32
    static const char kPathSeparator = ';';
#else
    static const char kPathSeparator = ':';
#endif

    // Environment variable name corresponding to the library search
    // list for shared libraries.
    static const char* kLibrarySearchListEnvVarName;

    // Return the name of the sub-directory containing libraries
    // for the current platform, i.e. "lib" or "lib64" depending
    // on the value of kProgramBitness.
    static const char* kLibSubDir;

    // Return the name of the sub-directory containing executables
    // for the current platform, i.e. "bin" or "bin64" depending
    // on the value of kProgramBitness.
    static const char* kBinSubDir;

    // Name of the 32-bit binaries subdirectory
    static const char* kBin32SubDir;

    // Return program's bitness, either 32 or 64.
    static int getProgramBitness() { return kProgramBitness; }

    // /////////////////////////////////////////////////////////////////////////
    // Environment variables.
    // /////////////////////////////////////////////////////////////////////////

    // Retrieve the value of a given environment variable.
    // Equivalent to getenv() but returns a std::string instance.
    // If the variable is not defined, return an empty string.
    // NOTE: On Windows, this uses _wgetenv() and returns the corresponding
    // UTF-8 text string.
    virtual std::string envGet(StringView varname) const = 0;

    // Set the value of a given environment variable.
    // If |varvalue| is NULL or empty, this unsets the variable.
    // Equivalent to setenv().
    virtual void envSet(StringView varname, StringView varvalue) = 0;

    // Returns true if environment variable |varname| is set and non-empty.
    virtual bool envTest(StringView varname) const = 0;

    // Returns all environment variables from the current process in a
    // "name=value" form
    virtual std::vector<std::string> envGetAll() const = 0;

    // Prepend a new directory to the system's library search path. This
    // only alters an environment variable like PATH or LD_LIBRARY_PATH,
    // and thus typically takes effect only after spawning/executing a new
    // process.
    static void addLibrarySearchDir(StringView dirPath);

    // /////////////////////////////////////////////////////////////////////////
    // Path functions that interact with the file system.
    //     Pure path manipulation functions are in android::base::PathUtils.
    // /////////////////////////////////////////////////////////////////////////

    // Return true iff |path| exists on the file system.
    virtual bool pathExists(StringView path) const = 0;

    // Return true iff |path| exists and is a regular file on the file system.
    virtual bool pathIsFile(StringView path) const = 0;

    // Return true iff |path| exists and is a directory on the file system.
    virtual bool pathIsDir(StringView path) const = 0;

    // Return true iff |path| exists and is a symbolic link the file system.
    virtual bool pathIsLink(StringView path) const = 0;

    // Return true iff |path| exists and can be read by the current user.
    virtual bool pathCanRead(StringView path) const = 0;

    // Return true iff |path| exists and can be written to by the current
    // user.
    virtual bool pathCanWrite(StringView path) const = 0;

    // Return true iff |path| exists and can be executed to by the current
    // user.
    virtual bool pathCanExec(StringView path) const = 0;

    // A wrapper for int open(filename, oflag, pmode) to support unicode paths
    // on Windows.
    virtual int pathOpen(const char *filename, int oflag, int pmode) const = 0;

    // Function for deleting files. Return true iff
    // (|path| is a file and we have successfully deleted it)
    virtual bool deleteFile(StringView path) const = 0;

    // Get the size of file at |path|.
    // Fails if path is not a file or not readable, and in case of other errors.
    virtual bool pathFileSize(StringView path, FileSize* outFileSize) const = 0;
    virtual bool fileSize(int fd, FileSize* outFileSize) const = 0;
    Optional<FileSize> pathFileSize(StringView path) {
        FileSize res;
        return pathFileSize(path, &res) ? makeOptional(res) : kNullopt;
    }
    Optional<FileSize> fileSize(int fd) {
        FileSize res;
        return fileSize(fd, &res) ? makeOptional(res) : kNullopt;
    }

    // Get the size of the directory at |path|. Include all files
    // and subdirectories, recursively.
    // If |path| is a regular file, return the size of that file.
    virtual FileSize recursiveSize(StringView path) const = 0;

    // Get the amount of free disk space, in bytes, at |path|.
    // Returns 'false' on error.
    virtual bool pathFreeSpace(StringView path, FileSize* spaceInBytes) const = 0;

    // Locate a command if it is not an absolute path, note that on win32 the
    // current directory is included in the search!
    virtual Optional<std::string> which(StringView executable) const = 0;

    // Gets the file creation timestamp as a Unix epoch time with microsecond
    // resolution. Returns an empty optional for systems that don't support
    // creation times (Linux) or if the operation failed.
    virtual Optional<Duration> pathCreationTime(StringView path) const = 0;

    // Gets the file modification timestamp as a Unix epoch time with
    // microsecond resolution. Returns an empty optional for systems that don't
    // support modification times or if the operation failed.
    virtual Optional<Duration> pathModificationTime(StringView path) const = 0;

    // Known distinct kinds of disks.
    enum class DiskKind {
        Hdd,
        Ssd
    };
    virtual Optional<DiskKind> pathDiskKind(StringView path) = 0;
    virtual Optional<DiskKind> diskKind(int fd) = 0;

    // Scan directory |dirPath| for entries, and return them as a sorted
    // vector or entries. If |fullPath| is true, then each item of the
    // result vector contains a full path.
    virtual std::vector<std::string> scanDirEntries(
            StringView dirPath,
            bool fullPath = false) const = 0;

    // Find a bundled executable named |programName|, it must appear in the
    // kBinSubDir of getLauncherDirectory(). The name should not include the
    // executable extension (.exe) on Windows.
    // Return an empty string if the file doesn't exist.
    static std::string findBundledExecutable(StringView programName);

    // Return the path of the current program's directory.
    virtual const std::string& getProgramDirectory() const = 0;

    // Return the path of the emulator launcher's directory.
    virtual const std::string& getLauncherDirectory() const = 0;

    // Return the path to user's home directory (as defined in the
    // underlying platform) or an empty string if it can't be found
    virtual const std::string& getHomeDirectory() const = 0;

    // Return the path to user's App Data directory (only applies
    // in Microsoft Windows) or an empty string if it can't be found
    virtual const std::string& getAppDataDirectory() const = 0;

    // Return the current directory path. Because this can change at
    // runtime, this returns a new std::string instance, not a const-reference
    // to a constant one held by the object. Return an empty string if there is
    // a problem with the system when getting the current directory.
    virtual std::string getCurrentDirectory() const = 0;

    // Set the current directory path. Returns true if the directory was
    // successfully changed.
    virtual bool setCurrentDirectory(StringView directory) = 0;

    // Return the path of a temporary directory appropriate for the system.
    virtual std::string getTempDir() const = 0;

    // Return if enable the crash reporting
    virtual bool getEnableCrashReporting() const = 0;

    // /////////////////////////////////////////////////////////////////////////
    // Time related functions.
    // /////////////////////////////////////////////////////////////////////////

    // Checks the system to see if it is running under a remoting session
    // like Nomachine's NX, Chrome Remote Desktop or Windows Terminal Services.
    // On success, return true and sets |*sessionType| to the detected
    // session type. Otherwise, just return false.
    virtual bool isRemoteSession(std::string* sessionType) const = 0;

    // Returns Times structure for the current process
    virtual Times getProcessTimes() const = 0;

    // Returns the current Unix timestamp
    virtual time_t getUnixTime() const = 0;

    // Returns the current Unix timestamp with microsecond resolution
    virtual Duration getUnixTimeUs() const = 0;

    // Returns the OS-specific high resolution timestamp.
    virtual WallDuration getHighResTimeUs() const = 0;

    // Sleep for |n| milliseconds
    virtual void sleepMs(unsigned n) const = 0;

    // Sleep for |n| microseconds
    virtual void sleepUs(unsigned n) const = 0;

    // Yield the remaining part of current thread's CPU time slice to another
    // thread that's ready to run.
    virtual void yield() const = 0;

    // Setup system specific handlers. For example on msvc you might
    // want to redirect parameter validation.
    virtual void configureHost() const = 0;

    virtual void cleanupWaitingPids() const = 0;

    // /////////////////////////////////////////////////////////////////////////
    // Execute commands.
    // /////////////////////////////////////////////////////////////////////////

    // Run a shell command.
    // Args:
    //     commandLine: Set of command to run + its arguments
    //     options: options to controls shell behaviour in various ways.
    //     timeoutMs: If |options| includes WaitForCompletion, this argument
    //             specifies the maximum time to wait before aborting the
    //             command. Waits forever if set to 0.
    //     outExitCode: An optional pointer to an existing location where the
    //             exit code for the launched process is returned.
    //     outChildPid: An optional pointer to an existing location where the
    //             pid for the launched child process is returned. Only valid
    //             if function returns true.
    //     outTempFile: An optional string containing the name of a file to
    //             redirect stdout and stderr to. Only used if |options|
    //             includes DumpOutputToFile.
    //
    // Returns true if the function succeeded in running the command. If you
    // request |RunOptions::WaitForCompletion|, this does not mean that the
    // command finished with "success" exit status, just that we succeeded in
    // running it, cf |outExitCode|.
    static const System::Duration kInfinite = 0;
    virtual bool runCommand(const std::vector<std::string>& commandLine,
                            RunOptions options = RunOptions::Default,
                            System::Duration timeoutMs = kInfinite,
                            System::ProcessExitCode* outExitCode = nullptr,
                            System::Pid* outChildPid = nullptr,
                            const std::string& outputFile = "") = 0;

    // Run a shell command, and returning the generated stderr/stdout in a string.
    //
    // Args:
    //     commandLine: Set of command to run + its arguments
    //     timeoutMs: The maximum time to wait before aborting the
    //             command. Waits forever if set to 0.
    //     outExitCode: An optional pointer to an existing location where the
    //             exit code for the launched process is returned.
    //
    // Returns kNullopt in case of failure, or the captured stdout/stderr.
    virtual Optional<std::string> runCommandWithResult( const std::vector<std::string>& commandLine,
                            System::Duration timeoutMs = kInfinite,
                            System::ProcessExitCode* outExitCode = nullptr) = 0;

    static std::vector<Pid> queryRunningProcessPids(
        const std::vector<StringView>& targets, bool approxMatch = true);

    static void deleteTempDir();

    static void killProcess(Pid);
    static void stopAllEmulatorProcesses();

    // bug: 117641628
    // Newer Linux systems might have slow CoW filesystems
    // like btrfs that interfere with emulator performance.
    // This is a function to attempt to disable them in performance-
    // critical directories.
    static void disableCopyOnWriteForPath(StringView path);

    // bug: 117923532
    // macOS will make the emulator nap, which will mess up timers
    // and cause mayhem like hang detection.
    // On other platforms, this function doesn't do anything.
    static void disableAppNap();

    // Hide the emulator icon displayed on macOS dock. On other
    // platforms, this function doesn't do anything.
    static void hideDockIcon();
    // Returns the wallclock (high res time us) user, and system time spent
    // in the current thread.
    static CpuTime cpuTime();

    // Static version that sets or queries host environment variables
    // regardless of being TestSystem.
    static void setEnvironmentVariable(StringView varname, StringView varvalue);
    static std::string getEnvironmentVariable(StringView varname);
    static std::string getProgramDirectoryFromPlatform();
    static WallDuration getSystemTimeUs();

    // Windows driver file querying functions
    static bool queryFileVersionInfo(StringView path, int* major, int* minor, int* build_1, int* build_2);

protected:
    size_t mMemorySize = 0;

    static System* setForTesting(System* system);
    static System* hostSystem();

    // Internal implementation of scanDirEntries() that can be used by
    // mock implementation using a fake file system rooted into a temporary
    // directory or something like that. Always returns short paths.
    static std::vector<std::string> scanDirInternal(StringView dirPath);

    static bool pathExistsInternal(StringView path);
    static bool pathIsFileInternal(StringView path);
    static bool pathIsDirInternal(StringView path);
    static bool pathIsLinkInternal(StringView path);
    static bool pathCanReadInternal(StringView path);
    static bool pathCanWriteInternal(StringView path);
    static bool pathCanExecInternal(StringView path);
    static int pathOpenInternal(const char *filename, int oflag, int pmode);
    static bool deleteFileInternal(StringView path);
    static bool pathFileSizeInternal(StringView path, FileSize* outFileSize);
    static bool fileSizeInternal(int fd, FileSize* outFileSize);
    static bool pathFreeSpaceInternal(StringView path, FileSize* spaceInBytes);
    static FileSize recursiveSizeInternal(StringView path);
    static Optional<Duration> pathCreationTimeInternal(StringView path);
    static Optional<Duration> pathModificationTimeInternal(StringView path);
    static Optional<DiskKind> diskKindInternal(StringView path);
    static Optional<DiskKind> diskKindInternal(int fd);

private:
    DISALLOW_COPY_AND_ASSIGN(System);
};

}  // namespace base
}  // namespace android
