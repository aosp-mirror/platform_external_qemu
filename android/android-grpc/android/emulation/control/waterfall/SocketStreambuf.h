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

class SocketStreambuf : public std::streambuf {
public:
    SocketStreambuf(int fd, size_t buffer = 256, size_t putback = 8);
    void close();

private:
    std::streamsize xsputn(const char* s, std::streamsize n) override;
    int overflow(int c) override;
    int underflow() override;
    std::streamsize showmanyc() override;

    const std::size_t mPutback;
    std::vector<char> mBuffer;
    const int mFd;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
