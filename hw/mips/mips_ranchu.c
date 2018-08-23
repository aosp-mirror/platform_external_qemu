#include "qemu/osdep.h"
#include "cpu.h"
#include "elf.h"
#include "exec/address-spaces.h"
#include "hw/boards.h"
#include "hw/devices.h"
#include "hw/intc/goldfish_pic.h"
#include "hw/irq.h"
#include "hw/loader.h"
#include "hw/mips/bios.h"
#include "hw/mips/cpudevs.h"
#include "hw/mips/cps.h"
#include "hw/mips/mips.h"
#include "hw/sysbus.h"
#include "monitor/monitor.h"
#include "net/net.h"
#include "qapi/error.h"
#include "qemu/bitops.h"
#include "qemu/config-file.h"
#include "qemu/cutils.h"
#include "qemu/error-report.h"
#include "chardev/char.h"
#include "sysemu/device_tree.h"
#include "sysemu/kvm.h"
#include "sysemu/ranchu.h"
#include "sysemu/sysemu.h"

#define FATAL(fmt, ...)                                                 \
do {                                                                    \
    fprintf(stderr, "%s:%d : " fmt, __func__, __LINE__, ##__VA_ARGS__); \
    exit(-errno);                                                       \
} while (0)

#define LOGI(fmt, ...)                                                  \
do {                                                                    \
    fprintf(stdout, "%s() : " fmt, __func__, ##__VA_ARGS__);            \
} while (0)

#define MIPS_CPU_SAVE_VERSION 1

#define HIGHMEM_OFFSET    0x20000000
#define GOLDFISH_IO_SPACE 0x1f000000
#define RESET_ADDRESS     0x1fc00000

#define RANCHU_BIOS_SIZE  0x100000    /* 1024 * 1024 = 1 MB */

/* Store the environment arguments table in the second page from the top
 * of the bios memory. Two pages are required since the environment
 * arguments table is 7232 bytes in size.
 */
#define ENVP_ADDR         cpu_mips_phys_to_kseg0(NULL,                   \
                                                 RESET_ADDRESS +         \
                                                 RANCHU_BIOS_SIZE -      \
                                                 (2 * TARGET_PAGE_SIZE))
#define ENVP_NB_ENTRIES   16
#define ENVP_ENTRY_SIZE   448

#define VIRTIO_TRANSPORTS 16
#define MAX_GF_TTYS       3

#define HIGH(addr) ((addr >> 16) & 0xFFFF)
#define LOW(addr) (addr & 0xFFFF)

#define TYPE_MIPS_RANCHU "mips-ranchu"
#define MIPS_RANCHU(obj) OBJECT_CHECK(RanchuState, (obj), TYPE_MIPS_RANCHU)
#define MIPS_RANCHU_REV "1"

struct mips_boot_info {
    const char *kernel_filename;
    const char *kernel_cmdline;
    const char *initrd_filename;
    uint64_t ram_size;
    uint64_t highmem_size;
    bool use_uhi;
};

typedef struct machine_params {
    struct mips_boot_info bootinfo;
    MIPSCPU *cpu;
    int fdt_size;
    void *fdt;
    hwaddr fdt_addr;
} ranchu_params;

typedef struct {
    SysBusDevice parent_obj;

    MIPSCPSState *cps;
    qemu_irq *gfpic;
    ranchu_params *rp;
} RanchuState;

typedef qemu_irq (*get_qemu_irq_cb)(RanchuState *s, int irq);

get_qemu_irq_cb get_irq;

#define MAX_RAM_SIZE_MB 4079UL

enum {
    RANCHU_MIPS_CPU_INTC,
    RANCHU_MIPS_GIC,
    RANCHU_MIPS_CPC,
    RANCHU_GOLDFISH_PIC,
    RANCHU_GOLDFISH_TTY,
    RANCHU_GOLDFISH_RTC,
    RANCHU_GOLDFISH_BATTERY,
    RANCHU_GOLDFISH_FB,
    RANCHU_GOLDFISH_EVDEV,
    RANCHU_GOLDFISH_PIPE,
    RANCHU_GOLDFISH_AUDIO,
    RANCHU_GOLDFISH_SYNC,
    RANCHU_GOLDFISH_RESET,
    RANCHU_GOLDFISH_PSTORE,
    RANCHU_GOLDFISH_TIMER,
    RANCHU_MMIO,
};

/* Power Management Unit commands */
enum {
    GOLDFISH_PM_CMD_GORESET      = 0x42,
    GOLDFISH_PM_CMD_GOSHUTDOWN   = 0x43
};

typedef struct DevMapEntry {
    hwaddr base;
    hwaddr size;
    int irq;
    const char *qemu_name;
    const char *dt_name;
    const char *dt_compatible;
    int dt_num_compatible;
    int dt_phandle;
} DevMapEntry;

static DevMapEntry devmap[] = {
    [RANCHU_MIPS_CPU_INTC] = {
        0, 0, -1, "cpuintc", "cpuintc",
          "mti,cpu-interrupt-controller", 1
        },
    [RANCHU_GOLDFISH_PIC] = {
        GOLDFISH_IO_SPACE, 0x1000, 2,
          "goldfish_pic", "goldfish_pic",
          "google,goldfish-pic\0generic,goldfish-pic", 2
        },
    [RANCHU_MIPS_GIC] = {
        GOLDFISH_IO_SPACE, 0x20000, -1,
        "gic", "gic", "mti,gic", 1
        },
    /* The following region of 64K is reserved for Goldfish pstore device */
    [RANCHU_GOLDFISH_PSTORE] = {
        GOLDFISH_IO_SPACE + 0x20000, 0x10000, -1,
          "goldfish_pstore", "goldfish_pstore",
          "google,goldfish-pstore\0generic,goldfish-pstore", 2
        },
    /* ttyGF0 base address remains hardcoded in the kernel.
     * Early printing (prom_putchar()) relies on finding device
     * mapped on this address. DT can not be used at that early stage
     * for acquiring the base address of the device in the kernel.
     * Repeats for a total of MAX_GF_TTYS
     */
    [RANCHU_GOLDFISH_TTY] = {
        GOLDFISH_IO_SPACE + 0x30000, 0x1000, 2,
          "goldfish_tty", "goldfish_tty",
          "google,goldfish-tty\0generic,goldfish-tty", 2
        },
    /* This is just a fake node to fool kernels < v4.4.
     * It's required during a call to estimate_cpu_frequency().
     * Essentially it will be using Goldfish RTC device.
     * This should be removed once we officially switch to v4.4.
     */
    [RANCHU_GOLDFISH_TIMER] = {
        GOLDFISH_IO_SPACE + 0x33000, 0x1000, 5,
          "goldfish_timer", "goldfish_timer",
          "google,goldfish-timer\0generic,goldfish-timer", 2
        },
    [RANCHU_GOLDFISH_RTC] = {
        GOLDFISH_IO_SPACE + 0x33000, 0x1000, 5,
          "goldfish_rtc", "goldfish_rtc",
          "google,goldfish-rtc\0generic,goldfish-rtc", 2
        },
    [RANCHU_GOLDFISH_BATTERY] = {
        GOLDFISH_IO_SPACE + 0x34000, 0x1000, 6,
          "goldfish_battery", "goldfish_battery",
          "google,goldfish-battery\0generic,goldfish-battery", 2
        },
    [RANCHU_GOLDFISH_FB] = {
        GOLDFISH_IO_SPACE + 0x35000, 0x0100, 7,
          "goldfish_fb", "goldfish_fb",
          "google,goldfish-fb\0generic,goldfish-fb", 2
        },
    [RANCHU_GOLDFISH_EVDEV] = {
        GOLDFISH_IO_SPACE + 0x36000, 0x1000, 8,
          "goldfish-events", "goldfish_events",
          "google,goldfish-events-keypad\0generic,goldfish-events-keypad", 2
        },
    [RANCHU_GOLDFISH_PIPE] = {
        GOLDFISH_IO_SPACE + 0x37000, 0x2000, 9,
          "goldfish_pipe", "android_pipe",
          "google,android-pipe\0generic,android-pipe", 2
        },
    [RANCHU_GOLDFISH_AUDIO] = {
        GOLDFISH_IO_SPACE + 0x39000, 0x0100, 10,
          "goldfish_audio", "goldfish_audio",
          "google,goldfish-audio\0generic,goldfish-audio", 2
        },
    [RANCHU_GOLDFISH_SYNC] = {
        GOLDFISH_IO_SPACE + 0x3A000, 0x2000, 11,
          "goldfish_sync", "goldfish_sync",
          "google,goldfish-sync\0generic,goldfish-sync", 2
        },
    [RANCHU_GOLDFISH_RESET] = {
        GOLDFISH_IO_SPACE + 0x3C000, 0x0100, -1,
          "goldfish_reset", "goldfish_reset",
          "google,goldfish-reset\0generic,goldfish-reset\0syscon\0simple-mfd", 4
        },
    /* ...repeating for a total of VIRTIO_TRANSPORTS, each of that size */
    [RANCHU_MMIO] = {
        GOLDFISH_IO_SPACE + 0x3D000, 0x0200, 16,
        "virtio-mmio", "virtio_mmio", "virtio,mmio", 1
        },
    [RANCHU_MIPS_CPC] = {
        GOLDFISH_IO_SPACE + 0x40000, 0x8000, -1,
        "cpc", "cpc", "mti,mips-cpc", 1
        },
};

#ifdef _WIN32
/*
 * Returns a pointer to the first occurrence of |needle| in |haystack|, or a
 * NULL pointer if |needle| is not part of |haystack|.
 * Intentionally in global namespace. This is already provided by the system
 * C library on Linux and OS X.
 */
static void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen)
{
    register char *cur, *last;
    const char *cl = (const char *)haystack;
    const char *cs = (const char *)needle;

    /* we need something to compare */
    if (haystacklen == 0 || needlelen == 0)
        return NULL;

    /* "needle" must be smaller or equal to "haystack" */
    if (haystacklen < needlelen)
        return NULL;

    /* special case where needlelen == 1 */
    if (needlelen == 1)
        return memchr(haystack, (int)*cs, haystacklen);

    /* the last position where its possible to find "needle" in "haystack" */
    last = (char *)cl + haystacklen - needlelen;

    for (cur = (char *)cl; cur <= last; cur++)
        if (cur[0] == cs[0] && memcmp(cur, cs, needlelen) == 0)
            return cur;

    return NULL;
}
#endif

