#include "hw/sysbus.h"
#include "hw/devices.h"
#include "hw/mips/mips.h"
#include "hw/mips/cpudevs.h"
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
#include "hw/misc/android_pipe.h"
#include "hw/loader.h"
#include "elf.h"
#include "hw/intc/goldfish_pic.h"
#include "hw/irq.h"
#include "hw/display/goldfish_fb.h"
#include "hw/input/goldfish_sensors.h"

#define PHYS_TO_VIRT(x) ((x) | ~(target_ulong)0x7fffffff)

#define ANDROID_CONSOLE_BASEPORT 5554
#define MAX_ANDROID_EMULATORS    64

#define MIPS_CPU_SAVE_VERSION 1
#define MIPS_CPU_IRQ_BASE     8

#define GOLDFISH_IO_SPACE 0x1f000000

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
    RANCHU_GF_PIC,
    RANCHU_GF_TTY,
    RANCHU_GF_TIMER,
    RANCHU_GF_RTC,
    RANCHU_GF_BATTERY,
    RANCHU_GF_FB,
    RANCHU_GF_EVDEV,
    RANCHU_ANDROID_PIPE,
    RANCHU_GF_AUDIO,
    RANCHU_MMIO,
};

typedef struct DevMapEntry {
    hwaddr base;
    hwaddr size;
    int irq;
    const char* qemu_name;
    const char* dt_name;
    const char* dt_compatible;
} DevMapEntry;

static DevMapEntry devmap[] = {
    [RANCHU_GF_PIC] =       { GOLDFISH_IO_SPACE, 0x1000, 0,
        NULL, "goldfish_pic", "generic,goldfish-pic" },
    /* ttyGF0 base address remains hardcoded in the kernel.
     * Early printing (prom_putchar()) relies on finding device
     * mapped on this address. DT can not be used at that early stage
     * for acquiring the base address of the device in the kernel.
     */
    [RANCHU_GF_TTY] =       { GOLDFISH_IO_SPACE + 0x02000, 0x1000, 2,
        "goldfish_tty", "goldfish_tty", "generic,goldfish-tty" },
    /* repeats for a total of MAX_GF_TTYS */
    [RANCHU_GF_TIMER] =     { GOLDFISH_IO_SPACE + 0x05000, 0x1000, 5,
        "goldfish_timer", "goldfish_timer", "generic,goldfish-timer" },
    [RANCHU_GF_RTC] =       { GOLDFISH_IO_SPACE + 0x06000, 0x1000, 6,
        "goldfish_rtc", "goldfish_rtc", "generic,goldfish-rtc" },
    [RANCHU_GF_BATTERY] =   { GOLDFISH_IO_SPACE + 0x07000, 0x1000, 7,
        "goldfish_battery", "goldfish_battery", "generic,goldfish-battery" },
    [RANCHU_GF_FB] =        { GOLDFISH_IO_SPACE + 0x08000, 0x0100, 8,
        "goldfish_fb", "goldfish_fb", "generic,goldfish-fb" },
    [RANCHU_GF_EVDEV] =     { GOLDFISH_IO_SPACE + 0x09000, 0x1000, 9,
        "goldfish-events", "goldfish_events", "generic,goldfish-events-keypad" },
    [RANCHU_ANDROID_PIPE] = { GOLDFISH_IO_SPACE + 0x0A000, 0x2000, 10,
        "android_pipe", "android_pipe", "generic,android-pipe" },
    [RANCHU_GF_AUDIO] =     { GOLDFISH_IO_SPACE + 0x0C000, 0x0100, 11,
        "goldfish_audio", "goldfish_audio", "generic,goldfish-audio" },
    [RANCHU_MMIO] =         { GOLDFISH_IO_SPACE + 0x10000, 0x0200, 16,
        "virtio-mmio", "virtio_mmio", "virtio,mmio" },
    /* ...repeating for a total of VIRTIO_TRANSPORTS, each of that size */
};

struct machine_params {
    uint64_t kernel_entry;
    uint64_t cmdline_ptr;
    MIPSCPU *cpu;
} ranchu_params;

