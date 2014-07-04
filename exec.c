/*
 *  virtual page mapping and translated block handling
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
#include "config.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu-common.h"
#include "tcg.h"
#include "hw/hw.h"
#include "hw/qdev.h"
#include "hw/xen/xen.h"
#include "qemu/osdep.h"
#include "qemu/tls.h"
#include "sysemu/kvm.h"
#include "exec/cputlb.h"
#include "exec/hax.h"
#include "qemu/timer.h"
#if defined(CONFIG_USER_ONLY)
#include <qemu.h>
#endif

//#define DEBUG_SUBPAGE

#if !defined(CONFIG_USER_ONLY)
int phys_ram_fd;
static int in_migration;

RAMList ram_list = { .blocks = QTAILQ_HEAD_INITIALIZER(ram_list.blocks) };
#endif

struct CPUTailQ cpus = QTAILQ_HEAD_INITIALIZER(cpus);
DEFINE_TLS(CPUState *, current_cpu);

/* 0 = Do not count executed instructions.
   1 = Precise instruction counting.
   2 = Adaptive rate instruction counting.  */
int use_icount = 0;
/* Current instruction counter.  While executing translated code this may
   include some instructions that have not yet been executed.  */
int64_t qemu_icount;

#if !defined(CONFIG_USER_ONLY)
static void io_mem_init(void);

/* io memory support */
CPUWriteMemoryFunc *_io_mem_write[IO_MEM_NB_ENTRIES][4];
CPUReadMemoryFunc *_io_mem_read[IO_MEM_NB_ENTRIES][4];
void *io_mem_opaque[IO_MEM_NB_ENTRIES];
static char io_mem_used[IO_MEM_NB_ENTRIES];
int io_mem_watch;
#endif

/* log support */
#ifdef WIN32
static const char *logfilename = "qemu.log";
#else
static const char *logfilename = "/tmp/qemu.log";
#endif
FILE *logfile;
int loglevel;
static int log_append = 0;

#define SUBPAGE_IDX(addr) ((addr) & ~TARGET_PAGE_MASK)
typedef struct subpage_t {
    hwaddr base;
    CPUReadMemoryFunc **mem_read[TARGET_PAGE_SIZE][4];
    CPUWriteMemoryFunc **mem_write[TARGET_PAGE_SIZE][4];
    void *opaque[TARGET_PAGE_SIZE][2][4];
    ram_addr_t region_offset[TARGET_PAGE_SIZE][2][4];
} subpage_t;

/* Must be called before using the QEMU cpus. 'tb_size' is the size
   (in bytes) allocated to the translation buffer. Zero means default
   size. */
void cpu_exec_init_all(unsigned long tb_size)
{
    //cpu_gen_init();
    //code_gen_alloc(tb_size);
    //code_gen_ptr = code_gen_buffer;
    //page_init();
    tcg_exec_init(tb_size);
#if !defined(CONFIG_USER_ONLY)
    qemu_mutex_init(&ram_list.mutex);
    io_mem_init();
#endif
}

#if defined(CPU_SAVE_VERSION) && !defined(CONFIG_USER_ONLY)

#define CPU_COMMON_SAVE_VERSION 1

static void cpu_common_save(QEMUFile *f, void *opaque)
{
    CPUOldState *env = opaque;
    CPUState *cpu = ENV_GET_CPU(env);

    cpu_synchronize_state(cpu, 0);

    qemu_put_be32s(f, &cpu->halted);
    qemu_put_be32s(f, &cpu->interrupt_request);
}

static int cpu_common_load(QEMUFile *f, void *opaque, int version_id)
{
    CPUOldState *env = opaque;
    CPUState *cpu = ENV_GET_CPU(env);

    if (version_id != CPU_COMMON_SAVE_VERSION)
        return -EINVAL;

    qemu_get_be32s(f, &cpu->halted);
    qemu_get_be32s(f, &cpu->interrupt_request);
    /* 0x01 was CPU_INTERRUPT_EXIT. This line can be removed when the
       version_id is increased. */
    cpu->interrupt_request &= ~0x01;
    tlb_flush(env, 1);
    cpu_synchronize_state(cpu, 1);

    return 0;
}
#endif

CPUState *qemu_get_cpu(int cpu_index)
{
    CPUState *cpu;

    CPU_FOREACH(cpu) {
        if (cpu->cpu_index == cpu_index)
            return cpu;
    }
    return NULL;
}

void cpu_exec_init(CPUArchState *env)
{
    CPUState *cpu = ENV_GET_CPU(env);

#if defined(CONFIG_USER_ONLY)
    cpu_list_lock();
#endif
    // Compute CPU index from list position.
    int cpu_index = 0;
    CPUState *cpu1;
    CPU_FOREACH(cpu1) {
        cpu_index++;
    }
    cpu->cpu_index = cpu_index;
    QTAILQ_INSERT_TAIL(&cpus, cpu, node);

    cpu->numa_node = 0;
    QTAILQ_INIT(&env->breakpoints);
    QTAILQ_INIT(&env->watchpoints);
#if defined(CONFIG_USER_ONLY)
    cpu_list_unlock();
#endif
#if defined(CPU_SAVE_VERSION) && !defined(CONFIG_USER_ONLY)
    register_savevm(NULL,
                    "cpu_common",
                    cpu_index,
                    CPU_COMMON_SAVE_VERSION,
                    cpu_common_save,
                    cpu_common_load,
                    env);
    register_savevm(NULL,
                    "cpu",
                    cpu_index,
                    CPU_SAVE_VERSION,
                    cpu_save,
                    cpu_load,
                    env);
#endif
}

#if defined(TARGET_HAS_ICE)
static void breakpoint_invalidate(CPUArchState *env, target_ulong pc)
{
    hwaddr addr;
    target_ulong pd;
    ram_addr_t ram_addr;
    PhysPageDesc *p;

    addr = cpu_get_phys_page_debug(env, pc);
    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }
    ram_addr = (pd & TARGET_PAGE_MASK) | (pc & ~TARGET_PAGE_MASK);
    tb_invalidate_phys_page_range(ram_addr, ram_addr + 1, 0);
}
#endif

#if defined(CONFIG_USER_ONLY)
void cpu_watchpoint_remove_all(CPUArchState *env, int mask)

{
}

int cpu_watchpoint_insert(CPUArchState *env, target_ulong addr, target_ulong len,
                          int flags, CPUWatchpoint **watchpoint)
{
    return -ENOSYS;
}
#else
/* Add a watchpoint.  */
int cpu_watchpoint_insert(CPUArchState *env, target_ulong addr, target_ulong len,
                          int flags, CPUWatchpoint **watchpoint)
{
    target_ulong len_mask = ~(len - 1);
    CPUWatchpoint *wp;

    /* sanity checks: allow power-of-2 lengths, deny unaligned watchpoints */
    if ((len & (len - 1)) || (addr & ~len_mask) ||
            len == 0 || len > TARGET_PAGE_SIZE) {
        fprintf(stderr, "qemu: tried to set invalid watchpoint at "
                TARGET_FMT_lx ", len=" TARGET_FMT_lu "\n", addr, len);
        return -EINVAL;
    }
    wp = g_malloc(sizeof(*wp));

    wp->vaddr = addr;
    wp->len_mask = len_mask;
    wp->flags = flags;

    /* keep all GDB-injected watchpoints in front */
    if (flags & BP_GDB)
        QTAILQ_INSERT_HEAD(&env->watchpoints, wp, entry);
    else
        QTAILQ_INSERT_TAIL(&env->watchpoints, wp, entry);

    tlb_flush_page(env, addr);

    if (watchpoint)
        *watchpoint = wp;
    return 0;
}

/* Remove a specific watchpoint.  */
int cpu_watchpoint_remove(CPUArchState *env, target_ulong addr, target_ulong len,
                          int flags)
{
    target_ulong len_mask = ~(len - 1);
    CPUWatchpoint *wp;

    QTAILQ_FOREACH(wp, &env->watchpoints, entry) {
        if (addr == wp->vaddr && len_mask == wp->len_mask
                && flags == (wp->flags & ~BP_WATCHPOINT_HIT)) {
            cpu_watchpoint_remove_by_ref(env, wp);
            return 0;
        }
    }
    return -ENOENT;
}

/* Remove a specific watchpoint by reference.  */
void cpu_watchpoint_remove_by_ref(CPUArchState *env, CPUWatchpoint *watchpoint)
{
    QTAILQ_REMOVE(&env->watchpoints, watchpoint, entry);

    tlb_flush_page(env, watchpoint->vaddr);

    g_free(watchpoint);
}

/* Remove all matching watchpoints.  */
void cpu_watchpoint_remove_all(CPUArchState *env, int mask)
{
    CPUWatchpoint *wp, *next;

    QTAILQ_FOREACH_SAFE(wp, &env->watchpoints, entry, next) {
        if (wp->flags & mask)
            cpu_watchpoint_remove_by_ref(env, wp);
    }
}
#endif

/* Add a breakpoint.  */
int cpu_breakpoint_insert(CPUArchState *env, target_ulong pc, int flags,
                          CPUBreakpoint **breakpoint)
{
#if defined(TARGET_HAS_ICE)
    CPUBreakpoint *bp;

    bp = g_malloc(sizeof(*bp));

    bp->pc = pc;
    bp->flags = flags;

    /* keep all GDB-injected breakpoints in front */
    if (flags & BP_GDB) {
        QTAILQ_INSERT_HEAD(&env->breakpoints, bp, entry);
    } else {
        QTAILQ_INSERT_TAIL(&env->breakpoints, bp, entry);
    }

    breakpoint_invalidate(env, pc);

    if (breakpoint) {
        *breakpoint = bp;
    }
    return 0;
#else
    return -ENOSYS;
#endif
}

/* Remove a specific breakpoint.  */
int cpu_breakpoint_remove(CPUArchState *env, target_ulong pc, int flags)
{
#if defined(TARGET_HAS_ICE)
    CPUBreakpoint *bp;

    QTAILQ_FOREACH(bp, &env->breakpoints, entry) {
        if (bp->pc == pc && bp->flags == flags) {
            cpu_breakpoint_remove_by_ref(env, bp);
            return 0;
        }
    }
    return -ENOENT;
#else
    return -ENOSYS;
#endif
}

