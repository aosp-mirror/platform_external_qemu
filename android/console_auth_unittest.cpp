// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/ScopedFd.h"
#include "android/console_auth.h"
#include "android/console_auth_internal.h"
#include "android/utils/tempfile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtest/gtest.h>

#include <limits>
#include <memory>

using android::base::String;
using android::base::ScopedFd;

namespace android {
namespace console_auth {

static String temp_auth_token_file;
String get_temp_auth_token_file() {
    return temp_auth_token_file;
}

TEST(ConsoleAuth, generate_random_bytes) {
    char buf[20];
    memset(buf, 0, sizeof(buf));

    // test what we can
    EXPECT_TRUE(generate_random_bytes(buf, 16));

    // make sure we haven't overflowed
    EXPECT_EQ(0, buf[16]);

    // confirm that all of the bytes have been effected
    for (int i = 0; i < 16; i++) {
        // if we find a 0, it might be a legit random 0, let's be sure
        while (buf[i] == 0) {
            EXPECT_TRUE(generate_random_bytes(buf, 16));
        }
    }
}

TEST(ConsoleAuth, write_file_to_end) {
    const char test_pattern[] = "test pattern";
    TempFile* tf = tempfile_create();

    int fd = EINTR_RETRY(open(tempfile_path(tf), O_RDWR, 0600));
    EXPECT_NE(-1, fd);

    EXPECT_FALSE(write_file_to_end(-1, test_pattern));
    EXPECT_TRUE(write_file_to_end(fd, test_pattern));

    String dummy_buffer("dummy");
    EXPECT_FALSE(read_file_to_end(-1, &dummy_buffer));
    EXPECT_STREQ("dummy", dummy_buffer.c_str());

    String read_buffer;
    EXPECT_TRUE(read_file_to_end(fd, &read_buffer));
    EXPECT_STREQ(test_pattern, read_buffer.c_str());

    EINTR_RETRY(close(fd));

    tempfile_close(tf);
}

TEST(ConsoleAuth, trim) {
    struct {
        const char* before;
        const char* after;
    } test_data[] = {
            {"no change", "no change"},
            {"", ""},
            {" left", "left"},
            {"right ", "right"},
            {"\nnewline\r", "newline"},
            {" \r\n\tmulti \t\r\n", "multi"},
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        String before(test_data[i].before);
        String after = trim(before);
        EXPECT_STREQ(test_data[i].after, after.c_str());
    }
}

#define TEST_SKIP(string, skipped) \
    EXPECT_EQ(string + skipped, str_skip_white_space_if_any(string))
TEST(ConsoleAuth, str_skip_white_space_if_any) {
    TEST_SKIP("no change", 0);
    TEST_SKIP("", 0);
    TEST_SKIP(" left", 1);
    TEST_SKIP("right ", 0);
    TEST_SKIP("\nnewline\r", 1);
    TEST_SKIP(" \r\n\tmulti \t\r\n", 4);
}

TEST(ConsoleAuth, str_begins_with) {
    EXPECT_TRUE(str_begins_with("12345678", "1234"));
    EXPECT_TRUE(str_begins_with("auth", "auth"));
    EXPECT_TRUE(str_begins_with("", ""));
    EXPECT_TRUE(str_begins_with("12345678", ""));
    EXPECT_FALSE(str_begins_with("12345678", "abcd"));
    EXPECT_FALSE(str_begins_with("12345678", "1234abcd"));
    EXPECT_FALSE(str_begins_with("1234", "12345678"));
    EXPECT_FALSE(str_begins_with("", "abcd"));
}

TEST(ConsoleAuth, get_auth_token_from_exists) {
    String auth_token;

    const char* test_data[] = {
            "test pattern",
            "test pattern\r\n",
            "  test pattern",
            "  test pattern\r\n",
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    TempFile* tf = tempfile_create();
    tempfile_close(tf);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        int fd = EINTR_RETRY(
                open(tempfile_path(tf), O_CREAT | O_TRUNC | O_WRONLY, 0600));
        EXPECT_NE(-1, fd);
        EXPECT_TRUE(write_file_to_end(fd, test_data[i]));
        EINTR_RETRY(close(fd));

        EXPECT_TRUE(get_auth_token_from(tempfile_path(tf), &auth_token));
        EXPECT_STREQ("test pattern", auth_token.c_str());
        tempfile_close(tf);
    }
}

bool isbase64(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || c == '+' || c == '/' || c == '=';
}

TEST(ConsoleAuth, get_auth_token_from_create) {
    String auth_token;

    TempFile* tf = tempfile_create();
    tempfile_close(tf);

    EXPECT_TRUE(get_auth_token_from(tempfile_path(tf), &auth_token));
    struct stat stat_info;
    EXPECT_EQ(0, stat(tempfile_path(tf), &stat_info));
    EXPECT_EQ(24, stat_info.st_size);
    // all we know about this token is that it will be 16 bytes base64 encoded
    for (int i = 0; i < 24; i++) {
        EXPECT_TRUE(isbase64(auth_token[i]));
    }
    EXPECT_EQ(24, auth_token.size());

    tempfile_close(tf);
}

TEST(ConsoleAuth, get_auth_token_from_fail) {
    String auth_token;
    EXPECT_FALSE(get_auth_token_from("/absolutely/bogus/path", &auth_token));
}

String read_file(const String& path) {
    ScopedFd fd(EINTR_RETRY(open(path.c_str(), O_RDONLY)));
    EXPECT_NE(-1, fd.get());
    String result;
    EXPECT_TRUE(read_file_to_end(fd.get(), &result));
    return result;
}

TEST(ConsoleAuth, android_console_auth_get_status) {
    TempFile* tf = tempfile_create();

    // override default auth token file path
    temp_auth_token_file.assign(tempfile_path(tf));
    g_get_auth_token_path = get_temp_auth_token_file;

    // test with an empty auth token file
    EXPECT_EQ(CONSOLE_AUTH_STATUS_DISABLED, android_console_auth_get_status());

    // test with no auth token file (it should be created)
    tempfile_close(tf);
    EXPECT_EQ(CONSOLE_AUTH_STATUS_REQUIRED, android_console_auth_get_status());

    String auth_token = read_file(temp_auth_token_file);
    String command("auth ");
    command.append(auth_token);
    EXPECT_TRUE(
            android_console_auth_check_authorization_command(command.c_str()));
    command.append("\r\n");
    EXPECT_TRUE(
            android_console_auth_check_authorization_command(command.c_str()));
    EXPECT_FALSE(android_console_auth_check_authorization_command("auth 1234"));
    command.assign("auth ");
    command.append(auth_token.c_str(), 12);  // only the first half
    EXPECT_FALSE(
            android_console_auth_check_authorization_command(command.c_str()));
    temp_auth_token_file.assign("/very/bad/path");
    EXPECT_EQ(CONSOLE_AUTH_STATUS_ERROR, android_console_auth_get_status());

    char buf[1024];
    android_console_auth_get_token_path(buf, sizeof(buf));
    EXPECT_STREQ("/very/bad/path", buf);
}

}  // namespace console_auth
}  // namespace android
