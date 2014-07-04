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
#include "net/net.h"
#include "hw/arm/pic.h"
#include "sysemu/sysemu.h"
#include "hw/android/goldfish/device.h"
#include "android/globals.h"
#include "audio/audio.h"
#include "hw/arm/arm.h"
#include "ui/console.h"
#include "sysemu/blockdev.h"
#include "hw/android/goldfish/pipe.h"

#include "android/utils/debug.h"

#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)

#define ARM_CPU_SAVE_VERSION  1

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

static void android_arm_init_(ram_addr_t ram_size,
    const char *boot_device,
    const char *kernel_filename,
    const char *kernel_cmdline,
    const char *initrd_filename,
    const char *cpu_model)
{
    CPUARMState *env;
    qemu_irq *cpu_pic;
    qemu_irq *goldfish_pic;
    int i;
    struct arm_boot_info  info;
    ram_addr_t ram_offset;

    if (!cpu_model)
        cpu_model = "arm926";

    env = cpu_init(cpu_model);

    ram_offset = qemu_ram_alloc(NULL,"android_arm",ram_size);
    cpu_register_physical_memory(0, ram_size, ram_offset | IO_MEM_RAM);

    cpu_pic = arm_pic_init_cpu(env);
    goldfish_pic = goldfish_interrupt_init(0xff000000, cpu_pic[ARM_PIC_CPU_IRQ], cpu_pic[ARM_PIC_CPU_FIQ]);
    goldfish_device_init(goldfish_pic, 0xff010000, 0x7f0000, 10, 22);

    goldfish_device_bus_init(0xff001000, 1);

    goldfish_timer_and_rtc_init(0xff003000, 3);

    goldfish_tty_add(serial_hds[0], 0, 0xff002000, 4);
    for(i = 1; i < MAX_SERIAL_PORTS; i++) {
        //printf("android_arm_init serial %d %x\n", i, serial_hds[i]);
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
                fprintf(stderr, "qemu: Unsupported NIC: %s\n", nd_table[0].model);
                exit (1);
            }
        }
    }

    goldfish_fb_init(0);
#ifdef HAS_AUDIO
    goldfish_audio_init(0xff004000, 0, audio_input_source);
#endif
    {
        DriveInfo* info = drive_get( IF_IDE, 0, 0 );
        if (info != NULL) {
            goldfish_mmc_init(0xff005000, 0, info->bdrv);
        }
    }

    goldfish_battery_init(android_hw->hw_battery);

    goldfish_add_device_no_io(&event0_device);
    events_dev_init(event0_device.base, goldfish_pic[event0_device.irq]);

#ifdef CONFIG_NAND
    goldfish_add_device_no_io(&nand_device);
    nand_dev_init(nand_device.base);
#endif

    bool newDeviceNaming =
            (androidHwConfig_getKernelDeviceNaming(android_hw) >= 1);
    pipe_dev_init(newDeviceNaming);

    memset(&info, 0, sizeof info);
    info.ram_size        = ram_size;
    info.kernel_filename = kernel_filename;
    info.kernel_cmdline  = kernel_cmdline;
    info.initrd_filename = initrd_filename;
    info.nb_cpus         = 1;
    info.is_linux        = 1;
    info.board_id        = 1441;

    arm_load_kernel(env, &info);
}

QEMUMachine android_arm_machine = {
    "android_arm",
    "ARM Android Emulator",
    android_arm_init_,
    0,
    0,
    1,
    NULL
};

static void android_arm_init(void)
{
    qemu_register_machine(&android_arm_machine);
}

machine_init(android_arm_init);