/* Remove a specific breakpoint by reference.  */
void cpu_breakpoint_remove_by_ref(CPUArchState *env, CPUBreakpoint *breakpoint)
{
#if defined(TARGET_HAS_ICE)
    QTAILQ_REMOVE(&env->breakpoints, breakpoint, entry);

    breakpoint_invalidate(env, breakpoint->pc);

    g_free(breakpoint);
#endif
}

/* Remove all matching breakpoints. */
void cpu_breakpoint_remove_all(CPUArchState *env, int mask)
{
#if defined(TARGET_HAS_ICE)
    CPUBreakpoint *bp, *next;

    QTAILQ_FOREACH_SAFE(bp, &env->breakpoints, entry, next) {
        if (bp->flags & mask)
            cpu_breakpoint_remove_by_ref(env, bp);
    }
#endif
}

/* enable or disable single step mode. EXCP_DEBUG is returned by the
   CPU loop after each instruction */
void cpu_single_step(CPUState *cpu, int enabled)
{
#if defined(TARGET_HAS_ICE)
    if (cpu->singlestep_enabled != enabled) {
        cpu->singlestep_enabled = enabled;
        if (kvm_enabled()) {
            kvm_update_guest_debug(cpu->env_ptr, 0);
        } else {
            /* must flush all the translated code to avoid inconsistencies */
            /* XXX: only flush what is necessary */
            tb_flush(cpu->env_ptr);
        }
    }
#endif
}

/* enable or disable low levels log */
void cpu_set_log(int log_flags)
{
    loglevel = log_flags;
    if (loglevel && !logfile) {
        logfile = fopen(logfilename, log_append ? "a" : "w");
        if (!logfile) {
            perror(logfilename);
            exit(1);
        }
#if !defined(CONFIG_SOFTMMU)
        /* must avoid mmap() usage of glibc by setting a buffer "by hand" */
        {
            static char logfile_buf[4096];
            setvbuf(logfile, logfile_buf, _IOLBF, sizeof(logfile_buf));
        }
#elif !defined(_WIN32)
        /* Win32 doesn't support line-buffering and requires size >= 2 */
        setvbuf(logfile, NULL, _IOLBF, 0);
#endif
        log_append = 1;
    }
    if (!loglevel && logfile) {
        fclose(logfile);
        logfile = NULL;
    }
}

void cpu_set_log_filename(const char *filename)
{
    logfilename = strdup(filename);
    if (logfile) {
        fclose(logfile);
        logfile = NULL;
    }
    cpu_set_log(loglevel);
}

void cpu_unlink_tb(CPUOldState *env)
{
    /* FIXME: TB unchaining isn't SMP safe.  For now just ignore the
       problem and hope the cpu will stop of its own accord.  For userspace
       emulation this often isn't actually as bad as it sounds.  Often
       signals are used primarily to interrupt blocking syscalls.  */
    TranslationBlock *tb;
    static spinlock_t interrupt_lock = SPIN_LOCK_UNLOCKED;

    spin_lock(&interrupt_lock);
    tb = env->current_tb;
    /* if the cpu is currently executing code, we must unlink it and
       all the potentially executing TB */
    if (tb) {
        env->current_tb = NULL;
        tb_reset_jump_recursive(tb);
    }
    spin_unlock(&interrupt_lock);
}

void cpu_reset_interrupt(CPUState *cpu, int mask)
{
    cpu->interrupt_request &= ~mask;
}

void cpu_exit(CPUState *cpu)
{
    cpu->exit_request = 1;
    cpu_unlink_tb(cpu->env_ptr);
}

void cpu_abort(CPUArchState *env, const char *fmt, ...)
{
    CPUState *cpu = ENV_GET_CPU(env);

    va_list ap;
    va_list ap2;

    va_start(ap, fmt);
    va_copy(ap2, ap);
    fprintf(stderr, "qemu: fatal: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
#ifdef TARGET_I386
    cpu_dump_state(cpu, stderr, fprintf, X86_DUMP_FPU | X86_DUMP_CCOP);
#else
    cpu_dump_state(cpu, stderr, fprintf, 0);
#endif
    if (qemu_log_enabled()) {
        qemu_log("qemu: fatal: ");
        qemu_log_vprintf(fmt, ap2);
        qemu_log("\n");
#ifdef TARGET_I386
        log_cpu_state(cpu, X86_DUMP_FPU | X86_DUMP_CCOP);
#else
        log_cpu_state(cpu, 0);
#endif
        qemu_log_flush();
        qemu_log_close();
    }
    va_end(ap2);
    va_end(ap);
#if defined(CONFIG_USER_ONLY)
    {
        struct sigaction act;
        sigfillset(&act.sa_mask);
        act.sa_handler = SIG_DFL;
        sigaction(SIGABRT, &act, NULL);
    }
#endif
    abort();
}

#if !defined(CONFIG_USER_ONLY)
static RAMBlock *qemu_get_ram_block(ram_addr_t addr)
{
    RAMBlock *block;

    /* The list is protected by the iothread lock here.  */
    block = ram_list.mru_block;
    if (block && addr - block->offset < block->length) {
        goto found;
    }
    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (addr - block->offset < block->length) {
            goto found;
        }
    }

    fprintf(stderr, "Bad ram offset %" PRIx64 "\n", (uint64_t)addr);
    abort();

found:
    ram_list.mru_block = block;
    return block;
}

/* Note: start and end must be within the same ram block.  */
void cpu_physical_memory_reset_dirty(ram_addr_t start, ram_addr_t end,
                                     int dirty_flags)
{
    unsigned long length, start1;
    int i;

    start &= TARGET_PAGE_MASK;
    end = TARGET_PAGE_ALIGN(end);

    length = end - start;
    if (length == 0)
        return;
    cpu_physical_memory_mask_dirty_range(start, length, dirty_flags);

    /* we modify the TLB cache so that the dirty bit will be set again
       when accessing the range */
    start1 = (unsigned long)qemu_safe_ram_ptr(start);
    /* Chek that we don't span multiple blocks - this breaks the
       address comparisons below.  */
    if ((unsigned long)qemu_safe_ram_ptr(end - 1) - start1
            != (end - 1) - start) {
        abort();
    }

    CPUState *cpu;
    CPU_FOREACH(cpu) {
        int mmu_idx;
        for (mmu_idx = 0; mmu_idx < NB_MMU_MODES; mmu_idx++) {
            for(i = 0; i < CPU_TLB_SIZE; i++) {
                CPUArchState* env = cpu->env_ptr;
                tlb_reset_dirty_range(&env->tlb_table[mmu_idx][i],
                                      start1, length);
            }
        }
    }
}

int cpu_physical_memory_set_dirty_tracking(int enable)
{
    in_migration = enable;
    if (kvm_enabled()) {
        return kvm_set_migration_log(enable);
    }
    return 0;
}

int cpu_physical_memory_get_dirty_tracking(void)
{
    return in_migration;
}

int cpu_physical_sync_dirty_bitmap(hwaddr start_addr,
                                   hwaddr end_addr)
{
    int ret = 0;

    if (kvm_enabled())
        ret = kvm_physical_sync_dirty_bitmap(start_addr, end_addr);
    return ret;
}

static inline void tlb_update_dirty(CPUTLBEntry *tlb_entry)
{
    ram_addr_t ram_addr;
    void *p;

    if ((tlb_entry->addr_write & ~TARGET_PAGE_MASK) == IO_MEM_RAM) {
        p = (void *)(unsigned long)((tlb_entry->addr_write & TARGET_PAGE_MASK)
            + tlb_entry->addend);
        ram_addr = qemu_ram_addr_from_host_nofail(p);
        if (!cpu_physical_memory_is_dirty(ram_addr)) {
            tlb_entry->addr_write |= TLB_NOTDIRTY;
        }
    }
}

/* update the TLB according to the current state of the dirty bits */
void cpu_tlb_update_dirty(CPUArchState *env)
{
    int i;
    int mmu_idx;
    for (mmu_idx = 0; mmu_idx < NB_MMU_MODES; mmu_idx++) {
        for(i = 0; i < CPU_TLB_SIZE; i++)
            tlb_update_dirty(&env->tlb_table[mmu_idx][i]);
    }
}


#else

void tlb_flush(CPUArchState *env, int flush_global)
{
}

void tlb_flush_page(CPUArchState *env, target_ulong addr)
{
}

int tlb_set_page_exec(CPUArchState *env, target_ulong vaddr,
                      hwaddr paddr, int prot,
                      int mmu_idx, int is_softmmu)
{
    return 0;
}

static inline void tlb_set_dirty(CPUOldState *env,
                                 unsigned long addr, target_ulong vaddr)
{
}
#endif /* defined(CONFIG_USER_ONLY) */

#if !defined(CONFIG_USER_ONLY)

static int subpage_register (subpage_t *mmio, uint32_t start, uint32_t end,
                             ram_addr_t memory, ram_addr_t region_offset);
static void *subpage_init (hwaddr base, ram_addr_t *phys,
                           ram_addr_t orig_memory, ram_addr_t region_offset);

static void *(*phys_mem_alloc)(size_t size) = qemu_anon_ram_alloc;

/*
 * Set a custom physical guest memory alloator.
 * Accelerators with unusual needs may need this.  Hopefully, we can
 * get rid of it eventually.
 */
void phys_mem_set_alloc(void *(*alloc)(size_t))
{
    phys_mem_alloc = alloc;
}

#define CHECK_SUBPAGE(addr, start_addr, start_addr2, end_addr, end_addr2, \
                      need_subpage)                                     \
    do {                                                                \
        if (addr > start_addr)                                          \
            start_addr2 = 0;                                            \
        else {                                                          \
            start_addr2 = start_addr & ~TARGET_PAGE_MASK;               \
            if (start_addr2 > 0)                                        \
                need_subpage = 1;                                       \
        }                                                               \
                                                                        \
        if ((start_addr + orig_size) - addr >= TARGET_PAGE_SIZE)        \
            end_addr2 = TARGET_PAGE_SIZE - 1;                           \
        else {                                                          \
            end_addr2 = (start_addr + orig_size - 1) & ~TARGET_PAGE_MASK; \
            if (end_addr2 < TARGET_PAGE_SIZE - 1)                       \
                need_subpage = 1;                                       \
        }                                                               \
    } while (0)

