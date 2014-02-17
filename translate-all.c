/*
 *  Host code generation
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "config.h"

#define NO_CPU_IO_DEFS
#include "cpu.h"
#include "exec/cputlb.h"
#include "exec/exec-all.h"
#include "disas/disas.h"
#include "tcg.h"
#include "qemu/timer.h"

#define SMC_BITMAP_USE_THRESHOLD 10

typedef struct PageDesc {
    /* list of TBs intersecting this ram page */
    TranslationBlock *first_tb;
    /* in order to optimize self modifying code, we count the number
       of lookups we do to a given page to use a bitmap */
    unsigned int code_write_count;
    uint8_t *code_bitmap;
#if defined(CONFIG_USER_ONLY)
    unsigned long flags;
#endif
} PageDesc;

#define L2_BITS 10
#if defined(CONFIG_USER_ONLY) && defined(TARGET_VIRT_ADDR_SPACE_BITS)
/* XXX: this is a temporary hack for alpha target.
 *      In the future, this is to be replaced by a multi-level table
 *      to actually be able to handle the complete 64 bits address space.
 */
#define L1_BITS (TARGET_VIRT_ADDR_SPACE_BITS - L2_BITS - TARGET_PAGE_BITS)
#else
#define L1_BITS (32 - L2_BITS - TARGET_PAGE_BITS)
#endif

#define L1_SIZE (1 << L1_BITS)
#define L2_SIZE (1 << L2_BITS)

uintptr_t qemu_real_host_page_size;
uintptr_t qemu_host_page_size;
uintptr_t qemu_host_page_mask;

/* XXX: for system emulation, it could just be an array */
static PageDesc *l1_map[L1_SIZE];
static PhysPageDesc **l1_phys_map;

/* code generation context */
TCGContext tcg_ctx;

#ifdef CONFIG_MEMCHECK
/*
 * Memchecker code in this module copies TB PC <-> Guest PC map to the TB
 * descriptor after guest code has been translated in cpu_gen_init routine.
 */
#include "memcheck/memcheck_api.h"

/* Array of (tb_pc, guest_pc) pairs, big enough for all translations. This
 * array is used to obtain guest PC address from a translated PC address.
 * tcg_gen_code_common will fill it up when memchecker is enabled. */
static void* gen_opc_tpc2gpc[OPC_BUF_SIZE * 2];
void** gen_opc_tpc2gpc_ptr = &gen_opc_tpc2gpc[0];
/* Number of (tb_pc, guest_pc) pairs stored in gen_opc_tpc2gpc array. */
unsigned int gen_opc_tpc2gpc_pairs;
#endif  // CONFIG_MEMCHECK

/* XXX: suppress that */
unsigned long code_gen_max_block_size(void)
{
    static unsigned long max;

    if (max == 0) {
        max = TCG_MAX_OP_SIZE;
#define DEF(name, iarg, oarg, carg, flags) DEF2((iarg) + (oarg) + (carg))
#define DEF2(copy_size) max = (copy_size > max) ? copy_size : max;
#include "tcg-opc.h"
#undef DEF
#undef DEF2
        max *= OPC_MAX_SIZE;
    }

    return max;
}

void cpu_gen_init(void)
{
    tcg_context_init(&tcg_ctx);
    tcg_set_frame(&tcg_ctx, TCG_AREG0, offsetof(CPUOldState, temp_buf),
                  CPU_TEMP_BUF_NLONGS * sizeof(long));
}

/* return non zero if the very first instruction is invalid so that
   the virtual CPU can trigger an exception.

   '*gen_code_size_ptr' contains the size of the generated code (host
   code).
*/
int cpu_gen_code(CPUArchState *env, TranslationBlock *tb, int *gen_code_size_ptr)
{
    TCGContext *s = &tcg_ctx;
    uint8_t *gen_code_buf;
    int gen_code_size;
#ifdef CONFIG_PROFILER
    int64_t ti;
#endif

#ifdef CONFIG_PROFILER
    s->tb_count1++; /* includes aborted translations because of
                       exceptions */
    ti = profile_getclock();
#endif
    tcg_func_start(s);

    gen_intermediate_code(env, tb);

    /* generate machine code */
    gen_code_buf = tb->tc_ptr;
    tb->tb_next_offset[0] = 0xffff;
    tb->tb_next_offset[1] = 0xffff;
    s->tb_next_offset = tb->tb_next_offset;
#ifdef USE_DIRECT_JUMP
    s->tb_jmp_offset = tb->tb_jmp_offset;
    s->tb_next = NULL;
    /* the following two entries are optional (only used for string ops) */
    /* XXX: not used ? */
    tb->tb_jmp_offset[2] = 0xffff;
    tb->tb_jmp_offset[3] = 0xffff;
#else
    s->tb_jmp_offset = NULL;
    s->tb_next = tb->tb_next;
#endif

#ifdef CONFIG_PROFILER
    s->tb_count++;
    s->interm_time += profile_getclock() - ti;
    s->code_time -= profile_getclock();
#endif
    gen_code_size = tcg_gen_code(s, gen_code_buf);
    *gen_code_size_ptr = gen_code_size;
#ifdef CONFIG_PROFILER
    s->code_time += profile_getclock();
    s->code_in_len += tb->size;
    s->code_out_len += gen_code_size;
#endif

#ifdef CONFIG_MEMCHECK
    /* Save translated PC -> guest PC map into TB. */
    if (memcheck_enabled && gen_opc_tpc2gpc_pairs && is_cpu_user(env)) {
        tb->tpc2gpc =
                g_malloc(gen_opc_tpc2gpc_pairs * 2 * sizeof(uintptr_t));
        if (tb->tpc2gpc != NULL) {
            memcpy(tb->tpc2gpc, gen_opc_tpc2gpc_ptr,
                   gen_opc_tpc2gpc_pairs * 2 * sizeof(uintptr_t));
            tb->tpc2gpc_pairs = gen_opc_tpc2gpc_pairs;
        }

    }
#endif  // CONFIG_MEMCHECK

#ifdef DEBUG_DISAS
    if (qemu_loglevel_mask(CPU_LOG_TB_OUT_ASM)) {
        qemu_log("OUT: [size=%d]\n", *gen_code_size_ptr);
        log_disas(tb->tc_ptr, *gen_code_size_ptr);
        qemu_log("\n");
        qemu_log_flush();
    }
#endif
    return 0;
}

/* The cpu state corresponding to 'searched_pc' is restored.
 */
