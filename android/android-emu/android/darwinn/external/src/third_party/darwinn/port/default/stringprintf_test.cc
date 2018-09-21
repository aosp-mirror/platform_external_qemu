// Copyright 2006 Google Inc. All Rights Reserved.
// Author: jyasskin@google.com (Jeffrey Yasskin)

#include "third_party/darwinn/port/default/stringprintf.h"

#include <string>

#include "testing/base/public/gunit.h"
#include "third_party/darwinn/port/default/macros.h"

namespace platforms {
namespace darwinn {
namespace {

TEST(StringPrintfTest, Empty) {
  EXPECT_EQ("", StringPrintf("%s", std::string().c_str()));
  EXPECT_EQ("", StringPrintf("%s", ""));
}

TEST(StringPrintfTest, Multibyte) {
  // If we are in multibyte mode and feed invalid multibyte sequence,
  // StringPrintf should return an empty string instead of running
  // out of memory while trying to determine destination buffer size.
  // see b/4194543.

  char* old_locale = setlocale(LC_CTYPE, nullptr);
  // Push locale with multibyte mode
  setlocale(LC_CTYPE, "en_US.utf8");

  const char kInvalidCodePoint[] = "\375\067s";
  std::string value = StringPrintf("%.*s", 3, kInvalidCodePoint);

  // In some versions of glibc (e.g. eglibc-2.11.1, aka GRTEv2), snprintf
  // returns error given an invalid codepoint. Other versions
  // (e.g. eglibc-2.15, aka pre-GRTEv3) emit the codepoint verbatim.
  // We test that the output is one of the above.
  EXPECT_TRUE(value.empty() || value == kInvalidCodePoint);

  // Repeat with longer string, to make sure that the dynamically
  // allocated path in StringAppendV is handled correctly.
  int n = 2048;
  char* buf = new char[n+1];
  memset(buf, ' ', n-3);
  memcpy(buf + n - 3, kInvalidCodePoint, 4);
  value =  StringPrintf("%.*s", n, buf);
  // See GRTEv2 vs. GRTEv3 comment above.
  EXPECT_TRUE(value.empty() || value == buf);
  delete[] buf;

  setlocale(LC_CTYPE, old_locale);
}

TEST(StringPrintfTest, NoMultibyte) {
  // No multibyte handling, but the string contains funny chars.
  char* old_locale = setlocale(LC_CTYPE, nullptr);
  setlocale(LC_CTYPE, "POSIX");
  std::string value = StringPrintf("%.*s", 3, "\375\067s");
  setlocale(LC_CTYPE, old_locale);
  EXPECT_EQ("\375\067s", value);
}

TEST(StringPrintfTest, DontOverwriteErrno) {
  // Check that errno isn't overwritten unless we're printing
  // something significantly larger than what people are normally
  // printing in their badly written PLOG() statements.
  errno = ECHILD;
  std::string value = StringPrintf("Hello, %s!", "World");
  EXPECT_EQ(ECHILD, errno);
}

TEST(StringPrintfTest, LargeBuf) {
  // Check that the large buffer is handled correctly.
  int n = 2048;
  char* buf = new char[n+1];
  memset(buf, ' ', n);
  buf[n] = 0;
  string value = StringPrintf("%s", buf);
  EXPECT_EQ(buf, value);
  delete[] buf;
}

}  // namespace
}  // namespace darwinn
}  // namespace platforms