static int ranchu_rotation_state = 0;       /* 0-3 */

static void android_console_rotate_screen(Monitor *mon, const QDict *qdict)
{
    ranchu_rotation_state = ((ranchu_rotation_state + 1) % 4);
    goldfish_sensors_set_rotation(ranchu_rotation_state);
    /* The mapping between QEMU and Android's idea of rotation are
       reversed */
    switch (ranchu_rotation_state) {
    case 0:
        goldfish_fb_set_rotation(0);
        graphic_rotate = 0;
        break;
    case 1:
        goldfish_fb_set_rotation(3);
        graphic_rotate = 90;
        break;
    case 2:
        goldfish_fb_set_rotation(2);
        graphic_rotate = 180;
        break;
    case 3:
        goldfish_fb_set_rotation(1);
        graphic_rotate = 270;
        break;
    default:
        g_assert_not_reached();
    }
}

static mon_cmd_t rotate_cmd = {
    .name = "rotate",
    .args_type = "",
    .params = "",
    .help = "rotate the screen by 90 degrees",
    .mhandler.cmd = android_console_rotate_screen,
};

static void main_cpu_reset(void* opaque1)
{
    struct machine_params *opaque = (struct machine_params *)opaque1;
    MIPSCPU *cpu = opaque->cpu;

    cpu_reset(CPU(cpu));

    cpu->env.active_tc.PC = opaque->kernel_entry;
    cpu->env.active_tc.gpr[4] = opaque->cmdline_ptr; /* a0 */
    cpu->env.active_tc.gpr[5] = 0;                   /* a1 */
    cpu->env.active_tc.gpr[6] = 0;                   /* a2 */
    cpu->env.active_tc.gpr[7] = 0;                   /* a3 */

}

static CharDriverState *try_to_create_console_chardev(int portno)
{
    /* Try to create the chardev for the Android console on the specified port.
     * This is equivalent to the command line options
     *  -chardev socket,id=monitor,host=127.0.0.1,port=NNN,server,nowait,telnet

     *  -mon chardev=monitor,mode=android-console
     * Return true on success, false on failure (presumably port-in-use).
     */
    Error *err = NULL;
    CharDriverState *chr;
    QemuOpts *opts;
    const char *chardev_opts =
        "socket,id=private-chardev-for-android-monitor,"
        "host=127.0.0.1,server,nowait,telnet";

    opts = qemu_opts_parse(qemu_find_opts("chardev"), chardev_opts, 1);
    assert(opts);
    qemu_opt_set_number(opts, "port", portno);
    chr = qemu_chr_new_from_opts(opts, NULL, &err);
    if (err) {
        /* Assume this was port-in-use */
        qemu_opts_del(opts);
        error_free(err);
        return NULL;
    }

    qemu_chr_fe_claim_no_fail(chr);
    return chr;
}

// android_base_port is used across AndroidEmu library to report
// the control console and adb server ports
#ifdef USE_ANDROID_EMU
extern int android_base_port;
#else
// just a fake to reduce the number of conditionals
static int android_base_port = 0;
#endif

static void initialize_console_and_adb(void)
{
    /* Initialize the console and ADB, which must listen on two
     * consecutive TCP ports starting from 5555 and working up until
     * we manage to open both connections.
     */
    int baseport = (android_base_port > ANDROID_CONSOLE_BASEPORT) ?
        android_base_port : ANDROID_CONSOLE_BASEPORT;
    int tries = MAX_ANDROID_EMULATORS;
    CharDriverState *chr;
    struct Monitor* android_monitor;

    for (; tries > 0; tries--, baseport += 2) {
        chr = try_to_create_console_chardev(baseport);
        if (!chr) {
            continue;
        }

        if (!qemu2_adb_server_init(baseport + 1)) {
            qemu_chr_delete(chr);
            chr = NULL;
            continue;
        }

        /* Confirmed we have both ports, now we can create the console itself.
         * This is equivalent to
         * "-mon chardev=private-chardev,mode=android-console"
         */
        /* monitor_init(chr, MONITOR_ANDROID_CONSOLE | MONITOR_USE_READLINE); */
        android_monitor = monitor_init(chr,
                                       MONITOR_ANDROID_CONSOLE |
                                       MONITOR_USE_READLINE |
                                       MONITOR_DYNAMIC_CMDS);
        monitor_add_command(android_monitor,
                            &rotate_cmd);

        android_base_port = baseport;

        printf("console on port %d, ADB on port %d\n", baseport, baseport + 1);
        return;
    }
    error_report("it seems too many emulator instances are running "
                 "on this machine. Aborting\n");
    exit(1);
}