bool cpu_restore_state(TranslationBlock *tb,
                       CPUArchState *env, uintptr_t searched_pc)
{
    TCGContext *s = &tcg_ctx;
    int j;
    uintptr_t tc_ptr;
#ifdef CONFIG_PROFILER
    int64_t ti;
#endif

#ifdef CONFIG_PROFILER
    ti = profile_getclock();
#endif
    tcg_func_start(s);

    gen_intermediate_code_pc(env, tb);

    if (use_icount) {
        /* Reset the cycle counter to the start of the block.  */
        env->icount_decr.u16.low += tb->icount;
        /* Clear the IO flag.  */
        env->can_do_io = 0;
    }

    /* find opc index corresponding to search_pc */
    tc_ptr = (uintptr_t)tb->tc_ptr;
    if (searched_pc < tc_ptr)
        return -1;

    s->tb_next_offset = tb->tb_next_offset;
#ifdef USE_DIRECT_JUMP
    s->tb_jmp_offset = tb->tb_jmp_offset;
    s->tb_next = NULL;
#else
    s->tb_jmp_offset = NULL;
    s->tb_next = tb->tb_next;
#endif
    j = tcg_gen_code_search_pc(s, (uint8_t *)tc_ptr, searched_pc - tc_ptr);
    if (j < 0)
        return false;
    /* now find start of instruction before */
    while (tcg_ctx.gen_opc_instr_start[j] == 0)
        j--;
    env->icount_decr.u16.low -= tcg_ctx.gen_opc_icount[j];

    restore_state_to_opc(env, tb, j);

#ifdef CONFIG_PROFILER
    s->restore_time += profile_getclock() - ti;
    s->restore_count++;
#endif
    return true;
}

#ifdef _WIN32
static inline void map_exec(void *addr, long size)
{
    DWORD old_protect;
    VirtualProtect(addr, size,
                   PAGE_EXECUTE_READWRITE, &old_protect);

}
#else
static inline void map_exec(void *addr, long size)
{
    unsigned long start, end, page_size;

    page_size = getpagesize();
    start = (unsigned long)addr;
    start &= ~(page_size - 1);

    end = (unsigned long)addr + size;
    end += page_size - 1;
    end &= ~(page_size - 1);

    mprotect((void *)start, end - start,
             PROT_READ | PROT_WRITE | PROT_EXEC);
}
#endif

static void page_init(void)
{
    /* NOTE: we can always suppose that qemu_host_page_size >=
       TARGET_PAGE_SIZE */
#ifdef _WIN32
    {
        SYSTEM_INFO system_info;

        GetSystemInfo(&system_info);
        qemu_real_host_page_size = system_info.dwPageSize;
    }
#else
    qemu_real_host_page_size = getpagesize();
#endif
    if (qemu_host_page_size == 0)
        qemu_host_page_size = qemu_real_host_page_size;
    if (qemu_host_page_size < TARGET_PAGE_SIZE)
        qemu_host_page_size = TARGET_PAGE_SIZE;
    qemu_host_page_mask = ~(qemu_host_page_size - 1);
    l1_phys_map = qemu_vmalloc(L1_SIZE * sizeof(void *));
    memset(l1_phys_map, 0, L1_SIZE * sizeof(void *));

#if !defined(_WIN32) && defined(CONFIG_USER_ONLY)
    {
        long long startaddr, endaddr;
        FILE *f;
        int n;

        mmap_lock();
        last_brk = (unsigned long)sbrk(0);
        f = fopen("/proc/self/maps", "r");
        if (f) {
            do {
                n = fscanf (f, "%llx-%llx %*[^\n]\n", &startaddr, &endaddr);
                if (n == 2) {
                    startaddr = MIN(startaddr,
                                    (1ULL << TARGET_PHYS_ADDR_SPACE_BITS) - 1);
                    endaddr = MIN(endaddr,
                                    (1ULL << TARGET_PHYS_ADDR_SPACE_BITS) - 1);
                    page_set_flags(startaddr & TARGET_PAGE_MASK,
                                   TARGET_PAGE_ALIGN(endaddr),
                                   PAGE_RESERVED);
                }
            } while (!feof(f));
            fclose(f);
        }
        mmap_unlock();
    }
#endif
}

static inline PageDesc **page_l1_map(target_ulong index)
{
#if TARGET_LONG_BITS > 32
    /* Host memory outside guest VM.  For 32-bit targets we have already
       excluded high addresses.  */
    if (index > ((target_ulong)L2_SIZE * L1_SIZE))
        return NULL;
#endif
    return &l1_map[index >> L2_BITS];
}

static inline PageDesc *page_find_alloc(target_ulong index)
{
    PageDesc **lp, *p;
    lp = page_l1_map(index);
    if (!lp)
        return NULL;

    p = *lp;
    if (!p) {
        /* allocate if not found */
#if defined(CONFIG_USER_ONLY)
        size_t len = sizeof(PageDesc) * L2_SIZE;
        /* Don't use g_malloc because it may recurse.  */
        p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        *lp = p;
        if (h2g_valid(p)) {
            unsigned long addr = h2g(p);
            page_set_flags(addr & TARGET_PAGE_MASK,
                           TARGET_PAGE_ALIGN(addr + len),
                           PAGE_RESERVED);
        }
#else
        p = g_malloc0(sizeof(PageDesc) * L2_SIZE);
        *lp = p;
#endif
    }
    return p + (index & (L2_SIZE - 1));
}

static inline PageDesc *page_find(target_ulong index)
{
    PageDesc **lp, *p;
    lp = page_l1_map(index);
    if (!lp)
        return NULL;

    p = *lp;
    if (!p) {
        return NULL;
    }
    return p + (index & (L2_SIZE - 1));
}

PhysPageDesc *phys_page_find_alloc(hwaddr index, int alloc)
{
    void **lp, **p;
    PhysPageDesc *pd;

    p = (void **)l1_phys_map;
#if TARGET_PHYS_ADDR_SPACE_BITS > 32

#if TARGET_PHYS_ADDR_SPACE_BITS > (32 + L1_BITS)
#error unsupported TARGET_PHYS_ADDR_SPACE_BITS
#endif
    lp = p + ((index >> (L1_BITS + L2_BITS)) & (L1_SIZE - 1));
    p = *lp;
    if (!p) {
        /* allocate if not found */
        if (!alloc)
            return NULL;
        p = qemu_vmalloc(sizeof(void *) * L1_SIZE);
        memset(p, 0, sizeof(void *) * L1_SIZE);
        *lp = p;
    }
#endif
    lp = p + ((index >> L2_BITS) & (L1_SIZE - 1));
    pd = *lp;
    if (!pd) {
        int i;
        /* allocate if not found */
        if (!alloc)
            return NULL;
        pd = qemu_vmalloc(sizeof(PhysPageDesc) * L2_SIZE);
        *lp = pd;
        for (i = 0; i < L2_SIZE; i++) {
          pd[i].phys_offset = IO_MEM_UNASSIGNED;
          pd[i].region_offset = (index + i) << TARGET_PAGE_BITS;
        }
    }
    return ((PhysPageDesc *)pd) + (index & (L2_SIZE - 1));
}

