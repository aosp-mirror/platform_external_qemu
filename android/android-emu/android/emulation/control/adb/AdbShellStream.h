// Copyright (C) 2019 The Android Open Source Project
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
#include <stddef.h>                                       // for size_t
#include <memory>                                         // for shared_ptr
#include <string>                                         // for string
#include <vector>                                         // for vector

#include "android/emulation/control/adb/AdbConnection.h"  // for AdbConnection

namespace android {
namespace emulation {

// An AdbShellStream abstracts the ADBD shell service so that v1 protocol has the same
// interface as the v2 protocol. More details on the ShellProtocol can be found as
// part of the adb source here:
// https://android.googlesource.com/platform/system/core/+/master/adb/shell_protocol.h
class AdbShellStream {
public:
    // cmd to be executed on shell. Shell V2 will be chosen if
    // the system image supports it.
    AdbShellStream(std::string cmd, std::shared_ptr<AdbConnection> connection = AdbConnection::connection(2000));
    ~AdbShellStream();

    bool good();

    // Reads stdout, stderr and exitcode from the stream.
    // This call blocks until bytes are available or the
    // stream closed.
    //
    // Note for protocol v1:
    // - The exit code never gets set and will always be 0
    // - Stderr never gets written to.
    // - This will immediately return all the bytes that
    //   are available. If no bytes are available it will
    //   block and wait until at least one byte is available
    //   or the stream is closed. Bytes are being pulled in:
    //    - Until the vector is filled up to capacity
    //    - The stream has no more bytes available.
    // Keep this in mind that there is a tradeoff between
    // efficiency and responsiveness.
    bool read(std::vector<char>& sout,
              std::vector<char>& serr,
              int& exitCode);

    // Reads until the stream is bad / eof, returning the exitcode.
    // note the exit code is only valid if the stream is eof.
    int readAll(std::vector<char>& sout,
              std::vector<char>& serr);

    // Write bytes to stdin
    bool write(std::string sin);

    // Write bytes to stdin
    bool write(std::vector<char> sin);

    // Write bytes to stdin
    bool write(const char* data, size_t cData);

    // True when using shell v1.
    bool isV1() { return !mShellV2; }

private:
    bool readV1(std::vector<char>& sout,
                std::vector<char>& serr,
                int& exitCode);
    bool readV2(std::vector<char>& sout,
                std::vector<char>& serr,
                int& exitCode);

    std::shared_ptr<AdbStream> mAdbStream;
    bool mShellV2;
    std::shared_ptr<AdbConnection> mAdb;
};

}  // namespace emulation
}  // namespace android
