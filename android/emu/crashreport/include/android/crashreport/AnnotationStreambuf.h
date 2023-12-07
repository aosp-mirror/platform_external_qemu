// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once
#include <cstring>
#include <streambuf>
#include "client/annotation.h"

namespace android {
namespace crashreport {

using crashpad::Annotation;

// A streambuffer that writes to an crashpad annotation, use your favorite
// std::stream that ends up in a minidump!
//
// AnnotationStreambuf should be declared with static storage duration!
//
// For example:
//
// AnnotationStreambuf<4096> mLogBuf{"my_label"};
// std::ostream log(&mLogBuf);
// log << "Some useful info when you crash later on.";
template <Annotation::ValueSizeType MaxSize>
class AnnotationStreambuf : public crashpad::Annotation, public std::streambuf {
public:
    AnnotationStreambuf(const AnnotationStreambuf&) = delete;
    AnnotationStreambuf& operator=(const AnnotationStreambuf&) = delete;

    // Name of the annotation.. This is how it will show up in a minidump.
    explicit AnnotationStreambuf(const char name[])
        : Annotation(Type::kString, name, mBuffer), mBuffer() {
        setp(mBuffer, mBuffer + MaxSize);
    }

    // Mainly for testing.
    int sync() override {
        setg(mBuffer, mBuffer, mBuffer + (pptr() - pbase()));
        return 0;
    }

protected:
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        std::streamsize available = epptr() - pptr();
        std::streamsize to_copy = std::min(n, available);
        if (available <= 0) {
            return traits_type::eof();  // No space left in the buffer
        }
        std::memcpy(pptr(), s, static_cast<std::size_t>(to_copy));
        pbump(static_cast<int>(to_copy));

        SetSize(pptr() - pbase());
        return to_copy;
    };

    int_type overflow(int_type ch) override { return traits_type::eof(); }

    int_type underflow() override {
        return gptr() == egptr() ? traits_type::eof() : *gptr();
    }

private:
    char mBuffer[MaxSize];
};

// A stream buffer that can hold 8kb of annotation data.
using DefaultAnnotationStreambuf = AnnotationStreambuf<8192>;
}  // namespace crashreport
}  // namespace android
