// Copyright (C) 2018 The Android Open Source Project
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
#include "VulkanStream.h"

#include <gtest/gtest.h>

#include <string.h>

namespace goldfish_vk {

class VulkanStreamForTesting : public VulkanStream {
public:
    ssize_t read(void* buffer, size_t size) override {
        EXPECT_LE(mReadCursor + size, mBuffer.size());
        memcpy(buffer, mBuffer.data() + mReadCursor, size);
        return size;
    }

    ssize_t write(const void* buffer, size_t size) override {
        if (mBuffer.size() < mWriteCursor + size) {
            mBuffer.resize(mWriteCursor + size);
        }
        memcpy(mBuffer.data() + mWriteCursor, buffer, size);
        return size;
    }
private:
    size_t mReadCursor = 0;
    size_t mWriteCursor = 0;
    std::vector<char> mBuffer;
};

TEST(VulkanStream, Basic) {
    VulkanStreamForTesting stream;

    stream.putBe32(6);
    EXPECT_EQ(6, stream.getBe32());
}

} // namespace goldfish_vk
