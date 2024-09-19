// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "aemu/base/logging/Log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "aemu/base/logging/LogSeverity.h"
#include "absl/log/globals.h"
#include "absl/log/absl_log.h"
#include "absl/log/log_sink_registry.h"
#include "absl/strings/str_format.h"

namespace android {
namespace base {

// Create a severity level which is guaranteed to never generate a log
// message. See LogOnlyEvaluatesArgumentsIfNeeded for usage.
const LogSeverity EMULATOR_LOG_INVISIBLE = static_cast<LogSeverity>(-10000);

class CaptureLogSink : public absl::LogSink {
   public:
    void Send(const absl::LogEntry& entry) override {
        char level = 'I';
        switch (entry.log_severity()) {
            case absl::LogSeverity::kInfo:
                level = 'I';
                break;
            case absl::LogSeverity::kError:
                level = 'E';
                break;
            case absl::LogSeverity::kWarning:
                level = 'W';
                break;

            case absl::LogSeverity::kFatal:
                level = 'F';
                break;
        }
        captured_log_ = absl::StrFormat("%c %s:%d |%s|- %s", level, entry.source_filename(),
                                        entry.source_line(), absl::FormatTime(entry.timestamp()),
                                        entry.text_message());
    }

    std::string captured_log_;
};

class LogTest : public ::testing::Test, android::base::testing::LogOutput {
public:
    LogTest() : mFatal(false) {
        mSavedOutput = ::android::base::testing::LogOutput::setNewOutput(this);
        mExpected[0] = '\0';
        mBuffer[0] = '\x7f';
        mBuffer[1] = '\0';
    }

    ~LogTest() {
        ::android::base::testing::LogOutput::setNewOutput(mSavedOutput);
    }

    void setExpected(LogSeverity severity, int line, const char* suffix) {
        mExpectedParams.file = __FILE__;
        mExpectedParams.lineno = line;
        mExpectedParams.severity = severity;
        snprintf(mExpected, sizeof(mExpected), "%s", suffix);
    }

    // LogOutput override
    void logMessage(const LogParams& params,
                    const char* message,
                    size_t messageLen) {
        mParams = params;
        if (messageLen > sizeof(mBuffer) - 1)
            messageLen = sizeof(mBuffer) - 1;
        ::memcpy(mBuffer, message, messageLen);
        mBuffer[messageLen] = '\0';
        mFatal = (params.severity >= EMULATOR_LOG_FATAL);
    }

protected:
    ::android::base::testing::LogOutput* mSavedOutput;
    LogParams mParams;
    LogParams mExpectedParams;
    char mExpected[1024];
    char mBuffer[1024];
    bool mFatal;
};

class CheckTest : public LogTest {};

#if ENABLE_DCHECK != 0
class DCheckEnabledTest : public LogTest {
public:
    DCheckEnabledTest() : LogTest() {
        // Ensure DCHECKS() always run.
        mSavedLevel = setDcheckLevel(true);
    }

    ~DCheckEnabledTest() { setDcheckLevel(mSavedLevel); }

private:
    bool mSavedLevel;
};
#endif  // ENABLE_DCHECK == 0

#if ENABLE_DCHECK != 2
class DCheckDisabledTest : public LogTest {
public:
    DCheckDisabledTest() : LogTest() { mSavedLevel = setDcheckLevel(false); }

    ~DCheckDisabledTest() { setDcheckLevel(mSavedLevel); }

private:
    bool mSavedLevel;
};
#endif  // ENABLE_DCHECK != 2

class PLogTest : public LogTest {
public:
    PLogTest() : LogTest(), mForcedErrno(-1000) {}

    void setForcedErrno(int errnoCode) { mForcedErrno = errnoCode; }

    void setExpectedErrno(LogSeverity severity,
                          int line,
                          int errnoCode,
                          const char* suffix) {
        mExpectedParams.file = __FILE__;
        mExpectedParams.lineno = line;
        mExpectedParams.severity = severity;
        snprintf(mExpected, sizeof(mExpected), "%sError message: %s", suffix,
                 strerror(errnoCode));
    }