/* register physical memory.
   For RAM, 'size' must be a multiple of the target page size.
   If (phys_offset & ~TARGET_PAGE_MASK) != 0, then it is an
   io memory page.  The address used when calling the IO function is
   the offset from the start of the region, plus region_offset.  Both
   start_addr and region_offset are rounded down to a page boundary
   before calculating this offset.  This should not be a problem unless
   the low bits of start_addr and region_offset differ.  */
void cpu_register_physical_memory_log(hwaddr start_addr,
                                         ram_addr_t size,
                                         ram_addr_t phys_offset,
                                         ram_addr_t region_offset,
                                         bool log_dirty)
{
    hwaddr addr, end_addr;
    PhysPageDesc *p;
    CPUState *cpu;
    ram_addr_t orig_size = size;
    subpage_t *subpage;

    if (kvm_enabled())
        kvm_set_phys_mem(start_addr, size, phys_offset);
#ifdef CONFIG_HAX
    if (hax_enabled())
        hax_set_phys_mem(start_addr, size, phys_offset);
#endif

    if (phys_offset == IO_MEM_UNASSIGNED) {
        region_offset = start_addr;
    }
    region_offset &= TARGET_PAGE_MASK;
    size = (size + TARGET_PAGE_SIZE - 1) & TARGET_PAGE_MASK;
    end_addr = start_addr + (hwaddr)size;

    addr = start_addr;
    do {
        p = phys_page_find(addr >> TARGET_PAGE_BITS);
        if (p && p->phys_offset != IO_MEM_UNASSIGNED) {
            ram_addr_t orig_memory = p->phys_offset;
            hwaddr start_addr2, end_addr2;
            int need_subpage = 0;

            CHECK_SUBPAGE(addr, start_addr, start_addr2, end_addr, end_addr2,
                          need_subpage);
            if (need_subpage) {
                if (!(orig_memory & IO_MEM_SUBPAGE)) {
                    subpage = subpage_init((addr & TARGET_PAGE_MASK),
                                           &p->phys_offset, orig_memory,
                                           p->region_offset);
                } else {
                    subpage = io_mem_opaque[(orig_memory & ~TARGET_PAGE_MASK)
                                            >> IO_MEM_SHIFT];
                }
                subpage_register(subpage, start_addr2, end_addr2, phys_offset,
                                 region_offset);
                p->region_offset = 0;
            } else {
                p->phys_offset = phys_offset;
                if ((phys_offset & ~TARGET_PAGE_MASK) <= IO_MEM_ROM ||
                    (phys_offset & IO_MEM_ROMD))
                    phys_offset += TARGET_PAGE_SIZE;
            }
        } else {
            p = phys_page_find_alloc(addr >> TARGET_PAGE_BITS, 1);
            p->phys_offset = phys_offset;
            p->region_offset = region_offset;
            if ((phys_offset & ~TARGET_PAGE_MASK) <= IO_MEM_ROM ||
                (phys_offset & IO_MEM_ROMD)) {
                phys_offset += TARGET_PAGE_SIZE;
            } else {
                hwaddr start_addr2, end_addr2;
                int need_subpage = 0;

                CHECK_SUBPAGE(addr, start_addr, start_addr2, end_addr,
                              end_addr2, need_subpage);

                if (need_subpage) {
                    subpage = subpage_init((addr & TARGET_PAGE_MASK),
                                           &p->phys_offset, IO_MEM_UNASSIGNED,
                                           addr & TARGET_PAGE_MASK);
                    subpage_register(subpage, start_addr2, end_addr2,
                                     phys_offset, region_offset);
                    p->region_offset = 0;
                }
            }
        }
        region_offset += TARGET_PAGE_SIZE;
        addr += TARGET_PAGE_SIZE;
    } while (addr != end_addr);

    /* since each CPU stores ram addresses in its TLB cache, we must
       reset the modified entries */
    /* XXX: slow ! */
    CPU_FOREACH(cpu) {
        tlb_flush(cpu->env_ptr, 1);
    }
}

/* XXX: temporary until new memory mapping API */
ram_addr_t cpu_get_physical_page_desc(hwaddr addr)
{
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p)
        return IO_MEM_UNASSIGNED;
    return p->phys_offset;
}

void qemu_register_coalesced_mmio(hwaddr addr, ram_addr_t size)
{
    if (kvm_enabled())
        kvm_coalesce_mmio_region(addr, size);
}

void qemu_unregister_coalesced_mmio(hwaddr addr, ram_addr_t size)
{
    if (kvm_enabled())
        kvm_uncoalesce_mmio_region(addr, size);
}

void qemu_mutex_lock_ramlist(void)
{
    qemu_mutex_lock(&ram_list.mutex);
}

void qemu_mutex_unlock_ramlist(void)
{
    qemu_mutex_unlock(&ram_list.mutex);
}

#if defined(__linux__) && !defined(CONFIG_ANDROID)

#include <sys/vfs.h>

#define HUGETLBFS_MAGIC       0x958458f6

static long gethugepagesize(const char *path)
{
    struct statfs fs;
    int ret;

    do {
        ret = statfs(path, &fs);
    } while (ret != 0 && errno == EINTR);

    if (ret != 0) {
        perror(path);
        return 0;
    }

    if (fs.f_type != HUGETLBFS_MAGIC)
        fprintf(stderr, "Warning: path not on HugeTLBFS: %s\n", path);

    return fs.f_bsize;
}

static sigjmp_buf sigjump;

static void sigbus_handler(int signal)
{
    siglongjmp(sigjump, 1);
}

static void *file_ram_alloc(RAMBlock *block,
                            ram_addr_t memory,
                            const char *path)
{
    char *filename;
    char *sanitized_name;
    char *c;
    void *area;
    int fd;
    unsigned long hpagesize;

    hpagesize = gethugepagesize(path);
    if (!hpagesize) {
        return NULL;
    }

    if (memory < hpagesize) {
        return NULL;
    }

    if (kvm_enabled() && !kvm_has_sync_mmu()) {
        fprintf(stderr, "host lacks kvm mmu notifiers, -mem-path unsupported\n");
        return NULL;
    }

    /* Make name safe to use with mkstemp by replacing '/' with '_'. */
    sanitized_name = g_strdup(block->mr->name);
    for (c = sanitized_name; *c != '\0'; c++) {
        if (*c == '/')
            *c = '_';
    }

    filename = g_strdup_printf("%s/qemu_back_mem.%s.XXXXXX", path,
                               sanitized_name);
    g_free(sanitized_name);

    fd = mkstemp(filename);
    if (fd < 0) {
        perror("unable to create backing store for hugepages");
        g_free(filename);
        return NULL;
    }
    unlink(filename);
    g_free(filename);

    memory = (memory+hpagesize-1) & ~(hpagesize-1);

    /*
     * ftruncate is not supported by hugetlbfs in older
     * hosts, so don't bother bailing out on errors.
     * If anything goes wrong with it under other filesystems,
     * mmap will fail.
     */
    if (ftruncate(fd, memory))
        perror("ftruncate");

    area = mmap(0, memory, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (area == MAP_FAILED) {
        perror("file_ram_alloc: can't mmap RAM pages");
        close(fd);
        return (NULL);
    }

    if (mem_prealloc) {
        int ret, i;
        struct sigaction act, oldact;
        sigset_t set, oldset;

        memset(&act, 0, sizeof(act));
        act.sa_handler = &sigbus_handler;
        act.sa_flags = 0;

        ret = sigaction(SIGBUS, &act, &oldact);
        if (ret) {
            perror("file_ram_alloc: failed to install signal handler");
            exit(1);
        }

        /* unblock SIGBUS */
        sigemptyset(&set);
        sigaddset(&set, SIGBUS);
        pthread_sigmask(SIG_UNBLOCK, &set, &oldset);

        if (sigsetjmp(sigjump, 1)) {
            fprintf(stderr, "file_ram_alloc: failed to preallocate pages\n");
            exit(1);
        }

        /* MAP_POPULATE silently ignores failures */
        for (i = 0; i < (memory/hpagesize)-1; i++) {
            memset(area + (hpagesize*i), 0, 1);
        }

        ret = sigaction(SIGBUS, &oldact, NULL);
        if (ret) {
            perror("file_ram_alloc: failed to reinstall signal handler");
            exit(1);
        }

        pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    }

    block->fd = fd;
    return area;
}
#else
static void *file_ram_alloc(RAMBlock *block,
                            ram_addr_t memory,
                            const char *path)
{
    fprintf(stderr, "-mem-path not supported on this host\n");
    exit(1);
}
#endif

static ram_addr_t find_ram_offset(ram_addr_t size)
{
    RAMBlock *block, *next_block;
    ram_addr_t offset = RAM_ADDR_MAX, mingap = RAM_ADDR_MAX;

    assert(size != 0); /* it would hand out same offset multiple times */

    if (QTAILQ_EMPTY(&ram_list.blocks))
        return 0;

    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        ram_addr_t end, next = RAM_ADDR_MAX;

        end = block->offset + block->length;

        QTAILQ_FOREACH(next_block, &ram_list.blocks, next) {
            if (next_block->offset >= end) {
                next = MIN(next, next_block->offset);
            }
        }
        if (next - end >= size && next - end < mingap) {
            offset = end;
            mingap = next - end;
        }
    }

    if (offset == RAM_ADDR_MAX) {
        fprintf(stderr, "Failed to find gap of requested size: %" PRIu64 "\n",
                (uint64_t)size);
        abort();
    }

    return offset;
}

ram_addr_t last_ram_offset(void)
{
    RAMBlock *block;
    ram_addr_t last = 0;

    QTAILQ_FOREACH(block, &ram_list.blocks, next)
        last = MAX(last, block->offset + block->length);

    return last;
}

static void qemu_ram_setup_dump(void *addr, ram_addr_t size)
{
#ifndef CONFIG_ANDROID
    int ret;

    /* Use MADV_DONTDUMP, if user doesn't want the guest memory in the core */
    if (!qemu_opt_get_bool(qemu_get_machine_opts(),
                           "dump-guest-core", true)) {
        ret = qemu_madvise(addr, size, QEMU_MADV_DONTDUMP);
        if (ret) {
            perror("qemu_madvise");
            fprintf(stderr, "madvise doesn't support MADV_DONTDUMP, "
                            "but dump_guest_core=off specified\n");
        }
    }
#endif  // !CONFIG_ANDROID
}

