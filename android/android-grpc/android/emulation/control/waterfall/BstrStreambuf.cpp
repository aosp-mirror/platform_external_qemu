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

#include "android/emulation/control/waterfall/BstrStreambuf.h"

/* set to 1 for debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("BstrStreambuf: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("(%p:%p) :", this, mInner);              \
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

BstrStreambuf::BstrStreambuf(std::streambuf* inner,
                             size_t buffer,
                             size_t putback)
    : mInner(inner),
      mPutback(std::max(putback, size_t(1))),
      mBuffer(std::max(buffer, mPutback) + mPutback) {
    char* end = &mBuffer.front() + mBuffer.size();
    setg(end, end, end);
}

void BstrStreambuf::close() {
    char bytes[4] = {0};
    mInner->sputn(bytes, 4);
}

int BstrStreambuf::overflow(int c) {
    DD("Bstr: overflow %d", c);
    if (c == traits_type::eof()) {
        close();
    }
    // Ouch.. single bytes are being written to our stream...
    // (std::endl anyone?)
    char a = (char)c;
    return xsputn(&a, 1) == 1 ? a : traits_type::eof();
}

std::streamsize BstrStreambuf::xsputn(const char* s, std::streamsize n) {
#ifndef __LITTLE_ENDIAN__
#error "The header needs to be converted to little endian!."
#endif
    DD("Bstr, xsputn: %ld bytes", n);
    DD_BUF(s, n);
    uint32_t header = n;
    char* bytes = (char*)&header;
    if (mInner->sputn(bytes, kHeaderLen) != kHeaderLen)
        return 0;
    return mInner->sputn(s, n);
}

int BstrStreambuf::underflow() {
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

    // Ok, 2 cases
    // 1. We need to get the bstrlen from the underlying stream:
    if (mBstrLen == 0) {
        // Unable to read header, or header indicates eof.
        if (!readHeader() || mBstrLen == 0) {
            return traits_type::eof();
        }
    }

    // 2. We have mBstrLen..
    // To keep things simple we will chunk up in bstrlens..
    // if the perf is really bad we can revisit this
    // (i.e. bstrlens are always a lot smaller than mBuffer.size())
    uint32_t maxRead =
            std::min<uint32_t>(mBuffer.size() - (start - base), mBstrLen);
    std::streamsize n = mInner->sgetn(start, maxRead);

    DD("Retreived %ld bytes from inner %p", n, mInner);
    if (n == 0)
        return traits_type::eof();

    mBstrLen -= n;
    // Set buffer pointers
    setg(base, start, start + n);
    return traits_type::to_int_type(*gptr());
}

std::streamsize BstrStreambuf::showmanyc() {
    if (underflow() == traits_type::eof())
        return -1;
    DD("showmanyc: (%p:%p): %ld", this, mInner, egptr() - gptr());
    return egptr() - gptr();
}

bool BstrStreambuf::readHeader() {
    if (mHeaderPos == kHeaderLen) {
        mHeaderPos = 0;
    }
    char* buf = (char*)&mBstrLen;
    mHeaderPos += mInner->sgetn(buf + mHeaderPos, kHeaderLen - mHeaderPos);
    DD("Bstr, read header: %p:%d", mInner, mBstrLen);
    return mHeaderPos == kHeaderLen;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
