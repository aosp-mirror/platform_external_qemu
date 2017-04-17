#include "qemu/osdep.h"
#include "cpu.h"
#include "hw/sysbus.h"
#include "hw/devices.h"
#include "hw/mips/mips.h"
#include "hw/mips/cpudevs.h"
#include "hw/mips/bios.h"
#include "net/net.h"
#include "sysemu/device_tree.h"
#include "sysemu/sysemu.h"
#include "sysemu/kvm.h"
#include "hw/boards.h"
#include "exec/address-spaces.h"
#include "qemu/bitops.h"
#include "qemu/error-report.h"
#include "qemu/config-file.h"
#include "sysemu/char.h"
#include "monitor/monitor.h"
#include "hw/loader.h"
#include "elf.h"
#include "hw/intc/goldfish_pic.h"
#include "hw/irq.h"
#include "qapi/error.h"

#define PHYS_TO_VIRT(x) ((x) | ~(target_ulong)0x7fffffff)

#define MIPS_CPU_SAVE_VERSION 1
#define MIPS_CPU_IRQ_BASE     8

#define HIGHMEM_OFFSET    0x20000000
#define GOLDFISH_IO_SPACE 0x1f000000
#define RESET_ADDRESS     0x1fc00000

#define RANCHU_BIOS_SIZE  0x100000    /* 1024 * 1024 = 1 MB */

/* Store the environment arguments table in the second page from the top
 * of the bios memory. Two pages are required since the environment
 * arguments table is 4160 bytes in size.
 */
#define ENVP_ADDR         PHYS_TO_VIRT(RESET_ADDRESS + RANCHU_BIOS_SIZE - 2*TARGET_PAGE_SIZE)
#define ENVP_NB_ENTRIES   16
#define ENVP_ENTRY_SIZE   256

#define VIRTIO_TRANSPORTS 16
#define MAX_GF_TTYS       3

#define TYPE_MIPS_RANCHU "mips-ranchu"
#define MIPS_RANCHU(obj) OBJECT_CHECK(RanchuState, (obj), TYPE_MIPS_RANCHU)
#define MIPS_RANCHU_REV "1"

typedef struct {
    SysBusDevice parent_obj;

    qemu_irq *gfpic;
} RanchuState;

#define MAX_RAM_SIZE_MB 4079UL

enum {
    RANCHU_GOLDFISH_PIC,
    RANCHU_GOLDFISH_TTY,
    RANCHU_GOLDFISH_TIMER,
    RANCHU_GOLDFISH_RTC,
    RANCHU_GOLDFISH_BATTERY,
    RANCHU_GOLDFISH_FB,
    RANCHU_GOLDFISH_EVDEV,
    RANCHU_GOLDFISH_PIPE,
    RANCHU_GOLDFISH_AUDIO,
    RANCHU_GOLDFISH_SYNC,
    RANCHU_GOLDFISH_RESET,
    RANCHU_MMIO,
};

typedef struct DevMapEntry {
    hwaddr base;
    hwaddr size;
    int irq;
    const char* qemu_name;
    const char* dt_name;
    const char* dt_compatible;
    int dt_num_compatible;
} DevMapEntry;

