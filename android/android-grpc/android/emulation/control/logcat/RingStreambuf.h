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
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"

namespace android {
namespace emulation {
namespace control {

using namespace base;


// RingStreambuf - a thread safe streambuffer backed by a ring buffer.
// This thing acts as a sliding window over a stream of data.
//
// Usage examples:
//
// This creates an output stream that can write at most 5 characters, before
// it start overwriting old characters.
//
//   RingStreambuf buf(5);
//   std::ostream stream(&buf);
//   stream << "Hello";
//
// Be very careful when using this as an input stream!
// - It will block when nothing is available
// - It will consume the stream (i.e. read pointers will move)
class RingStreambuf : public std::streambuf {
public:
    // |capacity| the number of bytes that can be stored
    RingStreambuf(int capacity);

    // Retrieves the string stored at the given offset.
    // It will block at most timeoutMs.
    // Returns the available data, and the offset at which
    // the first character was retrieved.
    // This call will not modify any read pointers.
    std::pair<int, std::string> bufferAtOffset(
            uint64_t offset,
            System::Duration timeoutMs = 0);

protected:
    // Implement streambuf interface.
    std::streamsize xsputn(const char* s, std::streamsize n) override;
    int overflow(int c = EOF) override;
    std::streamsize showmanyc();
    std::streamsize xsgetn(char* s, std::streamsize n);
    int underflow() override;
    int uflow() override;

private:
.
    std::vector<char> mRingBuffer;

    uint16_t mWrite{0};  // Ringbuffer write pointer
    uint16_t mRead{0};   // Ringbuffer read pointer
    uint64_t mWriteOffset{0}; // Accumulated offset.


    Lock mLock;
    ConditionVariable mCanRead;
};  // namespace control

}  // namespace control
}  // namespace emulation
}  // namespace android