void qemu_ram_set_idstr(ram_addr_t addr, const char *name, DeviceState *dev)
{
    RAMBlock *new_block, *block;

    new_block = NULL;
    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (block->offset == addr) {
            new_block = block;
            break;
        }
    }
    assert(new_block);
    assert(!new_block->idstr[0]);

    if (dev) {
        char *id = qdev_get_dev_path(dev);
        if (id) {
            snprintf(new_block->idstr, sizeof(new_block->idstr), "%s/", id);
            g_free(id);
        }
    }
    pstrcat(new_block->idstr, sizeof(new_block->idstr), name);

    /* This assumes the iothread lock is taken here too.  */
    qemu_mutex_lock_ramlist();
    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (block != new_block && !strcmp(block->idstr, new_block->idstr)) {
            fprintf(stderr, "RAMBlock \"%s\" already registered, abort!\n",
                    new_block->idstr);
            abort();
        }
    }
    qemu_mutex_unlock_ramlist();
}

static int memory_try_enable_merging(void *addr, size_t len)
{
#ifndef CONFIG_ANDROID
    if (!qemu_opt_get_bool(qemu_get_machine_opts(), "mem-merge", true)) {
        /* disabled by the user */
        return 0;
    }

    return qemu_madvise(addr, len, QEMU_MADV_MERGEABLE);
#else  // CONFIG_ANDROID
    return qemu_madvise(addr, len, QEMU_MADV_MERGEABLE);
#endif  // CONFIG_ANDROID
}

ram_addr_t qemu_ram_alloc_from_ptr(DeviceState *dev, const char *name,
                                   ram_addr_t size, void *host)
{
    RAMBlock *block, *new_block;

    size = TARGET_PAGE_ALIGN(size);
    new_block = g_malloc0(sizeof(*new_block));
    new_block->fd = -1;

    /* This assumes the iothread lock is taken here too.  */
    qemu_mutex_lock_ramlist();
    //new_block->mr = mr;
    new_block->offset = find_ram_offset(size);
    if (host) {
        new_block->host = host;
        new_block->flags |= RAM_PREALLOC_MASK;
    } else if (xen_enabled()) {
        if (mem_path) {
            fprintf(stderr, "-mem-path not supported with Xen\n");
            exit(1);
        }
        //xen_ram_alloc(new_block->offset, size, mr);
    } else {
        if (mem_path) {
            if (phys_mem_alloc != qemu_anon_ram_alloc) {
                /*
                 * file_ram_alloc() needs to allocate just like
                 * phys_mem_alloc, but we haven't bothered to provide
                 * a hook there.
                 */
                fprintf(stderr,
                        "-mem-path not supported with this accelerator\n");
                exit(1);
            }
            new_block->host = file_ram_alloc(new_block, size, mem_path);
        }
        if (!new_block->host) {
            new_block->host = phys_mem_alloc(size);
            if (!new_block->host) {
                fprintf(stderr, "Cannot set up guest memory '%s': %s\n",
                        name, strerror(errno));
                exit(1);
            }
#ifdef CONFIG_HAX
            if (hax_enabled()) {
                /*
                 * In HAX, qemu allocates the virtual address, and HAX kernel
                 * module populates the region with physical memory. Currently
                 * we don’t populate guest memory on demand, thus we should
                 * make sure that sufficient amount of memory is available in
                 * advance.
                 */
                int ret = hax_populate_ram(
                        (uint64_t)(uintptr_t)new_block->host,
                        (uint32_t)size);
                if (ret < 0) {
                    fprintf(stderr, "Hax failed to populate ram\n");
                    exit(-1);
                }
            }
#endif  // CONFIG_HAX
            memory_try_enable_merging(new_block->host, size);
        }
    }
    new_block->length = size;

    if (dev) {
        char *id = qdev_get_dev_path(dev);
        if (id) {
            snprintf(new_block->idstr, sizeof(new_block->idstr), "%s/", id);
            g_free(id);
        }
    }
    pstrcat(new_block->idstr, sizeof(new_block->idstr), name);

    /* Keep the list sorted from biggest to smallest block.  */
    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (block->length < new_block->length) {
            break;
        }
    }
    if (block) {
        QTAILQ_INSERT_BEFORE(block, new_block, next);
    } else {
        QTAILQ_INSERT_TAIL(&ram_list.blocks, new_block, next);
    }
    ram_list.mru_block = NULL;

    ram_list.version++;
    qemu_mutex_unlock_ramlist();

    ram_list.phys_dirty = g_realloc(ram_list.phys_dirty,
                                       last_ram_offset() >> TARGET_PAGE_BITS);
    memset(ram_list.phys_dirty + (new_block->offset >> TARGET_PAGE_BITS),
           0xff, size >> TARGET_PAGE_BITS);
    //cpu_physical_memory_set_dirty_range(new_block->offset, size, 0xff);

    qemu_ram_setup_dump(new_block->host, size);
    //qemu_madvise(new_block->host, size, QEMU_MADV_HUGEPAGE);
    //qemu_madvise(new_block->host, size, QEMU_MADV_DONTFORK);

    if (kvm_enabled())
        kvm_setup_guest_memory(new_block->host, size);

    return new_block->offset;
}

ram_addr_t qemu_ram_alloc(DeviceState *dev, const char *name, ram_addr_t size)
{
    return qemu_ram_alloc_from_ptr(dev, name, size, NULL);
}

void qemu_ram_free_from_ptr(ram_addr_t addr)
{
    RAMBlock *block;

    /* This assumes the iothread lock is taken here too.  */
    qemu_mutex_lock_ramlist();
    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (addr == block->offset) {
            QTAILQ_REMOVE(&ram_list.blocks, block, next);
            ram_list.mru_block = NULL;
            ram_list.version++;
            g_free(block);
            break;
        }
    }
    qemu_mutex_unlock_ramlist();
}

void qemu_ram_free(ram_addr_t addr)
{
    RAMBlock *block;

    /* This assumes the iothread lock is taken here too.  */
    qemu_mutex_lock_ramlist();
    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (addr == block->offset) {
            QTAILQ_REMOVE(&ram_list.blocks, block, next);
            ram_list.mru_block = NULL;
            ram_list.version++;
            if (block->flags & RAM_PREALLOC_MASK) {
                ;
            } else if (xen_enabled()) {
                //xen_invalidate_map_cache_entry(block->host);
#ifndef _WIN32
            } else if (block->fd >= 0) {
                munmap(block->host, block->length);
                close(block->fd);
#endif
            } else {
                qemu_anon_ram_free(block->host, block->length);
            }
            g_free(block);
            break;
        }
    }
    qemu_mutex_unlock_ramlist();

}

#ifndef _WIN32
void qemu_ram_remap(ram_addr_t addr, ram_addr_t length)
{
    RAMBlock *block;
    ram_addr_t offset;
    int flags;
    void *area, *vaddr;

    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        offset = addr - block->offset;
        if (offset < block->length) {
            vaddr = block->host + offset;
            if (block->flags & RAM_PREALLOC_MASK) {
                ;
            } else if (xen_enabled()) {
                abort();
            } else {
                flags = MAP_FIXED;
                munmap(vaddr, length);
                if (block->fd >= 0) {
#ifdef MAP_POPULATE
                    flags |= mem_prealloc ? MAP_POPULATE | MAP_SHARED :
                        MAP_PRIVATE;
#else
                    flags |= MAP_PRIVATE;
#endif
                    area = mmap(vaddr, length, PROT_READ | PROT_WRITE,
                                flags, block->fd, offset);
                } else {
                    /*
                     * Remap needs to match alloc.  Accelerators that
                     * set phys_mem_alloc never remap.  If they did,
                     * we'd need a remap hook here.
                     */
                    assert(phys_mem_alloc == qemu_anon_ram_alloc);

                    flags |= MAP_PRIVATE | MAP_ANONYMOUS;
                    area = mmap(vaddr, length, PROT_READ | PROT_WRITE,
                                flags, -1, 0);
                }
                if (area != vaddr) {
                    fprintf(stderr, "Could not remap addr: "
                            RAM_ADDR_FMT "@" RAM_ADDR_FMT "\n",
                            length, addr);
                    exit(1);
                }
                memory_try_enable_merging(vaddr, length);
                qemu_ram_setup_dump(vaddr, length);
            }
            return;
        }
    }
}
#endif /* !_WIN32 */

/* Return a host pointer to ram allocated with qemu_ram_alloc.
   With the exception of the softmmu code in this file, this should
   only be used for local memory (e.g. video ram) that the device owns,
   and knows it isn't going to access beyond the end of the block.

   It should not be used for general purpose DMA.
   Use cpu_physical_memory_map/cpu_physical_memory_rw instead.
 */
void *qemu_get_ram_ptr(ram_addr_t addr)
{
    RAMBlock *block = qemu_get_ram_block(addr);
#if 0
    if (xen_enabled()) {
        /* We need to check if the requested address is in the RAM
         * because we don't want to map the entire memory in QEMU.
         * In that case just map until the end of the page.
         */
        if (block->offset == 0) {
            return xen_map_cache(addr, 0, 0);
        } else if (block->host == NULL) {
            block->host =
                xen_map_cache(block->offset, block->length, 1);
        }
    }
#endif
    return block->host + (addr - block->offset);
}

/* Return a host pointer to ram allocated with qemu_ram_alloc.
 * Same as qemu_get_ram_ptr but avoid reordering ramblocks.
 */
void *qemu_safe_ram_ptr(ram_addr_t addr)
{
    RAMBlock *block;

    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        if (addr - block->offset < block->length) {
            return block->host + (addr - block->offset);
        }
    }

    fprintf(stderr, "Bad ram offset %" PRIx64 "\n", (uint64_t)addr);
    abort();

    return NULL;
}

/* Some of the softmmu routines need to translate from a host pointer
   (typically a TLB entry) back to a ram offset.  */
int qemu_ram_addr_from_host(void *ptr, ram_addr_t *ram_addr)
{
    RAMBlock *block;
    uint8_t *host = ptr;
#if 0
    if (xen_enabled()) {
        *ram_addr = xen_ram_addr_from_mapcache(ptr);
        return qemu_get_ram_block(*ram_addr)->mr;
    }
#endif
    block = ram_list.mru_block;
    if (block && block->host && host - block->host < block->length) {
        goto found;
    }

    QTAILQ_FOREACH(block, &ram_list.blocks, next) {
        /* This case append when the block is not mapped. */
        if (block->host == NULL) {
            continue;
        }
        if (host - block->host < block->length) {
            goto found;
        }
    }

    return -1;

found:
    *ram_addr = block->offset + (host - block->host);
    return 0;
}