static QemuDeviceTreeSetupFunc device_tree_setup_func;
void qemu_device_tree_setup_callback(QemuDeviceTreeSetupFunc setup_func)
{
    device_tree_setup_func = setup_func;
}

typedef enum {
    KERNEL_VERSION_4_4_0 = 0x040400,
} KernelVersion;

const char kLinuxVersionStringPrefix[] = "Linux version ";
const size_t kLinuxVersionStringPrefixLen =
        sizeof(kLinuxVersionStringPrefix) - 1U;

static void parse_vmlinux_version_string(const char *versionString,
                                         KernelVersion *kernelVersion) {
    uint32_t parsedVersion;
    int i, err = 0;

    if (strncmp(versionString, kLinuxVersionStringPrefix,
            kLinuxVersionStringPrefixLen)) {
        FATAL("Unsupported kernel version string: %s\n", versionString);
    }
    /* skip the linux prefix string to start parsing the version number */
    versionString += kLinuxVersionStringPrefixLen;

    parsedVersion = 0;
    for (i = 0; i < 3; i++) {
        const char *end;
        unsigned long number;

        /* skip '.' delimeters */
        while (i > 0 && *versionString == '.') {
            versionString++;
        }

        err = qemu_strtoul(versionString, &end, 10, &number);
        if (end == versionString || number > 0xff || err) {
            FATAL("Unsupported kernel version string: %s\n", versionString);
        }
        parsedVersion <<= 8;
        parsedVersion |= number;
        versionString = end;
    }
    *kernelVersion = (KernelVersion)parsedVersion;
}

