/*
 * Support for RAM backed by mmaped host memory.
 *
 * Copyright (c) 2015 Red Hat, Inc.
 *
 * Authors:
 *  Michael S. Tsirkin <mst@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * later.  See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/mmap-alloc.h"
#include "qemu/host-utils.h"

#define HUGETLBFS_MAGIC       0x958458f6

#ifdef CONFIG_LINUX
#include <sys/vfs.h>
#endif

#if defined(__linux__)
typedef int (*MallocHook_MunmapReplacement)(const void* ptr, size_t size,
                                            int* result);
typedef int (*MallocHook_MmapReplacement)(const void* start, size_t size,
                                          int protection, int flags, int fd,
                                          off_t offset, void** result);

extern int MallocHook_SetMunmapReplacement(MallocHook_MunmapReplacement hook);
extern int MallocHook_SetMmapReplacement(MallocHook_MmapReplacement hook);
#endif
#ifdef _WIN32
static uint64_t windowsGetFilePageSize() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    // Use dwAllocationGranularity
    // as that is what we need to align
    // the pointer to (64k on most systems)
    return (uint64_t)sysinfo.dwAllocationGranularity;
}
#endif

#ifdef __APPLE__
static uint64_t macOSGetFilePageSize() {
    return getpagesize();
}
#endif

size_t qemu_fd_getpagesize(int fd)
{
#ifdef CONFIG_LINUX
    struct statfs fs;
    int ret;

    if (fd != -1) {
        do {
            ret = fstatfs(fd, &fs);
        } while (ret != 0 && errno == EINTR);

        if (ret == 0 && fs.f_type == HUGETLBFS_MAGIC) {
            return fs.f_bsize;
        }
    }
#ifdef __sparc__
    /* SPARC Linux needs greater alignment than the pagesize */
    return QEMU_VMALLOC_ALIGN;
#endif
    return getpagesize();
#endif

#ifdef _WIN32
    return windowsGetFilePageSize();
#endif

#ifdef __APPLE__
    return macOSGetFilePageSize();
#endif

}

size_t qemu_mempath_getpagesize(const char *mem_path)
{
#ifdef CONFIG_LINUX
    struct statfs fs;
    int ret;

    do {
        ret = statfs(mem_path, &fs);
    } while (ret != 0 && errno == EINTR);

    if (ret != 0) {
        fprintf(stderr, "Couldn't statfs() memory path: %s\n",
                strerror(errno));
        exit(1);
    }

    if (fs.f_type == HUGETLBFS_MAGIC) {
        /* It's hugepage, return the huge page size */
        return fs.f_bsize;
    }
#ifdef __sparc__
    /* SPARC Linux needs greater alignment than the pagesize */
    return QEMU_VMALLOC_ALIGN;
#endif

    return getpagesize();
#endif

#ifdef _WIN32
    return windowsGetFilePageSize();
#endif

#ifdef __APPLE__
    return macOSGetFilePageSize();
#endif
}

#if defined(__linux__)
static void* s_protected_address_start = NULL;
static void* s_protected_address_end = NULL;

// we need this to work around a bug in tcmalloc that insists we have
// mmapreplacement. The following just says it will leave everything to
// the system mmap.
static int MmapReplacement(const void* start, size_t size, int protection,
                           int flags, int fd, off_t offset, void** result) {
    return false;
}

static int MunmapReplacement(const void* ptr, size_t size, int* result) {
    if (!s_protected_address_start || !s_protected_address_end) {
        return false;
    }
    if (ptr) {
        long* myptr = (long*)ptr;
        long* my_start = (long*)s_protected_address_start;
        long* my_end = (long*)s_protected_address_end;
        if (myptr < my_start || myptr >= my_end) {
            return false;
        } else {
            fprintf(stderr,
                    "WARNING: cannnot unmap ptr %p as it is in the protected "
                    "range from %p to %p\n",
                    ptr, s_protected_address_start, s_protected_address_end);
            *result = 0;
            return true;
        }
    }
    return false;
}
#endif