static void android_load_kernel(CPUMIPSState *env, int ram_size,
                                const char *kernel_filename,
                                const char *kernel_cmdline,
                                const char *initrd_filename,
                                void* fdt, int fdt_size)
{
    int initrd_size;
    ram_addr_t initrd_offset;
    uint64_t kernel_entry, kernel_low, kernel_high;
    unsigned int cmdline;

    /* Load the kernel.  */
    if (!kernel_filename) {
        fprintf(stderr, "Kernel image must be specified\n");
        exit(1);
    }

    if (load_elf(kernel_filename, cpu_mips_kseg0_to_phys, NULL,
        (uint64_t *)&kernel_entry, (uint64_t *)&kernel_low,
        (uint64_t *)&kernel_high, 0, ELF_MACHINE, 1) < 0) {
        fprintf(stderr, "qemu: could not load kernel '%s'\n", kernel_filename);
        exit(1);
    }

    /* Load the DTB at the kernel_high address, that is the place where
     * kernel with Appended DT support enabled will look for it
     */
    if (fdt) {
        cpu_physical_memory_write(kernel_high, fdt, fdt_size);
        kernel_high += fdt_size;
    }

    ranchu_params.kernel_entry = kernel_entry;

    /* load initrd */
    initrd_size = 0;
    initrd_offset = 0;
    if (initrd_filename) {
        initrd_size = get_image_size (initrd_filename);
        if (initrd_size > 0) {
            initrd_offset = (kernel_high + ~TARGET_PAGE_MASK) & TARGET_PAGE_MASK;
            if (initrd_offset + initrd_size > ram_size) {
                fprintf(stderr,
                    "qemu: memory too small for initial ram disk '%s'\n",
                    initrd_filename);
                exit(1);
            }
            initrd_size = load_image_targphys(initrd_filename,
                                               initrd_offset,
                                               ram_size - initrd_offset);
        }

        if (initrd_size == (target_ulong) -1) {
            fprintf(stderr,
                "qemu: could not load initial ram disk '%s'\n",
                initrd_filename);
            exit(1);
        }
    }

    /* Store command line in top page of memory
     * kernel will copy the command line to a local buffer
     */
    cmdline = ram_size - TARGET_PAGE_SIZE;
    char kernel_cmd[1024];
    if (initrd_size > 0)
        sprintf (kernel_cmd, "%s rd_start=0x%" HWADDR_PRIx " rd_size=%li",
                 kernel_cmdline,
                 (hwaddr)PHYS_TO_VIRT(initrd_offset),
                 (long int)initrd_size);
    else
        strcpy (kernel_cmd, kernel_cmdline);

    cpu_physical_memory_write(ram_size - TARGET_PAGE_SIZE,
                              (void *)kernel_cmd,
                              strlen(kernel_cmd) + 1);

    ranchu_params.cmdline_ptr = PHYS_TO_VIRT(cmdline);
}

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
    int i;

    for (i = 0; i < num_devices; i++) {
        hwaddr base = dev->base + i * dev->size;

        char* nodename = g_strdup_printf("/%s@%" PRIx64, dev->dt_name, base);
        qemu_fdt_add_subnode(fdt, nodename);
        qemu_fdt_setprop(fdt, nodename, "compatible",
                         dev->dt_compatible,
                         strlen(dev->dt_compatible) + 1);
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

static void ranchu_init(MachineState *machine)
{
    MIPSCPU *cpu;
    CPUMIPSState *env;
    qemu_irq *goldfish_pic;
    int i, fdt_size;
    void *fdt;

    DeviceState *dev = qdev_create(NULL, TYPE_MIPS_RANCHU);
    RanchuState *s = MIPS_RANCHU(dev);

    MemoryRegion *ram_lo = g_new(MemoryRegion, 1);
    MemoryRegion *ram_hi = g_new(MemoryRegion, 1);

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

    env = &cpu->env;

    register_savevm(NULL,
                    "cpu",
                    0,
                    MIPS_CPU_SAVE_VERSION,
                    cpu_save,
                    cpu_load,
                    env);

    qemu_register_reset(main_cpu_reset, &ranchu_params);

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

    /* post IO hole, if there is enough RAM */
    if (ram_size > GOLDFISH_IO_SPACE) {
        memory_region_init_ram(ram_hi, NULL, "ranchu_high.ram",
            ram_size - GOLDFISH_IO_SPACE, &error_abort);
        vmstate_register_ram_global(ram_hi);
        memory_region_add_subregion(get_system_memory(), 0x20000000, ram_hi);
    }

    ranchu_params.cpu = cpu;

    cpu_mips_irq_init_cpu(env);
    cpu_mips_clock_init(env);

    /* Initialize Goldfish PIC */
    s->gfpic = goldfish_pic = goldfish_interrupt_init(devmap[RANCHU_GF_PIC].base,
                                            env->irq[2], env->irq[3]);
    /* Alocate dt handle (label) for interrupt-parent */
    devmap[RANCHU_GF_PIC].irq = qemu_fdt_alloc_phandle(fdt);

    qemu_fdt_setprop_string(fdt, "/", "model", "ranchu");
    qemu_fdt_setprop_string(fdt, "/", "compatible", "mti,goldfish");
    qemu_fdt_setprop_cell(fdt, "/", "#address-cells", 0x1);
    qemu_fdt_setprop_cell(fdt, "/", "#size-cells", 0x2);
    qemu_fdt_setprop_cell(fdt, "/", "interrupt-parent", devmap[RANCHU_GF_PIC].irq);

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
                    1, 0x20000000, 2, ram_size - GOLDFISH_IO_SPACE);
    } else {
        qemu_fdt_setprop_sized_cells(fdt, "/memory", "reg", 1, 0, 2, ram_size);
    }

    /* Create goldfish_pic controller node in dt */
    create_device(fdt, &devmap[RANCHU_GF_PIC], NULL, 1, 0);

    /* Create 4 Goldfish TTYs */
    create_device(fdt, &devmap[RANCHU_GF_TTY], goldfish_pic, MAX_GF_TTYS, 0);

    /* Other Goldfish Platform devices */
    for (i = RANCHU_GF_AUDIO; i >= RANCHU_GF_TIMER ; i--) {
        create_device(fdt, &devmap[i], goldfish_pic, 1, 0);
    }

    /* Virtio MMIO devices */
    create_device(fdt, &devmap[RANCHU_MMIO], goldfish_pic, VIRTIO_TRANSPORTS, 1);

    initialize_console_and_adb();

    qemu_fdt_dumpdtb(fdt, fdt_size);

    android_load_kernel(env, MIN(ram_size, GOLDFISH_IO_SPACE),
                        machine->kernel_filename,
                        machine->kernel_cmdline,
                        machine->initrd_filename,
                        fdt, fdt_size);
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
    .name          = TYPE_MIPS_RANCHU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RanchuState),
    .class_init    = mips_ranchu_class_init,
};

static QEMUMachine ranchu_machine = {
    .name = "ranchu",
    .desc = "Ranchu Virtual Machine for Android Emulator",
    .init = ranchu_init,
    .max_cpus = 1,
};

static void mips_ranchu_register_types(void)
{
    type_register_static(&mips_ranchu_device);
}

static void ranchu_machine_init(void)
{
    qemu_register_machine(&ranchu_machine);
}

type_init(mips_ranchu_register_types)
machine_init(ranchu_machine_init);