static void probe_vmlinux_version_string(const char *kernelFilePath, char *dst,
                                         size_t dstLen) {
    const uint8_t *uncompressedKernel = NULL;
    size_t uncompressedKernelLen = 0;
    int ret = 0;
    long kernelFileSize;
    uint8_t *kernelFileData;

    FILE *f = fopen(kernelFilePath, "rb");
    if (!f) {
        FATAL("Failed to fopen() kernel file %s\n", kernelFilePath);
    }

    do {
        if (fseek(f, 0, SEEK_END) < 0) {
            ret = -errno;
            break;
        }

        kernelFileSize = ftell(f);
        if (kernelFileSize < 0) {
            ret = -errno;
            break;
        }

        if (fseek(f, 0, SEEK_SET) < 0) {
            ret = -errno;
            break;
        }

        kernelFileData = malloc((size_t)kernelFileSize);
        if (!kernelFileData) {
            ret = -errno;
            break;
        }

        size_t readLen = fread(kernelFileData, 1, (size_t)kernelFileSize, f);
        if (readLen != (size_t)kernelFileSize) {
            if (feof(f)) {
                ret = -EIO;
            } else {
                ret = -ferror(f);
            }
            free(kernelFileData);
            break;
        }

    } while (0);

    fclose(f);
    if (ret) {
        FATAL("Failed to load kernel file %s errno %d\n", kernelFilePath, ret);
    }

    const char kElfHeader[] = { 0x7f, 'E', 'L', 'F' };

    if (kernelFileSize < sizeof(kElfHeader)) {
        FATAL("Kernel image too short\n");
    }

    const char *versionStringStart = NULL;

    if (0 == memcmp(kElfHeader, kernelFileData, sizeof(kElfHeader))) {
        /* Ranchu uses uncompressed kernel ELF file */
        uncompressedKernel = kernelFileData;
        uncompressedKernelLen = kernelFileSize;
    }

    if (!versionStringStart) {
        versionStringStart = (const char *)memmem(
                uncompressedKernel,
                uncompressedKernelLen,
                kLinuxVersionStringPrefix,
                kLinuxVersionStringPrefixLen);

        if (!versionStringStart) {
            FATAL("Could not find 'Linux version' in kernel!\n");
        }
    }

    snprintf(dst, dstLen, "%s", versionStringStart);
}

static void boot_protocol_detect(ranchu_params *rp, const char *kernelFilePath)
{
    char version_string[256];
    KernelVersion kernelVersion = 0;

    probe_vmlinux_version_string(kernelFilePath, version_string,
                                 sizeof(version_string));
    parse_vmlinux_version_string(version_string, &kernelVersion);

    if (kernelVersion >= KERNEL_VERSION_4_4_0) {
        LOGI("Using UHI boot protocol!\n");
        rp->bootinfo.use_uhi = true;
    } else {
        LOGI("Using legacy boot protocol!\n");
        rp->bootinfo.use_uhi = false;
    }
}

static void ranchu_mips_config(MIPSCPU *cpu)
{
    CPUMIPSState *env = &cpu->env;
    CPUState *cs = CPU(cpu);

    env->mvp->CP0_MVPConf0 |= ((smp_cpus - 1) << CP0MVPC0_PVPE) |
                         ((smp_cpus * cs->nr_threads - 1) << CP0MVPC0_PTC);
}

static void main_cpu_reset(void *opaque)
{
    MIPSCPU *cpu = opaque;

    cpu_reset(CPU(cpu));

    ranchu_mips_config(cpu);
}

static qemu_irq gfpic_get_irq(RanchuState *s, int irq)
{
    return s->gfpic[irq];
}

static qemu_irq gic_get_irq(RanchuState *s, int irq)
{
    return get_cps_irq(s->cps, irq);
}

static void create_cpu_without_cps(const char *cpu_model,
                                   qemu_irq *gfpic_irq)
{
    CPUMIPSState *env;
    MIPSCPU *cpu;
    int i;
    const char* cpu_type = parse_cpu_model(cpu_model);
    for (i = 0; i < smp_cpus; i++) {

        cpu = MIPS_CPU(cpu_create(cpu_type));
        if (cpu == NULL) {
            FATAL("Unable to find CPU definition\n");
        }

        /* Init internal devices */
        cpu_mips_irq_init_cpu(cpu);
        cpu_mips_clock_init(cpu);
        qemu_register_reset(main_cpu_reset, cpu);
    }

    cpu = MIPS_CPU(first_cpu);
    env = &cpu->env;
    *gfpic_irq = env->irq[2];
}

