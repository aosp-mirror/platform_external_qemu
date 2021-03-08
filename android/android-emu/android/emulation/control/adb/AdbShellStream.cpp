
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
#include "android/emulation/control/adb/AdbShellStream.h"

#include <stdint.h>   // for uint32_t
#include <algorithm>  // for min
#include <istream>    // for basic_istream<>::__istream_type, basic...
#include <iterator>   // for begin, end

#include "android/base/Log.h"  // for LOG, LogMessage, LogStream

#define DEBUG 0

#if DEBUG >= 2
#define DD(str) LOG(INFO) << str
#else
#define DD(...) (void)0
#endif

constexpr size_t SHELL_V1_MIN_BUF = 512;

namespace android {
namespace emulation {

// A shell stream that will automatically switch to v1/v2 depending on support
// of your adb protocol. Note that shell v1 will not provide status codes.
AdbShellStream::AdbShellStream(std::string service,
                               std::shared_ptr<AdbConnection> connection)
    : mAdb(connection) {
    // Bad stream..
    if (!mAdb)
        return;

    mShellV2 = mAdb->hasFeature("shell_v2");
    if (mShellV2) {
        mAdbStream = mAdb->open("shell,v2,raw:" + service);
    } else {
        mAdbStream = mAdb->open("shell:" + service);
    }
}
AdbShellStream::~AdbShellStream() {
    if (mAdbStream)
        mAdbStream->close();
}

bool AdbShellStream::good() {
    return mAdb && mAdb->state() == AdbState::connected && mAdbStream &&
           mAdbStream->good();
}

bool AdbShellStream::read(std::vector<char>& sout,
                          std::vector<char>& serr,
                          int& exitCode) {
    if (!good())
        return false;
    if (mShellV2) {
        return readV2(sout, serr, exitCode);
    }
    return readV1(sout, serr, exitCode);
}

int AdbShellStream::readAll(std::vector<char>& sout, std::vector<char>& serr) {
    int exitcode = 0;
    do {
        read(sout, serr, exitcode);
    } while (good());
    return exitcode;
}

bool AdbShellStream::write(const char* data, size_t cData) {
    bool ok = good();
    if (mShellV2) {
        ShellHeader hdr{.id = ShellHeader::kIdStdin,
                        .length = static_cast<uint32_t>(cData)};
        mAdbStream->write((char*)&hdr, sizeof(hdr));
    }
    mAdbStream->write(data, cData);
    return mAdbStream->good();
}

bool AdbShellStream::write(std::string sin) {
    return write(sin.data(), sin.size());
}

bool AdbShellStream::write(std::vector<char> sin) {
    return write(sin.data(), sin.size());
}

bool AdbShellStream::readV1(std::vector<char>& sout,
                            std::vector<char>& serr,
                            int& exitCode) {
    // Note, the smaller the capacity the more read calls have to be made,
    // but at least you will be very responsive.(i.e. not waiting on logcat to
    // produce more data for example.)
    if (sout.capacity() < SHELL_V1_MIN_BUF)
        sout.reserve(SHELL_V1_MIN_BUF);

    auto strmbuf = mAdbStream->rdbuf();
    // Max chunk size we are willing to collect before sending.
    int chunksize = sout.capacity();
    // Slurp up available bytes, worst case we send a single byte..
    // but this will be very responsive..
    if (strmbuf->in_avail() == 0) {
        // Block and wait until we have 1 byte, (probably more, but
        // we'll get to those)
        char end;
        if (mAdbStream->read(&end, 1)) {
            sout.push_back(end);
            chunksize -= mAdbStream->gcount();
        }
    }

    // Read while we have bytes and space in our buffer.
    while (strmbuf->in_avail() > 0 && chunksize > 0) {
        size_t toRead = std::min<size_t>(strmbuf->in_avail(), chunksize);
        sout.resize(sout.size() + toRead);
        char* end = &sout[sout.size() - toRead];
        if (mAdbStream->read(end, toRead)) {
            chunksize -= mAdbStream->gcount();
        }
    }

    return mAdbStream->good();
};

bool AdbShellStream::readV2(std::vector<char>& sout,
                            std::vector<char>& serr,
                            int& exitCode) {
    ShellHeader hdr;
    sout.clear();
    serr.clear();
    if (mAdbStream->read((char*)&hdr, sizeof(hdr))) {
        std::vector<char> msg(hdr.length);
        mAdbStream->read(msg.data(), hdr.length);
        switch (hdr.id) {
            case ShellHeader::kIdStderr:
                serr.insert(serr.end(), std::begin(msg), std::end(msg));
                break;
            case ShellHeader::kIdStdout:
                sout.insert(sout.end(), std::begin(msg), std::end(msg));
                break;
            case ShellHeader::kIdExit:
                exitCode = (int)msg[0];
                break;
            default:
                // Ignonring the others..
                LOG(WARNING) << "Don't know how to handle shell "
                                "header of type: "
                             << hdr.id;
        }
    }
    DD("stdout: " << std::string(sout.begin(), sout.end()));
    DD("stderr: " << std::string(serr.begin(), serr.end()));
    DD("exit: " << exitCode);
    return mAdbStream->good();
}

}  // namespace emulation
}  // namespace android
