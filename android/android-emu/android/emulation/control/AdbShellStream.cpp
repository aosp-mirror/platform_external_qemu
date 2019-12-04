
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
#include "android/emulation/control/AdbShellStream.h"

namespace android {
namespace emulation {

// A shell stream that will automatically switch to v1/v2 depending on support
// of your adb protocol. Note that shell v1 will not provide status codes.

AdbShellStream::AdbShellStream(std::string service) {
    auto adb = AdbConnection::connection();
    mShellV2 = adb->hasFeature("shell_v2");
    if (mShellV2) {
        mAdbStream = adb->open("shell,v2,raw:" + service);
    } else {
        mAdbStream = adb->open("shell:" + service);
    }
}

bool AdbShellStream::good() {
    return AdbConnection::connection()->state() == AdbState::connected &&
           mAdbStream->good();
}

bool AdbShellStream::read(std::vector<char>& sout,
                          std::vector<char> serr,
                          int& exitCode) {
    if (mShellV2) {
        return readV2(sout, serr, exitCode);
    }
    return readV1(sout, serr, exitCode);
}

bool AdbShellStream::write(std::string sin) {
    size_t size = sin.size();
    bool ok = true;
    if (mShellV2) {
        ShellHeader hdr{.id = ShellHeader::kIdStdin,
                        .length = static_cast<uint32_t>(size)};
        mAdbStream->write((char*)&hdr, sizeof(hdr));
        if (mAdbStream->gcount() != sizeof(hdr))
            return false;
    }
    mAdbStream->write(sin.data(), size);
    return mAdbStream->gcount() == size && mAdbStream->good();
}

bool AdbShellStream::write(std::vector<char> sin) {
    size_t size = sin.size();
    bool ok = true;
    if (mShellV2) {
        ShellHeader hdr{.id = ShellHeader::kIdStdin,
                        .length = static_cast<uint32_t>(size)};
        mAdbStream->write((char*)&hdr, sizeof(hdr));
        if (mAdbStream->gcount() != sizeof(hdr))
            return false;
    }
    mAdbStream->write(sin.data(), size);
    return mAdbStream->gcount() == size && mAdbStream->good();
}

bool AdbShellStream::readV1(std::vector<char>& sout,
                            std::vector<char> serr,
                            int& exitCode) {
    if (sout.capacity() < 32)
        sout.reserve(128);

    auto strmbuf = mAdbStream->rdbuf();
    // Max chunk size we are willing to collect before sending.
    int chunksize = sout.capacity();
    char* end = sout.data();
    // Slurp up available bytes, worst case we send a single byte..
    // but this will be very responsive..
    if (strmbuf->in_avail() == 0) {
        // Block and wait until we have 1 byte, (probably more, but
        // we'll get to those)
        if (mAdbStream->read(end, 1)) {
            end += mAdbStream->gcount();
            chunksize -= mAdbStream->gcount();
        }
    }
    while (strmbuf->in_avail() > 0 && chunksize > 0) {
        if (mAdbStream->read(
                    end, std::min<size_t>(strmbuf->in_avail(), chunksize))) {
            end += mAdbStream->gcount();
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
    return mAdbStream->good();
}

}  // namespace emulation
}  // namespace android