static void create_cps(RanchuState *s, const char *cpu_model)
{
    Error *err = NULL;
    s->cps = g_new0(MIPSCPSState, 1);

    object_initialize(s->cps, sizeof(MIPSCPSState), TYPE_MIPS_CPS);
    qdev_set_parent_bus(DEVICE(s->cps), sysbus_get_default());

    object_property_set_str(OBJECT(s->cps), cpu_model, "cpu-model", &err);
    object_property_set_int(OBJECT(s->cps), smp_cpus, "num-vp", &err);
    object_property_set_bool(OBJECT(s->cps), true, "realized", &err);

    if (err != NULL) {
        error_report("%s", error_get_pretty(err));
        exit(1);
    }

    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(s->cps), 0, 0, 1);
}

static void create_cpu(RanchuState *s, const char *cpu_model)
{
    qemu_irq gfpic_out_irq;

    if (cpu_model == NULL) {
#ifdef TARGET_MIPS64
        cpu_model = "MIPS64R6-generic";
#else
        cpu_model = "74Kf";
#endif
    }

    if (s->rp->bootinfo.use_uhi && cpu_supports_cps_smp(cpu_model)) {
        create_cps(s, cpu_model);
        get_irq = &gic_get_irq;
        s->gfpic = NULL;
    } else {
        create_cpu_without_cps(cpu_model, &gfpic_out_irq);
        /* Initialize Goldfish PIC */
        s->gfpic = goldfish_pic_init(devmap[RANCHU_GOLDFISH_PIC].base,
                                     gfpic_out_irq);
        get_irq = &gfpic_get_irq;
        s->cps = NULL;
    }
}

/*
 * write_bootloader() - Writes a small boot loader into ROM
 *
 * @base         - ROM base address
 * @run_addr     - Second boot loader address
 * @kernel_entry - Kernel entry address
 * @rp           - Ranchu machine parameters
 *
 * Legacy boot loader arguments:
 *
 * a0 = 32-bit address of the command line
 * a1 = RAM size in bytes (ram_size)
 * a2 = RAM size in bytes (highmen_size)
 * a3 = 0
 *
 * UHI boot loader - DTB passover mode
 *
 * a0 = -2
 * a1 = DTB address
 */
static void write_bootloader(uint8_t *base, int64_t run_addr,
                             int64_t kernel_entry, ranchu_params *rp)
{
    uint32_t addr, addr_hi, addr_low;
    uint32_t *p;

    /* Small bootloader */
    p = (uint32_t *)base;

    addr = ((run_addr + 0x040) & 0x0fffffff) >> 2;
    stl_p(p++, 0x08000000 | addr);              /* j 0x1fc00040              */
    stl_p(p++, 0x00000000);                     /* nop                       */

    /* Second part of the bootloader */
    p = (uint32_t *) (base + 0x040);

    if (rp->bootinfo.use_uhi) {
        /* UHI boot protocol, device tree handover mode */
        addr = cpu_mips_phys_to_kseg0(NULL, rp->fdt_addr);
        stl_p(p++, 0x2404fffe);                 /* addiu a0, zero, -2        */
        stl_p(p++, 0x3c050000 | HIGH(addr));    /* lui a1, HI(fdt_addr)      */
        stl_p(p++, 0x34a50000 | LOW(addr));     /* ori a1, a1, LO(fdt_addr)  */
    } else {
        /* legacy boot protocol */
        addr = ENVP_ADDR + 64;
        stl_p(p++, 0x3c040000 | HIGH(addr));    /* lui a0, HI(envp_addr)     */
        stl_p(p++, 0x34840000 | LOW(addr));     /* ori a0, a0, LO(envp_addr) */
        addr = rp->bootinfo.ram_size;
        stl_p(p++, 0x3c050000 | HIGH(addr));    /* lui a1, HI(ram_size)      */
        stl_p(p++, 0x34a50000 | LOW(addr));     /* ori a1, a1, LO(ram_size)  */
        addr = rp->bootinfo.highmem_size;
        stl_p(p++, 0x3c060000 | HIGH(addr));    /* lui a2, HI(highmem)       */
        stl_p(p++, 0x34c60000 | LOW(addr));     /* ori a2, a2, LO(highmem)   */
        stl_p(p++, 0x24070000);                 /* addiu a3, zero, 0         */
    }

    /* Jump to kernel code */
    addr = kernel_entry;
    stl_p(p++, 0x3c1f0000 | HIGH(addr));        /* lui ra, HI(kentry)        */
    stl_p(p++, 0x37ff0000 | LOW(addr));         /* ori ra, ra, LO(kentry)    */
    stl_p(p++, 0x03e00009);                     /* jalr ra                   */
    stl_p(p++, 0x00000000);                     /* nop                       */
}

static void GCC_FMT_ATTR(3, 4) prom_set(uint32_t *prom_buf, int index,
                                        const char *string, ...)
{
    va_list ap;
    int32_t table_addr;

    if (index >= ENVP_NB_ENTRIES) {
        return;
    }

    if (string == NULL) {
        prom_buf[index] = 0;
        return;
    }

    table_addr = sizeof(int32_t) * ENVP_NB_ENTRIES + index * ENVP_ENTRY_SIZE;
    prom_buf[index] = tswap32(ENVP_ADDR + table_addr);

    va_start(ap, string);
    vsnprintf((char *)prom_buf + table_addr, ENVP_ENTRY_SIZE, string, ap);
    va_end(ap);
}