/* Some of the softmmu routines need to translate from a host pointer
   (typically a TLB entry) back to a ram offset.  */
ram_addr_t qemu_ram_addr_from_host_nofail(void *ptr)
{
    ram_addr_t ram_addr;

    if (qemu_ram_addr_from_host(ptr, &ram_addr)) {
        fprintf(stderr, "Bad ram pointer %p\n", ptr);
        abort();
    }
    return ram_addr;
}

static uint32_t unassigned_mem_readb(void *opaque, hwaddr addr)
{
#ifdef DEBUG_UNASSIGNED
    printf("Unassigned mem read " TARGET_FMT_plx "\n", addr);
#endif
#if defined(TARGET_SPARC) || defined(TARGET_MICROBLAZE)
    cpu_unassigned_access(cpu_single_env, addr, 0, 0, 0, 1);
#endif
    return 0;
}

static uint32_t unassigned_mem_readw(void *opaque, hwaddr addr)
{
#ifdef DEBUG_UNASSIGNED
    printf("Unassigned mem read " TARGET_FMT_plx "\n", addr);
#endif
#if defined(TARGET_SPARC) || defined(TARGET_MICROBLAZE)
    cpu_unassigned_access(cpu_single_env, addr, 0, 0, 0, 2);
#endif
    return 0;
}

static uint32_t unassigned_mem_readl(void *opaque, hwaddr addr)
{
#ifdef DEBUG_UNASSIGNED
    printf("Unassigned mem read " TARGET_FMT_plx "\n", addr);
#endif
#if defined(TARGET_SPARC) || defined(TARGET_MICROBLAZE)
    cpu_unassigned_access(cpu_single_env, addr, 0, 0, 0, 4);
#endif
    return 0;
}

static void unassigned_mem_writeb(void *opaque, hwaddr addr, uint32_t val)
{
#ifdef DEBUG_UNASSIGNED
    printf("Unassigned mem write " TARGET_FMT_plx " = 0x%x\n", addr, val);
#endif
#if defined(TARGET_SPARC) || defined(TARGET_MICROBLAZE)
    cpu_unassigned_access(cpu_single_env, addr, 1, 0, 0, 1);
#endif
}

static void unassigned_mem_writew(void *opaque, hwaddr addr, uint32_t val)
{
#ifdef DEBUG_UNASSIGNED
    printf("Unassigned mem write " TARGET_FMT_plx " = 0x%x\n", addr, val);
#endif
#if defined(TARGET_SPARC) || defined(TARGET_MICROBLAZE)
    cpu_unassigned_access(cpu_single_env, addr, 1, 0, 0, 2);
#endif
}

static void unassigned_mem_writel(void *opaque, hwaddr addr, uint32_t val)
{
#ifdef DEBUG_UNASSIGNED
    printf("Unassigned mem write " TARGET_FMT_plx " = 0x%x\n", addr, val);
#endif
#if defined(TARGET_SPARC) || defined(TARGET_MICROBLAZE)
    cpu_unassigned_access(cpu_single_env, addr, 1, 0, 0, 4);
#endif
}

static CPUReadMemoryFunc * const unassigned_mem_read[3] = {
    unassigned_mem_readb,
    unassigned_mem_readw,
    unassigned_mem_readl,
};

static CPUWriteMemoryFunc * const unassigned_mem_write[3] = {
    unassigned_mem_writeb,
    unassigned_mem_writew,
    unassigned_mem_writel,
};

static void notdirty_mem_writeb(void *opaque, hwaddr ram_addr,
                                uint32_t val)
{
    int dirty_flags;
    dirty_flags = cpu_physical_memory_get_dirty_flags(ram_addr);
    if (!(dirty_flags & CODE_DIRTY_FLAG)) {
#if !defined(CONFIG_USER_ONLY)
        tb_invalidate_phys_page_fast0(ram_addr, 1);
        dirty_flags = cpu_physical_memory_get_dirty_flags(ram_addr);
#endif
    }
    stb_p(qemu_get_ram_ptr(ram_addr), val);
    dirty_flags |= (0xff & ~CODE_DIRTY_FLAG);
    cpu_physical_memory_set_dirty_flags(ram_addr, dirty_flags);
    /* we remove the notdirty callback only if the code has been
       flushed */
    if (dirty_flags == 0xff)
        tlb_set_dirty(cpu_single_env, cpu_single_env->mem_io_vaddr);
}

static void notdirty_mem_writew(void *opaque, hwaddr ram_addr,
                                uint32_t val)
{
    int dirty_flags;
    dirty_flags = cpu_physical_memory_get_dirty_flags(ram_addr);
    if (!(dirty_flags & CODE_DIRTY_FLAG)) {
#if !defined(CONFIG_USER_ONLY)
        tb_invalidate_phys_page_fast0(ram_addr, 2);
        dirty_flags = cpu_physical_memory_get_dirty_flags(ram_addr);
#endif
    }
    stw_p(qemu_get_ram_ptr(ram_addr), val);
    dirty_flags |= (0xff & ~CODE_DIRTY_FLAG);
    cpu_physical_memory_set_dirty_flags(ram_addr, dirty_flags);
    /* we remove the notdirty callback only if the code has been
       flushed */
    if (dirty_flags == 0xff)
        tlb_set_dirty(cpu_single_env, cpu_single_env->mem_io_vaddr);
}

static void notdirty_mem_writel(void *opaque, hwaddr ram_addr,
                                uint32_t val)
{
    int dirty_flags;
    dirty_flags = cpu_physical_memory_get_dirty_flags(ram_addr);
    if (!(dirty_flags & CODE_DIRTY_FLAG)) {
#if !defined(CONFIG_USER_ONLY)
        tb_invalidate_phys_page_fast0(ram_addr, 4);
        dirty_flags = cpu_physical_memory_get_dirty_flags(ram_addr);
#endif
    }
    stl_p(qemu_get_ram_ptr(ram_addr), val);
    dirty_flags |= (0xff & ~CODE_DIRTY_FLAG);
    cpu_physical_memory_set_dirty_flags(ram_addr, dirty_flags);
    /* we remove the notdirty callback only if the code has been
       flushed */
    if (dirty_flags == 0xff)
        tlb_set_dirty(cpu_single_env, cpu_single_env->mem_io_vaddr);
}

static CPUReadMemoryFunc * const error_mem_read[3] = {
    NULL, /* never used */
    NULL, /* never used */
    NULL, /* never used */
};

static CPUWriteMemoryFunc * const notdirty_mem_write[3] = {
    notdirty_mem_writeb,
    notdirty_mem_writew,
    notdirty_mem_writel,
};

static void tb_check_watchpoint(CPUArchState* env)
{
    TranslationBlock *tb = tb_find_pc(env->mem_io_pc);
    if (!tb) {
        cpu_abort(env, "check_watchpoint: could not find TB for "
                  "pc=%p", (void *)env->mem_io_pc);
    }
    cpu_restore_state(env, env->mem_io_pc);
    tb_phys_invalidate(tb, -1);
}

/* Generate a debug exception if a watchpoint has been hit.  */
static void check_watchpoint(int offset, int len_mask, int flags)
{
    CPUState *cpu = current_cpu;
    CPUArchState *env = cpu->env_ptr;
    target_ulong pc, cs_base;
    target_ulong vaddr;
    CPUWatchpoint *wp;
    int cpu_flags;

    if (env->watchpoint_hit) {
        /* We re-entered the check after replacing the TB. Now raise
         * the debug interrupt so that is will trigger after the
         * current instruction. */
        cpu_interrupt(cpu, CPU_INTERRUPT_DEBUG);
        return;
    }
    vaddr = (env->mem_io_vaddr & TARGET_PAGE_MASK) + offset;
    QTAILQ_FOREACH(wp, &env->watchpoints, entry) {
        if ((vaddr == (wp->vaddr & len_mask) ||
             (vaddr & wp->len_mask) == wp->vaddr) && (wp->flags & flags)) {
            wp->flags |= BP_WATCHPOINT_HIT;
            if (!env->watchpoint_hit) {
                env->watchpoint_hit = wp;
                tb_check_watchpoint(env);
                if (wp->flags & BP_STOP_BEFORE_ACCESS) {
                    env->exception_index = EXCP_DEBUG;
                    cpu_loop_exit(env);
                } else {
                    cpu_get_tb_cpu_state(env, &pc, &cs_base, &cpu_flags);
                    tb_gen_code(env, pc, cs_base, cpu_flags, 1);
                    cpu_resume_from_signal(env, NULL);
                }
            }
        } else {
            wp->flags &= ~BP_WATCHPOINT_HIT;
        }
    }
}

/* Watchpoint access routines.  Watchpoints are inserted using TLB tricks,
   so these check for a hit then pass through to the normal out-of-line
   phys routines.  */
static uint32_t watch_mem_readb(void *opaque, hwaddr addr)
{
    check_watchpoint(addr & ~TARGET_PAGE_MASK, ~0x0, BP_MEM_READ);
    return ldub_phys(addr);
}

static uint32_t watch_mem_readw(void *opaque, hwaddr addr)
{
    check_watchpoint(addr & ~TARGET_PAGE_MASK, ~0x1, BP_MEM_READ);
    return lduw_phys(addr);
}

static uint32_t watch_mem_readl(void *opaque, hwaddr addr)
{
    check_watchpoint(addr & ~TARGET_PAGE_MASK, ~0x3, BP_MEM_READ);
    return ldl_phys(addr);
}

static void watch_mem_writeb(void *opaque, hwaddr addr,
                             uint32_t val)
{
    check_watchpoint(addr & ~TARGET_PAGE_MASK, ~0x0, BP_MEM_WRITE);
    stb_phys(addr, val);
}

static void watch_mem_writew(void *opaque, hwaddr addr,
                             uint32_t val)
{
    check_watchpoint(addr & ~TARGET_PAGE_MASK, ~0x1, BP_MEM_WRITE);
    stw_phys(addr, val);
}

static void watch_mem_writel(void *opaque, hwaddr addr,
                             uint32_t val)
{
    check_watchpoint(addr & ~TARGET_PAGE_MASK, ~0x3, BP_MEM_WRITE);
    stl_phys(addr, val);
}