PhysPageDesc *phys_page_find(hwaddr index)
{
    return phys_page_find_alloc(index, 0);
}

#if !defined(CONFIG_USER_ONLY)
#define mmap_lock() do { } while(0)
#define mmap_unlock() do { } while(0)
#endif

#if defined(CONFIG_USER_ONLY)
/* Currently it is not recommended to allocate big chunks of data in
   user mode. It will change when a dedicated libc will be used.  */
/* ??? 64-bit hosts ought to have no problem mmaping data outside the
   region in which the guest needs to run.  Revisit this.  */
#define USE_STATIC_CODE_GEN_BUFFER
#endif

/* ??? Should configure for this, not list operating systems here.  */
#if (defined(__linux__) \
    || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
    || defined(__DragonFly__) || defined(__OpenBSD__) \
    || defined(__NetBSD__))
# define USE_MMAP
#endif

/* Minimum size of the code gen buffer.  This number is randomly chosen,
   but not so small that we can't have a fair number of TB's live.  */
#define MIN_CODE_GEN_BUFFER_SIZE     (1024u * 1024)

/* Maximum size of the code gen buffer we'd like to use.  Unless otherwise
   indicated, this is constrained by the range of direct branches on the
   host cpu, as used by the TCG implementation of goto_tb.  */
#if defined(__x86_64__)
# define MAX_CODE_GEN_BUFFER_SIZE  (2ul * 1024 * 1024 * 1024)
#elif defined(__sparc__)
# define MAX_CODE_GEN_BUFFER_SIZE  (2ul * 1024 * 1024 * 1024)
#elif defined(__aarch64__)
# define MAX_CODE_GEN_BUFFER_SIZE  (128ul * 1024 * 1024)
#elif defined(__arm__)
# define MAX_CODE_GEN_BUFFER_SIZE  (16u * 1024 * 1024)
#elif defined(__s390x__)
  /* We have a +- 4GB range on the branches; leave some slop.  */
# define MAX_CODE_GEN_BUFFER_SIZE  (3ul * 1024 * 1024 * 1024)
#else
# define MAX_CODE_GEN_BUFFER_SIZE  ((size_t)-1)
#endif

#define DEFAULT_CODE_GEN_BUFFER_SIZE_1 (32u * 1024 * 1024)

#define DEFAULT_CODE_GEN_BUFFER_SIZE \
  (DEFAULT_CODE_GEN_BUFFER_SIZE_1 < MAX_CODE_GEN_BUFFER_SIZE \
   ? DEFAULT_CODE_GEN_BUFFER_SIZE_1 : MAX_CODE_GEN_BUFFER_SIZE)

static inline size_t size_code_gen_buffer(size_t tb_size)
{
    /* Size the buffer.  */
    if (tb_size == 0) {
#ifdef USE_STATIC_CODE_GEN_BUFFER
        tb_size = DEFAULT_CODE_GEN_BUFFER_SIZE;
#else
        /* ??? Needs adjustments.  */
        /* ??? If we relax the requirement that CONFIG_USER_ONLY use the
           static buffer, we could size this on RESERVED_VA, on the text
           segment size of the executable, or continue to use the default.  */
        tb_size = (unsigned long)(ram_size / 4);
#endif
    }
    if (tb_size < MIN_CODE_GEN_BUFFER_SIZE) {
        tb_size = MIN_CODE_GEN_BUFFER_SIZE;
    }
    if (tb_size > MAX_CODE_GEN_BUFFER_SIZE) {
        tb_size = MAX_CODE_GEN_BUFFER_SIZE;
    }
    tcg_ctx.code_gen_buffer_size = tb_size;
    return tb_size;
}

#ifdef USE_STATIC_CODE_GEN_BUFFER
static uint8_t static_code_gen_buffer[DEFAULT_CODE_GEN_BUFFER_SIZE];
    __attribute__((aligned(CODE_GEN_ALIGN)));

static inline void *alloc_code_gen_buffer(void)
{
    map_exec(static_code_gen_buffer, tcg_ctx.code_gen_buffer_size);
    return static_code_gen_buffer;
}
#elif defined(USE_MMAP)
static inline void *alloc_code_gen_buffer(void)
{
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    uintptr_t start = 0;
    void *buf;

    /* Constrain the position of the buffer based on the host cpu.
       Note that these addresses are chosen in concert with the
       addresses assigned in the relevant linker script file.  */
# if defined(__PIE__) || defined(__PIC__)
    /* Don't bother setting a preferred location if we're building
       a position-independent executable.  We're more likely to get
       an address near the main executable if we let the kernel
       choose the address.  */
# elif defined(__x86_64__) && defined(MAP_32BIT)
    /* Force the memory down into low memory with the executable.
       Leave the choice of exact location with the kernel.  */
    flags |= MAP_32BIT;
    /* Cannot expect to map more than 800MB in low memory.  */
    if (tcg_ctx.code_gen_buffer_size > 800u * 1024 * 1024) {
        tcg_ctx.code_gen_buffer_size = 800u * 1024 * 1024;
    }
# elif defined(__sparc__)
    start = 0x40000000ul;
# elif defined(__s390x__)
    start = 0x90000000ul;
# endif

    buf = mmap((void *)start, tcg_ctx.code_gen_buffer_size,
               PROT_WRITE | PROT_READ | PROT_EXEC, flags, -1, 0);
    return buf == MAP_FAILED ? NULL : buf;
}
#else
static inline void *alloc_code_gen_buffer(void)
{
    void *buf = g_malloc(tcg_ctx.code_gen_buffer_size);

    if (buf) {
        map_exec(buf, tcg_ctx.code_gen_buffer_size);
    }
    return buf;
}
#endif /* USE_STATIC_CODE_GEN_BUFFER, USE_MMAP */

static void code_gen_alloc(unsigned long tb_size)
{
    tcg_ctx.code_gen_buffer_size = size_code_gen_buffer(tb_size);
    tcg_ctx.code_gen_buffer = alloc_code_gen_buffer();
    if (tcg_ctx.code_gen_buffer == NULL) {
        fprintf(stderr, "Could not allocate dynamic translator buffer\n");
        exit(1);
    }

    qemu_madvise(tcg_ctx.code_gen_buffer, tcg_ctx.code_gen_buffer_size,
            QEMU_MADV_HUGEPAGE);

    /* Steal room for the prologue at the end of the buffer.  This ensures
       (via the MAX_CODE_GEN_BUFFER_SIZE limits above) that direct branches
       from TB's to the prologue are going to be in range.  It also means
       that we don't need to mark (additional) portions of the data segment
       as executable.  */
    tcg_ctx.code_gen_prologue = tcg_ctx.code_gen_buffer +
            tcg_ctx.code_gen_buffer_size - 1024;
    tcg_ctx.code_gen_buffer_size -= 1024;

    tcg_ctx.code_gen_buffer_max_size = tcg_ctx.code_gen_buffer_size -
        (TCG_MAX_OP_SIZE * OPC_BUF_SIZE);
    tcg_ctx.code_gen_max_blocks = tcg_ctx.code_gen_buffer_size /
            CODE_GEN_AVG_BLOCK_SIZE;
    tcg_ctx.tb_ctx.tbs =
            g_malloc(tcg_ctx.code_gen_max_blocks * sizeof(TranslationBlock));
}