#define KERNEL_CMD_MAX_LEN 1024
#define STDOUT_PATH_MAX_LEN  64

static int64_t android_load_kernel(ranchu_params *rp)
{
    int initrd_size;
    ram_addr_t initrd_offset;
    uint64_t kernel_entry, kernel_low, kernel_high;
    uint32_t *prom_buf;
    long prom_size;
    GString *kernel_cmd;
    char stdout_path[STDOUT_PATH_MAX_LEN];
    int rc, prom_index = 0;

    /* Load the kernel.  */
    if (!rp->bootinfo.kernel_filename) {
        FATAL("Kernel image must be specified\n");
    }

    if (load_elf(rp->bootinfo.kernel_filename, cpu_mips_kseg0_to_phys, NULL,
        (uint64_t *)&kernel_entry, (uint64_t *)&kernel_low,
        (uint64_t *)&kernel_high, 0, EM_MIPS, 1, 0) < 0) {
        FATAL("qemu: could not load kernel '%s'\n",
              rp->bootinfo.kernel_filename);
    }

    rp->fdt_addr = kernel_high;
    kernel_high += rp->fdt_size;

    /* load initrd */
    initrd_size = 0;
    initrd_offset = 0;
    if (rp->bootinfo.initrd_filename) {
        initrd_size = get_image_size(rp->bootinfo.initrd_filename);
        if (initrd_size > 0) {
            initrd_offset = (kernel_high + ~TARGET_PAGE_MASK) &
                                            TARGET_PAGE_MASK;
            if (initrd_offset + initrd_size > rp->bootinfo.ram_size) {
                FATAL("qemu: memory too small for initial ram disk '%s'\n",
                      rp->bootinfo.initrd_filename);
            }
            initrd_size = load_image_targphys(rp->bootinfo.initrd_filename,
                                              initrd_offset,
                                              rp->bootinfo.ram_size -
                                              initrd_offset);
        }

        if (initrd_size == (target_ulong) -1) {
            FATAL("qemu: could not load initial ram disk '%s'\n",
                  rp->bootinfo.initrd_filename);
        }
    }

    snprintf(stdout_path, STDOUT_PATH_MAX_LEN, "/%s@%" HWADDR_PRIx,
             devmap[RANCHU_GOLDFISH_TTY].qemu_name,
             devmap[RANCHU_GOLDFISH_TTY].base +
             devmap[RANCHU_GOLDFISH_TTY].size *
             (MAX_GF_TTYS - 1));

    qemu_fdt_add_subnode(rp->fdt, "/chosen");
    rc = qemu_fdt_setprop_string(rp->fdt, "/chosen", "stdout-path",
                                 stdout_path);
    if (rc < 0) {
        FATAL("couldn't set /chosen/stdout-path\n");
    }

    kernel_cmd = g_string_new(rp->bootinfo.kernel_cmdline);
    g_string_append(kernel_cmd, " ieee754=relaxed");

    if (rp->bootinfo.use_uhi) {
        g_string_append(kernel_cmd, " mipsr2emu");

        rc = qemu_fdt_setprop_string(rp->fdt, "/chosen", "bootargs",
                                     kernel_cmd->str);
        if (rc < 0) {
            FATAL("couldn't set /chosen/bootargs\n");
        }
        rc = qemu_fdt_setprop_cell(rp->fdt, "/chosen", "linux,initrd-start",
                                   initrd_offset);
        if (rc < 0) {
            FATAL("couldn't set /chosen/linux,initrd-start\n");
        }
        rc = qemu_fdt_setprop_cell(rp->fdt, "/chosen", "linux,initrd-end",
                                   initrd_offset + (long int)initrd_size);
        if (rc < 0) {
            FATAL("couldn't set /chosen/linux,initrd-end\n");
        }
    } else {
        g_string_append(kernel_cmd, " earlycon");

        if (initrd_size > 0) {
            g_string_append_printf(kernel_cmd,
                     " rd_start=0x%" HWADDR_PRIx " rd_size=%li",
                     (hwaddr)cpu_mips_phys_to_kseg0(NULL, initrd_offset),
                     (long int)initrd_size);
        }

        /* Setup Highmem */
        if (rp->bootinfo.highmem_size) {
            g_string_append_printf(kernel_cmd,
                     " mem=%um@0x0 mem=%" PRIu64 "m@0x%x",
                     GOLDFISH_IO_SPACE / (1024 * 1024),
                     rp->bootinfo.highmem_size / (1024 * 1024),
                     HIGHMEM_OFFSET);
        }

        /* Prepare the environment arguments table */
        prom_size = ENVP_NB_ENTRIES * (sizeof(int32_t) + ENVP_ENTRY_SIZE);
        prom_buf = g_malloc(prom_size);

        prom_set(prom_buf, prom_index++, "%s", kernel_cmd->str);
        prom_set(prom_buf, prom_index++, NULL);

        rom_add_blob_fixed("prom", prom_buf, prom_size,
                           cpu_mips_kseg0_to_phys(NULL, ENVP_ADDR));
    }

    /* Put the DTB into the memory map as a ROM image: this will ensure
     * the DTB is copied again upon reset, even if addr points into RAM.
     */
    rom_add_blob_fixed("dtb", rp->fdt, rp->fdt_size, rp->fdt_addr);

    qemu_fdt_dumpdtb(rp->fdt, rp->fdt_size);

    g_string_free(kernel_cmd, true);

    return kernel_entry;
}

