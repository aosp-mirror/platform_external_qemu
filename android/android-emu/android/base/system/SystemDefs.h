#pragma once

namespace android {
namespace base {

#ifdef _WIN32
    using Pid = DWORD;
    using ProcessExitCode = DWORD;
#else
    using Pid = pid_t;
    using ProcessExitCode = int;
#endif

    typedef int64_t  Duration;
    typedef uint64_t WallDuration;
    using FileSize = uint64_t;
} // namespace base
} // namespace android