static DevMapEntry devmap[] = {
    [RANCHU_GOLDFISH_PIC] =       { GOLDFISH_IO_SPACE, 0x1000, 0,
        NULL, "goldfish_pic", "generic,goldfish-pic", 1 },
    /* ttyGF0 base address remains hardcoded in the kernel.
     * Early printing (prom_putchar()) relies on finding device
     * mapped on this address. DT can not be used at that early stage
     * for acquiring the base address of the device in the kernel.
     */
    [RANCHU_GOLDFISH_TTY] =       { GOLDFISH_IO_SPACE + 0x02000, 0x1000, 2,
        "goldfish_tty", "goldfish_tty", "google,goldfish-tty\0generic,goldfish-tty", 2 },
    /* repeats for a total of MAX_GF_TTYS */
    [RANCHU_GOLDFISH_TIMER] =     { GOLDFISH_IO_SPACE + 0x05000, 0x1000, 5,
        "goldfish_timer", "goldfish_timer", "google,goldfish-timer\0generic,goldfish-timer", 2 },
    [RANCHU_GOLDFISH_RTC] =       { GOLDFISH_IO_SPACE + 0x06000, 0x1000, 6,
        "goldfish_rtc", "goldfish_rtc", "google,goldfish-rtc\0generic,goldfish-rtc", 2 },
    [RANCHU_GOLDFISH_BATTERY] =   { GOLDFISH_IO_SPACE + 0x07000, 0x1000, 7,
        "goldfish_battery", "goldfish_battery",
        "google,goldfish-battery\0generic,goldfish-battery", 2 },
    [RANCHU_GOLDFISH_FB] =        { GOLDFISH_IO_SPACE + 0x08000, 0x0100, 8,
        "goldfish_fb", "goldfish_fb", "google,goldfish-fb\0generic,goldfish-fb", 2 },
    [RANCHU_GOLDFISH_EVDEV] =     { GOLDFISH_IO_SPACE + 0x09000, 0x1000, 9,
        "goldfish-events", "goldfish_events",
        "google,goldfish-events-keypad\0generic,goldfish-events-keypad", 2 },
    [RANCHU_GOLDFISH_PIPE] = { GOLDFISH_IO_SPACE + 0x0A000, 0x2000, 10,
        "goldfish_pipe", "android_pipe", "google,android-pipe\0generic,android-pipe", 2 },
    [RANCHU_GOLDFISH_AUDIO] =     { GOLDFISH_IO_SPACE + 0x0C000, 0x0100, 11,
        "goldfish_audio", "goldfish_audio", "google,goldfish-audio\0generic,goldfish-audio", 2 },
    [RANCHU_GOLDFISH_SYNC] =     { GOLDFISH_IO_SPACE + 0x0D000, 0x0100, 11,
        "goldfish_sync", "goldfish_sync", "google,goldfish-sync\0generic,goldfish-sync", 2 },
    [RANCHU_GOLDFISH_RESET] =     { GOLDFISH_IO_SPACE + 0x0D000, 0x0100, 12,
        "goldfish_reset", "goldfish_reset", "google,goldfish-reset\0generic,goldfish-reset", 2 },
    [RANCHU_MMIO] =         { GOLDFISH_IO_SPACE + 0x10000, 0x0200, 16,
        "virtio-mmio", "virtio_mmio", "virtio,mmio", 1 },
    /* ...repeating for a total of VIRTIO_TRANSPORTS, each of that size */
};

struct mips_boot_info {
    const char* kernel_filename;
    const char* kernel_cmdline;
    const char* initrd_filename;
    unsigned ram_size;
    unsigned highmem_size;
};

typedef struct machine_params {
    struct mips_boot_info bootinfo;
    MIPSCPU *cpu;
    int fdt_size;
    void *fdt;
} ranchu_params;

static void main_cpu_reset(void* opaque1)
{
    struct machine_params *opaque = (struct machine_params *)opaque1;
    MIPSCPU *cpu = opaque->cpu;

    cpu_reset(CPU(cpu));
}