static void goldfish_reset_io_write(void *opaque, hwaddr addr,
                                    uint64_t val, unsigned size)
{
    switch (val) {
    case GOLDFISH_PM_CMD_GORESET:
        qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
        break;
    case GOLDFISH_PM_CMD_GOSHUTDOWN:
        qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
        break;
    default:
        LOGI("Unknown command %llx\n", (unsigned long long)val);
        break;
    }
}

static uint64_t goldfish_reset_io_read(void *opaque, hwaddr addr,
                                       unsigned size)
{
    return 0;
}

static const MemoryRegionOps goldfish_reset_io_ops = {
    .read = goldfish_reset_io_read,
    .write = goldfish_reset_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

/* create_device(void* fdt, DevMapEntry* dev, qemu_irq* pic,
 *               int num_devices, bool is_virtio, bool to_attach)
 *
 * In case of interrupt controller dev->irq stores
 * dt handle previously referenced as interrupt-parent
 *
 * @s   - RanchuState pointer
 * @fdt - Place where DT nodes will be stored
 * @dev - Device information (base, irq, name)
 * @num_devices - If you want to allocate multiple continuous device mappings
 * @is_virtio - Virtio devices should be added in revers order
 * @to_attach - Controls whether we should try to attach this device to a sysbus
 */
static void create_device(RanchuState *s, void *fdt, DevMapEntry *dev,
                          int num_devices, bool is_virtio, bool to_attach)
{
    int i, j;

    for (i = 0; i < num_devices; i++) {
        hwaddr base = dev->base + i * dev->size;
        int dt_compat_sz = 0;

        char *nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, base);
        qemu_fdt_add_subnode(fdt, nodename);
        dt_compat_sz = 0;
        for (j = 0; j < dev->dt_num_compatible; j++) {
            dt_compat_sz += strlen(dev->dt_compatible + dt_compat_sz) + 1;
        }
        qemu_fdt_setprop(fdt, nodename, "compatible", dev->dt_compatible,
                         dt_compat_sz);
        qemu_fdt_setprop_sized_cells(fdt, nodename, "reg", 1, base, 2,
                                     dev->size);
        if (dev->irq > 0) {
            if (s->cps == NULL) {
                qemu_fdt_setprop_cells(fdt, nodename, "interrupts",
                                       dev->irq + i);
            } else {
                qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 0,
                                       dev->irq + i, 4);
            }
            if (s->cps == NULL) {
                qemu_fdt_setprop_cell(fdt, nodename, "interrupt-parent",
                                      devmap[RANCHU_GOLDFISH_PIC].dt_phandle);
            } else {
                qemu_fdt_setprop_cell(fdt, nodename, "interrupt-parent",
                                      devmap[RANCHU_MIPS_GIC].dt_phandle);
            }
        }

        if (is_virtio) {
            /* Note that we have to create the transports in forwards order
             * so that command line devices are inserted lowest address
             * first, and then add dtb nodes in reverse order so that they
             * appear in the finished device tree lowest address first.
             */
            if (to_attach) {
                sysbus_create_simple(dev->qemu_name,
                    dev->base + (num_devices - i - 1) * dev->size,
                    get_irq(s, dev->irq + num_devices - i - 1));
            }
        } else if (to_attach) {
            sysbus_create_simple(dev->qemu_name, base,
                get_irq(s, dev->irq + i));
        }
        g_free(nodename);
    }
}

/* create_intc_device(void *fdt, DevMapEntry *dev, DevMapEntry *parent)
 *
 * Create interrupt device DT node
 *
 * @fdt - Place where DT node will be stored
 * @dev - Interrupt Device information (base, irq, name)
 * @parent - If not NULL, it represents a parent Interrupt controller
 */
static void create_intc_device(void *fdt, DevMapEntry *dev,
                               DevMapEntry *parent)
{
    int i, dt_compat_sz = 0;
    char *nodename;

    if (dev->base != 0) {
        nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, dev->base);
        qemu_fdt_add_subnode(fdt, nodename);
        qemu_fdt_setprop_sized_cells(fdt, nodename, "reg", 1, dev->base, 2,
                                     dev->size);
    } else {
        nodename = g_strdup_printf("/%s", dev->dt_name);
        qemu_fdt_add_subnode(fdt, nodename);
    }

    qemu_fdt_setprop(fdt, nodename, "interrupt-controller", NULL, 0);
    qemu_fdt_setprop_cell(fdt, nodename, "#interrupt-cells", 0x1);

    dev->dt_phandle = qemu_fdt_alloc_phandle(fdt);
    qemu_fdt_setprop_cell(fdt, nodename, "phandle", dev->dt_phandle);
    for (i = 0; i < dev->dt_num_compatible; i++) {
        dt_compat_sz += strlen(dev->dt_compatible + dt_compat_sz) + 1;
    }
    qemu_fdt_setprop(fdt, nodename, "compatible",
                     dev->dt_compatible, dt_compat_sz);
    if (parent) {
        qemu_fdt_setprop_cell(fdt, nodename, "interrupt-parent",
                              parent->dt_phandle);
    }
    if (dev->irq > 0) {
        qemu_fdt_setprop_cells(fdt, nodename, "interrupts", dev->irq);
    }

    g_free(nodename);
}

/* create_gic_device(void *fdt, DevMapEntry *dev)
 *
 * Create GIC device DT node
 *
 * @fdt - Place where DT node will be stored
 * @dev - Interrupt Device information (base, irq, name)
 */
