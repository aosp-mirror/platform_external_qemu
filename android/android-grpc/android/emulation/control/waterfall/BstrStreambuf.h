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

#include <streambuf>
#include <vector>

namespace android {
namespace emulation {
namespace control {

// This is a stream buffer that encodes an decodes the waterfall protocol.
// The protocol is:
//
//   1. Length prefix. A four-byte integer that contains the number of bytes
//       in the following data string. It appears immediately before the first
//       character of the data string. The integer is encoded in little endian.
//   2. Data string.
//
// Note that unlike the classic windows BStr, this string is not \000
// terminated.
class BstrStreambuf : public std::streambuf {
public:
    BstrStreambuf(std::streambuf* inner,
                  size_t buffer = 256,
                  size_t putback = 8);
    void close();

private:
    int overflow(int c) override;
    std::streamsize xsputn(const char* s, std::streamsize n) override;
    int underflow() override;
    std::streamsize showmanyc() override;

    bool readHeader();

    const uint8_t kHeaderLen = sizeof(uint32_t);
    std::streambuf* mInner;
    uint32_t mBstrLen = 0;
    int mHeaderPos = 0;

    const std::size_t mPutback;
    std::vector<char> mBuffer;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