static CPUReadMemoryFunc * const watch_mem_read[3] = {
    watch_mem_readb,
    watch_mem_readw,
    watch_mem_readl,
};

static CPUWriteMemoryFunc * const watch_mem_write[3] = {
    watch_mem_writeb,
    watch_mem_writew,
    watch_mem_writel,
};

static inline uint32_t subpage_readlen (subpage_t *mmio, hwaddr addr,
                                 unsigned int len)
{
    uint32_t ret;
    unsigned int idx;

    idx = SUBPAGE_IDX(addr);
#if defined(DEBUG_SUBPAGE)
    printf("%s: subpage %p len %d addr " TARGET_FMT_plx " idx %d\n", __func__,
           mmio, len, addr, idx);
#endif
    ret = (**mmio->mem_read[idx][len])(mmio->opaque[idx][0][len],
                                       addr + mmio->region_offset[idx][0][len]);

    return ret;
}

static inline void subpage_writelen (subpage_t *mmio, hwaddr addr,
                              uint32_t value, unsigned int len)
{
    unsigned int idx;

    idx = SUBPAGE_IDX(addr);
#if defined(DEBUG_SUBPAGE)
    printf("%s: subpage %p len %d addr " TARGET_FMT_plx " idx %d value %08x\n", __func__,
           mmio, len, addr, idx, value);
#endif
    (**mmio->mem_write[idx][len])(mmio->opaque[idx][1][len],
                                  addr + mmio->region_offset[idx][1][len],
                                  value);
}

static uint32_t subpage_readb (void *opaque, hwaddr addr)
{
#if defined(DEBUG_SUBPAGE)
    printf("%s: addr " TARGET_FMT_plx "\n", __func__, addr);
#endif

    return subpage_readlen(opaque, addr, 0);
}

static void subpage_writeb (void *opaque, hwaddr addr,
                            uint32_t value)
{
#if defined(DEBUG_SUBPAGE)
    printf("%s: addr " TARGET_FMT_plx " val %08x\n", __func__, addr, value);
#endif
    subpage_writelen(opaque, addr, value, 0);
}

static uint32_t subpage_readw (void *opaque, hwaddr addr)
{
#if defined(DEBUG_SUBPAGE)
    printf("%s: addr " TARGET_FMT_plx "\n", __func__, addr);
#endif

    return subpage_readlen(opaque, addr, 1);
}

static void subpage_writew (void *opaque, hwaddr addr,
                            uint32_t value)
{
#if defined(DEBUG_SUBPAGE)
    printf("%s: addr " TARGET_FMT_plx " val %08x\n", __func__, addr, value);
#endif
    subpage_writelen(opaque, addr, value, 1);
}

static uint32_t subpage_readl (void *opaque, hwaddr addr)
{
#if defined(DEBUG_SUBPAGE)
    printf("%s: addr " TARGET_FMT_plx "\n", __func__, addr);
#endif

    return subpage_readlen(opaque, addr, 2);
}

static void subpage_writel (void *opaque,
                         hwaddr addr, uint32_t value)
{
#if defined(DEBUG_SUBPAGE)
    printf("%s: addr " TARGET_FMT_plx " val %08x\n", __func__, addr, value);
#endif
    subpage_writelen(opaque, addr, value, 2);
}

static CPUReadMemoryFunc * const subpage_read[] = {
    &subpage_readb,
    &subpage_readw,
    &subpage_readl,
};

static CPUWriteMemoryFunc * const subpage_write[] = {
    &subpage_writeb,
    &subpage_writew,
    &subpage_writel,
};

static int subpage_register (subpage_t *mmio, uint32_t start, uint32_t end,
                             ram_addr_t memory, ram_addr_t region_offset)
{
    int idx, eidx;
    unsigned int i;

    if (start >= TARGET_PAGE_SIZE || end >= TARGET_PAGE_SIZE)
        return -1;
    idx = SUBPAGE_IDX(start);
    eidx = SUBPAGE_IDX(end);
#if defined(DEBUG_SUBPAGE)
    printf("%s: %p start %08x end %08x idx %08x eidx %08x mem %ld\n", __func__,
           mmio, start, end, idx, eidx, memory);
#endif
    memory >>= IO_MEM_SHIFT;
    for (; idx <= eidx; idx++) {
        for (i = 0; i < 4; i++) {
            if (_io_mem_read[memory][i]) {
                mmio->mem_read[idx][i] = &_io_mem_read[memory][i];
                mmio->opaque[idx][0][i] = io_mem_opaque[memory];
                mmio->region_offset[idx][0][i] = region_offset;
            }
            if (_io_mem_write[memory][i]) {
                mmio->mem_write[idx][i] = &_io_mem_write[memory][i];
                mmio->opaque[idx][1][i] = io_mem_opaque[memory];
                mmio->region_offset[idx][1][i] = region_offset;
            }
        }
    }

    return 0;
}

static void *subpage_init (hwaddr base, ram_addr_t *phys,
                           ram_addr_t orig_memory, ram_addr_t region_offset)
{
    subpage_t *mmio;
    int subpage_memory;

    mmio = g_malloc0(sizeof(subpage_t));

    mmio->base = base;
    subpage_memory = cpu_register_io_memory(subpage_read, subpage_write, mmio);
#if defined(DEBUG_SUBPAGE)
    printf("%s: %p base " TARGET_FMT_plx " len %08x %d\n", __func__,
           mmio, base, TARGET_PAGE_SIZE, subpage_memory);
#endif
    *phys = subpage_memory | IO_MEM_SUBPAGE;
    subpage_register(mmio, 0, TARGET_PAGE_SIZE - 1, orig_memory,
                         region_offset);

    return mmio;
}

static int get_free_io_mem_idx(void)
{
    int i;

    for (i = 0; i<IO_MEM_NB_ENTRIES; i++)
        if (!io_mem_used[i]) {
            io_mem_used[i] = 1;
            return i;
        }
    fprintf(stderr, "RAN out out io_mem_idx, max %d !\n", IO_MEM_NB_ENTRIES);
    return -1;
}

/* mem_read and mem_write are arrays of functions containing the
   function to access byte (index 0), word (index 1) and dword (index
   2). Functions can be omitted with a NULL function pointer.
   If io_index is non zero, the corresponding io zone is
   modified. If it is zero, a new io zone is allocated. The return
   value can be used with cpu_register_physical_memory(). (-1) is
   returned if error. */
static int cpu_register_io_memory_fixed(int io_index,
                                        CPUReadMemoryFunc * const *mem_read,
                                        CPUWriteMemoryFunc * const *mem_write,
                                        void *opaque)
{
    int i, subwidth = 0;

    if (io_index <= 0) {
        io_index = get_free_io_mem_idx();
        if (io_index == -1)
            return io_index;
    } else {
        io_index >>= IO_MEM_SHIFT;
        if (io_index >= IO_MEM_NB_ENTRIES)
            return -1;
    }

    for(i = 0;i < 3; i++) {
        if (!mem_read[i] || !mem_write[i])
            subwidth = IO_MEM_SUBWIDTH;
        _io_mem_read[io_index][i] = mem_read[i];
        _io_mem_write[io_index][i] = mem_write[i];
    }
    io_mem_opaque[io_index] = opaque;
    return (io_index << IO_MEM_SHIFT) | subwidth;
}

int cpu_register_io_memory(CPUReadMemoryFunc * const *mem_read,
                           CPUWriteMemoryFunc * const *mem_write,
                           void *opaque)
{
    return cpu_register_io_memory_fixed(0, mem_read, mem_write, opaque);
}

void cpu_unregister_io_memory(int io_table_address)
{
    int i;
    int io_index = io_table_address >> IO_MEM_SHIFT;

    for (i=0;i < 3; i++) {
        _io_mem_read[io_index][i] = unassigned_mem_read[i];
        _io_mem_write[io_index][i] = unassigned_mem_write[i];
    }
    io_mem_opaque[io_index] = NULL;
    io_mem_used[io_index] = 0;
}

static void io_mem_init(void)
{
    int i;

    cpu_register_io_memory_fixed(IO_MEM_ROM, error_mem_read, unassigned_mem_write, NULL);
    cpu_register_io_memory_fixed(IO_MEM_UNASSIGNED, unassigned_mem_read, unassigned_mem_write, NULL);
    cpu_register_io_memory_fixed(IO_MEM_NOTDIRTY, error_mem_read, notdirty_mem_write, NULL);
    for (i=0; i<5; i++)
        io_mem_used[i] = 1;

    io_mem_watch = cpu_register_io_memory(watch_mem_read,
                                          watch_mem_write, NULL);
}

#endif /* !defined(CONFIG_USER_ONLY) */

/* physical memory access (slow version, mainly for debug) */
#if defined(CONFIG_USER_ONLY)
void cpu_physical_memory_rw(hwaddr addr, void *buf,
                            int len, int is_write)
{
    int l, flags;
    target_ulong page;
    void * p;

    while (len > 0) {
        page = addr & TARGET_PAGE_MASK;
        l = (page + TARGET_PAGE_SIZE) - addr;
        if (l > len)
            l = len;
        flags = page_get_flags(page);
        if (!(flags & PAGE_VALID))
            return;
        if (is_write) {
            if (!(flags & PAGE_WRITE))
                return;
            /* XXX: this code should not depend on lock_user */
            if (!(p = lock_user(VERIFY_WRITE, addr, l, 0)))
                /* FIXME - should this return an error rather than just fail? */
                return;
            memcpy(p, buf, l);
            unlock_user(p, addr, l);
        } else {
            if (!(flags & PAGE_READ))
                return;
            /* XXX: this code should not depend on lock_user */
            if (!(p = lock_user(VERIFY_READ, addr, l, 1)))
                /* FIXME - should this return an error rather than just fail? */
                return;
            memcpy(buf, p, l);
            unlock_user(p, addr, 0);
        }
        len -= l;
        buf += l;
        addr += l;
    }
}

#else

static void invalidate_and_set_dirty(hwaddr addr,
                                     hwaddr length)
{
    if (!cpu_physical_memory_is_dirty(addr)) {
        /* invalidate code */
        tb_invalidate_phys_page_range(addr, addr + length, 0);
        /* set dirty bit */
        cpu_physical_memory_set_dirty_flags(addr, (0xff & ~CODE_DIRTY_FLAG));
    }
}