static void create_gic_device(void *fdt, DevMapEntry *dev)
{
    int i, dt_compat_sz = 0;
    char *nodename;

    nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, dev->base);
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "reg", 1,
                                 dev->base, 2, dev->size);

    qemu_fdt_setprop(fdt, nodename, "interrupt-controller", NULL, 0);
    qemu_fdt_setprop_cell(fdt, nodename, "#interrupt-cells", 0x3);

    dev->dt_phandle = qemu_fdt_alloc_phandle(fdt);
    qemu_fdt_setprop_cell(fdt, nodename, "phandle", dev->dt_phandle);
    for (i = 0; i < dev->dt_num_compatible; i++) {
        dt_compat_sz += strlen(dev->dt_compatible + dt_compat_sz) + 1;
    }
    qemu_fdt_setprop(fdt, nodename, "compatible",
                     dev->dt_compatible, dt_compat_sz);

    g_free(nodename);

    /* GIC Timer node */
    nodename = g_strdup_printf("/%s@%" PRIx64 "/timer",
                               dev->dt_name, dev->base);
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop(fdt, nodename, "compatible",
                     "mti,gic-timer", strlen("mti,gic-timer") + 1);
    qemu_fdt_setprop_cells(fdt, nodename, "interrupts", 1, 1, 0);
    qemu_fdt_setprop_cells(fdt, nodename, "clock-frequency", 100000000);

    g_free(nodename);
}

/* create_pm_device(void* fdt, DevMapEntry* dev)
 *
 * Create power management DT node which will be used by kernel
 * in order to attach PM routines for reset/halt/shutdown
 *
 * @fdt - Place where DT node will be stored
 * @dev - Device information (base, irq, name)
 */
static void create_pm_device(void *fdt, DevMapEntry *dev)
{
    hwaddr base = dev->base;
    int i, dt_compat_sz = 0;
    uint32_t pm_regs;
    char *nodename;

    nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, base);
    qemu_fdt_add_subnode(fdt, nodename);
    pm_regs = qemu_fdt_alloc_phandle(fdt);
    qemu_fdt_setprop_cell(fdt, nodename, "phandle", pm_regs);
    for (i = 0; i < dev->dt_num_compatible; i++) {
        dt_compat_sz += strlen(dev->dt_compatible + dt_compat_sz) + 1;
    }
    qemu_fdt_setprop(fdt, nodename, "compatible",
                     dev->dt_compatible, dt_compat_sz);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "reg", 1, base, 2, dev->size);

    g_free(nodename);
    nodename = g_strdup_printf("/%s/reboot", dev->dt_name);
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "syscon-reboot");
    qemu_fdt_setprop_cell(fdt, nodename, "regmap", pm_regs);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "offset", 1, 0x00);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "mask", 1,
                                 GOLDFISH_PM_CMD_GORESET);

    g_free(nodename);
    nodename = g_strdup_printf("/%s/poweroff", dev->dt_name);
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "syscon-poweroff");
    qemu_fdt_setprop_cell(fdt, nodename, "regmap", pm_regs);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "offset", 1, 0x00);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "mask", 1,
                                 GOLDFISH_PM_CMD_GOSHUTDOWN);
    g_free(nodename);
}

