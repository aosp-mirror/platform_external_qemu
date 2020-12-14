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

#include "android/base/files/TarStream.h"

#include <gtest/gtest.h>  // for Assert...
#include <string.h>       // for memcmp
#include <fstream>        // for ostream

#include "android/base/Optional.h"             // for Optional
#include "android/base/StringView.h"           // for String...
#include "android/base/files/GzipStreambuf.h"  // for GzipIn...
#include "android/base/files/PathUtils.h"      // for pj
#include "android/base/system/System.h"        // for System
#include "android/base/testing/TestSystem.h"   // for TestSy...
#include "android/base/testing/TestTempDir.h"  // for TestTe...

using android::base::GzipInputStream;
using android::base::GzipOutputStream;
using android::base::PathUtils;
using android::base::pj;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;

namespace android {
namespace base {

class TarStreamTest : public ::testing::Test {
protected:
    TarStreamTest() : mTestSystem("/", System::kProgramBitness) {}

    void SetUp() {
        auto testDir = mTestSystem.getTempRoot();
        testDir->makeSubDir("in");
        testDir->makeSubDir("out");
        indir = pj(testDir->path(), "in");
        outdir = pj(testDir->path(), "out");
        std::string hi = pj(indir, "hello.txt");
        std::string hi2 = pj(indir, "hello2.txt");

        std::ofstream hello(hi, std::ios_base::binary);
        hello << "Hello World!";
        hello.close();

        std::ofstream hello2(hi2, std::ios_base::binary);
        hello2 << "World! How are you?";
        hello2.close();
    }

    bool is_equal(std::string file1, std::string file2) {
        std::ifstream ifs1(file1, std::ios::binary);
        std::ifstream ifs2(file2, std::ios::binary);
        char buf1[512] = {0};
        char buf2[512] = {0};
        bool same = true;
        while (same && ifs1.good() && ifs2.good()) {
            ifs1.read(buf1, sizeof(buf1));
            ifs2.read(buf2, sizeof(buf2));

            same = memcmp(buf1, buf2, sizeof(buf1)) == 0;
        }

        return same && (ifs1.good() == ifs2.good());
    }

    TestSystem mTestSystem;
    std::string indir;
    std::string outdir;
};

TEST_F(TarStreamTest, bad_header_no_ustar) {
    std::stringstream ss;
    TarWriter tw(mTestSystem.getTempRoot()->path(), ss);
    EXPECT_TRUE(tw.addDirectoryEntry("in"));
    EXPECT_TRUE(tw.close());

    // Wrong offset will cause bad headers.
    ss.ignore(100);
    TarReader tr(outdir, ss);
    auto fst = tr.first();

    EXPECT_FALSE(fst.valid);
    EXPECT_STREQ(tr.error_msg().c_str(),
                 "Incorrect tar header. Expected ustar format not: ");

    EXPECT_TRUE(tr.fail());
};

TEST_F(TarStreamTest, bad_header_chksum) {
    std::stringstream ss;
    TarWriter tw(mTestSystem.getTempRoot()->path(), ss);
    EXPECT_TRUE(tw.addDirectoryEntry("in"));
    EXPECT_TRUE(tw.close());

    // Let's mess up the header..
    auto hacked = ss.str();
    hacked[1] = 'B';
    std::stringstream bad(hacked);
    TarReader tr(outdir, bad);
    auto fst = tr.first();

    EXPECT_TRUE(tr.fail());
    EXPECT_FALSE(fst.valid);
    // Note header is checksum not constant, so just check for the start msg.
    EXPECT_NE(tr.error_msg().find("Incorrect tar header."), std::string::npos);
};

TEST_F(TarStreamTest, incomplete_archive) {
    std::stringstream ss;
    TarWriter tw(indir, ss);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));

    // Let's remove some bytes from the end.
    const int sz = TARBLOCK + 5;
    auto hacked = std::make_unique<char[]>(sz);
    printf("%d\n", sz);
    ss.read(hacked.get(), sz);
    std::stringstream bad;
    bad.write(hacked.get(), sz);
    TarReader tr(outdir, bad);

    EXPECT_TRUE(tr.good());
    auto fst = tr.first();

    EXPECT_TRUE(fst.valid);
    EXPECT_FALSE(tr.extract(fst));
    EXPECT_FALSE(tr.good());
    EXPECT_EQ(tr.error_msg(),
              "Unexpected EOF during extraction of hello.txt at offset 517");
};

TEST_F(TarStreamTest, handle_unclosed_tar_properly) {
    std::stringstream ss;
    TarWriter tw(mTestSystem.getTempRoot()->path(), ss);
    EXPECT_TRUE(tw.addDirectoryEntry("in"));

    TarReader tr(outdir, ss);
    auto fst = tr.first();

    EXPECT_TRUE(fst.valid);
};

TEST_F(TarStreamTest, cannot_write_to_closed_tar) {
    std::stringstream ss;
    TarWriter tw(indir, ss);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));
    EXPECT_TRUE(tw.close());
    EXPECT_FALSE(tw.addFileEntry("hello.txt"));
};

