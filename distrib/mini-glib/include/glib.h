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

// Types

typedef char gchar;
typedef int gint;
typedef unsigned int guint;
typedef int gboolean;
typedef void* gpointer;
typedef const void* gconstpointer;

typedef void (*GFunc)(gpointer data, gpointer user_data);

typedef int (*GCompareFunc)(gconstpointer a,
                            gconstpointer b);

typedef int (*GCompareDataFunc)(gconstpointer a,
                                gconstpointer b,
                                gpointer user_data);

typedef gboolean (*GEqualFunc)(gconstpointer a, gconstpointer b);

typedef guint (*GHashFunc)(gconstpointer key);

typedef void (*GHFunc)(gpointer key,
                       gpointer value,
                       gpointer user_data);

typedef gboolean (*GHRFunc)(gpointer key,
                            gpointer value,
                            gpointer user_data);

// Testing

// TODO(digit): Turn assertions on.


void g_panic(const char* fmt, ...) __attribute__((noreturn));

#define g_assert(condition)  do { \
    if (!(condition)) { \
      g_panic("%s:%d: Assertion failure: %s\n", \
              __FILE__, \
              __LINE__, \
              #condition); \
      } \
  } while (0)

#define g_assert_not_reached()  \
    g_panic("%s:%d: Assertion failure: NOT REACHED\n", __FILE__, __LINE__)

// Heap allocation.
void* g_malloc(size_t size);
void* g_malloc0(size_t size);
void* g_realloc(void* ptr, size_t size);
void g_free(void* ptr);

#define g_new(type, count)         ((type*) g_malloc(sizeof(type) * (count)))
#define g_new0(type, count)        ((type*) g_malloc0(sizeof(type) * (count)))

#define g_renew(type, mem, count)  \
    ((type*) g_realloc((mem), sizeof(type) * (count)))

// Strings.
int g_vasprintf(char** str, const char* fmt, va_list args);
char* g_strdup(const char* str);
char* g_strndup(const char* str, size_t size);
char* g_strdup_printf(const char* fmt, ...);
char* g_strdup_vprintf(const char* fmt, va_list args);

gboolean g_str_equal(const void* s1, const void* s2);
guint g_str_hash(const void* str);

// Atomic operations

void g_atomic_int_inc(int volatile* atomic);

gboolean g_atomic_int_dec_and_test(int volatile* atomic);

// Single-linked lists

typedef struct _GSList {
  void* data;
  struct _GSList* next;
} GSList;

void g_slist_free(GSList* list);
GSList* g_slist_last(GSList* list);
GSList* g_slist_find(GSList* list, gconstpointer data);
GSList* g_slist_append(GSList* list, gpointer data);
GSList* g_slist_prepend(GSList* list, gpointer data);
GSList* g_slist_remove(GSList* list, gconstpointer data);
void g_slist_foreach(GSList* list, GFunc func, gpointer user_data);
GSList* g_slist_sort(GSList* list, GCompareFunc compare_func);

// Hash tables

typedef struct _GHashTable GHashTable;

GHashTable* g_hash_table_new(GHashFunc hash_func,
                             GEqualFunc key_equal_func);

void g_hash_table_destroy(GHashTable* hash_table);

void g_hash_table_insert(GHashTable* hash_table,
                         void* key,
                         void* value);

void* g_hash_table_lookup(GHashTable* hash_table,
                          const void* key);

gboolean g_hash_table_remove(GHashTable* hash_table,
                         const void* key);

void g_hash_table_foreach(GHashTable* hash_table,
                          GHFunc func,
                          gpointer user_data);

gpointer g_hash_table_find(GHashTable* hash_table,
                           GHRFunc predicate,
                           gpointer user_data);

guint g_hash_table_size(GHashTable* hash_table);

GHashTable* g_hash_table_ref(GHashTable* hash_table);

void g_hash_table_unref(GHashTable* hash_table);


#ifdef _WIN32
char* g_win32_error_message(int error);
#endif

#endif  // GLIB_H
