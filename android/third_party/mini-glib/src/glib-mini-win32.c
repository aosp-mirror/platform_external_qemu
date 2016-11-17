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

#ifdef _WIN32

#include <glib.h>

#include <assert.h>
#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Atomic operations

void g_atomic_int_inc(int volatile* atomic) {
  assert(sizeof(LONG) == sizeof(int));
  InterlockedIncrement((LONG volatile*)atomic);
}

gboolean g_atomic_int_dec_and_test(int volatile* atomic) {
  assert(sizeof(LONG) == sizeof(int));
  return !InterlockedIncrement((LONG volatile*)atomic);
}

// Win32 error messages.

static char*
utf16_to_utf8(const wchar_t* wstring, int wstring_len)
{
  int utf8_len = WideCharToMultiByte(CP_UTF8,          // CodePage
                                     0,                // dwFlags
                                     (LPWSTR) wstring, // lpWideCharStr
                                     wstring_len,      // cchWideChar
                                     NULL,             // lpMultiByteStr
                                     0,                // cbMultiByte
                                     NULL,             // lpDefaultChar
                                     NULL);            // lpUsedDefaultChar
  if (utf8_len == 0)
    return g_strdup("");

  char* result = g_malloc(utf8_len + 1);

  WideCharToMultiByte(CP_UTF8, 0, (LPWSTR) wstring, wstring_len,
                      result, utf8_len, NULL, NULL);
  result[utf8_len] = '\0';
  return result;
}

char *
g_win32_error_message (int error)
{
  LPWSTR msg = NULL;
  int nchars;
  char* result;

  // Work around for compiler warning, due to brain-dead API.
  union {
    LPWSTR* address;
    LPWSTR  value;
  } msg_param;
  msg_param.address = &msg;

  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                 |FORMAT_MESSAGE_IGNORE_INSERTS
                 |FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, error, 0,
                 msg_param.value,
                 0, NULL);
  if (!msg)
    return g_strdup("");

  // Get rid of trailing \r\n if any.
  nchars = wcslen (msg);
  if (nchars > 2 && msg[nchars-1] == '\n' && msg[nchars-2] == '\r')
    msg[nchars-2] = '\0';

  result = utf16_to_utf8 (msg, nchars);

  LocalFree (msg);

  return result;
}

#endif  // _WIN32
