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
#include <memory>
#include <string>
#include <vector>
#include "android/emulation/control/AdbConnection.h"

namespace android {
namespace emulation {

// A shell stream that will automatically switch to v1/v2 depending on support
// of your adb protocol. Note that shell v1 will not provide status codes.
class AdbShellStream {
public:
    // cmd to be executed on shell. Shell V2 will be chosen if
    // the system image supports it.
    AdbShellStream(std::string cmd);
    bool good();

    // Reads sout, serr and exitcode from the stream.
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
    //
    //  Note that it is totally possible to spin for a long
    //  time if you use a large buffer.
    bool read(std::vector<char>& sout,
              std::vector<char> serr,
              int& exitCode);

    // Write bytes to sin
    bool write(std::string sin);

    // Write bytes to sin
    bool write(std::vector<char> sin);

    // True when using shell v1.
    bool isV1() { return !mShellV2; }

private:
    bool readV1(std::vector<char>& sout,
                std::vector<char> serr,
                int& exitCode);
    bool readV2(std::vector<char>& sout,
                std::vector<char>& serr,
                int& exitCode);

    std::shared_ptr<AdbStream> mAdbStream;
    bool mShellV2;
};

}  // namespace emulation
}  // namespace android