/* ROM and pseudo bootloader

   The following code implements a very simple bootloader. It first
   loads the registers a0, a1, a2 and a3 to the values expected by the kernel,
   and then jumps at the kernel entry address.

   The registers a0 to a3 should contain the following values:
     a0 - 32-bit address of the command line
     a1 - RAM size in bytes (ram_size)
     a2 - RAM size in bytes (highmen_size)
     a3 - 0
*/
static void write_bootloader(uint8_t *base, int64_t run_addr,
                              int64_t kernel_entry, ranchu_params *rp)
{
    uint32_t *p;

    /* Small bootloader */
    p = (uint32_t *)base;

    stl_p(p++, 0x08000000 | ((run_addr + 0x040) & 0x0fffffff) >> 2); /* j 0x1fc00040 */
    stl_p(p++, 0x00000000);                                          /* nop */

    /* Second part of the bootloader */
    p = (uint32_t *) (base + 0x040);
    stl_p(p++, 0x3c040000 | (((ENVP_ADDR + 64) >> 16) & 0xffff));    /* lui a0, high(ENVP_ADDR + 64) */
    stl_p(p++, 0x34840000 | ((ENVP_ADDR + 64) & 0xffff));            /* ori a0, a0, low(ENVP_ADDR + 64) */
    stl_p(p++, 0x3c050000 | (rp->bootinfo.ram_size >> 16));          /* lui a1, high(ram_size) */
    stl_p(p++, 0x34a50000 | (rp->bootinfo.ram_size & 0xffff));       /* ori a1, a1, low(ram_size) */
    stl_p(p++, 0x3c060000 | (rp->bootinfo.highmem_size >> 16));      /* lui a2, high(highmem_size) */
    stl_p(p++, 0x34c60000 | (rp->bootinfo.highmem_size & 0xffff));   /* ori a2, a2, low(highmem_size) */
    stl_p(p++, 0x24070000);                                          /* addiu a3, zero, 0 */

    /* Jump to kernel code */
    stl_p(p++, 0x3c1f0000 | ((kernel_entry >> 16) & 0xffff));        /* lui ra, high(kernel_entry) */
    stl_p(p++, 0x37ff0000 | (kernel_entry & 0xffff));                /* ori ra, ra, low(kernel_entry) */
    stl_p(p++, 0x03e00009);                                          /* jalr ra */
    stl_p(p++, 0x00000000);                                          /* nop */
}

