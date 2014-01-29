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

#include <glib.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Print a panic message then exit the program immediately.
static  __attribute__((noreturn)) void g_panic(const char* fmt, ...){
  va_list args;
  fprintf(stderr, "MiniGLib:PANIC: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(1);
}


// Heap allocation.

void* g_malloc(size_t size) {
  if (size == 0)
    return NULL;

  void* ptr = malloc(size);
  if (ptr == NULL)
    g_panic("Can't allocate %zu bytes!\n", size);

  return ptr;
}


void* g_malloc0(size_t size) {
  void* ptr = g_malloc(size);
  if (ptr == NULL)
    return NULL;

  memset(ptr, 0, size);
  return ptr;
}


void* g_realloc(void* ptr, size_t size) {
  if (size == 0) {
    g_free(ptr);
    return NULL;
  }
  void* new_ptr = realloc(ptr, size);
  if (new_ptr == NULL)
    g_panic("Can't reallocate pointer %p to %zu bytes!\n", ptr, size);

  return new_ptr;
}


void g_free(void* ptr) {
  if (ptr)
    free(ptr);
}

// Strings.

char* g_strdup(const char* str) {
  if (str == NULL)
    return NULL;

  size_t len = strlen(str);
  char* copy = g_malloc(len + 1);
  memcpy(copy, str, len + 1);
  return copy;
}


char* g_strndup(const char* str, size_t size) {
  const char *end = memchr(str, 0, size);
  char *copy;

  if (end)
    size = end - str;

  copy = g_malloc(size + 1);
  memcpy(copy, str, size);
  copy[size] = 0;
  return copy;
}


char* g_strdup_printf(const char* fmt, ...) {
  char* result;
  va_list args;
  va_start(args, fmt);
  g_vasprintf(&result, fmt, args);
  va_end(args);
  return result;
}


char* g_strdup_vprintf(const char* fmt, va_list args) {
  char* result;
  g_vasprintf(&result, fmt, args);
  return result;
}


int g_vasprintf(char** str, const char* fmt, va_list args) {
#ifdef _WIN32
  // On Win32, vsnprintf() is broken and always returns -1 in case of truncation,
  // so make an educated guess, and try again if that fails with a larger pool.
  size_t capacity = 128;
  char* buffer = g_malloc(capacity);
  for (;;) {
    va_list args2;
    va_copy(args2, args);
    int len = vsnprintf(buffer, capacity, fmt, args2);
    if (len >= 0) {
      *str = buffer;
      return len;
    }
    va_end(args2);

    capacity *= 2;
    if (capacity > INT_MAX)
      g_panic("Formatted string is too long!\n");

    buffer = g_realloc(buffer, capacity);
  }
#else
  // On other systems, vsnprintf() works properly and returns the size of the
  // formatted string without truncation.
  va_list args2;
  va_copy(args2, args);
  int len = vsnprintf(NULL, 0, fmt, args2);
  va_end(args2);
  if (len < 0)
    g_panic("Can't format string!\n");

  char* result = g_malloc(len + 1);
  va_copy(args2, args);
  vsnprintf(result, (size_t)len, fmt, args2);
  va_end(args2);

  *str = result;
  return len;
#endif
}