/* Must be called before using the QEMU cpus. 'tb_size' is the size
   (in bytes) allocated to the translation buffer. Zero means default
   size. */
void tcg_exec_init(unsigned long tb_size)
{
    cpu_gen_init();
    code_gen_alloc(tb_size);
    tcg_ctx.code_gen_ptr = tcg_ctx.code_gen_buffer;
    page_init();
#if !defined(CONFIG_USER_ONLY) || !defined(CONFIG_USE_GUEST_BASE)
    /* There's no guest base to take into account, so go ahead and
       initialize the prologue now.  */
    tcg_prologue_init(&tcg_ctx);
#endif
}

bool tcg_enabled(void)
{
    return tcg_ctx.code_gen_buffer != NULL;
}

/* add the tb in the target page and protect it if necessary */
void tb_free(TranslationBlock *tb)
{
    /* In practice this is mostly used for single use temporary TB
       Ignore the hard cases and just back up if this TB happens to
       be the last one generated.  */
    if (tcg_ctx.tb_ctx.nb_tbs > 0 && tb == &tcg_ctx.tb_ctx.tbs[tcg_ctx.tb_ctx.nb_tbs - 1]) {
        tcg_ctx.code_gen_ptr = tb->tc_ptr;
        tcg_ctx.tb_ctx.nb_tbs--;
    }
}

static inline void invalidate_page_bitmap(PageDesc *p)
{
    if (p->code_bitmap) {
        g_free(p->code_bitmap);
        p->code_bitmap = NULL;
    }
    p->code_write_count = 0;
}

static inline void tb_alloc_page(TranslationBlock *tb,
                                 unsigned int n, target_ulong page_addr)
{
    PageDesc *p;
    TranslationBlock *last_first_tb;

    tb->page_addr[n] = page_addr;
    p = page_find_alloc(page_addr >> TARGET_PAGE_BITS);
    tb->page_next[n] = p->first_tb;
    last_first_tb = p->first_tb;
    p->first_tb = (TranslationBlock *)((long)tb | n);
    invalidate_page_bitmap(p);

#if defined(TARGET_HAS_SMC) || 1

#if defined(CONFIG_USER_ONLY)
    if (p->flags & PAGE_WRITE) {
        target_ulong addr;
        PageDesc *p2;
        int prot;

        /* force the host page as non writable (writes will have a
           page fault + mprotect overhead) */
        page_addr &= qemu_host_page_mask;
        prot = 0;
        for(addr = page_addr; addr < page_addr + qemu_host_page_size;
            addr += TARGET_PAGE_SIZE) {

            p2 = page_find (addr >> TARGET_PAGE_BITS);
            if (!p2)
                continue;
            prot |= p2->flags;
            p2->flags &= ~PAGE_WRITE;
            page_get_flags(addr);
          }
        mprotect(g2h(page_addr), qemu_host_page_size,
                 (prot & PAGE_BITS) & ~PAGE_WRITE);
#ifdef DEBUG_TB_INVALIDATE
        printf("protecting code page: 0x" TARGET_FMT_lx "\n",
               page_addr);
#endif
    }
#else
    /* if some code is already present, then the pages are already
       protected. So we handle the case where only the first TB is
       allocated in a physical page */
    if (!last_first_tb) {
        tlb_protect_code(page_addr);
    }
#endif

#endif /* TARGET_HAS_SMC */
}

/* Allocate a new translation block. Flush the translation buffer if
   too many translation blocks or too much generated code. */
TranslationBlock *tb_alloc(target_ulong pc)
{
    TranslationBlock *tb;

    if (tcg_ctx.tb_ctx.nb_tbs >= tcg_ctx.code_gen_max_blocks ||
        (tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer) >= tcg_ctx.code_gen_buffer_max_size)
        return NULL;
    tb = &tcg_ctx.tb_ctx.tbs[tcg_ctx.tb_ctx.nb_tbs++];
    tb->pc = pc;
    tb->cflags = 0;
#ifdef CONFIG_MEMCHECK
    tb->tpc2gpc = NULL;
    tb->tpc2gpc_pairs = 0;
#endif  // CONFIG_MEMCHECK
    return tb;
}

/* set to NULL all the 'first_tb' fields in all PageDescs */
static void page_flush_tb(void)
{
    int i, j;
    PageDesc *p;

    for(i = 0; i < L1_SIZE; i++) {
        p = l1_map[i];
        if (p) {
            for(j = 0; j < L2_SIZE; j++) {
                p->first_tb = NULL;
                invalidate_page_bitmap(p);
                p++;
            }
        }
    }
}

/* flush all the translation blocks */
/* XXX: tb_flush is currently not thread safe */
void tb_flush(CPUArchState *env1)
{
    CPUArchState *env;
#if defined(DEBUG_FLUSH)
    printf("qemu: flush code_size=%ld nb_tbs=%d avg_tb_size=%ld\n",
           (unsigned long)(tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer),
           nb_tbs, nb_tbs > 0 ?
           ((unsigned long)(tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer)) / nb_tbs : 0);
#endif
    if ((unsigned long)(tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer) > tcg_ctx.code_gen_buffer_size)
        cpu_abort(env1, "Internal error: code buffer overflow\n");

    tcg_ctx.tb_ctx.nb_tbs = 0;

    for(env = first_cpu; env != NULL; env = env->next_cpu) {
#ifdef CONFIG_MEMCHECK
        int tb_to_clean;
        for (tb_to_clean = 0; tb_to_clean < TB_JMP_CACHE_SIZE; tb_to_clean++) {
            if (env->tb_jmp_cache[tb_to_clean] != NULL &&
                env->tb_jmp_cache[tb_to_clean]->tpc2gpc != NULL) {
                g_free(env->tb_jmp_cache[tb_to_clean]->tpc2gpc);
                env->tb_jmp_cache[tb_to_clean]->tpc2gpc = NULL;
                env->tb_jmp_cache[tb_to_clean]->tpc2gpc_pairs = 0;
            }
        }
#endif  // CONFIG_MEMCHECK
        memset (env->tb_jmp_cache, 0, TB_JMP_CACHE_SIZE * sizeof (void *));
    }

    memset (tcg_ctx.tb_ctx.tb_phys_hash, 0, CODE_GEN_PHYS_HASH_SIZE * sizeof (void *));
    page_flush_tb();

    tcg_ctx.code_gen_ptr = tcg_ctx.code_gen_buffer;
    /* XXX: flush processor icache at this point if cache flush is
       expensive */
    tcg_ctx.tb_ctx.tb_flush_count++;
}