static void GCC_FMT_ATTR(3, 4) prom_set(uint32_t* prom_buf, int index,
                                        const char *string, ...)
{
    va_list ap;
    int32_t table_addr;

    if (index >= ENVP_NB_ENTRIES)
        return;

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

static int64_t android_load_kernel(ranchu_params *rp)
{
    int initrd_size;
    ram_addr_t initrd_offset;
    uint64_t kernel_entry, kernel_low, kernel_high;
    uint32_t *prom_buf;
    long prom_size;
    int prom_index = 0;

    /* Load the kernel.  */
    if (!rp->bootinfo.kernel_filename) {
        fprintf(stderr, "Kernel image must be specified\n");
        exit(1);
    }

    if (load_elf(rp->bootinfo.kernel_filename, cpu_mips_kseg0_to_phys, NULL,
        (uint64_t *)&kernel_entry, (uint64_t *)&kernel_low,
        (uint64_t *)&kernel_high, 0, EM_MIPS, 1, 0) < 0) {
        fprintf(stderr, "qemu: could not load kernel '%s'\n",
                rp->bootinfo.kernel_filename);
        exit(1);
    }

    /* Load the DTB at the kernel_high address, that is the place where
     * kernel with Appended DT support enabled will look for it
     */
    if (rp->fdt) {
        cpu_physical_memory_write(kernel_high, rp->fdt, rp->fdt_size);
        kernel_high += rp->fdt_size;
    }

    /* load initrd */
    initrd_size = 0;
    initrd_offset = 0;
    if (rp->bootinfo.initrd_filename) {
        initrd_size = get_image_size (rp->bootinfo.initrd_filename);
        if (initrd_size > 0) {
            initrd_offset = (kernel_high + ~TARGET_PAGE_MASK) & TARGET_PAGE_MASK;
            if (initrd_offset + initrd_size > rp->bootinfo.ram_size) {
                fprintf(stderr,
                    "qemu: memory too small for initial ram disk '%s'\n",
                    rp->bootinfo.initrd_filename);
                exit(1);
            }
            initrd_size = load_image_targphys(rp->bootinfo.initrd_filename,
                                              initrd_offset,
                                              rp->bootinfo.ram_size - initrd_offset);
        }

        if (initrd_size == (target_ulong) -1) {
            fprintf(stderr,
                "qemu: could not load initial ram disk '%s'\n",
                rp->bootinfo.initrd_filename);
            exit(1);
        }
    }

    char kernel_cmd[1024];
    if (initrd_size > 0)
        sprintf (kernel_cmd, "%s rd_start=0x%" HWADDR_PRIx " rd_size=%li",
                 rp->bootinfo.kernel_cmdline,
                 (hwaddr)PHYS_TO_VIRT(initrd_offset),
                 (long int)initrd_size);
    else
        strcpy (kernel_cmd, rp->bootinfo.kernel_cmdline);

    /* Setup Highmem */
    char kernel_cmd2[1024];
    if (rp->bootinfo.highmem_size) {
        sprintf (kernel_cmd2, "%s mem=%um@0x0 mem=%um@0x%x",
                 kernel_cmd,
                 GOLDFISH_IO_SPACE / (1024 * 1024),
                 rp->bootinfo.highmem_size / (1024 * 1024),
                 HIGHMEM_OFFSET);
    } else {
        strcpy (kernel_cmd2, kernel_cmd);
    }

    /* Prepare the environment arguments table */
    prom_size = ENVP_NB_ENTRIES * (sizeof(int32_t) + ENVP_ENTRY_SIZE);
    prom_buf = g_malloc(prom_size);

    prom_set(prom_buf, prom_index++, "%s", kernel_cmd2);
    prom_set(prom_buf, prom_index++, NULL);

    rom_add_blob_fixed("prom", prom_buf, prom_size,
                       cpu_mips_kseg0_to_phys(NULL, ENVP_ADDR));

    return kernel_entry;
}

static void goldfish_reset_io_write(void *opaque, hwaddr addr,
                             uint64_t val, unsigned size)
{
    switch (val) {
    case 0x42:
        qemu_system_reset_request();
        break;
    case 0x43:
        qemu_system_shutdown_request();
        break;
    default:
        fprintf(stdout, "%s: %d: Unknown command %llx\n",
                __func__, __LINE__, (unsigned long long)val);
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
 *               int num_devices, int is_virtio)
 *
 * In case of interrupt controller dev->irq stores
 * dt handle previously referenced as interrupt-parent
 *
 * @fdt - Place where DT nodes will be stored
 * @dev - Device information (base, irq, name)
 * @pic - Interrupt controller parent. If NULL, 'intc' node assumed.
 * @num_devices - If you want to allocate multiple continuous device mappings
 */
static void create_device(void* fdt, DevMapEntry* dev, qemu_irq* pic,
                          int num_devices, int is_virtio)
{
    int i, j, dt_compat_sz = 0;

    for (i = 0; i < num_devices; i++) {
        hwaddr base = dev->base + i * dev->size;

        char* nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, base);
        qemu_fdt_add_subnode(fdt, nodename);
        for (j = 0; j < dev->dt_num_compatible; j++) {
            dt_compat_sz += strlen(dev->dt_compatible + dt_compat_sz) + 1;
        }
        qemu_fdt_setprop(fdt, nodename, "compatible",
                         dev->dt_compatible, dt_compat_sz);
        qemu_fdt_setprop_sized_cells(fdt, nodename, "reg", 1, base, 2, dev->size);

        if (pic == NULL) {
            qemu_fdt_setprop(fdt, nodename, "interrupt-controller", NULL, 0);
            qemu_fdt_setprop_cell(fdt, nodename, "phandle", dev->irq);
            qemu_fdt_setprop_cell(fdt, nodename, "#interrupt-cells", 0x1);
        } else {
            qemu_fdt_setprop_cells(fdt, nodename, "interrupts",
                                   dev->irq + i + MIPS_CPU_IRQ_BASE);
            if (is_virtio) {
                /* Note that we have to create the transports in forwards order
                 * so that command line devices are inserted lowest address first,
                 * and then add dtb nodes in reverse order so that they appear in
                 * the finished device tree lowest address first.
                 */
                sysbus_create_simple(dev->qemu_name,
                    dev->base + (num_devices - i - 1) * dev->size,
                    pic[dev->irq + num_devices - i - 1]);
            } else {
                sysbus_create_simple(dev->qemu_name, base, pic[dev->irq + i]);
            }
        }
        g_free(nodename);
    }
}

/* create_pm_device(void* fdt, DevMapEntry* dev)
 *
 * Create power management DT node which will be used by kernel
 * in order to attach PM routines for reset/halt/shoutdown
 *
 * @fdt - Place where DT node will be stored
 * @dev - Device information (base, irq, name)
 */
static void create_pm_device(void* fdt, DevMapEntry* dev)
{
    hwaddr base = dev->base;

    char* nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, base);
    qemu_fdt_add_subnode(fdt, nodename);
    qemu_fdt_setprop(fdt, nodename, "compatible",
                     dev->dt_compatible,
                     strlen(dev->dt_compatible) + 1);
    qemu_fdt_setprop_sized_cells(fdt, nodename, "reg", 1, base, 2, dev->size);
    g_free(nodename);
}

