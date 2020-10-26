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

#include "android/base/files/GzipStreambuf.h"

#include <streambuf>  // for basic_streambuf<>::traits_type, basic_streambuf

#include "zconf.h"  // for Bytef
#include "zlib.h"   // for z_stream, Z_OK, Z_STREAM_END, deflate, deflateEnd

/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("GzipStream: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(fd, buf, len)                            \
    do {                                                \
        printf("GzipStream (%d):", fd);                 \
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
namespace base {

GzipInputStreambuf::GzipInputStreambuf(std::streambuf* src,
                                       std::size_t bufferCapcity)
    : mSrc(src), mCapacity(bufferCapcity) {
    // magic number from gz_read
    const int GZIP_WINDOW_BITS = 15 + 16;

    mIn = std::unique_ptr<char[]>(new char[mCapacity]);
    mOut = std::unique_ptr<char[]>(new char[mCapacity]);
    mInStart = mIn.get();
    mInEnd = mIn.get();
    setg(mOut.get(), mOut.get(), mOut.get());
    mErr = inflateInit2(&mZstream, GZIP_WINDOW_BITS);
}

GzipInputStreambuf::~GzipInputStreambuf() {
    inflateEnd(&mZstream);
}

// Returns the number of bytes decompressed, or traits_type::eof() if eof..
int GzipInputStreambuf::decompress() {
    // We need to decompress new things.
    if (mInStart == mInEnd) {
        // Fill up our empty buffers.
        auto retrieved = mSrc->sgetn(mIn.get(), mCapacity);
        mInStart = mIn.get();
        mInEnd = mIn.get() + retrieved;
        DD("Left on stream: %td, retrieved: %td", mSrc->in_avail(), retrieved);
        if (retrieved == 0) {
            DD("no more bytes left to decompress, start: (%p), end: (%p)",
               mInStart, mInEnd);
            return traits_type::eof();
        }
    }
    // Let's decompress thingies.
    mZstream.next_in = reinterpret_cast<Bytef*>(mInStart);
    mZstream.avail_in = mInEnd - mInStart;
    mZstream.next_out = reinterpret_cast<Bytef*>(mOut.get());
    mZstream.avail_out = mCapacity;
    mErr = inflate(&mZstream, Z_NO_FLUSH);
    if (mErr != Z_OK && mErr != Z_STREAM_END) {
        DD("Error %d while decompressing, returning eof", mErr);
        return traits_type::eof();
    }

    // update in&out pointers following inflate()
    mInStart = reinterpret_cast<char*>(mZstream.next_in);
    mInEnd = mInStart + mZstream.avail_in;
    return reinterpret_cast<char*>(mZstream.next_out) - mOut.get();
}

int GzipInputStreambuf::underflow() {
    if (mErr != Z_OK)
        return traits_type::eof();

    if (this->gptr() == this->egptr()) {
        int available;

        // 2 cases.. eof, or we need to read more data in order to compress.
        do {
            available = decompress();
        } while (available == 0);

        if (available == traits_type::eof()) {
            DD("Nothing available for decompression.");
            return traits_type::eof();
        }

        DD("available: %d", available);
        this->setg(mOut.get(), mOut.get(), mOut.get() + available);
    }
    return this->gptr() == this->egptr()
                   ? traits_type::eof()
                   : traits_type::to_int_type(*this->gptr());
}

GzipInputFileStreambuf::GzipInputFileStreambuf(const char* fileName,
                                               std::size_t chunk)
    : mCapacity(chunk) {
    mFile = gzopen(fileName, "rb");
    gzbuffer(mFile, mCapacity);
    mOut = std::unique_ptr<char[]>(new char[mCapacity]);
}

GzipInputFileStreambuf::~GzipInputFileStreambuf() {
    gzclose(mFile);
}

std::streampos GzipInputFileStreambuf::seekoff(std::streamoff off,
                                               std::ios_base::seekdir way,
                                               std::ios_base::openmode which) {
    int whence = 0;
    switch (way) {
        case std::ios_base::beg:
            whence = SEEK_SET;
            break;
        case std::ios_base::cur:
            whence = SEEK_CUR;
            off -= this->egptr() - this->gptr();
            break;
        case std::ios_base::end:
            whence = SEEK_END;
            break;
    }
    this->setg(mOut.get(), mOut.get(), mOut.get());
    return gzseek(mFile, off, whence);
}

std::streampos GzipInputFileStreambuf::seekpos(std::streampos pos,
                                               std::ios_base::openmode which) {
    this->setg(mOut.get(), mOut.get(), mOut.get());
    return gzseek(mFile, pos, SEEK_SET);
}

int GzipInputFileStreambuf::underflow() {
    if (mErr != Z_OK) {
        return traits_type::eof();
    }

    if (this->gptr() == this->egptr()) {
        int available;

        // 2 cases.. eof, or we need to read more data in order to compress.
        do {
            available = gzread(mFile, mOut.get(), mCapacity);
        } while (available == 0 && !gzeof(mFile));

        if (available < 0) {
            mErr = available;
            DD("gzread error: %d", mErr);
            return traits_type::eof();
        }

        if (gzeof(mFile)) {
            DD("Nothing available for decompression.");
            return traits_type::eof();
        }

        DD("available: %d", available);
        this->setg(mOut.get(), mOut.get(), mOut.get() + available);
    }
    return this->gptr() == this->egptr()
                   ? traits_type::eof()
                   : traits_type::to_int_type(*this->gptr());
}

GzipOutputStreambuf::GzipOutputStreambuf(std::streambuf* dst,
                                         int level,
                                         std::size_t bufferCapcity)
    : mDst(dst), mCapacity(bufferCapcity) {
    const int GZIP_WINDOW_BITS = 15 + 16;
    mErr = deflateInit2(&mZstream, level, Z_DEFLATED, GZIP_WINDOW_BITS, 8,
                        Z_DEFAULT_STRATEGY);

    mIn = std::unique_ptr<char[]>(new char[mCapacity]);
    mOut = std::unique_ptr<char[]>(new char[mCapacity]);
    setp(mIn.get(), mIn.get() + mCapacity);
}

GzipOutputStreambuf::~GzipOutputStreambuf() {
    sync();
    deflateEnd(&mZstream);
}

bool GzipOutputStreambuf::compress(int flush) {
    int written = 0;

    do {
        // Compress next available to our output block..
        mZstream.next_out = reinterpret_cast<Bytef*>(mOut.get());
        mZstream.avail_out = mCapacity;
        mErr = deflate(&mZstream, flush);
        if (mErr != Z_OK && mErr != Z_STREAM_END && mErr != Z_BUF_ERROR)
            return false;

        // Write out the size of our output block.. deflate will update
        // the input parameters accordingly..
        int cnt = reinterpret_cast<char*>(mZstream.next_out) - mOut.get();
        written = mDst->sputn(mOut.get(), cnt);
        if (written != cnt) {
            // Oh, our sink is not taking in the bytes we are writing..
            return false;
        }
    } while (mErr != Z_STREAM_END && mErr != Z_BUF_ERROR && written != 0);

    return true;
}

// Virtual function called by other member functions to put a character into
// the controlled output sequence without changing the current position. It
// is  called by public member functions such as sputc to write a character
// when there are no writing positions available at the put pointer (pptr).
// In our case we use it to compress the current buffer contents..
std::streambuf::int_type GzipOutputStreambuf::overflow(
        std::streambuf::int_type c) {
    mZstream.next_in = reinterpret_cast<Bytef*>(pbase());
    mZstream.avail_in = pptr() - pbase();
    while (mZstream.avail_in > 0) {
        if (!compress(Z_NO_FLUSH)) {
            setp(nullptr, nullptr);
            return traits_type::eof();
        }
    }
    setp(mIn.get(), mIn.get() + mCapacity);
    return c == traits_type::eof() ? traits_type::eof() : sputc(c);
}

// function called by the public member function pubsync to synchronize the
// contents in the buffer with those of the associated character sequence.
// Returns zero, which indicates success.
// A value of -1 would indicate failure.
int GzipOutputStreambuf::sync() {
    // Write out all the remaing things..
    overflow();
    if (!pptr())
        return -1;

    // And close up the stream..
    mZstream.next_in = nullptr;
    mZstream.avail_in = 0;
    if (!compress(Z_FINISH))
        return -1;

    deflateReset(&mZstream);
    return mDst->pubsync();
}

GzipOutputStream::GzipOutputStream(std::ostream& os)
    : std::ostream(new GzipOutputStreambuf(os.rdbuf())) {}

GzipOutputStream::GzipOutputStream(std::streambuf* sbuf)
    : std::ostream(new GzipOutputStreambuf(sbuf)) {}

GzipOutputStream::~GzipOutputStream() {
    delete rdbuf();
}

GzipInputStream::GzipInputStream(std::istream& os)
    : std::istream(new GzipInputStreambuf(os.rdbuf())) {}

GzipInputStream::GzipInputStream(const char* fileName)
    : std::istream(new GzipInputFileStreambuf(fileName)) {}

GzipInputStream::GzipInputStream(std::streambuf* sbuf)
    : std::istream(new GzipInputStreambuf(sbuf)) {}

GzipInputStream::~GzipInputStream() {
    delete rdbuf();
}
}  // namespace base
}  // namespace android