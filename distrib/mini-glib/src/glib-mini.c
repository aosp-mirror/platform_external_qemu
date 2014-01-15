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
void g_panic(const char* fmt, ...){
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


// Single-linked list

static GSList* _g_slist_alloc(void) {
  return (GSList*) g_malloc(sizeof(GSList));
}

void g_slist_free(GSList* list) {
  while (list) {
    GSList* next = list->next;
    g_free(list);
    list = next;
  }
}

GSList* g_slist_last(GSList* list) {
  while (list && list->next)
    list = list->next;
  return list;
}

GSList* g_slist_find(GSList* list, const void* data) {
  while (list) {
    if (list->data == data)
      break;
    list = list->next;
  }
  return list;
}

GSList* g_slist_append(GSList* list, void* data) {
  GSList* new_list = _g_slist_alloc();
  new_list->data = data;
  new_list->next = NULL;

  if (!list)
    return new_list;

  GSList* last = g_slist_last(list);
  last->next = new_list;
  return list;
}

GSList* g_slist_prepend(GSList* list, void* data) {
  GSList* new_list = _g_slist_alloc();
  new_list->data = data;
  new_list->next = list;
  return new_list;
}

GSList* g_slist_remove(GSList* list, const void* data) {
  GSList** pnode = &list;
  for (;;) {
    GSList* node = *pnode;
    if (!node)
      break;
    if (node->data == data) {
      *pnode = node->next;
      g_slist_free(node);
      break;
    }
    pnode = &node->next;
  }
  return list;
}

void g_slist_foreach(GSList* list, GFunc func, void* user_data) {
  while (list) {
    GSList* next = list->next;
    (*func)(list->data, user_data);
    list = next;
  }
}

// 
static GSList* g_slist_sort_merge(GSList* l1,
                                  GSList* l2,
                                  GFunc compare_func,
                                  void* user_data) {
  GSList* list = NULL;
  GSList** tail = &list;

  while (l1 && l2) {
    int cmp = ((GCompareDataFunc) compare_func)(l1->data, l2->data, user_data);
    if (cmp <= 0) {
      *tail = l1;
      tail = &l1->next;
      l1 = l1->next;
    } else {
      *tail = l2;
      tail = &l2->next;
      l2 = l2->next;
    }
  }
  *tail = l1 ? l1 : l2;

  return list;
}

static GSList* g_slist_sort_real(GSList* list,
                                 GFunc compare_func,
                                 void* user_data) {

  if (!list)
    return NULL;
  if (!list->next)
    return list;

  // Split list into two halves.
  GSList* l1 = list;
  GSList* l2 = list->next;

  while (l2->next && l2->next->next) {
    l2 = l2->next->next;
    l1 = l1->next;
  }
  l2 = l1->next;
  l1->next = NULL;

  return g_slist_sort_merge(
      g_slist_sort_real(list, compare_func, user_data),
      g_slist_sort_real(l2, compare_func, user_data),
      compare_func,
      user_data);
}

GSList* g_slist_sort(GSList* list, GCompareFunc compare_func) {
  return g_slist_sort_real(list, (GFunc) compare_func, NULL);
}
