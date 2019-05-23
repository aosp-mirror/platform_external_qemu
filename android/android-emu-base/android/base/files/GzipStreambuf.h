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
#include <stddef.h>  // for size_t
#include <zlib.h>    // for Z_OK, z_stream, Z_DEFAULT_COMPRESSION
#include <istream>   // for streambuf, istream, ostream, basic_streambuf<>::...
#include <memory>    // for unique_ptr

namespace android {
namespace base {

// An input stream buffer that inflates/decompresses a gzip stream.
class GzipInputStreambuf : public std::streambuf {
public:
    // |src| is the underlying buffer from which bytes will be retrieved.
    // |chunk| is simply the buffer size for feeding data to and pulling data
    // from the zlib routines. Larger buffer sizes would be more efficient. If
    // the memory is available, buffers sizes on the order of 128K or 256K bytes
    // should be used.
    GzipInputStreambuf(std::streambuf* src, std::size_t chunk = k16KB);
    ~GzipInputStreambuf();

protected:
    int underflow() override;

private:
    int decompress();
    std::streambuf* mSrc;
    std::size_t mCapacity;
    z_stream mZstream{0};
    std::unique_ptr<char[]> mIn;
    std::unique_ptr<char[]> mOut;
    char* mInStart;
    char* mInEnd;
    int mErr{Z_OK};

    static constexpr std::size_t k16KB = 16 * 1024;
};

// An input stream buffer that inflates/decompresses a gzip stream.
class GzipInputFileStreambuf : public std::streambuf {
public:
    GzipInputFileStreambuf(const char* fileName, std::size_t chunk = k16KB);
    ~GzipInputFileStreambuf();

protected:
    int underflow() override;
    std::streampos seekoff(std::streamoff off,
                           std::ios_base::seekdir way,
                           std::ios_base::openmode which) override;
    std::streampos seekpos(std::streampos pos,
                           std::ios_base::openmode which) override;

private:
    std::unique_ptr<char[]> mOut;
    static constexpr std::size_t k16KB = 16 * 1024;
    gzFile mFile;
    std::size_t mCapacity;
    int mErr{Z_OK};
};

class GzipInputStream : public std::istream {
public:
    GzipInputStream(std::istream& os);
    GzipInputStream(const char* fileName);
    explicit GzipInputStream(std::streambuf* sbuf);
    virtual ~GzipInputStream();
};

class GzipOutputStreambuf : public std::streambuf {
public:
    GzipOutputStreambuf(std::streambuf* dst,
                        int level = Z_DEFAULT_COMPRESSION,
                        std::size_t chunk = k16KB);
    ~GzipOutputStreambuf();

protected:
    std::streambuf::int_type overflow(
            std::streambuf::int_type c = traits_type::eof()) override;
    int sync() override;

private:
    bool compress(int flush);
    static constexpr std::size_t k16KB = 16 * 1024;
    std::streambuf* mDst;
    std::size_t mCapacity;
    z_stream mZstream{0};
    std::unique_ptr<char[]> mIn;
    std::unique_ptr<char[]> mOut;
    int mErr{Z_OK};
};

class GzipOutputStream : public std::ostream {
public:
    GzipOutputStream(std::ostream& os);
    explicit GzipOutputStream(std::streambuf* sbuf);
    virtual ~GzipOutputStream();
};

}  // namespace base
}  // namespace android