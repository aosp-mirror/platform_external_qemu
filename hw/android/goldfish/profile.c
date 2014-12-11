/* Copyright (C) 2014-2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "hw/android/goldfish/profile.h"
#include "exec/code-profile.h"
#include "cpu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SAMPLE_PERIOD 1000000
#define SAMPLE_BURST 100
#define OUTPUT_THRESHOLD 100

typedef struct MmapData {
  target_ulong start;
  target_ulong end;
  target_ulong offset;
  char *name;
  struct MmapData *next;
} MmapData;

typedef struct RangeProfile {
  uint64_t start;
  uint64_t size;
  uint64_t count;
  struct RangeProfile *next;
} RangeProfile;

typedef struct BranchProfile {
  uint64_t from;
  uint64_t to;
  uint64_t count;
  struct BranchProfile *next;
} BranchProfile;

typedef struct BinaryProfile {
  char *name;
  RangeProfile *range;
  BranchProfile *branch;
  struct BinaryProfile *next;
} BinaryProfile;

extern unsigned get_current_pid();

static MmapData *mmaps[65536];
static BinaryProfile *head_binary = NULL;

#define BUF_SIZE 4096

static void dump_profile() {
  BinaryProfile *curr = head_binary;
  int len = strlen(code_profile_dirname);
  char filename[BUF_SIZE];
  assert(len + 2 < BUF_SIZE);
  strcpy(filename, code_profile_dirname);
  filename[len] = '/';
  filename[len + 1] = 0;

  while (curr) {
    char *i;

    assert(len + strlen(curr->name) + 2 < BUF_SIZE);
    strcpy(filename + len + 1, curr->name);
    for (i = filename + len + 1; *i; i++) {
      if (*i == '/')
        *i = '_';
    }
    *i = '.';
    *(i + 1) = 't';
    *(i + 2) = 'x';
    *(i + 3) = 't';
    *(i + 4) = 0;
    FILE *f = fopen(filename, "w");

    RangeProfile *r = curr->range;
    int range_size = 0;
    while (r) {
      range_size++;
      r = r->next;
    }
    fprintf(f, "%d\n", range_size);
    r = curr->range;
    while (r) {
      fprintf(f, "%llx-%llx:%llu\n", (unsigned long long)r->start,
              (unsigned long long)(r->start + r->size),
              (unsigned long long)r->count);
      r = r->next;
    }

    fprintf(f, "%d\n", range_size);
    r = curr->range;
    while (r) {
      fprintf(f, "%llx:%llu\n", (unsigned long long)r->start,
              (unsigned long long)r->count);
      r = r->next;
    }

    BranchProfile *b = curr->branch;
    int branch_size = 0;
    while (b) {
      branch_size++;
      b = b->next;
    }
    fprintf(f, "%d\n", branch_size);
    b = curr->branch;
    while (b) {
      fprintf(f, "%llx->%llx:%llu\n", (unsigned long long)b->from,
              (unsigned long long)b->to, (unsigned long long)b->count);
      b = b->next;
    }
    fclose(f);
    curr = curr->next;
  }
}

static BinaryProfile *get_binary_profile(const char *name) {
  BinaryProfile *curr = head_binary;
  while (curr) {
    if (!strcmp(curr->name, name)) {
      return curr;
    }
    curr = curr->next;
  }
  curr = (BinaryProfile *)malloc(sizeof(BinaryProfile));
  curr->name = strdup(name);
  curr->range = NULL;
  curr->branch = NULL;
  curr->next = head_binary;
  head_binary = curr;
  return curr;
}

static void add_branch_count(BinaryProfile *profile, target_ulong from,
                             target_ulong to) {
  BranchProfile *curr = profile->branch;
  while (curr) {
    if (curr->from == from && curr->to == to) {
      curr->count++;
      return;
    }
    curr = curr->next;
  }
  BranchProfile *b = (BranchProfile *)malloc(sizeof(BranchProfile));
  b->from = from;
  b->to = to;
  b->count = 1;
  b->next = profile->branch;
  profile->branch = b;
}

static void add_range_count(BinaryProfile *profile, target_ulong start,
                            target_ulong size) {
  RangeProfile *curr = profile->range;
  while (curr) {
    if (curr->start == start && curr->size == size) {
      curr->count++;
      return;
    }
    curr = curr->next;
  }
  RangeProfile *r = (RangeProfile *)malloc(sizeof(RangeProfile));
  r->start = start;
  r->size = size;
  r->count = 1;
  r->next = profile->range;
  profile->range = r;
}

static const MmapData *get_mmap_data(target_ulong pc, int pid) {
  const MmapData *data = mmaps[pid];
  while (data) {
    if (pc >= data->start && pc < data->end) {
      return data;
    }
    data = data->next;
  }
  return NULL;
}

void record_mmap(target_ulong vstart, target_ulong vend, target_ulong offset,
                 const char *path, unsigned pid) {
  MmapData *data = (MmapData *)malloc(sizeof(MmapData));
  data->start = vstart;
  data->end = vend;
  data->offset = offset;
  data->name = strdup(path);
  data->next = mmaps[pid];
  mmaps[pid] = data;
}

void release_mmap(unsigned pid) {
  MmapData *curr = mmaps[pid];
  while (curr) {
    MmapData *next = curr->next;
    free(curr->name);
    free(curr);
    curr = next;
  }
  mmaps[pid] = 0;
}

void profile_bb_helper(target_ulong pc, uint32_t size) {
  static int bb_counter = 0;
  static int output_counter = 0;
  static int prev_pid;
  static const MmapData *prev_mmap;
  static target_ulong prev_pc;
  static BinaryProfile *curr_profile;
  if (code_profile_dirname == NULL)
    return;
  if (size == 0) {
    return;
  }
  /* pc + size is the start of the next bb, which does not belong to the
     current bb. We decrement size by 1 to make sure instructions in the
     range of [pc, pc+size] all belong to the current bb.  */
  size--;
  if (bb_counter++ == SAMPLE_PERIOD) {
    prev_pid = get_current_pid();
    prev_mmap = get_mmap_data(pc, prev_pid);
    if (prev_mmap == NULL) {
      bb_counter = 0;
      return;
    }
    prev_pc = pc + size;
    curr_profile = get_binary_profile(prev_mmap->name);

    if (output_counter++ == OUTPUT_THRESHOLD) {
      dump_profile();
      output_counter = 0;
    }
  } else if (bb_counter > SAMPLE_PERIOD) {
    if (bb_counter > SAMPLE_PERIOD + SAMPLE_BURST ||
        get_current_pid() != prev_pid) {
      bb_counter = 0;
      return;
    }
    const MmapData *data = get_mmap_data(pc, prev_pid);
    if (data != prev_mmap) {
      bb_counter = 0;
      return;
    }
    add_range_count(curr_profile, pc + data->offset - data->start, size);
    add_branch_count(curr_profile, prev_pc + data->offset - data->start,
                     pc + data->offset - data->start);
    prev_pc = pc + size;
  }
}
