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
#include <streambuf>
#include <vector>  // for vector

#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock

namespace android {
namespace base {

// Reclaim strategy for cleaning up the vector, used as:
// mReadOffset = ReclaimFunction(mReadOffset, vector)
using CharVec = std::vector<char>;
using ReclaimFunction = std::function<size_t(size_t, CharVec&)>;

// QueueStreambuf - a thread safe version of stringsteam backed by a vector
//
// Usage examples:
//
//   QueueStreambuf buf();
//   std::ostream stream(&buf);
//   stream << "Hello";
//
class QueueStreambuf : public std::streambuf {
public:
    // |capacity| initial capacity of the underlying vector.
    // |reclaim_strategy| strategy to cleanup unused parts of the buffer.
    QueueStreambuf(uint32_t capacity = 4096,
                   ReclaimFunction reclaim_strategy = &QueueStreambuf::reclaim);

    QueueStreambuf* close();

    // True if close has not been called.
    bool is_open();

    // Default reclaimer
    static size_t reclaim(size_t offset, CharVec& vect);

protected:
    // Implement streambuf interface, not that writes can overwrite existing
    // data and will report as though all bytes have been written.
    std::streamsize xsputn(const char* s, std::streamsize n) override;
    std::streamsize showmanyc() override;
    std::streamsize xsgetn(char* s, std::streamsize n) override;

private:
    CharVec mQueuebuffer;
    bool mOpen{true};
    bool mWrite{false};

    Lock mLock;
    ConditionVariable mCanRead;
    size_t mReadOffset{0};  // Invariant, mReadOffset <= mQueueBuffer.size
    ReclaimFunction mReclaim;
};
}  // namespace base
}  // namespace android
