// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/JsonWriter.h"

#include "android/base/misc/FileUtils.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <memory>

using namespace android::base;

static void doTestObj(JsonWriter& w) {
    w.setIndent("  ");
    w.beginObject()
        .name("TestObject1").value("SomeString1")
        .name("TestObject2").value("SomeString2")
        .beginArray()
            .value(0)
            .value(1)
            .value(2)
            .value("otherString")
            .valueBool(true)
            .valueBool(false)
            .beginObject()
                .name("TestObject3").value(5)
                .name("TestObject4").valueNull()
            .endObject()
        .endArray()
        .endObject();
    w.flush();
}

class JsonWriterTest : public ::testing::Test {
protected:
    void SetUp() override { mTempDir.reset(new TestTempDir("jsonwritertest")); }
    void TearDown() override { mTempDir.reset(); }
    std::unique_ptr<TestTempDir> mTempDir;
};

// Tests that we can generate nonzero strings in JSON.
TEST_F(JsonWriterTest, Basic) {
    JsonWriter w;
    doTestObj(w);
    EXPECT_GT(w.contents().size(), 0);
}

// Tests that we can write to a file.
TEST_F(JsonWriterTest, File) {
    std::string testFilePath = mTempDir->makeSubPath("test.json");

    {
        JsonWriter jsonFile(testFilePath);
        doTestObj(jsonFile);
    }

    // At this point, dtor is called and file is written
    const auto fileContents = android::readFileIntoString(testFilePath);
    EXPECT_TRUE(fileContents);
    EXPECT_GT(fileContents->size(), 0);
}

// Tests that the generated file is equal to the string.
TEST_F(JsonWriterTest, SelfConsistency) {
    std::string testFilePath = mTempDir->makeSubPath("test.json");

    JsonWriter jsonStandard;

    doTestObj(jsonStandard);

    {
        JsonWriter jsonFile(testFilePath);
        doTestObj(jsonFile);
    }

    std::string expected = jsonStandard.contents();
    const auto fileContents = android::readFileIntoString(testFilePath);
    EXPECT_TRUE(fileContents);
    EXPECT_EQ(expected.size(), fileContents->size());
    EXPECT_EQ(expected, fileContents);
}