static void mips_ranchu_init(MachineState *machine)
{
    int i, fdt_size;
    void *fdt;
    int64_t kernel_entry;
    char *nodename;

    DeviceState *dev = qdev_create(NULL, TYPE_MIPS_RANCHU);
    RanchuState *s = MIPS_RANCHU(dev);
    ranchu_params *rp = NULL;

    MemoryRegion *ram_lo = g_new(MemoryRegion, 1);
    MemoryRegion *ram_hi = g_new(MemoryRegion, 1);
    MemoryRegion *iomem = g_new(MemoryRegion, 1);
    MemoryRegion *bios = g_new(MemoryRegion, 1);

    rp = g_new0(ranchu_params, 1);
    s->rp = rp;

    rp->bootinfo.kernel_filename = strdup(machine->kernel_filename);
    rp->bootinfo.kernel_cmdline = strdup(machine->kernel_cmdline);
    rp->bootinfo.initrd_filename = strdup(machine->initrd_filename);

    /* Are we using UHI or legacy boot protocol? */
    boot_protocol_detect(rp, rp->bootinfo.kernel_filename);

    /* init CPUs */
    create_cpu(s, machine->cpu_type);

    rp->cpu = MIPS_CPU(first_cpu);

#if !defined(TARGET_MIPS64)
    if (ram_size > (MAX_RAM_SIZE_MB << 20)) {
        LOGI("qemu: Too much memory for this machine. "
             "RAM size reduced to %lu MB\n", MAX_RAM_SIZE_MB);
        ram_size = MAX_RAM_SIZE_MB << 20;
    }
#endif

    fdt = create_device_tree(&fdt_size);

    if (!fdt) {
        FATAL("create_device_tree() failed");
    }

    rp->fdt = fdt;
    rp->fdt_size = fdt_size;

    memory_region_init_ram_nomigrate(ram_lo, NULL, "ranchu_low.ram",
                           MIN(ram_size, GOLDFISH_IO_SPACE), &error_abort);
    vmstate_register_ram_global(ram_lo);
    memory_region_add_subregion(get_system_memory(), 0, ram_lo);

    memory_region_init_io(iomem, NULL, &goldfish_reset_io_ops, NULL,
                          devmap[RANCHU_GOLDFISH_RESET].qemu_name,
                          devmap[RANCHU_GOLDFISH_RESET].size);
    memory_region_add_subregion(get_system_memory(),
                                devmap[RANCHU_GOLDFISH_RESET].base, iomem);

    memory_region_init_ram_nomigrate(bios, NULL, "ranchu.bios", RANCHU_BIOS_SIZE,
                           &error_abort);
    vmstate_register_ram_global(bios);
    memory_region_set_readonly(bios, true);
    memory_region_add_subregion(get_system_memory(), RESET_ADDRESS, bios);

    /* post IO hole, if there is enough RAM */
    if (ram_size > GOLDFISH_IO_SPACE) {
        memory_region_init_ram_nomigrate(ram_hi, NULL, "ranchu_high.ram",
                               ram_size - GOLDFISH_IO_SPACE, &error_abort);
        vmstate_register_ram_global(ram_hi);
        memory_region_add_subregion(get_system_memory(),
                                    HIGHMEM_OFFSET, ram_hi);
        rp->bootinfo.highmem_size = ram_size - GOLDFISH_IO_SPACE;
    }

    rp->bootinfo.ram_size = MIN(ram_size, GOLDFISH_IO_SPACE);

    qemu_fdt_setprop_string(fdt, "/", "model", "ranchu");
    if (!rp->bootinfo.use_uhi) {
        qemu_fdt_setprop_string(fdt, "/", "compatible", "mti,goldfish");
    } else {
        qemu_fdt_setprop_string(fdt, "/", "compatible", "mti,ranchu");
    }
    qemu_fdt_setprop_cell(fdt, "/", "#address-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/", "#size-cells", 0x2);

    /* Firmware node */
    qemu_fdt_add_subnode(fdt, "/firmware");
    nodename = g_strdup_printf("/firmware/android");
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop_string(fdt, nodename, "compatible", "android,firmware");
    qemu_fdt_setprop_string(fdt, nodename, "hardware", "ranchu");
    qemu_fdt_setprop_string(fdt, nodename, "revision", MIPS_RANCHU_REV);
    g_free(nodename);

    /* fstab node */
    if (device_tree_setup_func) {
        device_tree_setup_func(fdt);
    }

    /* Memory node */
    qemu_fdt_add_subnode(fdt, "/memory");
    qemu_fdt_setprop_string(fdt, "/memory", "device_type", "memory");
    if (ram_size > GOLDFISH_IO_SPACE) {
        qemu_fdt_setprop_sized_cells(fdt, "/memory", "reg",
                                     1, 0, 2, GOLDFISH_IO_SPACE,
                                     1, HIGHMEM_OFFSET, 2, ram_size -
                                     GOLDFISH_IO_SPACE);
    } else {
        qemu_fdt_setprop_sized_cells(fdt, "/memory", "reg", 1, 0, 2, ram_size);
    }

    if (s->gfpic) {
        /* Create CPU Interrupt controller node in dt */
        create_intc_device(fdt, &devmap[RANCHU_MIPS_CPU_INTC], NULL);
        /* Create goldfish_pic controller node in dt */
        create_intc_device(fdt, &devmap[RANCHU_GOLDFISH_PIC],
                                &devmap[RANCHU_MIPS_CPU_INTC]);
    } else {
        /* Create MIPS Global Interrupt Controller node in dt */
        create_gic_device(fdt, &devmap[RANCHU_MIPS_GIC]);
        /* Create CPC node in DT */
        create_device(s, fdt, &devmap[RANCHU_MIPS_CPC], 1, true, false);
    }

    /* Make sure the first 3 serial ports are associated with a device. */
    for (i = 0; i < 3; i++) {
        if (!serial_hds[i]) {
            char label[32];
            snprintf(label, sizeof(label), "serial%d", i);
            serial_hds[i] = qemu_chr_new(label, "null");
        }
    }

    /* Create 3 Goldfish TTYs */
    create_device(s, fdt, &devmap[RANCHU_GOLDFISH_TTY], MAX_GF_TTYS,
                  true, true);

    /* Other Goldfish Platform devices */
    for (i = RANCHU_GOLDFISH_SYNC; i >= RANCHU_GOLDFISH_RTC ; i--) {
        create_device(s, fdt, &devmap[i], 1, false, true);
    }

    create_pm_device(fdt, &devmap[RANCHU_GOLDFISH_RESET]);

    /* Virtio MMIO devices */
    create_device(s, fdt, &devmap[RANCHU_MMIO], VIRTIO_TRANSPORTS, true, true);

    if (!rp->bootinfo.use_uhi) {
        /* Create a fake Goldfish Timer node
         * This is required for kernels < v4.4
         */
        create_device(s, fdt, &devmap[RANCHU_GOLDFISH_TIMER], 1, true, false);
    }

    kernel_entry = android_load_kernel(rp);

    write_bootloader(memory_region_get_ram_ptr(bios),
                     cpu_mips_phys_to_kseg0(NULL, RESET_ADDRESS),
                     kernel_entry, rp);
}

static int mips_ranchu_sysbus_device_init(SysBusDevice *sysbusdev)
{
    return 0;
}

static void mips_ranchu_class_init(ObjectClass *klass, void *data)
{
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = mips_ranchu_sysbus_device_init;
}

static const TypeInfo mips_ranchu_device = {
    .name = TYPE_MIPS_RANCHU,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RanchuState),
    .class_init = mips_ranchu_class_init,
};

static void mips_ranchu_machine_init(MachineClass *mc)
{
    mc->desc = "Android/MIPS ranchu";
    mc->init = mips_ranchu_init;
    mc->max_cpus = 16;
    mc->is_default = 1;
}

DEFINE_MACHINE("ranchu", mips_ranchu_machine_init)

static void mips_ranchu_register_types(void)
{
    type_register_static(&mips_ranchu_device);
}

type_init(mips_ranchu_register_types)
