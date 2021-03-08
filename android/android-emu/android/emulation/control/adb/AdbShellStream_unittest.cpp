// Copyright 2019 The Android Open Source Project
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

#include <gmock/gmock.h>  // for GMOCK_PP_INTERNAL_IF_0, GMOCK_PP_INTERNAL_...
#include <gtest/gtest.h>  // for Test, AssertionResult, Message, TestPartRe...

#include "android/base/testing/TestSystem.h"
#include "android/emulation/control/adb/AdbConnection.h"
#include "android/emulation/control/adb/AdbShellStream.h"

using namespace android::emulation;
using namespace testing;

// Nop deleter so we can have "shared" objects on the stack for our unittests.
struct NopDeleter {
    template <class T>
    void operator()(T* ptr) const {
    }
};

const int DEFAULT_TIMEOUT = 2000;

class MockAdbConnection : public AdbConnection {
public:
    MOCK_METHOD(std::shared_ptr<AdbStream>,
                open,
                (const std::string&, uint32_t),
                (override));
    MOCK_METHOD(AdbState, state, (), (const override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, hasFeature, (const std::string&), (const override));
    MOCK_METHOD(AdbConnection*, connection, (), ());
    MOCK_METHOD(void, setAdbPort, (int), ());
};

class MockAdbStream : public AdbStream {
public:
    MockAdbStream(std::streambuf* bf) : AdbStream(bf) {}
    void close() override { setstate(std::ios::eof()); }

    void setWriteTimeout(uint64_t timeoutMs) override {};
};

TEST(AdbShellStream, opens_service_v1) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(false));
    EXPECT_CALL(fake, open("shell:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
}

TEST(AdbShellStream, opens_service_v2) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(true));
    EXPECT_CALL(fake, open("shell,v2,raw:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
}

TEST(AdbShellStream, no_read_from_closed_v2) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(true));
    EXPECT_CALL(fake, open("shell,v2,raw:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
    std::vector<char> sout{};
    std::vector<char> serr{};
    int exitcode{0};

    stream->close();
    shell.read(sout, serr, exitcode);

    EXPECT_EQ(sout.size(), 0);
    EXPECT_EQ(serr.size(), 0);
    EXPECT_EQ(exitcode, 0);
}

TEST(AdbShellStream, handles_null) {
    AdbShellStream adb("foo", nullptr);
    EXPECT_FALSE(adb.good());
}

TEST(AdbShellStream, reads_exit_v2) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(true));
    EXPECT_CALL(fake, open("shell,v2,raw:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
    std::vector<char> sout{};
    std::vector<char> serr{};
    int exitcode{0};

    ShellHeader hdr{.id = ShellHeader::kIdExit, .length = 1};
    buf.sputn((char*)&hdr, sizeof(hdr));
    buf.sputc('A');

    shell.read(sout, serr, exitcode);

    EXPECT_EQ(sout.size(), 0);
    EXPECT_EQ(serr.size(), 0);
    EXPECT_EQ(exitcode, 'A');
}

TEST(AdbShellStream, reads_stdout_v2) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(true));
    EXPECT_CALL(fake, open("shell,v2,raw:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
    std::vector<char> sout{};
    std::vector<char> serr{};
    int exitcode{0};

    ShellHeader hdr{.id = ShellHeader::kIdStdout, .length = 5};
    buf.sputn((char*)&hdr, sizeof(hdr));
    buf.sputn("hello", 5);

    shell.read(sout, serr, exitcode);

    EXPECT_EQ(std::string(sout.begin(), sout.end()), "hello");
    EXPECT_EQ(sout.size(), 5);
    EXPECT_EQ(serr.size(), 0);
    EXPECT_EQ(exitcode, 0);
}

TEST(AdbShellStream, reads_stderr_v2) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(true));
    EXPECT_CALL(fake, open("shell,v2,raw:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
    std::vector<char> sout{};
    std::vector<char> serr{};
    int exitcode{0};

    ShellHeader hdr{.id = ShellHeader::kIdStderr, .length = 5};
    buf.sputn((char*)&hdr, sizeof(hdr));
    buf.sputn("hello", 5);

    shell.read(sout, serr, exitcode);

    EXPECT_EQ(sout.size(), 0);
    EXPECT_EQ(serr.size(), 5);
    EXPECT_EQ(std::string(serr.begin(), serr.end()), "hello");
    EXPECT_EQ(exitcode, 0);
}

TEST(AdbShellStream, write_stdin_v2) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillRepeatedly(Return(true));
    EXPECT_CALL(fake, open("shell,v2,raw:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));

    EXPECT_TRUE(shell.write("hello"));
    char data[512] = {0};
    ShellHeader hdr;
    buf.sgetn((char*)&hdr, sizeof(hdr));
    EXPECT_EQ(hdr.id, ShellHeader::kIdStdin);
    EXPECT_EQ(hdr.length, 5);

    buf.sgetn(data, hdr.length);
    EXPECT_STREQ("hello", data);
}

TEST(AdbShellStream, write_stdin_v1) {
    std::stringbuf buf;
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillRepeatedly(Return(false));
    EXPECT_CALL(fake, open("shell:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));

    EXPECT_TRUE(shell.write("hello"));
    char data[512] = {0};
    buf.sgetn(data, 5);
    EXPECT_STREQ("hello", data);
}


TEST(AdbShellStream, reads_stdout_v1) {
    std::stringbuf buf("hello");
    MockAdbConnection fake;
    std::shared_ptr<AdbStream> stream = std::make_shared<MockAdbStream>(&buf);

    EXPECT_CALL(fake, hasFeature("shell_v2")).WillOnce(Return(false));
    EXPECT_CALL(fake, open("shell:ls", DEFAULT_TIMEOUT)).WillOnce(Return(stream));
    EXPECT_CALL(fake, state()).WillOnce(Return(AdbState::connected));
    AdbShellStream shell("ls", std::shared_ptr<AdbConnection>(&fake, NopDeleter()));
    std::vector<char> sout{};
    std::vector<char> serr{};
    int exitcode{0};

    shell.read(sout, serr, exitcode);
    EXPECT_EQ(std::string(sout.begin(), sout.end()), "hello");
    EXPECT_EQ(sout.size(), 5);
    EXPECT_EQ(serr.size(), 0);
    EXPECT_EQ(exitcode, 0);
}