TEST_F(TarStreamTest, read_write_dir) {
    std::stringstream ss;
    TarWriter tw(mTestSystem.getTempRoot()->path(), ss);
    EXPECT_TRUE(tw.addDirectoryEntry("in"));
    EXPECT_TRUE(tw.close());

    TarReader tr(outdir, ss);
    auto fst = tr.first();

    EXPECT_TRUE(fst.valid);
    EXPECT_STREQ(fst.name.c_str(), "in");
    EXPECT_EQ(fst.type, TarType::DIRTYPE);
    EXPECT_TRUE(tr.extract(fst));
    EXPECT_TRUE(mTestSystem.host()->pathIsDir(pj(outdir, "in")));
}

TEST_F(TarStreamTest, read_write_id) {
    std::stringstream ss;
    TarWriter tw(indir, ss);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));
    EXPECT_TRUE(tw.close());

    TarReader tr(outdir, ss);
    auto fst = tr.first();

    EXPECT_TRUE(fst.valid);
    EXPECT_STREQ(fst.name.c_str(), "hello.txt");
    EXPECT_EQ(fst.type, TarType::REGTYPE);
    EXPECT_TRUE(tr.extract(fst));
    EXPECT_TRUE(is_equal(pj(indir, "hello.txt"), pj(outdir, "hello.txt")));
}

TEST_F(TarStreamTest, read_write_gzip_id) {
    std::stringstream ss;
    GzipOutputStream gos(ss);
    TarWriter tw(indir, gos);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));
    EXPECT_TRUE(tw.close());

    GzipInputStream gis(ss);
    TarReader tr(outdir, gis);
    auto fst = tr.first();

    EXPECT_TRUE(fst.valid);
    EXPECT_STREQ(fst.name.c_str(), "hello.txt");
    EXPECT_EQ(fst.type, TarType::REGTYPE);
    EXPECT_TRUE(tr.extract(fst));
    EXPECT_TRUE(is_equal(pj(indir, "hello.txt"), pj(outdir, "hello.txt")));
}

TEST_F(TarStreamTest, tar_can_list) {
    // Make sure native tar utility can read the contents..
    std::string tar = pj(outdir, "test.tar");
    std::ofstream tarfile(tar);
    TarWriter tw(indir, tarfile);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));
    EXPECT_TRUE(tw.close());
    tarfile.close();

    auto result = mTestSystem.host()->runCommandWithResult({"tar", "tf", tar});
    if (result) {
        EXPECT_NE(result->find("hello.txt"), std::string::npos);
    }
}

TEST_F(TarStreamTest, tar_can_extract) {
    // Make sure native tar utility can read the contents..
    std::string tar = pj(outdir, "test.tar");
    std::ofstream tarfile(tar);
    TarWriter tw(indir, tarfile);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));
    EXPECT_TRUE(tw.close());
    tarfile.close();

    // untar the things..
    auto currdir = mTestSystem.host()->getCurrentDirectory();
    mTestSystem.host()->setCurrentDirectory(outdir);
    auto result = mTestSystem.host()->runCommandWithResult({"tar", "xf", tar});
    mTestSystem.host()->setCurrentDirectory(currdir);

    if (result) {
        EXPECT_TRUE(is_equal(pj(indir, "hello.txt"), pj(outdir, "hello.txt")));
    }
}

TEST_F(TarStreamTest, tar_can_extract_multiple_files) {
    // Make sure native tar utility can read the contents..
    std::string tar = pj(outdir, "test.tar");
    std::ofstream tarfile(tar);
    TarWriter tw(indir, tarfile);
    EXPECT_TRUE(tw.addFileEntry("hello.txt"));
    EXPECT_TRUE(tw.addFileEntry("hello2.txt"));
    EXPECT_TRUE(tw.close());
    tarfile.close();

    // untar the things..
    auto currdir = mTestSystem.host()->getCurrentDirectory();
    mTestSystem.host()->setCurrentDirectory(outdir);
    auto result = mTestSystem.host()->runCommandWithResult({"tar", "xf", tar});
    mTestSystem.host()->setCurrentDirectory(currdir);

    if (result) {
        EXPECT_TRUE(is_equal(pj(indir, "hello.txt"), pj(outdir, "hello.txt")));
        EXPECT_TRUE(
                is_equal(pj(indir, "hello2.txt"), pj(outdir, "hello2.txt")));
    }
}

TEST_F(TarStreamTest, stream_can_extract_tar) {
    // Make sure we can handle tar files created by tar..
    std::string tar = pj(outdir, "test.tar");

    // tar the things..
    auto currdir = mTestSystem.host()->getCurrentDirectory();
    mTestSystem.host()->setCurrentDirectory(indir);
    auto result = mTestSystem.host()->runCommandWithResult(
            {"tar", "cf", tar, "hello.txt"});
    mTestSystem.host()->setCurrentDirectory(currdir);

    if (result) {
        std::ifstream tarfile(tar);
        TarReader tr(outdir, tarfile);
        auto fst = tr.first();

        EXPECT_TRUE(fst.valid);
        EXPECT_STREQ(fst.name.c_str(), "hello.txt");
        EXPECT_TRUE(tr.extract(fst));
        EXPECT_TRUE(is_equal(pj(indir, "hello.txt"), pj(outdir, "hello.txt")));
    }
}

}  // namespace base
}  // namespace android