    void logMessage(const LogParams& params,
                    const char* message,
                    size_t messageLen) {
        LogTest::logMessage(params, message, messageLen);

        if (mForcedErrno != -1000)
            errno = mForcedErrno;
    }

protected:
    int mForcedErrno;
};


#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

#define EXPECTED_STRING_PREFIX(prefix, line) \
    prefix ":" __FILE__ ":" STRINGIFY(line) ": "

#define CHECK_EXPECTATIONS()                               \
    EXPECT_STREQ(mExpectedParams.file, mParams.file);      \
    EXPECT_EQ(mExpectedParams.lineno, mParams.lineno);     \
    EXPECT_EQ(mExpectedParams.severity, mParams.severity); \
    EXPECT_STREQ(mExpected, mBuffer)

// Helper function used to set a boolean |flag|, then return |string|.
static const char* setFlag(bool* flag, const char* string) {
    *flag = true;
    return string;
}

TEST(LogString, EmptyString) {
    LogString ls("");
    EXPECT_STREQ("", ls.string());
}

TEST(LogString, LongString) {
    std::string longString =
            "hello this is a really long string, one that should result in a "
            "buffer overflow as it has so many "
            "characters that it will not fit in the default buffer. This means "
            "that we should allocate some additional "
            "memory to make sure the characters fit. This used to cause issues "
            "in the past.";

    LogString ls("%s", longString.c_str());
    EXPECT_EQ(longString, ls.string());
}

TEST(LogString, SimpleString) {
    LogString ls("Hello");
    EXPECT_STREQ("Hello", ls.string());
}

TEST(LogString, FormattedString) {
    LogString ls("%d plus %d equals %d", 12, 23, 35);
    EXPECT_STREQ("12 plus 23 equals 35", ls.string());
}

TEST_F(LogTest, LogInfoEmpty) {
    setExpected(EMULATOR_LOG_INFO, __LINE__ + 1, "");
    LOG(INFO) << "";
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogInfoWithString) {
    static const char kString[] = "Hello World!";
    setExpected(EMULATOR_LOG_INFO, __LINE__ + 1, kString);
    LOG(INFO) << kString;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogInfoWithTwoStrings) {
    setExpected(EMULATOR_LOG_INFO, __LINE__ + 1, "Hello Globe!");
    LOG(INFO) << "Hello "
              << "Globe!";
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogAReallyLongString) {
    std::string longString = "Hello World!";
    for (int i = 0; i < 8; i++) {
        longString += longString;
    }
    setExpected(EMULATOR_LOG_INFO, __LINE__ + 1, longString.c_str());
    LOG(INFO) << longString;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogInfoWithLogString) {
    LogString ls("Hello You!");
    setExpected(EMULATOR_LOG_INFO, __LINE__ + 1, ls.string());
    LOG(INFO) << ls;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogWarning) {
    static const char kWarning[] = "Elvis has left the building!";
    setExpected(EMULATOR_LOG_WARNING, __LINE__ + 1, kWarning);
    LOG(WARNING) << kWarning;
    CHECK_EXPECTATIONS();
}

TEST_F(LogTest, LogError) {
    static const char kError[] = "Bad Bad Robot!";
    setExpected(EMULATOR_LOG_ERROR, __LINE__ + 1, kError);
    LOG(ERROR) << kError;

    CHECK_EXPECTATIONS();
}


LOGGING_API extern "C" void __emu_log_print(LogSeverity prio,
                                            const char* file,
                                            int line,
                                            const char* fmt,
                                            ...);


// Test truncation when message exceeds buffer size

class OutputLogTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Add the CaptureLogSink
        log_sink_ = std::make_unique<CaptureLogSink>();
        absl::AddLogSink(log_sink_.get());

        absl::SetVLogLevel("*", 2);

        // Set log level to capture everything (adjust as needed)
        absl::SetStderrThreshold(absl::LogSeverity::kInfo);
    }

    void TearDown() override {
        // Remove the CaptureLogSink
        absl::RemoveLogSink(log_sink_.get());
    }

    std::unique_ptr<CaptureLogSink> log_sink_;
};


TEST_F(OutputLogTest, Truncation) {
    std::string long_msg(4100, 'x');  // Exceeds buffer size
    __emu_log_print(LogSeverity::EMULATOR_LOG_INFO, "foo", 10, "%s",
                    long_msg.c_str());

    std::string expected_msg = long_msg.substr(0, 4093) + "...";
    EXPECT_THAT(log_sink_->captured_log_, ::testing::HasSubstr(expected_msg));
}

TEST_F(LogTest, LogFatal) {
    static const char kFatalMessage[] = "I'm dying";
    setExpected(EMULATOR_LOG_FATAL, __LINE__ + 1, kFatalMessage);
    LOG(FATAL) << kFatalMessage;
    CHECK_EXPECTATIONS();
    EXPECT_TRUE(mFatal);
}