#ifdef DEBUG_TB_CHECK

static void tb_invalidate_check(target_ulong address)
{
    TranslationBlock *tb;
    int i;
    address &= TARGET_PAGE_MASK;
    for(i = 0;i < CODE_GEN_PHYS_HASH_SIZE; i++) {
        for(tb = tb_phys_hash[i]; tb != NULL; tb = tb->phys_hash_next) {
            if (!(address + TARGET_PAGE_SIZE <= tb->pc ||
                  address >= tb->pc + tb->size)) {
                printf("ERROR invalidate: address=" TARGET_FMT_lx
                       " PC=%08lx size=%04x\n",
                       address, (long)tb->pc, tb->size);
            }
        }
    }
}

/* verify that all the pages have correct rights for code */
static void tb_page_check(void)
{
    TranslationBlock *tb;
    int i, flags1, flags2;

    for(i = 0;i < CODE_GEN_PHYS_HASH_SIZE; i++) {
        for(tb = tb_phys_hash[i]; tb != NULL; tb = tb->phys_hash_next) {
            flags1 = page_get_flags(tb->pc);
            flags2 = page_get_flags(tb->pc + tb->size - 1);
            if ((flags1 & PAGE_WRITE) || (flags2 & PAGE_WRITE)) {
                printf("ERROR page flags: PC=%08lx size=%04x f1=%x f2=%x\n",
                       (long)tb->pc, tb->size, flags1, flags2);
            }
        }
    }
}

#endif

/* invalidate one TB */
static inline void tb_remove(TranslationBlock **ptb, TranslationBlock *tb,
                             int next_offset)
{
    TranslationBlock *tb1;
    for(;;) {
        tb1 = *ptb;
        if (tb1 == tb) {
            *ptb = *(TranslationBlock **)((char *)tb1 + next_offset);
            break;
        }
        ptb = (TranslationBlock **)((char *)tb1 + next_offset);
    }
}

static inline void tb_page_remove(TranslationBlock **ptb, TranslationBlock *tb)
{
    TranslationBlock *tb1;
    unsigned int n1;

    for(;;) {
        tb1 = *ptb;
        n1 = (long)tb1 & 3;
        tb1 = (TranslationBlock *)((long)tb1 & ~3);
        if (tb1 == tb) {
            *ptb = tb1->page_next[n1];
            break;
        }
        ptb = &tb1->page_next[n1];
    }
}

static inline void tb_jmp_remove(TranslationBlock *tb, int n)
{
    TranslationBlock *tb1, **ptb;
    unsigned int n1;

    ptb = &tb->jmp_next[n];
    tb1 = *ptb;
    if (tb1) {
        /* find tb(n) in circular list */
        for(;;) {
            tb1 = *ptb;
            n1 = (long)tb1 & 3;
            tb1 = (TranslationBlock *)((long)tb1 & ~3);
            if (n1 == n && tb1 == tb)
                break;
            if (n1 == 2) {
                ptb = &tb1->jmp_first;
            } else {
                ptb = &tb1->jmp_next[n1];
            }
        }
        /* now we can suppress tb(n) from the list */
        *ptb = tb->jmp_next[n];

        tb->jmp_next[n] = NULL;
    }
}

/* reset the jump entry 'n' of a TB so that it is not chained to
   another TB */
static inline void tb_reset_jump(TranslationBlock *tb, int n)
{
    tb_set_jmp_target(tb, n, (unsigned long)(tb->tc_ptr + tb->tb_next_offset[n]));
}

void tb_phys_invalidate(TranslationBlock *tb, tb_page_addr_t page_addr)
{
    CPUArchState *env;
    PageDesc *p;
    unsigned int h, n1;
    hwaddr phys_pc;
    TranslationBlock *tb1, *tb2;

    /* remove the TB from the hash list */
    phys_pc = tb->page_addr[0] + (tb->pc & ~TARGET_PAGE_MASK);
    h = tb_phys_hash_func(phys_pc);
    tb_remove(&tcg_ctx.tb_ctx.tb_phys_hash[h], tb,
              offsetof(TranslationBlock, phys_hash_next));

    /* remove the TB from the page list */
    if (tb->page_addr[0] != page_addr) {
        p = page_find(tb->page_addr[0] >> TARGET_PAGE_BITS);
        tb_page_remove(&p->first_tb, tb);
        invalidate_page_bitmap(p);
    }
    if (tb->page_addr[1] != -1 && tb->page_addr[1] != page_addr) {
        p = page_find(tb->page_addr[1] >> TARGET_PAGE_BITS);
        tb_page_remove(&p->first_tb, tb);
        invalidate_page_bitmap(p);
    }

    tcg_ctx.tb_ctx.tb_invalidated_flag = 1;

    /* remove the TB from the hash list */
    h = tb_jmp_cache_hash_func(tb->pc);
    for(env = first_cpu; env != NULL; env = env->next_cpu) {
        if (env->tb_jmp_cache[h] == tb)
            env->tb_jmp_cache[h] = NULL;
    }

    /* suppress this TB from the two jump lists */
    tb_jmp_remove(tb, 0);
    tb_jmp_remove(tb, 1);

    /* suppress any remaining jumps to this TB */
    tb1 = tb->jmp_first;
    for(;;) {
        n1 = (long)tb1 & 3;
        if (n1 == 2)
            break;
        tb1 = (TranslationBlock *)((long)tb1 & ~3);
        tb2 = tb1->jmp_next[n1];
        tb_reset_jump(tb1, n1);
        tb1->jmp_next[n1] = NULL;
        tb1 = tb2;
    }
    tb->jmp_first = (TranslationBlock *)((long)tb | 2); /* fail safe */

#ifdef CONFIG_MEMCHECK
    if (tb->tpc2gpc != NULL) {
        g_free(tb->tpc2gpc);
        tb->tpc2gpc = NULL;
        tb->tpc2gpc_pairs = 0;
    }
#endif  // CONFIG_MEMCHECK

    tcg_ctx.tb_ctx.tb_phys_invalidate_count++;
}

static inline void set_bits(uint8_t *tab, int start, int len)
{
    int end, mask, end1;

    end = start + len;
    tab += start >> 3;
    mask = 0xff << (start & 7);
    if ((start & ~7) == (end & ~7)) {
        if (start < end) {
            mask &= ~(0xff << (end & 7));
            *tab |= mask;
        }
    } else {
        *tab++ |= mask;
        start = (start + 8) & ~7;
        end1 = end & ~7;
        while (start < end1) {
            *tab++ = 0xff;
            start += 8;
        }
        if (start < end) {
            mask = ~(0xff << (end & 7));
            *tab |= mask;
        }
    }
}