void *qemu_ram_mmap(int fd, size_t size, size_t align, bool shared)
{
#ifndef _WIN32
    /*
     * Note: this always allocates at least one extra page of virtual address
     * space, even if size is already aligned.
     */
    size_t total = size + align;
#if defined(__powerpc64__) && defined(__linux__)
    /* On ppc64 mappings in the same segment (aka slice) must share the same
     * page size. Since we will be re-allocating part of this segment
     * from the supplied fd, we should make sure to use the same page size, to
     * this end we mmap the supplied fd.  In this case, set MAP_NORESERVE to
     * avoid allocating backing store memory.
     * We do this unless we are using the system page size, in which case
     * anonymous memory is OK.
     */
    int anonfd = fd == -1 || qemu_fd_getpagesize(fd) == getpagesize() ? -1 : fd;
    int flags = anonfd == -1 ? MAP_ANONYMOUS : MAP_NORESERVE;
    void *ptr = mmap(0, total, PROT_NONE, flags | MAP_PRIVATE, anonfd, 0);
#else
    void *ptr = mmap(0, total, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#endif
    size_t offset;
    void *ptr1;

    if (ptr == MAP_FAILED) {
        return MAP_FAILED;
    }

#if defined(__linux__)
    // bug: 225541819
    // there are some un-intended munmap from gl threads that
    // punches hole in the guest address space, leading to kvm
    // error: Bad Address, this is to mitigate that problem
    if (size >= 0x40000000) {
        // bigger than 1G, protect it from un-intended munmap
        s_protected_address_start = ptr;
        s_protected_address_end = ptr + total;
        MallocHook_SetMmapReplacement(&MmapReplacement);
        MallocHook_SetMunmapReplacement(&MunmapReplacement);
    }
#endif
    assert(is_power_of_2(align));
    /* Always align to host page size */
    assert(align >= getpagesize());

    offset = QEMU_ALIGN_UP((uintptr_t)ptr, align) - (uintptr_t)ptr;
    ptr1 = mmap(ptr + offset, size, PROT_READ | PROT_WRITE,
                MAP_FIXED |
                (fd == -1 ? MAP_ANONYMOUS : 0) |
                (shared ? MAP_SHARED : MAP_PRIVATE),
                fd, 0);
    if (ptr1 == MAP_FAILED) {
        munmap(ptr, total);
        return MAP_FAILED;
    }

    if (offset > 0) {
        munmap(ptr, offset);
    }

    /*
     * Leave a single PROT_NONE page allocated after the RAM block, to serve as
     * a guard page guarding against potential buffer overflows.
     */
    total -= offset;
    if (total > size + getpagesize()) {
        munmap(ptr1 + size + getpagesize(), total - size - getpagesize());
    }

    return ptr1;
#else
    size_t total = size + align;

    HANDLE fileMapping =
        CreateFileMapping(
            (HANDLE)_get_osfhandle(fd),
            NULL, // security attribs
            PAGE_READWRITE,
            (uint32_t)(((uint64_t)(size + align)) >> 32),
            (uint32_t)(size + align),
            NULL);

    if (!fileMapping) {
        return MAP_FAILED;
    }

    void* ptr =
        MapViewOfFile(
            fileMapping,   // handle to map object
            shared ?  FILE_MAP_ALL_ACCESS : FILE_MAP_COPY, // read/write permission
            0, 0, 0);

    CloseHandle(fileMapping);

    if (!ptr) {
        return MAP_FAILED;
    }

    return ptr;
#endif
}

void qemu_ram_munmap(void *ptr, size_t size)
{
#ifndef _WIN32
    if (ptr) {
        /* Unmap both the RAM block and the guard page */
#if defined(__linux__)
        {
            long* myptr = (long*)ptr;
            if (myptr >= (long*)s_protected_address_start &&
                myptr < (long*)s_protected_address_end) {
              s_protected_address_start = NULL;
              s_protected_address_end = NULL;
            }
        }
#endif
        munmap(ptr, size + getpagesize());
    }
#else
    UnmapViewOfFile(ptr);
#endif
}
