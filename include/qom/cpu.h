#ifndef QEMU_CPU_H
#define QEMU_CPU_H

#include <signal.h>
#include "hw/qdev-core.h"
#include "exec/hwaddr.h"
#include "qemu/queue.h"
#include "qemu/thread.h"
#include "qemu/tls.h"
#include "qemu/typedefs.h"


typedef int (*WriteCoreDumpFunction)(void *buf, size_t size, void *opaque);

/**
 * vaddr:
 * Type wide enough to contain any #target_ulong virtual address.
 */
typedef uint64_t vaddr;
#define VADDR_PRId PRId64
#define VADDR_PRIu PRIu64
#define VADDR_PRIo PRIo64
#define VADDR_PRIx PRIx64
#define VADDR_PRIX PRIX64
#define VADDR_MAX UINT64_MAX

typedef struct CPUState CPUState;

typedef void (*CPUUnassignedAccess)(CPUState *cpu, hwaddr addr,
                                    bool is_write, bool is_exec, int opaque,
                                    unsigned size);

struct TranslationBlock;

// TODO(digit): Make this a proper QOM object that inherits from
// DeviceState/DeviceClass.
struct CPUState {
    int nr_cores;  /* number of cores within this CPU package */
    int nr_threads;/* number of threads within this CPU */
    int numa_node; /* NUMA node this cpu is belonging to  */


    struct QemuThread *thread;

    uint32_t host_tid; /* host thread ID */
    int running; /* Nonzero if cpu is currently running(usermode).  */
    struct QemuCond *halt_cond;
    struct qemu_work_item *queued_work_first, *queued_work_last;

    uint32_t created;
    uint32_t stop;   /* Stop request */
    uint32_t stopped; /* Artificially stopped */

    volatile sig_atomic_t exit_request;
    uint32_t interrupt_request;

    void *env_ptr; /* CPUArchState */
    struct TranslationBlock *current_tb; /* currently executing TB  */
    int singlestep_enabled;
    struct GDBRegisterState *gdb_regs;
    QTAILQ_ENTRY(CPUState) node;   /* next CPU sharing TB cache */

    const char *cpu_model_str;

    int kvm_fd;
    int kvm_vcpu_dirty;
    struct KVMState *kvm_state;
    struct kvm_run *kvm_run;

    struct hax_vcpu_state *hax_vcpu;

    /* TODO Move common fields from CPUArchState here. */
    int cpu_index; /* used by alpha TCG */
    uint32_t halted; /* used by alpha, cris, ppc TCG */
};

#define CPU(obj)  ((CPUState*)(obj))

QTAILQ_HEAD(CPUTailQ, CPUState);
extern struct CPUTailQ cpus;
#define CPU_NEXT(cpu) QTAILQ_NEXT(cpu, node)
#define CPU_FOREACH(cpu) QTAILQ_FOREACH(cpu, &cpus, node)
#define CPU_FOREACH_SAFE(cpu, next_cpu) \
    QTAILQ_FOREACH_SAFE(cpu, &cpus, node, next_cpu)
#define first_cpu QTAILQ_FIRST(&cpus)

DECLARE_TLS(CPUState *, current_cpu);
#define current_cpu tls_var(current_cpu)

// TODO(digit): Remove this.
#define cpu_single_env ((CPUArchState*)current_cpu->env_ptr)

#endif  // QEMU_CPU_H