static void build_page_bitmap(PageDesc *p)
{
    int n, tb_start, tb_end;
    TranslationBlock *tb;

    p->code_bitmap = g_malloc0(TARGET_PAGE_SIZE / 8);

    tb = p->first_tb;
    while (tb != NULL) {
        n = (long)tb & 3;
        tb = (TranslationBlock *)((long)tb & ~3);
        /* NOTE: this is subtle as a TB may span two physical pages */
        if (n == 0) {
            /* NOTE: tb_end may be after the end of the page, but
               it is not a problem */
            tb_start = tb->pc & ~TARGET_PAGE_MASK;
            tb_end = tb_start + tb->size;
            if (tb_end > TARGET_PAGE_SIZE)
                tb_end = TARGET_PAGE_SIZE;
        } else {
            tb_start = 0;
            tb_end = ((tb->pc + tb->size) & ~TARGET_PAGE_MASK);
        }
        set_bits(p->code_bitmap, tb_start, tb_end - tb_start);
        tb = tb->page_next[n];
    }
}

TranslationBlock *tb_gen_code(CPUArchState *env,
                              target_ulong pc, target_ulong cs_base,
                              int flags, int cflags)
{
    TranslationBlock *tb;
    uint8_t *tc_ptr;
    target_ulong phys_pc, phys_page2, virt_page2;
    int code_gen_size;

    phys_pc = get_page_addr_code(env, pc);
    tb = tb_alloc(pc);
    if (!tb) {
        /* flush must be done */
        tb_flush(env);
        /* cannot fail at this point */
        tb = tb_alloc(pc);
        /* Don't forget to invalidate previous TB info.  */
        tcg_ctx.tb_ctx.tb_invalidated_flag = 1;
    }
    tc_ptr = tcg_ctx.code_gen_ptr;
    tb->tc_ptr = tc_ptr;
    tb->cs_base = cs_base;
    tb->flags = flags;
    tb->cflags = cflags;
    cpu_gen_code(env, tb, &code_gen_size);
    tcg_ctx.code_gen_ptr = (void *)(((unsigned long)tcg_ctx.code_gen_ptr + code_gen_size + CODE_GEN_ALIGN - 1) & ~(CODE_GEN_ALIGN - 1));

    /* check next page if needed */
    virt_page2 = (pc + tb->size - 1) & TARGET_PAGE_MASK;
    phys_page2 = -1;
    if ((pc & TARGET_PAGE_MASK) != virt_page2) {
        phys_page2 = get_page_addr_code(env, virt_page2);
    }
    tb_link_phys(tb, phys_pc, phys_page2);
    return tb;
}

/* invalidate all TBs which intersect with the target physical page
   starting in range [start;end[. NOTE: start and end must refer to
   the same physical page. 'is_cpu_write_access' should be true if called
   from a real cpu write access: the virtual CPU will exit the current
   TB if code is modified inside this TB. */
void tb_invalidate_phys_page_range(hwaddr start, hwaddr end,
                                   int is_cpu_write_access)
{
    TranslationBlock *tb, *tb_next, *saved_tb;
    CPUArchState *env = cpu_single_env;
    target_ulong tb_start, tb_end;
    PageDesc *p;
    int n;
#ifdef TARGET_HAS_PRECISE_SMC
    int current_tb_not_found = is_cpu_write_access;
    TranslationBlock *current_tb = NULL;
    int current_tb_modified = 0;
    target_ulong current_pc = 0;
    target_ulong current_cs_base = 0;
    int current_flags = 0;
#endif /* TARGET_HAS_PRECISE_SMC */

    p = page_find(start >> TARGET_PAGE_BITS);
    if (!p)
        return;
    if (!p->code_bitmap &&
        ++p->code_write_count >= SMC_BITMAP_USE_THRESHOLD &&
        is_cpu_write_access) {
        /* build code bitmap */
        build_page_bitmap(p);
    }

    /* we remove all the TBs in the range [start, end[ */
    /* XXX: see if in some cases it could be faster to invalidate all the code */
    tb = p->first_tb;
    while (tb != NULL) {
        n = (long)tb & 3;
        tb = (TranslationBlock *)((long)tb & ~3);
        tb_next = tb->page_next[n];
        /* NOTE: this is subtle as a TB may span two physical pages */
        if (n == 0) {
            /* NOTE: tb_end may be after the end of the page, but
               it is not a problem */
            tb_start = tb->page_addr[0] + (tb->pc & ~TARGET_PAGE_MASK);
            tb_end = tb_start + tb->size;
        } else {
            tb_start = tb->page_addr[1];
            tb_end = tb_start + ((tb->pc + tb->size) & ~TARGET_PAGE_MASK);
        }
        if (!(tb_end <= start || tb_start >= end)) {
#ifdef TARGET_HAS_PRECISE_SMC
            if (current_tb_not_found) {
                current_tb_not_found = 0;
                current_tb = NULL;
                if (env->mem_io_pc) {
                    /* now we have a real cpu fault */
                    current_tb = tb_find_pc(env->mem_io_pc);
                }
            }
            if (current_tb == tb &&
                (current_tb->cflags & CF_COUNT_MASK) != 1) {
                /* If we are modifying the current TB, we must stop
                its execution. We could be more precise by checking
                that the modification is after the current PC, but it
                would require a specialized function to partially
                restore the CPU state */

                current_tb_modified = 1;
                cpu_restore_state(current_tb, env,  env->mem_io_pc);
                cpu_get_tb_cpu_state(env, &current_pc, &current_cs_base,
                                     &current_flags);
            }
#endif /* TARGET_HAS_PRECISE_SMC */
            /* we need to do that to handle the case where a signal
               occurs while doing tb_phys_invalidate() */
            saved_tb = NULL;
            if (env) {
                saved_tb = env->current_tb;
                env->current_tb = NULL;
            }
            tb_phys_invalidate(tb, -1);
            if (env) {
                env->current_tb = saved_tb;
                if (env->interrupt_request && env->current_tb)
                    cpu_interrupt(env, env->interrupt_request);
            }
        }
        tb = tb_next;
    }
#if !defined(CONFIG_USER_ONLY)
    /* if no code remaining, no need to continue to use slow writes */
    if (!p->first_tb) {
        invalidate_page_bitmap(p);
        if (is_cpu_write_access) {
            tlb_unprotect_code_phys(env, start, env->mem_io_vaddr);
        }
    }
#endif
#ifdef TARGET_HAS_PRECISE_SMC
    if (current_tb_modified) {
        /* we generate a block containing just the instruction
           modifying the memory. It will ensure that it cannot modify
           itself */
        env->current_tb = NULL;
        tb_gen_code(env, current_pc, current_cs_base, current_flags, 1);
        cpu_resume_from_signal(env, NULL);
    }
#endif
}

