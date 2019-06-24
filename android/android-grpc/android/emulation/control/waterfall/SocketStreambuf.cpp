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
#include "android/emulation/control/waterfall/SocketStreambuf.h"
#include "android/base/sockets/SocketUtils.h"

/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("SocketStreambuf: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("(%p:%d) :", this, mFd);                 \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#else
#define DD(...) (void)0
#define DD_BUF(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {

SocketStreambuf::SocketStreambuf(int fd, size_t buffer, size_t putback)
    : mFd(fd),
      mPutback(std::max(putback, size_t(1))),
      mBuffer(std::max(buffer, mPutback) + mPutback) {
    char* end = &mBuffer.front() + mBuffer.size();
    setg(end, end, end);

    // The std streambuf spec expects the read/write calls to be blocking
    // there are no proper semantics for handling async
    base::socketSetBlocking(mFd);
}

void SocketStreambuf::close() {
    base::socketClose(mFd);
}

std::streamsize SocketStreambuf::xsputn(const char* s, std::streamsize n) {
    int ret = base::socketSend(mFd, s, n);
    DD("xsputn: (%p:%d): %d, error: %d", this, mFd, ret, errno);
    return ret < 0 ? traits_type::eof() : ret;
}

int SocketStreambuf::overflow(int c) {
    // Ouch.. single bytes are being written to our stream...
    // (std::endl anyone?)
    if (c == traits_type::eof()) {
        return traits_type::eof();
    }
    char a = (char)c;
    return xsputn(&a, 1) == 1 ? a : traits_type::eof();
}

int SocketStreambuf::underflow() {
    if (gptr() < egptr()) {
        // buffer not exhausted, this shouldn't really happen..
        return traits_type::to_int_type(*gptr());
    }

    char* base = &mBuffer.front();
    char* start = base;

    if (eback() == base) {
        // Make arrangements for putback characters
        size_t putback = std::min<size_t>(egptr() - start, mPutback);
        std::memmove(base, egptr() - putback, putback);
        start += putback;
    }
    // start is now the start of the buffer, proper, with the front
    // dedicated to push back..

    // Next we are going to grab the next chunk from our socket.
    uint32_t maxRead = (mBuffer.size() - (start - base));
    std::streamsize n = base::socketRecv(mFd, start, maxRead);
    if (n == 0) {
        DD("Empty read (%p:%d), eof!", this, mFd);
        return traits_type::eof();
    }
    if (n < 0) {
        DD("Returning: (%p:%d): eof due to error: %d", this, mFd, errno);
        return traits_type::eof();
    }

    DD("underflow: (%p:%d): %ld bytes available", this, mFd, n);
    DD_BUF(start, n);
    // Set buffer pointers
    setg(base, start, start + n);
    return traits_type::to_int_type(*gptr());
}

std::streamsize SocketStreambuf::showmanyc() {
    if (underflow() == traits_type::eof())
        return -1;
    DD("showmanyc: (%p:%d): %ld", this, mFd, egptr() - gptr());
    return egptr() - gptr();
}
}  // namespace control
}  // namespace emulation
}  // namespace android
