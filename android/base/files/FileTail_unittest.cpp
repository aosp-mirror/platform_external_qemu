// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/files/FileTail.h"

#include "android/base/files/PathUtils.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <memory>

namespace android {
namespace base {

using std::endl;
using std::ofstream;
using std::string;
using std::unique_ptr;
using std::vector;

class FileTailTest : public ::testing::Test {
public:
    // Doo all SetUp in the SetUp function as opposed to constructor because all
    // test cases are constructed much before they're run. (and we don't want to
    // create tons of tempdirs)
    void SetUp() override {
        mTempDir.reset(new TestTempDir("FileTailTest"));
        ASSERT_TRUE(mTempDir->path());
        auto filePath = PathUtils::join(mTempDir->path(), "file.txt");
        mFile.reset(new ofstream(filePath));
        ASSERT_TRUE(bool(*mFile));
        mFileTail.reset(new FileTail(filePath));
        ASSERT_TRUE(mFileTail->good());
    }

protected:
    unique_ptr<TestTempDir> mTempDir;
    unique_ptr<ofstream> mFile;
    unique_ptr<FileTail> mFileTail;
};

TEST_F(FileTailTest, emptyFile) {
    auto line = mFileTail->get();
    EXPECT_EQ("", line);

    auto lines = mFileTail->get(1);
    EXPECT_TRUE(mFileTail->good());
    EXPECT_TRUE(lines.empty());
}

TEST_F(FileTailTest, singleLineWithEOL) {
    *mFile << "First line" << endl;
    *mFile << "Second line" << endl;
    *mFile << "Third line" << endl;

    string line = mFileTail->get();
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ("Third line", line);
}

TEST_F(FileTailTest, singleLineNoEOL) {
    *mFile << "First line" << endl;
    *mFile << "Second line" << endl;
    *mFile << "Third line";
    mFile->flush();

    auto line = mFileTail->get();
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ("Third line", line);
}

TEST_F(FileTailTest, multipleLinesNoEOL) {
    *mFile << "First line" << endl;
    *mFile << "Second line" << endl;
    *mFile << "Third line";
    mFile->flush();

    // Not enough lines.
    auto lines = mFileTail->get(5);
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ(3, lines.size());
    EXPECT_EQ("First line", lines[0]);
    EXPECT_EQ("Second line", lines[1]);
    EXPECT_EQ("Third line", lines[2]);
}

TEST_F(FileTailTest, tailSmall) {
    string line;

    *mFile << "First line" << endl;
    *mFile << "Second line" << endl;
    *mFile << "Third line";
    mFile->flush();
    // Not enough lines.
    auto lines = mFileTail->get(5);
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ(3, lines.size());
    EXPECT_EQ("First line", lines[0]);
    EXPECT_EQ("Second line", lines[1]);
    EXPECT_EQ("Third line", lines[2]);

    *mFile << " continues";
    mFile->flush();
    line = mFileTail->get();
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ("Third line continues", line);

    *mFile << endl;
    line = mFileTail->get();
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ("Third line continues", line);
}

TEST_F(FileTailTest, tailLarge) {
    string line;
    vector<string> lines;
    const unsigned chunk = 3 * FileTail::kSeekChunkSize - 37;
    string line1(chunk, '1');
    string line2(chunk, '2');
    string line3(FileTail::kSeekChunkSize, '3');
    string line4(10, '4');
    string line5(chunk, '5');
    string line6;

    *mFile << line1 << endl;
    line = mFileTail->get();
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ(line1, line);

    *mFile << line2 << endl;
    *mFile << line3 << endl;
    lines = mFileTail->get(4);
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ(3, lines.size());
    EXPECT_EQ(line1, lines[0]);
    EXPECT_EQ(line2, lines[1]);
    EXPECT_EQ(line3, lines[2]);

    *mFile << line4 << endl;
    *mFile << line5 << endl;
    *mFile << line6 << endl;
    lines = mFileTail->get(3);
    EXPECT_TRUE(mFileTail->good());
    EXPECT_EQ(3, lines.size());
    EXPECT_EQ(line4, lines[0]);
    EXPECT_EQ(line5, lines[1]);
    EXPECT_EQ(line6, lines[2]);
}

TEST(FileTailFailureTest, noFile) {
    FileTail fileTail("/non/existent/path");
    EXPECT_FALSE(fileTail.good());
    EXPECT_EQ("", fileTail.get());
}

}  // namespace base
}  // namespace android