/* len must be <= 8 and start must be a multiple of len */
static inline void tb_invalidate_phys_page_fast(hwaddr start, int len)
{
    PageDesc *p;
    int offset, b;
#if 0
    if (1) {
        qemu_log("modifying code at 0x%x size=%d EIP=%x PC=%08x\n",
                  cpu_single_env->mem_io_vaddr, len,
                  cpu_single_env->eip,
                  cpu_single_env->eip + (long)cpu_single_env->segs[R_CS].base);
    }
#endif
    p = page_find(start >> TARGET_PAGE_BITS);
    if (!p)
        return;
    if (p->code_bitmap) {
        offset = start & ~TARGET_PAGE_MASK;
        b = p->code_bitmap[offset >> 3] >> (offset & 7);
        if (b & ((1 << len) - 1))
            goto do_invalidate;
    } else {
    do_invalidate:
        tb_invalidate_phys_page_range(start, start + len, 1);
    }
}

void tb_invalidate_phys_page_fast0(hwaddr start, int len) {
    tb_invalidate_phys_page_fast(start, len);
}

#if !defined(CONFIG_SOFTMMU)
static void tb_invalidate_phys_page(hwaddr addr,
                                    unsigned long pc, void *puc)
{
    TranslationBlock *tb;
    PageDesc *p;
    int n;
#ifdef TARGET_HAS_PRECISE_SMC
    TranslationBlock *current_tb = NULL;
    CPUArchState *env = cpu_single_env;
    int current_tb_modified = 0;
    target_ulong current_pc = 0;
    target_ulong current_cs_base = 0;
    int current_flags = 0;
#endif

    addr &= TARGET_PAGE_MASK;
    p = page_find(addr >> TARGET_PAGE_BITS);
    if (!p)
        return;
    tb = p->first_tb;
#ifdef TARGET_HAS_PRECISE_SMC
    if (tb && pc != 0) {
        current_tb = tb_find_pc(pc);
    }
#endif
    while (tb != NULL) {
        n = (long)tb & 3;
        tb = (TranslationBlock *)((long)tb & ~3);
#ifdef TARGET_HAS_PRECISE_SMC
        if (current_tb == tb &&
            (current_tb->cflags & CF_COUNT_MASK) != 1) {
                /* If we are modifying the current TB, we must stop
                   its execution. We could be more precise by checking
                   that the modification is after the current PC, but it
                   would require a specialized function to partially
                   restore the CPU state */

            current_tb_modified = 1;
            cpu_restore_state(current_tb, env, pc);
            cpu_get_tb_cpu_state(env, &current_pc, &current_cs_base,
                                 &current_flags);
        }
#endif /* TARGET_HAS_PRECISE_SMC */
        tb_phys_invalidate(tb, addr);
        tb = tb->page_next[n];
    }
    p->first_tb = NULL;
#ifdef TARGET_HAS_PRECISE_SMC
    if (current_tb_modified) {
        /* we generate a block containing just the instruction
           modifying the memory. It will ensure that it cannot modify
           itself */
        env->current_tb = NULL;
        tb_gen_code(env, current_pc, current_cs_base, current_flags, 1);
        cpu_resume_from_signal(env, puc);
    }
#endif
}
#endif

/* add a new TB and link it to the physical page tables. phys_page2 is
   (-1) to indicate that only one page contains the TB. */
void tb_link_phys(TranslationBlock *tb,
                  target_ulong phys_pc, target_ulong phys_page2)
{
    unsigned int h;
    TranslationBlock **ptb;

    /* Grab the mmap lock to stop another thread invalidating this TB
       before we are done.  */
    mmap_lock();
    /* add in the physical hash table */
    h = tb_phys_hash_func(phys_pc);
    ptb = &tcg_ctx.tb_ctx.tb_phys_hash[h];
    tb->phys_hash_next = *ptb;
    *ptb = tb;

    /* add in the page list */
    tb_alloc_page(tb, 0, phys_pc & TARGET_PAGE_MASK);
    if (phys_page2 != -1)
        tb_alloc_page(tb, 1, phys_page2);
    else
        tb->page_addr[1] = -1;

    tb->jmp_first = (TranslationBlock *)((long)tb | 2);
    tb->jmp_next[0] = NULL;
    tb->jmp_next[1] = NULL;

    /* init original jump addresses */
    if (tb->tb_next_offset[0] != 0xffff)
        tb_reset_jump(tb, 0);
    if (tb->tb_next_offset[1] != 0xffff)
        tb_reset_jump(tb, 1);

#ifdef DEBUG_TB_CHECK
    tb_page_check();
#endif
    mmap_unlock();
}

/* find the TB 'tb' such that tb[0].tc_ptr <= tc_ptr <
   tb[1].tc_ptr. Return NULL if not found */
TranslationBlock *tb_find_pc(unsigned long tc_ptr)
{
    int m_min, m_max, m;
    unsigned long v;
    TranslationBlock *tb;

    if (tcg_ctx.tb_ctx.nb_tbs <= 0)
        return NULL;
    if (tc_ptr < (unsigned long)tcg_ctx.code_gen_buffer ||
        tc_ptr >= (unsigned long)tcg_ctx.code_gen_ptr)
        return NULL;
    /* binary search (cf Knuth) */
    m_min = 0;
    m_max = tcg_ctx.tb_ctx.nb_tbs - 1;
    while (m_min <= m_max) {
        m = (m_min + m_max) >> 1;
        tb = &tcg_ctx.tb_ctx.tbs[m];
        v = (unsigned long)tb->tc_ptr;
        if (v == tc_ptr)
            return tb;
        else if (tc_ptr < v) {
            m_max = m - 1;
        } else {
            m_min = m + 1;
        }
    }
    return &tcg_ctx.tb_ctx.tbs[m_max];
}

static inline void tb_reset_jump_recursive2(TranslationBlock *tb, int n)
{
    TranslationBlock *tb1, *tb_next, **ptb;
    unsigned int n1;

    tb1 = tb->jmp_next[n];
    if (tb1 != NULL) {
        /* find head of list */
        for(;;) {
            n1 = (long)tb1 & 3;
            tb1 = (TranslationBlock *)((long)tb1 & ~3);
            if (n1 == 2)
                break;
            tb1 = tb1->jmp_next[n1];
        }
        /* we are now sure now that tb jumps to tb1 */
        tb_next = tb1;

        /* remove tb from the jmp_first list */
        ptb = &tb_next->jmp_first;
        for(;;) {
            tb1 = *ptb;
            n1 = (long)tb1 & 3;
            tb1 = (TranslationBlock *)((long)tb1 & ~3);
            if (n1 == n && tb1 == tb)
                break;
            ptb = &tb1->jmp_next[n1];
        }
        *ptb = tb->jmp_next[n];
        tb->jmp_next[n] = NULL;

        /* suppress the jump to next tb in generated code */
        tb_reset_jump(tb, n);

        /* suppress jumps in the tb on which we could have jumped */
        tb_reset_jump_recursive(tb_next);
    }
}

