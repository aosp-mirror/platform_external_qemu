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

#include "android/utils/win32_cmdline_quote.h"

#include <string.h>

#include "android/utils/stralloc.h"
#include "android/utils/system.h"

char* win32_cmdline_quote(const char* param) {
  // If |param| doesn't contain any problematic character, just return it as-is.
  size_t n = strcspn(param, " \t\v\n\"");
  if (param[n] == '\0')
      return ASTRDUP(param);

  // Otherwise, we need to quote some of the characters.
  stralloc_t out = STRALLOC_INIT;

  // Add initial quote.
  stralloc_add_c(&out, '"');

  n = 0;
  while (param[n]) {
      size_t num_backslashes = 0;
      while (param[n] == '\\') {
          n++;
          num_backslashes++;
      }

      if (!param[n]) {
          // End of string, if there are backslashes, double them.
          for (; num_backslashes > 0; num_backslashes--)
              stralloc_add_str(&out, "\\\\");
          break;
      }

      if (param[n] == '"') {
          // Escape all backslashes as well as the quote that follows them.
          for (; num_backslashes > 0; num_backslashes--)
              stralloc_add_str(&out, "\\\\");
          stralloc_add_str(&out, "\\\"");
      } else {
          for (; num_backslashes > 0; num_backslashes--)
              stralloc_add_c(&out, '\\');
          stralloc_add_c(&out, param[n]);
      }
      n++;
  }

  // Add final quote.
  stralloc_add_c(&out, '"');

  char* result = ASTRDUP(stralloc_cstr(&out));
  stralloc_reset(&out);
  return result;
}
