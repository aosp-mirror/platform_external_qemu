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
#include <stdint.h>
#include <string.h>

// Print a panic message then exit the program immediately.
void g_critical(const char* fmt, ...) {
  va_list args;
  fprintf(stderr, "CRITICAL: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void g_panic(const char* fmt, ...) {
  va_list args;
  fprintf(stderr, "PANIC: ");
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

char** g_strsplit(const char* string, const char* delim, int max_tokens) {
  // Sanity checks.
  if (!string || !delim || !*delim)
    return NULL;

  if (max_tokens < 1)
    max_tokens = INT_MAX;

  // Create empty list of results.
  GSList* string_list = NULL;
  guint n = 0;

  if (*string) {
    // Input list is not empty, so try to split it.
    const char* remainder = string;
    const char* s = strstr(remainder, delim);
    if (s) {
      size_t delim_len = strlen(delim);
      while (--max_tokens && s) {
        size_t len = s - remainder;
        string_list = g_slist_prepend(string_list, g_strndup(remainder, len));
        n++;
        remainder = s + delim_len;
        s = strstr(remainder, delim);
      }
    }
    n++;
    string_list = g_slist_prepend(string_list, g_strdup(remainder));
  }

  // Convert list into NULL-terminated vector.
  char** result = g_new(char*, n + 1);
  result[n--] = NULL;
  GSList* slist = string_list;
  while (slist) {
    result[n--] = slist->data;
    slist = slist->next;
  }
  g_slist_free(string_list);

  return result;
}

void g_strfreev(char** strings) {
  guint n;
  for (n = 0; strings[n]; ++n) {
    g_free(strings[n]);
  }
  g_free(strings);
}

gboolean g_str_equal(const void* v1, const void* v2) {
  return !strcmp((const char*)v1, (const char*)v2);
}

guint g_str_hash(const void* str) {
  const signed char* p = str;
  guint hash = 5381U;

  for (; *p; ++p)
    hash = (hash << 5) + hash + (guint)*p;

  return hash;
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

// Atomic operations

#if !_WIN32
// Note: Windows implementation in glib-mini-win32.c

void g_atomic_int_inc(int volatile* atomic) {
  __sync_fetch_and_add(atomic, 1);
}

gboolean g_atomic_int_dec_and_test(int volatile* atomic) {
  return __sync_fetch_and_add(atomic, -1) == 1;
}
#endif  // !_WIN32

// Hash Tables

// This is a rather vanilla implementation, slightly simpler
// than the GLib one, since QEMU doesn't require the full features:
//
// - Uses a single dynamic array of (key,value,hash) tuples.
// - Array capacity is always 2^N
// - No use of modulo primes for simplicity, we don't expect
//   QEMU/QAPI to degenerate here.
// - Dumb container only: doesn't own and free keys and values.
// - No optimization for sets (i.e. when key == value for each entry).
// - No iterators.
//
typedef struct {
  gpointer key;
  gpointer value;
  guint    hash;
} GHashEntry;

#define HASH_UNUSED    0   // Must be 0, see _remove_all() below.
#define HASH_TOMBSTONE 1
#define HASH_IS_REAL(h)  ((h) >= 2)

#define HASH_MIN_SHIFT  3
#define HASH_MIN_CAPACITY  (1 << HASH_MIN_SHIFT)

#define HASH_LOAD_SCALE 1024
#define HASH_MIN_LOAD   256
#define HASH_MAX_LOAD   768

struct _GHashTable {
  int ref_count;
  int num_items;
  int num_used;  // count of items + tombstones
  int capacity;
  GHashEntry* entries;
  GHashFunc hash_func;
  GEqualFunc key_equal_func;
};

// Default hash function for pointers.
static guint _gpointer_hash(gconstpointer key) {
    return (guint)(uintptr_t)key;
}

// Compute the hash value of a given key.
static inline size_t
_g_hash_table_hash(GHashTable* table, gconstpointer key) {
  size_t hash = table->hash_func(key);
  return HASH_IS_REAL(hash) ? hash : 2;
}


GHashTable* g_hash_table_new(GHashFunc hash_func,
                             GEqualFunc key_equal_func) {
  GHashTable* hash_table = g_new0(GHashTable, 1);

  hash_table->ref_count = 1;
  hash_table->num_items = 0;
  hash_table->capacity = HASH_MIN_CAPACITY;
  hash_table->entries = g_new0(GHashEntry, hash_table->capacity);
  hash_table->hash_func = hash_func ? hash_func : &_gpointer_hash;
  hash_table->key_equal_func = key_equal_func;

  return hash_table;
}


static void _g_hash_table_remove_all(GHashTable* hash_table) {
  // NOTE: This uses the fact that HASH_UNUSED is 0!
  hash_table->num_items = 0;
  memset(hash_table->entries,
         0,
         sizeof(hash_table->entries[0]) * hash_table->capacity);
}


GHashTable* g_hash_table_ref(GHashTable* hash_table) {
  if (!hash_table)
    return NULL;

  g_atomic_int_inc(&hash_table->ref_count);
  return hash_table;
}


void g_hash_table_unref(GHashTable* hash_table) {
  if (!hash_table)
    return;

  if (!g_atomic_int_dec_and_test(&hash_table->ref_count))
    return;

  _g_hash_table_remove_all(hash_table);

  g_free(hash_table->entries);
  hash_table->capacity = 0;
  hash_table->entries = NULL;

  g_free(hash_table);
}


void g_hash_table_destroy(GHashTable* hash_table) {
  if (!hash_table)
    return;

  _g_hash_table_remove_all(hash_table);
  g_hash_table_unref(hash_table);
}


// Probe the hash table for |key|. If it is in the table, return the index
// to the corresponding entry. Otherwise, return the index of an unused
// or tombstone entry. Also sets |*key_hash| to the key hash value on
// return.
static guint _g_hash_table_lookup_index(GHashTable* hash_table,
                                        gconstpointer key,
                                        guint* key_hash) {
  guint hash = _g_hash_table_hash(hash_table, key);
  *key_hash = hash;

  guint probe_mask = (hash_table->capacity - 1);
  gint tombstone = -1;
  guint probe_index = hash & probe_mask;
  guint step = 0;

  GHashEntry* probe = &hash_table->entries[probe_index];
  while (probe->hash != HASH_UNUSED) {
    if (probe->hash == hash) {
      if (hash_table->key_equal_func) {
        if (hash_table->key_equal_func(probe->key, key))
          return probe_index;
      } else if (probe->key == key) {
        return probe_index;
      }
    } else if (probe->hash == HASH_TOMBSTONE && tombstone < 0) {
      tombstone = (int)probe_index;
    }

    step++;
    probe_index = (probe_index + step) & probe_mask;
    probe = &hash_table->entries[probe_index];
  }

  if (tombstone >= 0)
    return (guint)tombstone;

  return probe_index;
}


void* g_hash_table_lookup(GHashTable* hash_table,
                          const void* key) {
  guint key_hash = HASH_UNUSED;
  guint probe_index = _g_hash_table_lookup_index(hash_table, key, &key_hash);
  GHashEntry* entry = &hash_table->entries[probe_index];

  return HASH_IS_REAL(entry->hash) ? entry->value : NULL;
}


// Remove key/value pair at index position |i|.
static void _g_hash_table_remove_index(GHashTable* hash_table,
                                       int i) {
  GHashEntry* entry = &hash_table->entries[i];
  entry->hash = HASH_TOMBSTONE;
  entry->key = NULL;
  entry->value = NULL;
}


gboolean g_hash_table_remove(GHashTable* hash_table,
                             const void* key) {
  guint key_hash = HASH_UNUSED;
  guint probe_index = _g_hash_table_lookup_index(hash_table, key, &key_hash);
  GHashEntry* entry = &hash_table->entries[probe_index];
  if (!HASH_IS_REAL(entry->hash))
    return 0;

  _g_hash_table_remove_index(hash_table, probe_index);
  hash_table->num_items--;
  return 1;
}

// Resize the hash table, this also gets rid of all tombstones.
static void _g_hash_table_resize(GHashTable* hash_table) {
  guint old_capacity = hash_table->capacity;

  // Compute new capacity from new size
  guint new_size = hash_table->num_items * 2;
  guint new_capacity = HASH_MIN_CAPACITY;
  while (new_capacity < new_size)
    new_capacity <<= 1;

  GHashEntry* new_entries = g_new0(GHashEntry, new_capacity);
  guint n;
  for (n = 0; n < old_capacity; ++n) {
    GHashEntry* old_entry = &hash_table->entries[n];
    guint old_hash = old_entry->hash;

    if (!HASH_IS_REAL(old_hash))
      continue;

    guint probe_mask = (new_capacity - 1);
    guint probe_n = old_hash & probe_mask;
    GHashEntry* probe = &new_entries[probe_n];
    guint step = 0;
    while (probe->hash != HASH_UNUSED) {
      step++;
      probe_n = (probe_n + step) & probe_mask;
      probe = &new_entries[probe_n];
    }
    probe[0] = old_entry[0];
  }

  g_free(hash_table->entries);
  hash_table->entries = new_entries;
  hash_table->capacity = new_capacity;
  hash_table->num_used = hash_table->num_items;
}

// Resize the hash table if needed.
static void _g_hash_table_maybe_resize(GHashTable* hash_table) {
  guint count = hash_table->num_items;
  guint capacity = hash_table->capacity;
  // Computations explained.
  //
  // load < min_load i.e.
  // => used / capacity < min_load
  // => used < min_load * capacity
  // => used * HASH_SCALE < HASH_MIN_LOAD * capacity
  //
  // load > max_load
  // => used / capacity > max_load
  // => used * HASH_SCALER > HASH_MAX_LOAD * capacity
  uint64_t load = (uint64_t)count * HASH_LOAD_SCALE;
  uint64_t min_load = (uint64_t)capacity * HASH_MIN_LOAD;
  uint64_t max_load = (uint64_t)capacity * HASH_MAX_LOAD;
  if (load < min_load || load > max_load) {
    _g_hash_table_resize(hash_table);
  }
}

static void _g_hash_table_insert_index(GHashTable* hash_table,
                                       guint key_index,
                                       guint new_key_hash,
                                       gpointer new_key,
                                       gpointer new_value) {
  GHashEntry* entry = &hash_table->entries[key_index];
  guint old_hash = entry->hash;

  entry->key = new_key;
  entry->value = new_value;
  entry->hash = new_key_hash;

  if (HASH_IS_REAL(old_hash)) {
    // Simple replacement, exit immediately.
    return;
  }

  hash_table->num_items++;
  if (old_hash == HASH_TOMBSTONE) {
    // No need to resize when replacing a tombstone.
    return;
  }

  hash_table->num_used++;
  _g_hash_table_maybe_resize(hash_table);
}

void g_hash_table_insert(GHashTable* hash_table,
                         void* key,
                         void* value) {
  guint key_hash;
  guint key_index =
      _g_hash_table_lookup_index(hash_table, key, &key_hash);

  _g_hash_table_insert_index(hash_table, key_index, key_hash, key, value);
}


void g_hash_table_foreach(GHashTable* hash_table,
                          GHFunc func,
                          gpointer user_data) {
  guint n;
  for (n = 0; n < hash_table->capacity; ++n) {
    GHashEntry* entry = &hash_table->entries[n];
    if (HASH_IS_REAL(entry->hash))
      (*func)(entry->key, entry->value, user_data);
  }
}


gpointer g_hash_table_find(GHashTable* hash_table,
                           GHRFunc predicate,
                           gpointer user_data) {
  guint n;
  for (n = 0; n < hash_table->capacity; ++n) {
    GHashEntry* entry = &hash_table->entries[n];
    if (HASH_IS_REAL(entry->hash) &&
        (*predicate)(entry->key, entry->value, user_data)) {
      return entry->value;
    }
  }
  return NULL;
}


guint g_hash_table_size(GHashTable* hash_table) {
  return hash_table->num_items;
}

// Queues

struct _GQueueNode {
  void* data;
  GQueueNode* next;
  GQueueNode* prev;
};

static inline GQueueNode* _g_queue_node_alloc(void) {
  return g_new0(GQueueNode, 1);
}

static void inline _g_queue_node_free(GQueueNode* node) {
  g_free(node);
}

GQueue* g_queue_new(void) {
  GQueue* queue = g_new0(GQueue, 1);
  return queue;
}

void g_queue_free(GQueue* queue) {
  GQueueNode* node = queue->head;
  while (node) {
    GQueueNode* next = node->next;
    _g_queue_node_free(node);
    node = next;
  }
  queue->head = queue->tail = NULL;
  queue->length = 0;
  g_free(queue);
}

gboolean g_queue_is_empty(GQueue* queue) {
  return queue->head == NULL;
}

void g_queue_push_tail(GQueue* queue, void* data) {
  GQueueNode* node = _g_queue_node_alloc();
  node->data = data;
  node->next = NULL;
  node->prev = queue->tail;
  queue->tail = node;
  queue->length++;
}

void* g_queue_peek_head(GQueue* queue) {
  return (queue->head) ? queue->head->data : NULL;
}

void* g_queue_peek_tail(GQueue* queue) {
  return (queue->tail) ? queue->tail->data : NULL;
}

void* g_queue_pop_head(GQueue* queue) {
  GQueueNode* head = queue->head;
  if (!head)
    return NULL;

  void* result = head->data;

  if (head->next) {
    queue->head = head->next;
    head->next->prev = NULL;
  } else {
    queue->head = NULL;
    queue->tail = NULL;
  }
  queue->length--;

  return result;
}
