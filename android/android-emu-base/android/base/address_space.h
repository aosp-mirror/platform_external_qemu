// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/utils/compiler.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ANDROID_BEGIN_HEADER

#ifdef ADDRESS_SPACE_NAMESPACE
namespace ADDRESS_SPACE_NAMESPACE {
#endif

// This is ported from goldfish_address_space, allowing it to be used for
// general sub-allocations of any buffer range.
// It is also a pure header library, so there are no compiler tricks needed
// to use this in a particular implementation. please don't include this
// in a file that is included everywhere else, though.

/* Represents a continuous range of addresses and a flag if this block is
 * available
 */
struct address_block {
    uint64_t offset;
    union {
        uint64_t size_available; /* VMSTATE_x does not support bit fields */
        struct {
            uint64_t size : 63;
            uint64_t available : 1;
        };
    };
};

/* A dynamic array of address blocks, with the following invariant:
 * blocks[i].size > 0
 * blocks[i+1].offset = blocks[i].offset + blocks[i].size
 */
struct address_space_allocator {
    struct address_block *blocks;
    int size;
    int capacity;
    uint64_t total_bytes;
};

#define ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET (~(uint64_t)0)
#define ANDROID_EMU_ADDRESS_SPACE_DEFAULT_PAGE_SIZE 4096

/* The assert function to abort if something goes wrong. */
static void address_space_assert(bool condition) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_ASSERT_FUNC
    ANDROID_EMU_ADDRESS_SPACE_ASSERT_FUNC(condition);
#else
    assert(condition);
#endif
}

static void* address_space_malloc0(size_t size) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_MALLOC0_FUNC
    return ANDROID_EMU_ADDRESS_SPACE_MALLOC0_FUNC(size);
#else
    void* res = malloc(size);
    memset(res, 0, size);
    return res;
#endif
}

static void* address_space_realloc(void* ptr, size_t size) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_REALLOC_FUNC
    return ANDROID_EMU_ADDRESS_SPACE_REALLOC_FUNC(ptr, size);
#else
    void* res = realloc(ptr, size);
    return res;
#endif
}

static void address_space_free(void* ptr) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC
    return ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC(ptr);
#else
    free(ptr);
#endif
}

/* Looks for the smallest (to reduce fragmentation) available block with size to
 * fit the requested amount and returns its index or -1 if none is available.
 */
static int address_space_allocator_find_available_block(
    struct address_block *block,
    int n_blocks,
    uint64_t size_at_least)
{
    int index = -1;
    uint64_t size_at_index = 0;
    int i;

    address_space_assert(n_blocks >= 1);

    for (i = 0; i < n_blocks; ++i, ++block) {
        uint64_t this_size = block->size;
        address_space_assert(this_size > 0);

        if (this_size >= size_at_least && block->available &&
            (index < 0 || this_size < size_at_index)) {
            index = i;
            size_at_index = this_size;
        }
    }

    return index;
}

static int
address_space_allocator_grow_capacity(int old_capacity) {
    address_space_assert(old_capacity >= 1);

    return old_capacity + old_capacity;
}

/* Inserts one more address block right after i'th (by borrowing i'th size) and
 * adjusts sizes:
 * pre:
 *   size > blocks[i].size
 *
 * post:
 *   * might reallocate allocator->blocks if there is no capacity to insert one
 *   * blocks[i].size -= size;
 *   * blocks[i+1].size = size;
 */
static struct address_block*
address_space_allocator_split_block(
    struct address_space_allocator *allocator,
    int i,
    uint64_t size)
{
    address_space_assert(allocator->capacity >= 1);
    address_space_assert(allocator->size >= 1);
    address_space_assert(allocator->size <= allocator->capacity);
    address_space_assert(i >= 0);
    address_space_assert(i < allocator->size);
    address_space_assert(size < allocator->blocks[i].size);

    if (allocator->size == allocator->capacity) {
        int new_capacity = address_space_allocator_grow_capacity(allocator->capacity);
        allocator->blocks =
            (struct address_block*)
            address_space_realloc(
                allocator->blocks,
                sizeof(struct address_block) * new_capacity);
        address_space_assert(allocator->blocks);
        allocator->capacity = new_capacity;
    }

    struct address_block *blocks = allocator->blocks;

    /*   size = 5, i = 1
     *   [ 0 | 1 |  2  |  3  | 4 ]  =>  [ 0 | 1 | new |  2  | 3 | 4 ]
     *         i  (i+1) (i+2)                 i  (i+1) (i+2)
     */
    memmove(&blocks[i + 2], &blocks[i + 1],
            sizeof(struct address_block) * (allocator->size - i - 1));

    struct address_block *to_borrow_from = &blocks[i];
    struct address_block *new_block = to_borrow_from + 1;

    uint64_t new_size = to_borrow_from->size - size;

    to_borrow_from->size = new_size;

    new_block->offset = to_borrow_from->offset + new_size;
    new_block->size = size;
    new_block->available = 1;

    ++allocator->size;

    return new_block;
}

/* Marks i'th block as available. If adjacent ((i-1) and (i+1)) blocks are also
 * available, it merges i'th block with them.
 * pre:
 *   i < allocator->size
 * post:
 *   i'th block is merged with adjacent ones if they are available, blocks that
 *   were merged from are removed. allocator->size is updated if blocks were
 *   removed.
 */