static void mips_ranchu_init(MachineState *machine)
{
    MIPSCPU *cpu;
    CPUMIPSState *env;
    qemu_irq *goldfish_pic;
    int i, fdt_size;
    void *fdt;
    ranchu_params *rp;
    int64_t kernel_entry;

    DeviceState *dev = qdev_create(NULL, TYPE_MIPS_RANCHU);
    RanchuState *s = MIPS_RANCHU(dev);

    MemoryRegion *ram_lo = g_new(MemoryRegion, 1);
    MemoryRegion *ram_hi = g_new(MemoryRegion, 1);
    MemoryRegion *iomem = g_new(MemoryRegion, 1);
    MemoryRegion *bios = g_new(MemoryRegion, 1);

    /* init CPUs */
    if (machine->cpu_model == NULL) {
#ifdef TARGET_MIPS64
        machine->cpu_model = "MIPS64R2-generic";
#else
        machine->cpu_model = "74Kf";
#endif
    }
    cpu = cpu_mips_init(machine->cpu_model);
    if (cpu == NULL) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }

    rp = g_new0(ranchu_params, 1);

    env = &cpu->env;

    qemu_register_reset(main_cpu_reset, rp);

    if (ram_size > (MAX_RAM_SIZE_MB << 20)) {
        fprintf(stderr, "qemu: Too much memory for this machine. "
                        "RAM size reduced to %lu MB\n", MAX_RAM_SIZE_MB);
        ram_size = MAX_RAM_SIZE_MB << 20;
    }

    fdt = create_device_tree(&fdt_size);

    if (!fdt) {
        error_report("create_device_tree() failed");
        exit(1);
    }

    memory_region_init_ram(ram_lo, NULL, "ranchu_low.ram",
        MIN(ram_size, GOLDFISH_IO_SPACE), &error_abort);
    vmstate_register_ram_global(ram_lo);
    memory_region_add_subregion(get_system_memory(), 0, ram_lo);

    memory_region_init_io(iomem, NULL, &goldfish_reset_io_ops, NULL, "goldfish-reset", 0x0100);
    memory_region_add_subregion(get_system_memory(), devmap[RANCHU_GOLDFISH_RESET].base, iomem);

    memory_region_init_ram(bios, NULL, "ranchu.bios", RANCHU_BIOS_SIZE,
                           &error_abort);
    vmstate_register_ram_global(bios);
    memory_region_set_readonly(bios, true);
    memory_region_add_subregion(get_system_memory(), RESET_ADDRESS, bios);

    /* post IO hole, if there is enough RAM */
    if (ram_size > GOLDFISH_IO_SPACE) {
        memory_region_init_ram(ram_hi, NULL, "ranchu_high.ram",
            ram_size - GOLDFISH_IO_SPACE, &error_abort);
        vmstate_register_ram_global(ram_hi);
        memory_region_add_subregion(get_system_memory(), HIGHMEM_OFFSET, ram_hi);
        rp->bootinfo.highmem_size = ram_size - GOLDFISH_IO_SPACE;
    }

    rp->bootinfo.kernel_filename = strdup(machine->kernel_filename);
    rp->bootinfo.kernel_cmdline = strdup(machine->kernel_cmdline);
    rp->bootinfo.initrd_filename = strdup(machine->initrd_filename);
    rp->bootinfo.ram_size = MIN(ram_size, GOLDFISH_IO_SPACE);
    rp->fdt = fdt;
    rp->fdt_size = fdt_size;
    rp->cpu = cpu;

    cpu_mips_irq_init_cpu(cpu);
    cpu_mips_clock_init(cpu);

    /* Initialize Goldfish PIC */
    s->gfpic = goldfish_pic = goldfish_interrupt_init(devmap[RANCHU_GOLDFISH_PIC].base,
                                            env->irq[2], env->irq[3]);
    /* Alocate dt handle (label) for interrupt-parent */
    devmap[RANCHU_GOLDFISH_PIC].irq = qemu_fdt_alloc_phandle(fdt);

    qemu_fdt_setprop_string(fdt, "/", "model", "ranchu");
    qemu_fdt_setprop_string(fdt, "/", "compatible", "mti,goldfish");
    qemu_fdt_setprop_cell(fdt, "/", "#address-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/", "#size-cells", 0x2);
    qemu_fdt_setprop_cell(fdt, "/", "interrupt-parent", devmap[RANCHU_GOLDFISH_PIC].irq);

    /* Firmware node */
    qemu_fdt_add_subnode(fdt, "/firmware");
    qemu_fdt_add_subnode(fdt, "/firmware/android");
    qemu_fdt_setprop_string(fdt, "/firmware/android", "compatible", "android,firmware");
    qemu_fdt_setprop_string(fdt, "/firmware/android", "hardware", "ranchu");
    qemu_fdt_setprop_string(fdt, "/firmware/android", "revision", MIPS_RANCHU_REV);

    /* CPU node */
    qemu_fdt_add_subnode(fdt, "/cpus");
    qemu_fdt_add_subnode(fdt, "/cpus/cpu@0");
    qemu_fdt_setprop_string(fdt, "/cpus/cpu@0", "device_type", "cpu");
    qemu_fdt_setprop_string(fdt, "/cpus/cpu@0", "compatible", "mti,5KEf");

    /* Memory node */
    qemu_fdt_add_subnode(fdt, "/memory");
    qemu_fdt_setprop_string(fdt, "/memory", "device_type", "memory");
    if (ram_size > GOLDFISH_IO_SPACE) {
        qemu_fdt_setprop_sized_cells(fdt, "/memory", "reg",
                    1, 0, 2, GOLDFISH_IO_SPACE,
                    1, HIGHMEM_OFFSET, 2, ram_size - GOLDFISH_IO_SPACE);
    } else {
        qemu_fdt_setprop_sized_cells(fdt, "/memory", "reg", 1, 0, 2, ram_size);
    }

    /* Create goldfish_pic controller node in dt */
    create_device(fdt, &devmap[RANCHU_GOLDFISH_PIC], NULL, 1, 0);

    /* Make sure the first 3 serial ports are associated with a device. */
    for(i = 0; i < 3; i++) {
        if (!serial_hds[i]) {
            char label[32];
            snprintf(label, sizeof(label), "serial%d", i);
            serial_hds[i] = qemu_chr_new(label, "null");
        }
    }

    /* Create 3 Goldfish TTYs */
    create_device(fdt, &devmap[RANCHU_GOLDFISH_TTY], goldfish_pic, MAX_GF_TTYS, 1);

    /* Other Goldfish Platform devices */
    for (i = RANCHU_GOLDFISH_AUDIO; i >= RANCHU_GOLDFISH_TIMER ; i--) {
        create_device(fdt, &devmap[i], goldfish_pic, 1, 0);
    }

    create_pm_device(fdt, &devmap[RANCHU_GOLDFISH_RESET]);

    /* Virtio MMIO devices */
    create_device(fdt, &devmap[RANCHU_MMIO], goldfish_pic, VIRTIO_TRANSPORTS, 1);

    qemu_fdt_dumpdtb(fdt, fdt_size);

    kernel_entry = android_load_kernel(rp);

    write_bootloader(memory_region_get_ram_ptr(bios),
                     PHYS_TO_VIRT(RESET_ADDRESS), kernel_entry, rp);
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