void cpu_physical_memory_rw(hwaddr addr, void *buf,
                            int len, int is_write)
{
    int l, io_index;
    uint8_t *ptr;
    uint32_t val;
    hwaddr page;
    ram_addr_t pd;
    uint8_t* buf8 = (uint8_t*)buf;
    PhysPageDesc *p;

    while (len > 0) {
        page = addr & TARGET_PAGE_MASK;
        l = (page + TARGET_PAGE_SIZE) - addr;
        if (l > len)
            l = len;
        p = phys_page_find(page >> TARGET_PAGE_BITS);
        if (!p) {
            pd = IO_MEM_UNASSIGNED;
        } else {
            pd = p->phys_offset;
        }

        if (is_write) {
            if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM) {
                hwaddr addr1 = addr;
                io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
                if (p)
                    addr1 = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
                /* XXX: could force cpu_single_env to NULL to avoid
                   potential bugs */
                if (l >= 4 && ((addr1 & 3) == 0)) {
                    /* 32 bit write access */
                    val = ldl_p(buf8);
                    io_mem_write(io_index, addr1, val, 4);
                    l = 4;
                } else if (l >= 2 && ((addr1 & 1) == 0)) {
                    /* 16 bit write access */
                    val = lduw_p(buf8);
                    io_mem_write(io_index, addr1, val, 2);
                    l = 2;
                } else {
                    /* 8 bit write access */
                    val = ldub_p(buf8);
                    io_mem_write(io_index, addr1, val, 1);
                    l = 1;
                }
            } else {
                ram_addr_t addr1;
                addr1 = (pd & TARGET_PAGE_MASK) + (addr & ~TARGET_PAGE_MASK);
                /* RAM case */
                ptr = qemu_get_ram_ptr(addr1);
                memcpy(ptr, buf8, l);
                invalidate_and_set_dirty(addr1, l);
            }
        } else {
            if ((pd & ~TARGET_PAGE_MASK) > IO_MEM_ROM &&
                !(pd & IO_MEM_ROMD)) {
                hwaddr addr1 = addr;
                /* I/O case */
                io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
                if (p)
                    addr1 = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
                if (l >= 4 && ((addr1 & 3) == 0)) {
                    /* 32 bit read access */
                    val = io_mem_read(io_index, addr1, 4);
                    stl_p(buf8, val);
                    l = 4;
                } else if (l >= 2 && ((addr1 & 1) == 0)) {
                    /* 16 bit read access */
                    val = io_mem_read(io_index, addr1, 2);
                    stw_p(buf8, val);
                    l = 2;
                } else {
                    /* 8 bit read access */
                    val = io_mem_read(io_index, addr1, 1);
                    stb_p(buf8, val);
                    l = 1;
                }
            } else {
                /* RAM case */
                ptr = qemu_get_ram_ptr(pd & TARGET_PAGE_MASK) +
                    (addr & ~TARGET_PAGE_MASK);
                memcpy(buf8, ptr, l);
            }
        }
        len -= l;
        buf8 += l;
        addr += l;
    }
}

/* used for ROM loading : can write in RAM and ROM */
void cpu_physical_memory_write_rom(hwaddr addr,
                                   const void *buf, int len)
{
    int l;
    uint8_t *ptr;
    hwaddr page;
    unsigned long pd;
    const uint8_t* buf8 = (const uint8_t*)buf;
    PhysPageDesc *p;

    while (len > 0) {
        page = addr & TARGET_PAGE_MASK;
        l = (page + TARGET_PAGE_SIZE) - addr;
        if (l > len)
            l = len;
        p = phys_page_find(page >> TARGET_PAGE_BITS);
        if (!p) {
            pd = IO_MEM_UNASSIGNED;
        } else {
            pd = p->phys_offset;
        }

        if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM &&
            (pd & ~TARGET_PAGE_MASK) != IO_MEM_ROM &&
            !(pd & IO_MEM_ROMD)) {
            /* do nothing */
        } else {
            unsigned long addr1;
            addr1 = (pd & TARGET_PAGE_MASK) + (addr & ~TARGET_PAGE_MASK);
            /* ROM/RAM case */
            ptr = qemu_get_ram_ptr(addr1);
            memcpy(ptr, buf8, l);
            invalidate_and_set_dirty(addr1, l);
        }
        len -= l;
        buf8 += l;
        addr += l;
    }
}

typedef struct {
    void *buffer;
    hwaddr addr;
    hwaddr len;
} BounceBuffer;

static BounceBuffer bounce;

typedef struct MapClient {
    void *opaque;
    void (*callback)(void *opaque);
    QLIST_ENTRY(MapClient) link;
} MapClient;

static QLIST_HEAD(map_client_list, MapClient) map_client_list
    = QLIST_HEAD_INITIALIZER(map_client_list);

void *cpu_register_map_client(void *opaque, void (*callback)(void *opaque))
{
    MapClient *client = g_malloc(sizeof(*client));

    client->opaque = opaque;
    client->callback = callback;
    QLIST_INSERT_HEAD(&map_client_list, client, link);
    return client;
}

static void cpu_unregister_map_client(void *_client)
{
    MapClient *client = (MapClient *)_client;

    QLIST_REMOVE(client, link);
    g_free(client);
}

static void cpu_notify_map_clients(void)
{
    MapClient *client;

    while (!QLIST_EMPTY(&map_client_list)) {
        client = QLIST_FIRST(&map_client_list);
        client->callback(client->opaque);
        cpu_unregister_map_client(client);
    }
}

/* Map a physical memory region into a host virtual address.
 * May map a subset of the requested range, given by and returned in *plen.
 * May return NULL if resources needed to perform the mapping are exhausted.
 * Use only for reads OR writes - not for read-modify-write operations.
 * Use cpu_register_map_client() to know when retrying the map operation is
 * likely to succeed.
 */
void *cpu_physical_memory_map(hwaddr addr,
                              hwaddr *plen,
                              int is_write)
{
    hwaddr len = *plen;
    hwaddr done = 0;
    int l;
    uint8_t *ret = NULL;
    uint8_t *ptr;
    hwaddr page;
    unsigned long pd;
    PhysPageDesc *p;
    unsigned long addr1;

    while (len > 0) {
        page = addr & TARGET_PAGE_MASK;
        l = (page + TARGET_PAGE_SIZE) - addr;
        if (l > len)
            l = len;
        p = phys_page_find(page >> TARGET_PAGE_BITS);
        if (!p) {
            pd = IO_MEM_UNASSIGNED;
        } else {
            pd = p->phys_offset;
        }

        if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM) {
            if (done || bounce.buffer) {
                break;
            }
            bounce.buffer = qemu_memalign(TARGET_PAGE_SIZE, TARGET_PAGE_SIZE);
            bounce.addr = addr;
            bounce.len = l;
            if (!is_write) {
                cpu_physical_memory_read(addr, bounce.buffer, l);
            }
            ptr = bounce.buffer;
        } else {
            addr1 = (pd & TARGET_PAGE_MASK) + (addr & ~TARGET_PAGE_MASK);
            ptr = qemu_get_ram_ptr(addr1);
        }
        if (!done) {
            ret = ptr;
        } else if (ret + done != ptr) {
            break;
        }

        len -= l;
        addr += l;
        done += l;
    }
    *plen = done;
    return ret;
}

/* Unmaps a memory region previously mapped by cpu_physical_memory_map().
 * Will also mark the memory as dirty if is_write == 1.  access_len gives
 * the amount of memory that was actually read or written by the caller.
 */
void cpu_physical_memory_unmap(void *buffer, hwaddr len,
                               int is_write, hwaddr access_len)
{
    if (buffer != bounce.buffer) {
        if (is_write) {
            ram_addr_t addr1 = qemu_ram_addr_from_host_nofail(buffer);
            while (access_len) {
                unsigned l;
                l = TARGET_PAGE_SIZE;
                if (l > access_len)
                    l = access_len;
                invalidate_and_set_dirty(addr1, l);
                addr1 += l;
                access_len -= l;
            }
        }
        return;
    }
    if (is_write) {
        cpu_physical_memory_write(bounce.addr, bounce.buffer, access_len);
    }
    qemu_vfree(bounce.buffer);
    bounce.buffer = NULL;
    cpu_notify_map_clients();
}

/* warning: addr must be aligned */
static inline uint32_t ldl_phys_internal(hwaddr addr,
                                         enum device_endian endian)
{
    int io_index;
    uint8_t *ptr;
    uint32_t val;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) > IO_MEM_ROM &&
        !(pd & IO_MEM_ROMD)) {
        /* I/O case */
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
        val = io_mem_read(io_index, addr, 4);
#if defined(TARGET_WORDS_BIGENDIAN)
        if (endian == DEVICE_LITTLE_ENDIAN) {
            val = bswap32(val);
        }
#else
        if (endian == DEVICE_BIG_ENDIAN) {
            val = bswap32(val);
        }
#endif
    } else {
        /* RAM case */
        ptr = qemu_get_ram_ptr(pd & TARGET_PAGE_MASK) +
            (addr & ~TARGET_PAGE_MASK);
        switch (endian) {
            case DEVICE_LITTLE_ENDIAN:
                val = ldl_le_p(ptr);
                break;
            case DEVICE_BIG_ENDIAN:
                val = ldl_be_p(ptr);
                break;
            default:
                val = ldl_p(ptr);
                break;
        }
    }
    return val;
}

uint32_t ldl_phys(hwaddr addr)
{
    return ldl_phys_internal(addr, DEVICE_NATIVE_ENDIAN);
}

uint32_t ldl_le_phys(hwaddr addr)
{
    return ldl_phys_internal(addr, DEVICE_LITTLE_ENDIAN);
}

uint32_t ldl_be_phys(hwaddr addr)
{
    return ldl_phys_internal(addr, DEVICE_BIG_ENDIAN);
}

