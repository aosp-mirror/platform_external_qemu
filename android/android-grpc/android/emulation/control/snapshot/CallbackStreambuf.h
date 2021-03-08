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
#include <functional>
#include <memory>
#include <streambuf>  // for streambuf
#include <iostream>

namespace android {
namespace emulation {
namespace control {

using write_callback = std::function<bool(char* bytes, std::size_t len)>;

using read_callback = std::function<
        bool(char** new_eback, char** new_gptr, char** new_egptr)>;

class CallbackStreambufReader : public std::streambuf {
public:
    CallbackStreambufReader(read_callback callback) : mCallback(callback) {
        setg(nullptr, nullptr, nullptr);
    }

protected:
    int underflow() {
        char *new_eback = nullptr, *new_gptr = nullptr, *new_egptr = nullptr;
        bool avail = mCallback(&new_eback, &new_gptr, &new_egptr);
        if (!avail)
            return traits_type::eof();
        setg(new_eback, new_gptr, new_egptr);
        return this->gptr() == this->egptr()
                       ? traits_type::eof()
                       : traits_type::to_int_type(*this->gptr());
    }

private:
    read_callback mCallback;
};
class CallbackStreambufWriter : public std::streambuf {
public:
    CallbackStreambufWriter(std::size_t chunk, write_callback callback)
        : mChunk(chunk), mCallback(callback) {
        mBuffer = std::make_unique<char[]>(chunk);
        setp(mBuffer.get(), mBuffer.get() + chunk);
    }

protected:
    int overflow(std::streambuf::int_type c = traits_type::eof()) override {
        if (!mCallback(pbase(), pptr() - pbase()))
            return traits_type::eof();

        setp(mBuffer.get(), mBuffer.get() + mChunk);
        return c == traits_type::eof() ? traits_type::eof() : sputc(c);
    }

    int sync() override {
        overflow();
        return pptr() ? 0 : -1;
    }

private:
    std::size_t mChunk;
    write_callback mCallback;
    std::unique_ptr<char[]> mBuffer;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
