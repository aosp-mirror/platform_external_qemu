#include <cstdarg>

#include <stdio.h>

#define LOG_BUF_SIZE 1024

extern "C" {

extern int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  return 0;
}

}