static void address_space_allocator_release_block(
    struct address_space_allocator *allocator,
    int i)
{
    struct address_block *blocks = allocator->blocks;
    int before = i - 1;
    int after = i + 1;
    int size = allocator->size;

    address_space_assert(i >= 0);
    address_space_assert(i < size);

    blocks[i].available = 1;

    if (before >= 0 && blocks[before].available) {
        if (after < size && blocks[after].available) {
            // merge (before, i, after) into before
            blocks[before].size += (blocks[i].size + blocks[after].size);

            size -= 2;
            memmove(&blocks[i], &blocks[i + 2],
                sizeof(struct address_block) * (size - i));
            allocator->size = size;
        } else {
            // merge (before, i) into before
            blocks[before].size += blocks[i].size;

            --size;
            memmove(&blocks[i], &blocks[i + 1],
                sizeof(struct address_block) * (size - i));
            allocator->size = size;
        }
    } else if (after < size && blocks[after].available) {
        // merge (i, after) into i
        blocks[i].size += blocks[after].size;

        --size;
        memmove(&blocks[after], &blocks[after + 1],
            sizeof(struct address_block) * (size - after));
        allocator->size = size;
    }

}

/* Takes a size to allocate an address block and returns an offset where this
 * block is allocated. This block will not be available for other callers unless
 * it is explicitly deallocated (see address_space_allocator_deallocate below).
 */
static uint64_t address_space_allocator_allocate(
    struct address_space_allocator *allocator,
    uint64_t size)
{
    int i = address_space_allocator_find_available_block(allocator->blocks,
                                                         allocator->size,
                                                         size);
    if (i < 0) {
        return ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET;
    } else {
        address_space_assert(i < allocator->size);

        struct address_block *block = &allocator->blocks[i];
        address_space_assert(block->size >= size);

        if (block->size > size) {
            block = address_space_allocator_split_block(allocator, i, size);
        }

        address_space_assert(block->size == size);
        block->available = 0;

        return block->offset;
    }
}

/* Takes an offset returned from address_space_allocator_allocate ealier
 * (see above) and marks this block as available for further allocation.
 */
static uint32_t address_space_allocator_deallocate(
    struct address_space_allocator *allocator,
    uint64_t offset)
{
    struct address_block *block = allocator->blocks;
    int size = allocator->size;
    int i;

    address_space_assert(size >= 1);

    for (i = 0; i < size; ++i, ++block) {
        if (block->offset == offset) {
            if (block->available) {
                return EINVAL;
            } else {
                address_space_allocator_release_block(allocator, i);
                return 0;
            }
        }
    }

    return EINVAL;
}

/* Creates a seed block. */
static void address_space_allocator_init(
    struct address_space_allocator *allocator,
    uint64_t size,
    int initial_capacity)
{
    address_space_assert(initial_capacity >= 1);

    allocator->blocks =
        (struct address_block*)
        malloc(sizeof(struct address_block) * initial_capacity);
    memset(allocator->blocks, 0, sizeof(struct address_block) * initial_capacity);
    address_space_assert(allocator->blocks);

    struct address_block *block = allocator->blocks;

    block->offset = 0;
    block->size = size;
    block->available = 1;

    allocator->size = 1;
    allocator->capacity = initial_capacity;
    allocator->total_bytes = size;
}

/* At this point there should be no used blocks and all available blocks must
 * have been merged into one block.
 */
static void address_space_allocator_destroy(
    struct address_space_allocator *allocator)
{
    address_space_assert(allocator->size == 1);
    address_space_assert(allocator->capacity >= allocator->size);
    address_space_assert(allocator->blocks[0].available);
    address_space_free(allocator->blocks);
}

/* Destroy function if we don't care what was previoulsy allocated.
 * have been merged into one block.
 */
static void address_space_allocator_destroy_nocleanup(
    struct address_space_allocator *allocator)
{
    address_space_free(allocator->blocks);
}

/* Resets the state of the allocator to the initial state without
 * performing any dynamic memory management. */
static void address_space_allocator_reset(
    struct address_space_allocator *allocator)
{
    address_space_assert(allocator->size >= 1);

    allocator->size = 1;

    struct address_block* block = allocator->blocks;
    block->offset = 0;
    block->size = allocator->total_bytes;
    block->available = 1;
}

typedef void (*address_block_iter_func_t)(void* context, struct address_block*);
typedef void (*address_space_allocator_iter_func_t)(void* context, struct address_space_allocator*);

static void address_space_allocator_run(
    struct address_space_allocator *allocator,
    void* context,
    address_space_allocator_iter_func_t allocator_func,
    address_block_iter_func_t block_func)
{
    struct address_block *block = 0;
    int size;
    int i;

    allocator_func(context, allocator);

    block = allocator->blocks;
    size = allocator->size;

    address_space_assert(size >= 1);

    for (i = 0; i < size; ++i, ++block) {
        block_func(context, block);
    }
}

#ifdef ADDRESS_SPACE_NAMESPACE
}
#endif

ANDROID_END_HEADER