void tb_reset_jump_recursive(TranslationBlock *tb)
{
    tb_reset_jump_recursive2(tb, 0);
    tb_reset_jump_recursive2(tb, 1);
}

/* in deterministic execution mode, instructions doing device I/Os
   must be at the end of the TB */
void cpu_io_recompile(CPUArchState *env, uintptr_t retaddr)
{
    TranslationBlock *tb;
    uint32_t n, cflags;
    target_ulong pc, cs_base;
    uint64_t flags;

    tb = tb_find_pc(retaddr);
    if (!tb) {
        cpu_abort(env, "cpu_io_recompile: could not find TB for pc=%p",
                  (void*)retaddr);
    }
    n = env->icount_decr.u16.low + tb->icount;
    cpu_restore_state(tb, env, retaddr);
    /* Calculate how many instructions had been executed before the fault
       occurred.  */
    n = n - env->icount_decr.u16.low;
    /* Generate a new TB ending on the I/O insn.  */
    n++;
    /* On MIPS and SH, delay slot instructions can only be restarted if
       they were already the first instruction in the TB.  If this is not
       the first instruction in a TB then re-execute the preceding
       branch.  */
#if defined(TARGET_MIPS)
    if ((env->hflags & MIPS_HFLAG_BMASK) != 0 && n > 1) {
        env->active_tc.PC -= 4;
        env->icount_decr.u16.low++;
        env->hflags &= ~MIPS_HFLAG_BMASK;
    }
#elif defined(TARGET_SH4)
    if ((env->flags & ((DELAY_SLOT | DELAY_SLOT_CONDITIONAL))) != 0
            && n > 1) {
        env->pc -= 2;
        env->icount_decr.u16.low++;
        env->flags &= ~(DELAY_SLOT | DELAY_SLOT_CONDITIONAL);
    }
#endif
    /* This should never happen.  */
    if (n > CF_COUNT_MASK)
        cpu_abort(env, "TB too big during recompile");

    cflags = n | CF_LAST_IO;
    pc = tb->pc;
    cs_base = tb->cs_base;
    flags = tb->flags;
    tb_phys_invalidate(tb, -1);
    /* FIXME: In theory this could raise an exception.  In practice
       we have already translated the block once so it's probably ok.  */
    tb_gen_code(env, pc, cs_base, flags, cflags);
    /* TODO: If env->pc != tb->pc (i.e. the faulting instruction was not
       the first in the TB) then we end up generating a whole new TB and
       repeating the fault, which is horribly inefficient.
       Better would be to execute just this insn uncached, or generate a
       second new TB.  */
    cpu_resume_from_signal(env, NULL);
}

#if !defined(CONFIG_USER_ONLY)

void dump_exec_info(FILE *f, fprintf_function cpu_fprintf)
{
    int i, target_code_size, max_target_code_size;
    int direct_jmp_count, direct_jmp2_count, cross_page;
    TranslationBlock *tb;

    target_code_size = 0;
    max_target_code_size = 0;
    cross_page = 0;
    direct_jmp_count = 0;
    direct_jmp2_count = 0;
    for(i = 0; i < tcg_ctx.tb_ctx.nb_tbs; i++) {
        tb = &tcg_ctx.tb_ctx.tbs[i];
        target_code_size += tb->size;
        if (tb->size > max_target_code_size)
            max_target_code_size = tb->size;
        if (tb->page_addr[1] != -1)
            cross_page++;
        if (tb->tb_next_offset[0] != 0xffff) {
            direct_jmp_count++;
            if (tb->tb_next_offset[1] != 0xffff) {
                direct_jmp2_count++;
            }
        }
    }
    /* XXX: avoid using doubles ? */
    cpu_fprintf(f, "Translation buffer state:\n");
    cpu_fprintf(f, "gen code size       %td/%ld\n",
                tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer,
                (long)tcg_ctx.code_gen_buffer_max_size);
    cpu_fprintf(f, "TB count            %d/%d\n",
                tcg_ctx.tb_ctx.nb_tbs, tcg_ctx.code_gen_max_blocks);
    cpu_fprintf(f, "TB avg target size  %d max=%d bytes\n",
                tcg_ctx.tb_ctx.nb_tbs ? target_code_size / tcg_ctx.tb_ctx.nb_tbs : 0,
                max_target_code_size);
    cpu_fprintf(f, "TB avg host size    %td bytes (expansion ratio: %0.1f)\n",
                tcg_ctx.tb_ctx.nb_tbs ? (tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer) / tcg_ctx.tb_ctx.nb_tbs : 0,
                target_code_size ? (double) (tcg_ctx.code_gen_ptr - tcg_ctx.code_gen_buffer) / target_code_size : 0);
    cpu_fprintf(f, "cross page TB count %d (%d%%)\n",
            cross_page,
            tcg_ctx.tb_ctx.nb_tbs ? (cross_page * 100) / tcg_ctx.tb_ctx.nb_tbs : 0);
    cpu_fprintf(f, "direct jump count   %d (%d%%) (2 jumps=%d %d%%)\n",
                direct_jmp_count,
                tcg_ctx.tb_ctx.nb_tbs ? (direct_jmp_count * 100) / tcg_ctx.tb_ctx.nb_tbs : 0,
                direct_jmp2_count,
                tcg_ctx.tb_ctx.nb_tbs ? (direct_jmp2_count * 100) / tcg_ctx.tb_ctx.nb_tbs : 0);
    cpu_fprintf(f, "\nStatistics:\n");
    cpu_fprintf(f, "TB flush count      %d\n", tcg_ctx.tb_ctx.tb_flush_count);
    cpu_fprintf(f, "TB invalidate count %d\n", tcg_ctx.tb_ctx.tb_phys_invalidate_count);
    cpu_fprintf(f, "TLB flush count     %d\n", tlb_flush_count);
    tcg_dump_info(f, cpu_fprintf);
}

#endif

void tb_flush_jmp_cache(CPUArchState *env, target_ulong addr)
{
    unsigned int i;

    /* Discard jump cache entries for any tb which might potentially
       overlap the flushed page.  */
    i = tb_jmp_cache_hash_page(addr - TARGET_PAGE_SIZE);
    memset (&env->tb_jmp_cache[i], 0,
            TB_JMP_PAGE_SIZE * sizeof(TranslationBlock *));

    i = tb_jmp_cache_hash_page(addr);
    memset (&env->tb_jmp_cache[i], 0,
            TB_JMP_PAGE_SIZE * sizeof(TranslationBlock *));
}