TEST_F(LogTest, LogEvaluatesArgumentsIfNeeded) {
    // Use EMULATOR_LOG_FATAL since it is always active.
    bool flag = false;
    setExpected(EMULATOR_LOG_FATAL, __LINE__ + 1, "PANIC: Flag was set!");
    LOG(FATAL) << "PANIC: " << setFlag(&flag, "Flag was set!");
    CHECK_EXPECTATIONS();
    EXPECT_TRUE(mFatal);
    EXPECT_TRUE(flag);
}

TEST_F(LogTest, LogOnlyEvaluatesArgumentsIfNeeded) {
    bool flag = false;
    LOG(INVISIBLE) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
}

// TODO(digit): Convert this to a real death test when this is supported
// by our version of GTest.
TEST_F(CheckTest, CheckFalse) {
    setExpected(EMULATOR_LOG_FATAL, __LINE__ + 1, "Check failed: false. ");
    CHECK(false);
    CHECK_EXPECTATIONS();
}

TEST_F(CheckTest, CheckFalseEvaluatesArguments) {
    bool flag = false;
    setExpected(EMULATOR_LOG_FATAL, __LINE__ + 2,
                "Check failed: false. Flag was set!");
    CHECK(false) << setFlag(&flag, "Flag was set!");
    EXPECT_TRUE(flag);
    CHECK_EXPECTATIONS();
}


TEST_F(CheckTest, CheckTrue) {
    CHECK(true);
    EXPECT_FALSE(mFatal);
}

TEST_F(CheckTest, CheckTrueDoesNotEvaluateArguments) {
    bool flag = false;
    CHECK(true) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
    EXPECT_FALSE(mFatal);
}

#if ENABLE_DCHECK != 0
TEST_F(DCheckEnabledTest, DCheckIsOnReturnsTrue) {
    EXPECT_TRUE(DCHECK_IS_ON());
}

TEST_F(DCheckEnabledTest, DCheckFalse) {
    bool flag = false;
    setExpected(EMULATOR_LOG_FATAL, __LINE__ + 2,
                "Check failed: false. Flag was set!");
    DCHECK(false) << setFlag(&flag, "Flag was set!");
    CHECK_EXPECTATIONS();
}

TEST_F(DCheckEnabledTest, DCheckTrue) {
    bool flag = false;
    DCHECK(true) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
    EXPECT_FALSE(mFatal);
}
#endif  // ENABLE_DCHECK != 0

#if ENABLE_DCHECK != 2
TEST_F(DCheckDisabledTest, DCheckIsOnReturnsFalse) {
    EXPECT_FALSE(DCHECK_IS_ON());
}

TEST_F(DCheckDisabledTest, DCheckFalse) {
    bool flag = false;
    DCHECK(false) << setFlag(&flag, "Flag was set!");
    EXPECT_FALSE(flag);
    EXPECT_FALSE(mFatal);
}

TEST_F(DCheckDisabledTest, DCheckTrue) {
    DCHECK(true);
}
#endif  // ENABLE_DCHECK != 2

TEST_F(PLogTest, PLogInfoEmpty) {
    setExpectedErrno(EMULATOR_LOG_INFO, __LINE__ + 2, EINVAL, "");
    errno = EINVAL;
    PLOG(INFO) << "";
    CHECK_EXPECTATIONS();
}

TEST_F(PLogTest, PLogInfoPreservesErrno) {
    // Select a value that is unlikely to ever be raised by the logging
    // machinery.
    const int kErrnoCode = ENOEXEC;
    setForcedErrno(EINVAL);
    setExpectedErrno(EMULATOR_LOG_INFO, __LINE__ + 2, kErrnoCode, "Hi");
    errno = kErrnoCode;
    PLOG(INFO) << "Hi";
    EXPECT_EQ(kErrnoCode, errno);
    CHECK_EXPECTATIONS();
}

#if ENABLE_DLOG
TEST_F(PLogTest, DPlogInfoEmpty) {
    setExpectedErrno(EMULATOR_LOG_INFO, __LINE__ + 2, EINVAL, "");
    errno = EINVAL;
    DPLOG(INFO) << "";
    CHECK_EXPECTATIONS();
}

TEST_F(PLogTest, DPlogInfoPreservesErrno) {
    // Select a value that is unlikely to ever be raised by the logging
    // machinery.
    const int kErrnoCode = ENOEXEC;
    setForcedErrno(EINVAL);
    setExpectedErrno(EMULATOR_LOG_INFO, __LINE__ + 2, kErrnoCode, "Hi");
    errno = kErrnoCode;
    DPLOG(INFO) << "Hi";
    EXPECT_EQ(kErrnoCode, errno);
    CHECK_EXPECTATIONS();
}

#endif

}  // namespace base
}  // namespace android
