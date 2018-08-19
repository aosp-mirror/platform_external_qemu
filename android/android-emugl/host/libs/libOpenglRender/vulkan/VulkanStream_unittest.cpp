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

#include "android/base/ArraySize.h"

#include <gtest/gtest.h>

#include <string.h>

namespace goldfish_vk {

class VulkanStreamForTesting : public VulkanStream {
public:
    ssize_t read(void* buffer, size_t size) override {
        EXPECT_LE(mReadCursor + size, mBuffer.size());
        memcpy(buffer, mBuffer.data() + mReadCursor, size);

        mReadCursor += size;

        if (mReadCursor == mWriteCursor) {
            clear();
        }
        return size;
    }

    ssize_t write(const void* buffer, size_t size) override {
        if (mBuffer.size() < mWriteCursor + size) {
            mBuffer.resize(mWriteCursor + size);
        }

        memcpy(mBuffer.data() + mWriteCursor, buffer, size);

        mWriteCursor += size;

        if (mReadCursor == mWriteCursor) {
            clear();
        }
        return size;
    }

private:
    void clear() {
        mBuffer.clear();
        mReadCursor = 0;
        mWriteCursor = 0;
    }

    size_t mReadCursor = 0;
    size_t mWriteCursor = 0;
    std::vector<char> mBuffer;
};

// Just see whether the test class is OK
TEST(VulkanStream, Basic) {
    VulkanStreamForTesting stream;

    const uint32_t testInt = 6;
    stream.putBe32(testInt);
    EXPECT_EQ(testInt, stream.getBe32());

    const std::string testString = "Hello World";
    stream.putString(testString);
    EXPECT_EQ(testString, stream.getString());
}

// Write / read arrays of strings
TEST(VulkanStream, StringArray) {
    VulkanStreamForTesting stream;

    const char* const testStrings[] = {
        "Hello World",
        "asdf",
        "",
        "https://android.googlesource.com",
    };

    stream.putStringArray(testStrings, android::base::arraySize(testStrings));

    auto stringsReturned = stream.getStringArray();

    for (int i = 0; i < stringsReturned.size(); i++) {
        EXPECT_EQ(stringsReturned[i], testStrings[i]);
    }
}

} // namespace goldfish_vk