/* warning: addr must be aligned */
static inline uint64_t ldq_phys_internal(hwaddr addr,
                                         enum device_endian endian)
{
    int io_index;
    uint8_t *ptr;
    uint64_t val;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) > IO_MEM_ROM &&
        !(pd & IO_MEM_ROMD)) {
        /* I/O case */
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;

        /* XXX This is broken when device endian != cpu endian.
               Fix and add "endian" variable check */
#ifdef TARGET_WORDS_BIGENDIAN
        val = (uint64_t)io_mem_read(io_index, addr, 4) << 32;
        val |= io_mem_read(io_index, addr + 4, 4);
#else
        val = io_mem_read(io_index, addr, 4);
        val |= (uint64_t)io_mem_read(io_index, addr + 4, 4) << 32;
#endif
    } else {
        /* RAM case */
        ptr = qemu_get_ram_ptr(pd & TARGET_PAGE_MASK) +
            (addr & ~TARGET_PAGE_MASK);
        switch (endian) {
        case DEVICE_LITTLE_ENDIAN:
            val = ldq_le_p(ptr);
            break;
        case DEVICE_BIG_ENDIAN:
            val = ldq_be_p(ptr);
            break;
        default:
            val = ldq_p(ptr);
            break;
        }
    }
    return val;
}

uint64_t ldq_phys(hwaddr addr)
{
    return ldq_phys_internal(addr, DEVICE_NATIVE_ENDIAN);
}

uint64_t ldq_le_phys(hwaddr addr)
{
    return ldq_phys_internal(addr, DEVICE_LITTLE_ENDIAN);
}

uint64_t ldq_be_phys(hwaddr addr)
{
    return ldq_phys_internal(addr, DEVICE_BIG_ENDIAN);
}

/* XXX: optimize */
uint32_t ldub_phys(hwaddr addr)
{
    uint8_t val;
    cpu_physical_memory_read(addr, &val, 1);
    return val;
}

/* XXX: optimize */
static inline uint32_t lduw_phys_internal(hwaddr addr,
                                          enum device_endian endian)
{
    int io_index;
    uint8_t *ptr;
    uint64_t val;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) > IO_MEM_ROM &&
        !(pd & IO_MEM_ROMD)) {
        /* I/O case */
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
        val = io_mem_read(io_index, addr, 2);
#if defined(TARGET_WORDS_BIGENDIAN)
        if (endian == DEVICE_LITTLE_ENDIAN) {
            val = bswap16(val);
        }
#else
        if (endian == DEVICE_BIG_ENDIAN) {
            val = bswap16(val);
        }
#endif
    } else {
        /* RAM case */
        ptr = qemu_get_ram_ptr(pd & TARGET_PAGE_MASK) +
            (addr & ~TARGET_PAGE_MASK);
        switch (endian) {
        case DEVICE_LITTLE_ENDIAN:
            val = lduw_le_p(ptr);
            break;
        case DEVICE_BIG_ENDIAN:
            val = lduw_be_p(ptr);
            break;
        default:
            val = lduw_p(ptr);
            break;
        }
    }
    return val;
}

uint32_t lduw_phys(hwaddr addr)
{
    return lduw_phys_internal(addr, DEVICE_NATIVE_ENDIAN);
}

uint32_t lduw_le_phys(hwaddr addr)
{
    return lduw_phys_internal(addr, DEVICE_LITTLE_ENDIAN);
}

uint32_t lduw_be_phys(hwaddr addr)
{
    return lduw_phys_internal(addr, DEVICE_BIG_ENDIAN);
}

/* warning: addr must be aligned. The ram page is not masked as dirty
   and the code inside is not invalidated. It is useful if the dirty
   bits are used to track modified PTEs */
void stl_phys_notdirty(hwaddr addr, uint32_t val)
{
    int io_index;
    uint8_t *ptr;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM) {
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
        io_mem_write(io_index, addr, val, 4);
    } else {
        unsigned long addr1 = (pd & TARGET_PAGE_MASK) + (addr & ~TARGET_PAGE_MASK);
        ptr = qemu_get_ram_ptr(addr1);
        stl_p(ptr, val);

        if (unlikely(in_migration)) {
            if (!cpu_physical_memory_is_dirty(addr1)) {
                /* invalidate code */
                tb_invalidate_phys_page_range(addr1, addr1 + 4, 0);
                /* set dirty bit */
                cpu_physical_memory_set_dirty_flags(
                    addr1, (0xff & ~CODE_DIRTY_FLAG));
            }
        }
    }
}

void stq_phys_notdirty(hwaddr addr, uint64_t val)
{
    int io_index;
    uint8_t *ptr;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM) {
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
#ifdef TARGET_WORDS_BIGENDIAN
        io_mem_write(io_index, addr, val >> 32, 4);
        io_mem_write(io_index, addr + 4, val, 4);
#else
        io_mem_write(io_index, addr, val, 4);
        io_mem_write(io_index, addr + 4, val >> 32, 4);
#endif
    } else {
        ptr = qemu_get_ram_ptr(pd & TARGET_PAGE_MASK) +
            (addr & ~TARGET_PAGE_MASK);
        stq_p(ptr, val);
    }
}

/* warning: addr must be aligned */
static inline void stl_phys_internal(hwaddr addr, uint32_t val,
                                     enum device_endian endian)
{
    int io_index;
    uint8_t *ptr;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM) {
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
#if defined(TARGET_WORDS_BIGENDIAN)
        if (endian == DEVICE_LITTLE_ENDIAN) {
            val = bswap32(val);
        }
#else
        if (endian == DEVICE_BIG_ENDIAN) {
            val = bswap32(val);
        }
#endif
        io_mem_write(io_index, addr, val, 4);
    } else {
        unsigned long addr1;
        addr1 = (pd & TARGET_PAGE_MASK) + (addr & ~TARGET_PAGE_MASK);
        /* RAM case */
        ptr = qemu_get_ram_ptr(addr1);
        switch (endian) {
        case DEVICE_LITTLE_ENDIAN:
            stl_le_p(ptr, val);
            break;
        case DEVICE_BIG_ENDIAN:
            stl_be_p(ptr, val);
            break;
        default:
            stl_p(ptr, val);
            break;
        }
        invalidate_and_set_dirty(addr1, 4);
    }
}

void stl_phys(hwaddr addr, uint32_t val)
{
    stl_phys_internal(addr, val, DEVICE_NATIVE_ENDIAN);
}

void stl_le_phys(hwaddr addr, uint32_t val)
{
    stl_phys_internal(addr, val, DEVICE_LITTLE_ENDIAN);
}

void stl_be_phys(hwaddr addr, uint32_t val)
{
    stl_phys_internal(addr, val, DEVICE_BIG_ENDIAN);
}

/* XXX: optimize */
void stb_phys(hwaddr addr, uint32_t val)
{
    uint8_t v = val;
    cpu_physical_memory_write(addr, &v, 1);
}

/* XXX: optimize */
static inline void stw_phys_internal(hwaddr addr, uint32_t val,
                                     enum device_endian endian)
{
    int io_index;
    uint8_t *ptr;
    unsigned long pd;
    PhysPageDesc *p;

    p = phys_page_find(addr >> TARGET_PAGE_BITS);
    if (!p) {
        pd = IO_MEM_UNASSIGNED;
    } else {
        pd = p->phys_offset;
    }

    if ((pd & ~TARGET_PAGE_MASK) != IO_MEM_RAM) {
        io_index = (pd >> IO_MEM_SHIFT) & (IO_MEM_NB_ENTRIES - 1);
        if (p)
            addr = (addr & ~TARGET_PAGE_MASK) + p->region_offset;
#if defined(TARGET_WORDS_BIGENDIAN)
        if (endian == DEVICE_LITTLE_ENDIAN) {
            val = bswap16(val);
        }
#else
        if (endian == DEVICE_BIG_ENDIAN) {
            val = bswap16(val);
        }
#endif
        io_mem_write(io_index, addr, val, 2);
    } else {
        unsigned long addr1;
        addr1 = (pd & TARGET_PAGE_MASK) + (addr & ~TARGET_PAGE_MASK);
        /* RAM case */
        ptr = qemu_get_ram_ptr(addr1);
        switch (endian) {
        case DEVICE_LITTLE_ENDIAN:
            stw_le_p(ptr, val);
            break;
        case DEVICE_BIG_ENDIAN:
            stw_be_p(ptr, val);
            break;
        default:
            stw_p(ptr, val);
            break;
        }
        if (!cpu_physical_memory_is_dirty(addr1)) {
            /* invalidate code */
            tb_invalidate_phys_page_range(addr1, addr1 + 2, 0);
            /* set dirty bit */
            cpu_physical_memory_set_dirty_flags(addr1,
                (0xff & ~CODE_DIRTY_FLAG));
        }
    }
}

void stw_phys(hwaddr addr, uint32_t val)
{
    stw_phys_internal(addr, val, DEVICE_NATIVE_ENDIAN);
}

void stw_le_phys(hwaddr addr, uint32_t val)
{
    stw_phys_internal(addr, val, DEVICE_LITTLE_ENDIAN);
}

void stw_be_phys(hwaddr addr, uint32_t val)
{
    stw_phys_internal(addr, val, DEVICE_BIG_ENDIAN);
}

/* XXX: optimize */
void stq_phys(hwaddr addr, uint64_t val)
{
    val = tswap64(val);
    cpu_physical_memory_write(addr, &val, 8);
}


void stq_le_phys(hwaddr addr, uint64_t val)
{
    val = cpu_to_le64(val);
    cpu_physical_memory_write(addr, &val, 8);
}

void stq_be_phys(hwaddr addr, uint64_t val)
{
    val = cpu_to_be64(val);
    cpu_physical_memory_write(addr, &val, 8);
}

#endif

/* virtual memory access for debug (includes writing to ROM) */
int cpu_memory_rw_debug(CPUState *cpu, target_ulong addr,
                        void *buf, int len, int is_write)
{
    int l;
    hwaddr phys_addr;
    target_ulong page;
    uint8_t* buf8 = (uint8_t*)buf;
    CPUArchState *env = cpu->env_ptr;

    while (len > 0) {
        page = addr & TARGET_PAGE_MASK;
        phys_addr = cpu_get_phys_page_debug(env, page);
        /* if no physical page mapped, return an error */
        if (phys_addr == -1)
            return -1;
        l = (page + TARGET_PAGE_SIZE) - addr;
        if (l > len)
            l = len;
        phys_addr += (addr & ~TARGET_PAGE_MASK);
#if !defined(CONFIG_USER_ONLY)
        if (is_write)
            cpu_physical_memory_write_rom(phys_addr, buf8, l);
        else
#endif
            cpu_physical_memory_rw(phys_addr, buf8, l, is_write);
        len -= l;
        buf8 += l;
        addr += l;
    }
    return 0;
}
