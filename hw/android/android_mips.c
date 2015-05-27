/* Copyright (C) 2007-2008 The Android Open Source Project
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
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/devices.h"
#include "hw/loader.h"
#include "net/net.h"
#include "sysemu/sysemu.h"
#include "hw/mips/mips.h"
#include "hw/android/goldfish/device.h"
#include "hw/android/goldfish/pipe.h"
#include "android/globals.h"
#include "audio/audio.h"
#include "exec/ram_addr.h"
#include "sysemu/blockdev.h"

#include "android/utils/debug.h"

#define D(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#define E(...)  fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

#define MIPS_CPU_SAVE_VERSION  1

#define HIGHMEM_OFFSET      0x20000000
#define GOLDFISH_IO_SPACE   0x1f000000

#define GOLDFISH_INTERRUPT  (GOLDFISH_IO_SPACE + 0x00000)
#define GOLDFISH_DEVICEBUS  (GOLDFISH_IO_SPACE + 0x01000)
#define GOLDFISH_TTY        (GOLDFISH_IO_SPACE + 0x02000)
#define GOLDFISH_RTC        (GOLDFISH_IO_SPACE + 0x03000)
#define GOLDFISH_AUDIO      (GOLDFISH_IO_SPACE + 0x04000)
#define GOLDFISH_MMC        (GOLDFISH_IO_SPACE + 0x05000)
#define GOLDFISH_MEMLOG     (GOLDFISH_IO_SPACE + 0x06000)
#define GOLDFISH_DEVICES    (GOLDFISH_IO_SPACE + 0x10000)

#define MAX_RAM_SIZE_MB 4079UL

static unsigned int highmem_size = 0;

char* audio_input_source = NULL;

static struct goldfish_device event0_device = {
    .name = "goldfish_events",
    .id = 0,
    .size = 0x1000,
    .irq_count = 1
};

static struct goldfish_device nand_device = {
    .name = "goldfish_nand",
    .id = 0,
    .size = 0x1000
};

/* Board init.  */

#define VIRT_TO_PHYS_ADDEND (0xffffffff80000000LL)

#define PHYS_TO_VIRT(x) ((x) | ~(target_ulong)0x7fffffff)

static void android_load_kernel(CPUOldState *env, int ram_size,
                                const char *kernel_filename,
                                const char *kernel_cmdline,
                                const char *initrd_filename)
{
    int initrd_size;
    ram_addr_t initrd_offset;
    uint64_t kernel_entry, kernel_low, kernel_high;
    unsigned int cmdline;

    /* Load the kernel.  */
    if (!kernel_filename) {
        E("Kernel image must be specified\n");
        exit(1);
    }

    if (load_elf(kernel_filename, VIRT_TO_PHYS_ADDEND,
                 (uint64_t *)&kernel_entry,
                 (uint64_t *)&kernel_low,
                 (uint64_t *)&kernel_high) < 0) {
        E("qemu: could not load kernel '%s'\n", kernel_filename);
        exit(1);
    }
    env->active_tc.PC = (int32_t)kernel_entry;

    /* load initrd */
    initrd_size = 0;
    initrd_offset = 0;
    if (initrd_filename) {
        initrd_size = get_image_size (initrd_filename);
        if (initrd_size > 0) {
            initrd_offset = (kernel_high + ~TARGET_PAGE_MASK) & TARGET_PAGE_MASK;
            if (initrd_offset + initrd_size > ram_size) {
                E("qemu: memory too small for initial ram disk '%s'\n", initrd_filename);
                exit(1);
            }
            initrd_size = load_image_targphys(initrd_filename,
                                              initrd_offset,
                                              ram_size - initrd_offset);
        }
        if (initrd_size == (target_ulong) -1) {
            E("qemu: could not load initial ram disk '%s'\n", initrd_filename);
            exit(1);
        }
    }

    /* Store command line in top page of memory
     * kernel will copy the command line to a loca buffer
     */
    cmdline = ram_size - TARGET_PAGE_SIZE;
    char kernel_cmd[1024];
    if (initrd_size > 0) {
        sprintf (kernel_cmd, "%s rd_start=0x%" HWADDR_PRIx " rd_size=%li",
                 kernel_cmdline,
                 (hwaddr)PHYS_TO_VIRT(initrd_offset),
                 (long int)initrd_size);
    } else {
        strcpy (kernel_cmd, kernel_cmdline);
    }

    char kernel_cmd2[1024];
    if (highmem_size) {
        sprintf (kernel_cmd2, "%s mem=%um@0x0 mem=%um@0x%x",
                 kernel_cmd,
                 GOLDFISH_IO_SPACE / (1024 * 1024),
                 highmem_size / (1024 * 1024),
                 HIGHMEM_OFFSET);
    } else {
        strcpy (kernel_cmd2, kernel_cmd);
    }

    cpu_physical_memory_write(ram_size - TARGET_PAGE_SIZE, (void *)kernel_cmd2, strlen(kernel_cmd2) + 1);

#if 0
    if (initrd_size > 0)
        sprintf (phys_ram_base+cmdline, "%s rd_start=0x" TARGET_FMT_lx " rd_size=%li",
                       kernel_cmdline,
                       PHYS_TO_VIRT(initrd_offset), initrd_size);
    else
        strcpy (phys_ram_base+cmdline, kernel_cmdline);
#endif

    env->active_tc.gpr[4] = PHYS_TO_VIRT(cmdline);  /* a0 */
    env->active_tc.gpr[5] = ram_size;               /* a1 */
    env->active_tc.gpr[6] = highmem_size;           /* a2 */
    env->active_tc.gpr[7] = 0;                      /* a3 */
}


