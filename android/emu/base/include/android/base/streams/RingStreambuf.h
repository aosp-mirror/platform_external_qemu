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
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <mutex>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

using std::chrono::milliseconds;

namespace android {
namespace base {
namespace streams {

// RingStreambuf - a thread safe streambuffer backed by a ring buffer.
// This thing acts as a sliding window over a stream of data.
//
// Usage examples:
//
// This creates an output stream that can write at at least 5 characters, before
// it start overwriting old characters.
//
//   RingStreambuf buf(5);
//   std::ostream stream(&buf);
//   stream << "Hello";
//
// Be very careful when using this as an input stream!
// - It can block when nothing is available, for up to timeout ms.
// - It will consume the stream (i.e. read pointers will move)
class RingStreambuf : public std::streambuf {
public:
    // |capacity| the minimum number of chars that can be stored.
    // |timeout| the max time to wait for data when using it in a stream.
    // The real capacity will be a power of 2 above capacity.
    // For example:
    // A capacity of 4 allows you to store 7 characters, and takes up 2^3
    RingStreambuf(uint32_t capacity,
                  std::chrono::milliseconds timeout = milliseconds::max());

    // Retrieves the string stored at the given offset.
    // It will block at most timeoutMs.
    // Returns the available data, and the offset at which
    // the first character was retrieved.
    // This call will not modify any read pointers.
    std::pair<int, std::string> bufferAtOffset(
            std::streamsize offset,
            milliseconds timeoutMs = milliseconds(0));

    // Blocks and waits until at least n bytes can be written, or
    // the timeout has expired. Returns the number of bytes that
    // can be written if no other threads have written to the
    // buffer.
    //
    // Note: The return value can be less than n, in case of a timeout.
    std::streamsize waitForAvailableSpace(std::streamsize n);

    // The total number of bytes that can be stored in the buffer.
    size_t capacity() { return mRingbuffer.capacity() - 1; }

    // Closes this stream buffer for writing, reading can succeed until
    // eof, timeouts will be set to 1ms.
    void close();

protected:
    // Implement streambuf interface, not that writes can overwrite existing
    // data and will report as though all bytes have been written.
    std::streamsize xsputn(const char* s, std::streamsize n) override;
    int overflow(int c = EOF) override;
    std::streamsize showmanyc() override;

    // Amount of space available for writing, without erasing
    // the previous data.
    std::streamsize showmanyw();
    std::streamsize xsgetn(char* s, std::streamsize n) override;
    int underflow() override;
    int uflow() override;

private:
    std::vector<char> mRingbuffer;

    uint32_t mHead{0};        // Ringbuffer write pointer (front)
    uint32_t mTail{0};        // Ringbuffer read pointer (tail)
    uint64_t mHeadOffset{0};  // Accumulated offset.
    bool mFull{false};
    bool mClosed{false};
    std::chrono::milliseconds mTimeout;

    std::mutex mLock;
    std::condition_variable mCanRead;
};

}  // namespace streams
}  // namespace base
}  // namespace android
