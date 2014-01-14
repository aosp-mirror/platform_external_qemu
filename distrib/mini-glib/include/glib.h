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

#ifndef GLIB_H
#define GLIB_H

#include <stdarg.h>
#include <stddef.h>

// Heap allocation.
void* g_malloc(size_t size);
void* g_malloc0(size_t size);
void* g_realloc(void* ptr, size_t size);
void g_free(void* ptr);

#define g_new(type, count)         ((type*) g_malloc(sizeof(*type) * (count))
#define g_new0(type, count)        ((type*) g_malloc0(sizeof(*type) * (count))

#define g_renew(type, mem, count)  \
    ((type*) g_realloc((mem), sizeof(*type) * (count))

// Strings.
int g_vasprintf(char** str, const char* fmt, va_list args);
char* g_strdup(const char* str);
char* g_strndup(const char* str, size_t size);
char* g_strdup_printf(const char* fmt, ...);
char* g_strdup_vprintf(const char* fmt, va_list args);

#endif  // GLIB_H