static void android_mips_init_(ram_addr_t ram_size,
                               const char *boot_device,
                               const char *kernel_filename,
                               const char *kernel_cmdline,
                               const char *initrd_filename,
                               const char *cpu_model)
{
    CPUOldState *env;
    qemu_irq *goldfish_pic;
    int i;
    ram_addr_t ram_offset;

    if (!cpu_model)
        cpu_model = "24Kf";

    env = cpu_init(cpu_model);

    register_savevm(NULL,
                    "cpu",
                    0,
                    MIPS_CPU_SAVE_VERSION,
                    cpu_save,
                    cpu_load,
                    env);

    if (ram_size > (MAX_RAM_SIZE_MB << 20)) {
        D("qemu: Too much memory for this machine. "
          "RAM size reduced to %lu MB\n", MAX_RAM_SIZE_MB);
        ram_size = MAX_RAM_SIZE_MB << 20;
    }

    ram_offset = qemu_ram_alloc(NULL, "android_mips", ram_size);
    cpu_register_physical_memory(0, MIN(ram_size, GOLDFISH_IO_SPACE), ram_offset | IO_MEM_RAM);

    if (ram_size > GOLDFISH_IO_SPACE) {
        highmem_size = ram_size - GOLDFISH_IO_SPACE;
        cpu_register_physical_memory(HIGHMEM_OFFSET, highmem_size, ram_offset + GOLDFISH_IO_SPACE);
    }

    /* Init internal devices */
    cpu_mips_irq_init_cpu(env);
    cpu_mips_clock_init(env);

    goldfish_pic = goldfish_interrupt_init(GOLDFISH_INTERRUPT,
                                           env->irq[2], env->irq[3]);
    goldfish_device_init(goldfish_pic, GOLDFISH_DEVICES, 0x7f0000, 10, 22);

    goldfish_device_bus_init(GOLDFISH_DEVICEBUS, 1);

    goldfish_timer_and_rtc_init(GOLDFISH_RTC, 3);

    goldfish_tty_add(serial_hds[0], 0, GOLDFISH_TTY, 4);
    for(i = 1; i < MAX_SERIAL_PORTS; i++) {
        if(serial_hds[i]) {
            goldfish_tty_add(serial_hds[i], i, 0, 0);
        }
    }

    for(i = 0; i < MAX_NICS; i++) {
        if (nd_table[i].vlan) {
            if (nd_table[i].model == NULL
                || strcmp(nd_table[i].model, "smc91c111") == 0) {
                struct goldfish_device *smc_device;
                smc_device = g_malloc0(sizeof(*smc_device));
                smc_device->name = "smc91x";
                smc_device->id = i;
                smc_device->size = 0x1000;
                smc_device->irq_count = 1;
                goldfish_add_device_no_io(smc_device);
                smc91c111_init(&nd_table[i], smc_device->base, goldfish_pic[smc_device->irq]);
            } else {
                E("qemu: Unsupported NIC: %s\n", nd_table[0].model);
                exit (1);
            }
        }
    }

    goldfish_fb_init(0);
#ifdef HAS_AUDIO
    goldfish_audio_init(GOLDFISH_AUDIO, 0, audio_input_source);
#endif
    {
        DriveInfo* info = drive_get( IF_IDE, 0, 0 );
        if (info != NULL) {
            goldfish_mmc_init(GOLDFISH_MMC, 0, info->bdrv);
        }
    }
    goldfish_battery_init(android_hw->hw_battery);

    goldfish_add_device_no_io(&event0_device);
    events_dev_init(event0_device.base, goldfish_pic[event0_device.irq]);

    goldfish_add_device_no_io(&nand_device);
    nand_dev_init(nand_device.base);

    bool newDeviceNaming =
            (androidHwConfig_getKernelDeviceNaming(android_hw) >= 1);
    pipe_dev_init(newDeviceNaming);

    android_load_kernel(env, MIN(ram_size, GOLDFISH_IO_SPACE), kernel_filename, kernel_cmdline, initrd_filename);
}


QEMUMachine android_mips_machine = {
    "android_mips",
    "MIPS Android Emulator",
    android_mips_init_,
    0,
    0,
    1,
    NULL
};

static void android_mips_init(void)
{
    qemu_register_machine(&android_mips_machine);
}

machine_init(android_mips_init);
