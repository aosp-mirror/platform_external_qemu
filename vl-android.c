/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* the following is needed on Linux to define ptsname() in stdlib.h */
#if defined(__linux__)
#define _GNU_SOURCE 1
#endif

#include "qemu-common.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/usb.h"
#include "hw/pcmcia.h"
#include "hw/i386/pc.h"
#include "hw/audiodev.h"
#include "hw/isa/isa.h"
#include "hw/loader.h"
#include "hw/baum.h"
#include "hw/android/goldfish/nand.h"
#include "net/net.h"
#include "ui/console.h"
#include "sysemu/sysemu.h"
#include "exec/gdbstub.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "sysemu/char.h"
#include "sysemu/blockdev.h"
#include "audio/audio.h"

#include "migration/qemu-file.h"
#include "android/android.h"
#include "android/charpipe.h"
#include "android/log-rotate.h"
#include "modem_driver.h"
#include "android/filesystems/ext4_utils.h"
#include "android/filesystems/fstab_parser.h"
#include "android/filesystems/partition_types.h"
#include "android/filesystems/ramdisk_extractor.h"
#include "android/gps.h"
#include "android/hw-kmsg.h"
#include "android/hw-pipe-net.h"
#include "android/hw-qemud.h"
#include "android/camera/camera-service.h"
#include "android/multitouch-port.h"
#include "android/charmap.h"
#include "android/globals.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/path.h"
#include "android/utils/stralloc.h"
#include "android/utils/tempfile.h"
#include "android/display-core.h"
#include "android/utils/timezone.h"
#include "android/snapshot.h"
#include "android/opengles.h"
#include "android/multitouch-screen.h"
#include "exec/hwaddr.h"
#include "android/tcpdump.h"

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <zlib.h>

/* Needed early for CONFIG_BSD etc. */
#include "config-host.h"

#ifndef _WIN32
#include <libgen.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#if defined(__NetBSD__)
#include <net/if_tap.h>
#endif
#ifdef __linux__
#include <linux/if_tun.h>
#endif
#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <sys/select.h>
#ifdef CONFIG_BSD
#include <sys/stat.h>
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <libutil.h>
#else
#include <util.h>
#endif
#elif defined (__GLIBC__) && defined (__FreeBSD_kernel__)
#include <freebsd/stdlib.h>
#else
#ifdef __linux__
#include <pty.h>
#include <malloc.h>
#include <linux/rtc.h>

/* For the benefit of older linux systems which don't supply it,
   we use a local copy of hpet.h. */
/* #include <linux/hpet.h> */
#include "hw/timer/hpet.h"

#include <linux/ppdev.h>
#include <linux/parport.h>
#endif
#ifdef __sun__
#include <sys/stat.h>
#include <sys/ethernet.h>
#include <sys/sockio.h>
#include <netinet/arp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h> // must come after ip.h
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <syslog.h>
#include <stropts.h>
#endif
#endif
#endif

#if defined(__OpenBSD__)
#include <util.h>
#endif

#if defined(CONFIG_VDE)
#include <libvdeplug.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#include <sys/timeb.h>
#include <mmsystem.h>
#define getopt_long_only getopt_long
#define memalign(align, size) malloc(size)
#endif

#include "sysemu/cpus.h"
#include "sysemu/arch_init.h"

#ifdef CONFIG_COCOA
int qemu_main(int argc, char **argv, char **envp);
#undef main
#define main qemu_main
#endif /* CONFIG_COCOA */

#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/usb.h"
#include "hw/pcmcia.h"
#include "hw/i386/pc.h"
#include "hw/isa/isa.h"
#include "hw/baum.h"
#include "hw/bt.h"
#include "sysemu/watchdog.h"
#include "hw/i386/smbios.h"
#include "hw/xen/xen.h"
#include "sysemu/bt.h"
#include "net/net.h"
#include "monitor/monitor.h"
#include "ui/console.h"
#include "sysemu/sysemu.h"
#include "exec/gdbstub.h"
#include "qemu/timer.h"
#include "sysemu/char.h"
#include "qemu/cache-utils.h"
#include "block/block.h"
#include "sysemu/dma.h"
#include "audio/audio.h"
#include "migration/migration.h"
#include "sysemu/kvm.h"
#include "exec/hax.h"
#ifdef CONFIG_KVM
#include "android/kvm.h"
#endif
#include "sysemu/balloon.h"
#include "android/hw-lcd.h"
#include "android/boot-properties.h"
#include "android/hw-control.h"
#include "android/core-init-utils.h"
#include "android/audio-test.h"

#include "android/snaphost-android.h"

#if !defined(CONFIG_STANDALONE_CORE)
/* in android/qemulator.c */
extern void  android_emulator_set_base_port(int  port);
#endif

#if defined(CONFIG_SKINS) && !defined(CONFIG_STANDALONE_CORE)
#undef main
#define main qemu_main
#endif

#include "disas/disas.h"

#include "qemu/sockets.h"

#if defined(CONFIG_SLIRP)
#include "libslirp.h"
#endif

#define DEFAULT_RAM_SIZE 128

/* Max number of USB devices that can be specified on the commandline.  */
#define MAX_USB_CMDLINE 8

/* Max number of bluetooth switches on the commandline.  */
#define MAX_BT_CMDLINE 10

/* XXX: use a two level table to limit memory usage */

static const char *data_dir;
const char *bios_name = NULL;
static void *ioport_opaque[MAX_IOPORTS];
static IOPortReadFunc *ioport_read_table[3][MAX_IOPORTS];
static IOPortWriteFunc *ioport_write_table[3][MAX_IOPORTS];
#ifdef MAX_DRIVES
/* Note: drives_table[MAX_DRIVES] is a dummy block driver if none available
   to store the VM snapshots */
DriveInfo drives_table[MAX_DRIVES+1];
int nb_drives;
#endif
enum vga_retrace_method vga_retrace_method = VGA_RETRACE_DUMB;
DisplayType display_type = DT_DEFAULT;
const char* keyboard_layout = NULL;
int64_t ticks_per_sec;
ram_addr_t ram_size;
bool xen_allowed;
const char *mem_path = NULL;
#ifdef MAP_POPULATE
int mem_prealloc = 0; /* force preallocation of physical target memory */
#endif
int nb_nics;
NICInfo nd_table[MAX_NICS];
int vm_running;
int autostart;
static int rtc_utc = 1;
static int rtc_date_offset = -1; /* -1 means no change */
int cirrus_vga_enabled = 1;
int std_vga_enabled = 0;
int vmsvga_enabled = 0;
int xenfb_enabled = 0;
static int full_screen = 0;
#ifdef CONFIG_SDL
static int no_frame = 0;
#endif
int no_quit = 0;
CharDriverState *serial_hds[MAX_SERIAL_PORTS];
int              serial_hds_count;

CharDriverState *parallel_hds[MAX_PARALLEL_PORTS];
CharDriverState *virtcon_hds[MAX_VIRTIO_CONSOLES];
#ifdef TARGET_I386
int win2k_install_hack = 0;
int rtc_td_hack = 0;
#endif
int usb_enabled = 0;
int singlestep = 0;
int smp_cpus = 1;
const char *vnc_display;
int acpi_enabled = 1;
int no_hpet = 0;
int hax_disabled = 1;
int no_virtio_balloon = 0;
int fd_bootchk = 1;
int no_reboot = 0;
int no_shutdown = 0;
int cursor_hide = 1;
int graphic_rotate = 0;
WatchdogTimerModel *watchdog = NULL;
int watchdog_action = WDT_RESET;
const char *option_rom[MAX_OPTION_ROMS];
int nb_option_roms;
int semihosting_enabled = 0;
#ifdef TARGET_ARM
int old_param = 0;
#endif
const char *qemu_name;
int alt_grab = 0;
#if defined(TARGET_SPARC) || defined(TARGET_PPC)
unsigned int nb_prom_envs = 0;
const char *prom_envs[MAX_PROM_ENVS];
#endif
#ifdef MAX_DRIVES
int nb_drives_opt;
struct drive_opt drives_opt[MAX_DRIVES];
#endif
int nb_numa_nodes;
uint64_t node_mem[MAX_NODES];
uint64_t node_cpumask[MAX_NODES];

static QEMUTimer *nographic_timer;

uint8_t qemu_uuid[16];


int   qemu_cpu_delay;
extern char* audio_input_source;

extern char* android_op_ports;
extern char* android_op_port;
extern char* android_op_report_console;
extern char* op_http_proxy;
// Path to the file containing specific key character map.
char* op_charmap_file = NULL;

/* Path to hardware initialization file passed with -android-hw option. */
char* android_op_hwini = NULL;

/* Memory checker options. */
char* android_op_memcheck = NULL;

/* -dns-server option value. */
char* android_op_dns_server = NULL;

/* -radio option value. */
char* android_op_radio = NULL;

/* -gps option value. */
char* android_op_gps = NULL;

/* -audio option value. */
char* android_op_audio = NULL;

/* -cpu-delay option value. */
char* android_op_cpu_delay = NULL;

#ifdef CONFIG_NAND_LIMITS
/* -nand-limits option value. */
char* android_op_nand_limits = NULL;
#endif  // CONFIG_NAND_LIMITS

/* -netspeed option value. */
char* android_op_netspeed = NULL;

/* -netdelay option value. */
char* android_op_netdelay = NULL;

/* -netfast option value. */
int android_op_netfast = 0;

/* -tcpdump option value. */
char* android_op_tcpdump = NULL;

/* -lcd-density option value. */
char* android_op_lcd_density = NULL;

/* -ui-port option value. This port will be used to report the core
 * initialization completion.
 */
char* android_op_ui_port = NULL;

/* -ui-settings option value. This value will be passed to the UI when new UI
 * process is attaching to the core.
 */
char* android_op_ui_settings = NULL;

/* -android-avdname option value. */
char* android_op_avd_name = "unknown";

bool android_op_wipe_data = false;

extern int android_display_width;
extern int android_display_height;
extern int android_display_bpp;

extern void  dprint( const char* format, ... );

const char* dns_log_filename = NULL;
const char* drop_log_filename = NULL;

const char* savevm_on_exit = NULL;

#define TFR(expr) do { if ((expr) != -1) break; } while (errno == EINTR)

/* Reports the core initialization failure to the error stdout and to the UI
 * socket before exiting the application.
 * Parameters that are passed to this macro are used to format the error
 * mesage using sprintf routine.
 */
#ifdef CONFIG_ANDROID
#define  PANIC(...) android_core_init_failure(__VA_ARGS__)
#else
#define  PANIC(...) do { fprintf(stderr, __VA_ARGS__);  \
                         exit(1);                       \
                    } while (0)
#endif  // CONFIG_ANDROID

/* Exits the core during initialization. */
#ifdef CONFIG_ANDROID
#define  QEMU_EXIT(exit_code) android_core_init_exit(exit_code)
#else
#define  QEMU_EXIT(exit_code) exit(exit_code)
#endif  // CONFIG_ANDROID

/***********************************************************/
/* x86 ISA bus support */

hwaddr isa_mem_base = 0;
PicState2 *isa_pic;

static IOPortReadFunc default_ioport_readb, default_ioport_readw, default_ioport_readl;
static IOPortWriteFunc default_ioport_writeb, default_ioport_writew, default_ioport_writel;

static uint32_t ioport_read(int index, uint32_t address)
{
    static IOPortReadFunc *default_func[3] = {
        default_ioport_readb,
        default_ioport_readw,
        default_ioport_readl
    };
    IOPortReadFunc *func = ioport_read_table[index][address];
    if (!func)
        func = default_func[index];
    return func(ioport_opaque[address], address);
}

static void ioport_write(int index, uint32_t address, uint32_t data)
{
    static IOPortWriteFunc *default_func[3] = {
        default_ioport_writeb,
        default_ioport_writew,
        default_ioport_writel
    };
    IOPortWriteFunc *func = ioport_write_table[index][address];
    if (!func)
        func = default_func[index];
    func(ioport_opaque[address], address, data);
}

static uint32_t default_ioport_readb(void *opaque, uint32_t address)
{
#ifdef DEBUG_UNUSED_IOPORT
    fprintf(stderr, "unused inb: port=0x%04x\n", address);
#endif
    return 0xff;
}

static void default_ioport_writeb(void *opaque, uint32_t address, uint32_t data)
{
#ifdef DEBUG_UNUSED_IOPORT
    fprintf(stderr, "unused outb: port=0x%04x data=0x%02x\n", address, data);
#endif
}

/* default is to make two byte accesses */
static uint32_t default_ioport_readw(void *opaque, uint32_t address)
{
    uint32_t data;
    data = ioport_read(0, address);
    address = (address + 1) & (MAX_IOPORTS - 1);
    data |= ioport_read(0, address) << 8;
    return data;
}

static void default_ioport_writew(void *opaque, uint32_t address, uint32_t data)
{
    ioport_write(0, address, data & 0xff);
    address = (address + 1) & (MAX_IOPORTS - 1);
    ioport_write(0, address, (data >> 8) & 0xff);
}

static uint32_t default_ioport_readl(void *opaque, uint32_t address)
{
#ifdef DEBUG_UNUSED_IOPORT
    fprintf(stderr, "unused inl: port=0x%04x\n", address);
#endif
    return 0xffffffff;
}

static void default_ioport_writel(void *opaque, uint32_t address, uint32_t data)
{
#ifdef DEBUG_UNUSED_IOPORT
    fprintf(stderr, "unused outl: port=0x%04x data=0x%02x\n", address, data);
#endif
}

/***************/
/* ballooning */

static QEMUBalloonEvent *qemu_balloon_event;
void *qemu_balloon_event_opaque;

void qemu_add_balloon_handler(QEMUBalloonEvent *func, void *opaque)
{
    qemu_balloon_event = func;
    qemu_balloon_event_opaque = opaque;
}

void qemu_balloon(ram_addr_t target)
{
    if (qemu_balloon_event)
        qemu_balloon_event(qemu_balloon_event_opaque, target);
}

ram_addr_t qemu_balloon_status(void)
{
    if (qemu_balloon_event)
        return qemu_balloon_event(qemu_balloon_event_opaque, 0);
    return 0;
}

/***********************************************************/
/* host time/date access */
void qemu_get_timedate(struct tm *tm, int offset)
{
    time_t ti;
    struct tm *ret;

    time(&ti);
    ti += offset;
    if (rtc_date_offset == -1) {
        if (rtc_utc)
            ret = gmtime(&ti);
        else
            ret = localtime(&ti);
    } else {
        ti -= rtc_date_offset;
        ret = gmtime(&ti);
    }

    memcpy(tm, ret, sizeof(struct tm));
}

int qemu_timedate_diff(struct tm *tm)
{
    time_t seconds;

    if (rtc_date_offset == -1)
        if (rtc_utc)
            seconds = mktimegm(tm);
        else
            seconds = mktime(tm);
    else
        seconds = mktimegm(tm) + rtc_date_offset;

    return seconds - time(NULL);
}

/***********************************************************/
/* QEMU Block devices */

#define HD_ALIAS "index=%d,media=disk"
#define CDROM_ALIAS "index=2,media=cdrom"
#define FD_ALIAS "index=%d,if=floppy"
#define PFLASH_ALIAS "if=pflash"
#define MTD_ALIAS "if=mtd"
#define SD_ALIAS "index=0,if=sd"

static int drive_init_func(QemuOpts *opts, void *opaque)
{
    int *use_scsi = opaque;
    int fatal_error = 0;

    if (drive_init(opts, *use_scsi, &fatal_error) == NULL) {
        if (fatal_error)
            return 1;
    }
    return 0;
}

static int drive_enable_snapshot(QemuOpts *opts, void *opaque)
{
    if (NULL == qemu_opt_get(opts, "snapshot")) {
        qemu_opt_set(opts, "snapshot", "on");
    }
    return 0;
}

#ifdef MAX_DRIVES
static int drive_opt_get_free_idx(void)
{
    int index;

    for (index = 0; index < MAX_DRIVES; index++)
        if (!drives_opt[index].used) {
            drives_opt[index].used = 1;
            return index;
        }

    return -1;
}

static int drive_get_free_idx(void)
{
    int index;

    for (index = 0; index < MAX_DRIVES; index++)
        if (!drives_table[index].used) {
            drives_table[index].used = 1;
            return index;
        }

    return -1;
}

int drive_add(const char *file, const char *fmt, ...)
{
    va_list ap;
    int index = drive_opt_get_free_idx();

    if (nb_drives_opt >= MAX_DRIVES || index == -1) {
        fprintf(stderr, "qemu: too many drives\n");
        return -1;
    }

    drives_opt[index].file = file;
    va_start(ap, fmt);
    vsnprintf(drives_opt[index].opt,
              sizeof(drives_opt[0].opt), fmt, ap);
    va_end(ap);

    nb_drives_opt++;
    return index;
}

void drive_remove(int index)
{
    drives_opt[index].used = 0;
    nb_drives_opt--;
}

int drive_get_index(BlockInterfaceType type, int bus, int unit)
{
    int index;

    /* seek interface, bus and unit */

    for (index = 0; index < MAX_DRIVES; index++)
        if (drives_table[index].type == type &&
	    drives_table[index].bus == bus &&
	    drives_table[index].unit == unit &&
	    drives_table[index].used)
        return index;

    return -1;
}

int drive_get_max_bus(BlockInterfaceType type)
{
    int max_bus;
    int index;

    max_bus = -1;
    for (index = 0; index < nb_drives; index++) {
        if(drives_table[index].type == type &&
           drives_table[index].bus > max_bus)
            max_bus = drives_table[index].bus;
    }
    return max_bus;
}

const char *drive_get_serial(BlockDriverState *bdrv)
{
    int index;

    for (index = 0; index < nb_drives; index++)
        if (drives_table[index].bdrv == bdrv)
            return drives_table[index].serial;

    return "\0";
}

BlockInterfaceErrorAction drive_get_onerror(BlockDriverState *bdrv)
{
    int index;

    for (index = 0; index < nb_drives; index++)
        if (drives_table[index].bdrv == bdrv)
            return drives_table[index].onerror;

    return BLOCK_ERR_STOP_ENOSPC;
}

static void bdrv_format_print(void *opaque, const char *name)
{
    fprintf(stderr, " %s", name);
}

void drive_uninit(BlockDriverState *bdrv)
{
    int i;

    for (i = 0; i < MAX_DRIVES; i++)
        if (drives_table[i].bdrv == bdrv) {
            drives_table[i].bdrv = NULL;
            drives_table[i].used = 0;
            drive_remove(drives_table[i].drive_opt_idx);
            nb_drives--;
            break;
        }
}

int drive_init(struct drive_opt *arg, int snapshot, void *opaque)
{
    char buf[128];
    char file[1024];
    char devname[128];
    char serial[21];
    const char *mediastr = "";
    BlockInterfaceType type;
    enum { MEDIA_DISK, MEDIA_CDROM } media;
    int bus_id, unit_id;
    int cyls, heads, secs, translation;
    BlockDriverState *bdrv;
    BlockDriver *drv = NULL;
    QEMUMachine *machine = opaque;
    int max_devs;
    int index;
    int cache;
    int bdrv_flags, onerror;
    int drives_table_idx;
    char *str = arg->opt;
    static const char * const params[] = { "bus", "unit", "if", "index",
                                           "cyls", "heads", "secs", "trans",
                                           "media", "snapshot", "file",
                                           "cache", "format", "serial", "werror",
                                           NULL };

    if (check_params(buf, sizeof(buf), params, str) < 0) {
         fprintf(stderr, "qemu: unknown parameter '%s' in '%s'\n",
                         buf, str);
         return -1;
    }

    file[0] = 0;
    cyls = heads = secs = 0;
    bus_id = 0;
    unit_id = -1;
    translation = BIOS_ATA_TRANSLATION_AUTO;
    index = -1;
    cache = 3;

    if (machine->use_scsi) {
        type = IF_SCSI;
        max_devs = MAX_SCSI_DEVS;
        pstrcpy(devname, sizeof(devname), "scsi");
    } else {
        type = IF_IDE;
        max_devs = MAX_IDE_DEVS;
        pstrcpy(devname, sizeof(devname), "ide");
    }
    media = MEDIA_DISK;

    /* extract parameters */

    if (get_param_value(buf, sizeof(buf), "bus", str)) {
        bus_id = strtol(buf, NULL, 0);
	if (bus_id < 0) {
	    fprintf(stderr, "qemu: '%s' invalid bus id\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "unit", str)) {
        unit_id = strtol(buf, NULL, 0);
	if (unit_id < 0) {
	    fprintf(stderr, "qemu: '%s' invalid unit id\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "if", str)) {
        pstrcpy(devname, sizeof(devname), buf);
        if (!strcmp(buf, "ide")) {
	    type = IF_IDE;
            max_devs = MAX_IDE_DEVS;
        } else if (!strcmp(buf, "scsi")) {
	    type = IF_SCSI;
            max_devs = MAX_SCSI_DEVS;
        } else if (!strcmp(buf, "floppy")) {
	    type = IF_FLOPPY;
            max_devs = 0;
        } else if (!strcmp(buf, "pflash")) {
	    type = IF_PFLASH;
            max_devs = 0;
	} else if (!strcmp(buf, "mtd")) {
	    type = IF_MTD;
            max_devs = 0;
	} else if (!strcmp(buf, "sd")) {
	    type = IF_SD;
            max_devs = 0;
        } else if (!strcmp(buf, "virtio")) {
            type = IF_VIRTIO;
            max_devs = 0;
	} else if (!strcmp(buf, "xen")) {
	    type = IF_XEN;
            max_devs = 0;
	} else {
            fprintf(stderr, "qemu: '%s' unsupported bus type '%s'\n", str, buf);
            return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "index", str)) {
        index = strtol(buf, NULL, 0);
	if (index < 0) {
	    fprintf(stderr, "qemu: '%s' invalid index\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "cyls", str)) {
        cyls = strtol(buf, NULL, 0);
    }

    if (get_param_value(buf, sizeof(buf), "heads", str)) {
        heads = strtol(buf, NULL, 0);
    }

    if (get_param_value(buf, sizeof(buf), "secs", str)) {
        secs = strtol(buf, NULL, 0);
    }

    if (cyls || heads || secs) {
        if (cyls < 1 || cyls > 16383) {
            fprintf(stderr, "qemu: '%s' invalid physical cyls number\n", str);
	    return -1;
	}
        if (heads < 1 || heads > 16) {
            fprintf(stderr, "qemu: '%s' invalid physical heads number\n", str);
	    return -1;
	}
        if (secs < 1 || secs > 63) {
            fprintf(stderr, "qemu: '%s' invalid physical secs number\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "trans", str)) {
        if (!cyls) {
            fprintf(stderr,
                    "qemu: '%s' trans must be used with cyls,heads and secs\n",
                    str);
            return -1;
        }
        if (!strcmp(buf, "none"))
            translation = BIOS_ATA_TRANSLATION_NONE;
        else if (!strcmp(buf, "lba"))
            translation = BIOS_ATA_TRANSLATION_LBA;
        else if (!strcmp(buf, "auto"))
            translation = BIOS_ATA_TRANSLATION_AUTO;
	else {
            fprintf(stderr, "qemu: '%s' invalid translation type\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "media", str)) {
        if (!strcmp(buf, "disk")) {
	    media = MEDIA_DISK;
	} else if (!strcmp(buf, "cdrom")) {
            if (cyls || secs || heads) {
                fprintf(stderr,
                        "qemu: '%s' invalid physical CHS format\n", str);
	        return -1;
            }
	    media = MEDIA_CDROM;
	} else {
	    fprintf(stderr, "qemu: '%s' invalid media\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "snapshot", str)) {
        if (!strcmp(buf, "on"))
	    snapshot = 1;
        else if (!strcmp(buf, "off"))
	    snapshot = 0;
	else {
	    fprintf(stderr, "qemu: '%s' invalid snapshot option\n", str);
	    return -1;
	}
    }

    if (get_param_value(buf, sizeof(buf), "cache", str)) {
        if (!strcmp(buf, "off") || !strcmp(buf, "none"))
            cache = 0;
        else if (!strcmp(buf, "writethrough"))
            cache = 1;
        else if (!strcmp(buf, "writeback"))
            cache = 2;
        else {
           fprintf(stderr, "qemu: invalid cache option\n");
           return -1;
        }
    }

    if (get_param_value(buf, sizeof(buf), "format", str)) {
       if (strcmp(buf, "?") == 0) {
            fprintf(stderr, "qemu: Supported formats:");
            bdrv_iterate_format(bdrv_format_print, NULL);
            fprintf(stderr, "\n");
	    return -1;
        }
        drv = bdrv_find_format(buf);
        if (!drv) {
            fprintf(stderr, "qemu: '%s' invalid format\n", buf);
            return -1;
        }
    }

    if (arg->file == NULL)
        get_param_value(file, sizeof(file), "file", str);
    else
        pstrcpy(file, sizeof(file), arg->file);

    if (!get_param_value(serial, sizeof(serial), "serial", str))
	    memset(serial, 0,  sizeof(serial));

    onerror = BLOCK_ERR_STOP_ENOSPC;
    if (get_param_value(buf, sizeof(serial), "werror", str)) {
        if (type != IF_IDE && type != IF_SCSI && type != IF_VIRTIO) {
            fprintf(stderr, "werror is no supported by this format\n");
            return -1;
        }
        if (!strcmp(buf, "ignore"))
            onerror = BLOCK_ERR_IGNORE;
        else if (!strcmp(buf, "enospc"))
            onerror = BLOCK_ERR_STOP_ENOSPC;
        else if (!strcmp(buf, "stop"))
            onerror = BLOCK_ERR_STOP_ANY;
        else if (!strcmp(buf, "report"))
            onerror = BLOCK_ERR_REPORT;
        else {
            fprintf(stderr, "qemu: '%s' invalid write error action\n", buf);
            return -1;
        }
    }

    /* compute bus and unit according index */

    if (index != -1) {
        if (bus_id != 0 || unit_id != -1) {
            fprintf(stderr,
                    "qemu: '%s' index cannot be used with bus and unit\n", str);
            return -1;
        }
        if (max_devs == 0)
        {
            unit_id = index;
            bus_id = 0;
        } else {
            unit_id = index % max_devs;
            bus_id = index / max_devs;
        }
    }

    /* if user doesn't specify a unit_id,
     * try to find the first free
     */

    if (unit_id == -1) {
       unit_id = 0;
       while (drive_get_index(type, bus_id, unit_id) != -1) {
           unit_id++;
           if (max_devs && unit_id >= max_devs) {
               unit_id -= max_devs;
               bus_id++;
           }
       }
    }

    /* check unit id */

    if (max_devs && unit_id >= max_devs) {
        fprintf(stderr, "qemu: '%s' unit %d too big (max is %d)\n",
                        str, unit_id, max_devs - 1);
        return -1;
    }

    /*
     * ignore multiple definitions
     */

    if (drive_get_index(type, bus_id, unit_id) != -1)
        return -2;

    /* init */

    if (type == IF_IDE || type == IF_SCSI)
        mediastr = (media == MEDIA_CDROM) ? "-cd" : "-hd";
    if (max_devs)
        snprintf(buf, sizeof(buf), "%s%i%s%i",
                 devname, bus_id, mediastr, unit_id);
    else
        snprintf(buf, sizeof(buf), "%s%s%i",
                 devname, mediastr, unit_id);
    bdrv = bdrv_new(buf);
    drives_table_idx = drive_get_free_idx();
    drives_table[drives_table_idx].bdrv = bdrv;
    drives_table[drives_table_idx].type = type;
    drives_table[drives_table_idx].bus = bus_id;
    drives_table[drives_table_idx].unit = unit_id;
    drives_table[drives_table_idx].onerror = onerror;
    drives_table[drives_table_idx].drive_opt_idx = arg - drives_opt;
    strncpy(drives_table[drives_table_idx].serial, serial, sizeof(serial));
    nb_drives++;

    switch(type) {
    case IF_IDE:
    case IF_SCSI:
    case IF_XEN:
        switch(media) {
	case MEDIA_DISK:
            if (cyls != 0) {
                bdrv_set_geometry_hint(bdrv, cyls, heads, secs);
                bdrv_set_translation_hint(bdrv, translation);
            }
	    break;
	case MEDIA_CDROM:
            bdrv_set_type_hint(bdrv, BDRV_TYPE_CDROM);
	    break;
	}
        break;
    case IF_SD:
        /* FIXME: This isn't really a floppy, but it's a reasonable
           approximation.  */
    case IF_FLOPPY:
        bdrv_set_type_hint(bdrv, BDRV_TYPE_FLOPPY);
        break;
    case IF_PFLASH:
    case IF_MTD:
    case IF_VIRTIO:
        break;
    case IF_COUNT:
    case IF_NONE:
        abort();
    }
    if (!file[0])
        return -2;
    bdrv_flags = 0;
    if (snapshot) {
        bdrv_flags |= BDRV_O_SNAPSHOT;
        cache = 2; /* always use write-back with snapshot */
    }
    if (cache == 0) /* no caching */
        bdrv_flags |= BDRV_O_NOCACHE;
    else if (cache == 2) /* write-back */
        bdrv_flags |= BDRV_O_CACHE_WB;
    else if (cache == 3) /* not specified */
        bdrv_flags |= BDRV_O_CACHE_DEF;
    if (bdrv_open2(bdrv, file, bdrv_flags, drv) < 0) {
        fprintf(stderr, "qemu: could not open disk image %s\n",
                        file);
        return -1;
    }
    if (bdrv_key_required(bdrv))
        autostart = 0;
    return drives_table_idx;
}
#endif /* MAX_DRIVES */

static void numa_add(const char *optarg)
{
    char option[128];
    char *endptr;
    unsigned long long value, endvalue;
    int nodenr;

    optarg = get_opt_name(option, 128, optarg, ',') + 1;
    if (!strcmp(option, "node")) {
        if (get_param_value(option, 128, "nodeid", optarg) == 0) {
            nodenr = nb_numa_nodes;
        } else {
            nodenr = strtoull(option, NULL, 10);
        }

        if (get_param_value(option, 128, "mem", optarg) == 0) {
            node_mem[nodenr] = 0;
        } else {
            value = strtoull(option, &endptr, 0);
            switch (*endptr) {
            case 0: case 'M': case 'm':
                value <<= 20;
                break;
            case 'G': case 'g':
                value <<= 30;
                break;
            }
            node_mem[nodenr] = value;
        }
        if (get_param_value(option, 128, "cpus", optarg) == 0) {
            node_cpumask[nodenr] = 0;
        } else {
            value = strtoull(option, &endptr, 10);
            if (value >= 64) {
                value = 63;
                fprintf(stderr, "only 64 CPUs in NUMA mode supported.\n");
            } else {
                if (*endptr == '-') {
                    endvalue = strtoull(endptr+1, &endptr, 10);
                    if (endvalue >= 63) {
                        endvalue = 62;
                        fprintf(stderr,
                            "only 63 CPUs in NUMA mode supported.\n");
                    }
                    value = (1 << (endvalue + 1)) - (1 << value);
                } else {
                    value = 1 << value;
                }
            }
            node_cpumask[nodenr] = value;
        }
        nb_numa_nodes++;
    }
    return;
}

/***********************************************************/
/* PCMCIA/Cardbus */

static struct pcmcia_socket_entry_s {
    PCMCIASocket *socket;
    struct pcmcia_socket_entry_s *next;
} *pcmcia_sockets = 0;

void pcmcia_socket_register(PCMCIASocket *socket)
{
    struct pcmcia_socket_entry_s *entry;

    entry = g_malloc(sizeof(struct pcmcia_socket_entry_s));
    entry->socket = socket;
    entry->next = pcmcia_sockets;
    pcmcia_sockets = entry;
}

void pcmcia_socket_unregister(PCMCIASocket *socket)
{
    struct pcmcia_socket_entry_s *entry, **ptr;

    ptr = &pcmcia_sockets;
    for (entry = *ptr; entry; ptr = &entry->next, entry = *ptr)
        if (entry->socket == socket) {
            *ptr = entry->next;
            g_free(entry);
        }
}

void pcmcia_info(Monitor *mon)
{
    struct pcmcia_socket_entry_s *iter;

    if (!pcmcia_sockets)
        monitor_printf(mon, "No PCMCIA sockets\n");

    for (iter = pcmcia_sockets; iter; iter = iter->next)
        monitor_printf(mon, "%s: %s\n", iter->socket->slot_string,
                       iter->socket->attached ? iter->socket->card_string :
                       "Empty");
}

/***********************************************************/
/* machine registration */

static QEMUMachine *first_machine = NULL;
QEMUMachine *current_machine = NULL;

int qemu_register_machine(QEMUMachine *m)
{
    QEMUMachine **pm;
    pm = &first_machine;
    while (*pm != NULL)
        pm = &(*pm)->next;
    m->next = NULL;
    *pm = m;
    return 0;
}

static QEMUMachine *find_machine(const char *name)
{
    QEMUMachine *m;

    for(m = first_machine; m != NULL; m = m->next) {
        if (!strcmp(m->name, name))
            return m;
    }
    return NULL;
}

static QEMUMachine *find_default_machine(void)
{
    QEMUMachine *m;

    for(m = first_machine; m != NULL; m = m->next) {
        if (m->is_default) {
            return m;
        }
    }
    return NULL;
}

/***********************************************************/
/* main execution loop */

static void gui_update(void *opaque)
{
    uint64_t interval = GUI_REFRESH_INTERVAL;
    DisplayState *ds = opaque;
    DisplayChangeListener *dcl = ds->listeners;

    dpy_refresh(ds);

    while (dcl != NULL) {
        if (dcl->gui_timer_interval &&
            dcl->gui_timer_interval < interval)
            interval = dcl->gui_timer_interval;
        dcl = dcl->next;
    }
    timer_mod(ds->gui_timer, interval + qemu_clock_get_ms(QEMU_CLOCK_REALTIME));
}

static void nographic_update(void *opaque)
{
    uint64_t interval = GUI_REFRESH_INTERVAL;

    timer_mod(nographic_timer, interval + qemu_clock_get_ms(QEMU_CLOCK_REALTIME));
}

struct vm_change_state_entry {
    VMChangeStateHandler *cb;
    void *opaque;
    QLIST_ENTRY (vm_change_state_entry) entries;
};

static QLIST_HEAD(vm_change_state_head, vm_change_state_entry) vm_change_state_head;

VMChangeStateEntry *qemu_add_vm_change_state_handler(VMChangeStateHandler *cb,
                                                     void *opaque)
{
    VMChangeStateEntry *e;

    e = g_malloc0(sizeof (*e));

    e->cb = cb;
    e->opaque = opaque;
    QLIST_INSERT_HEAD(&vm_change_state_head, e, entries);
    return e;
}

void qemu_del_vm_change_state_handler(VMChangeStateEntry *e)
{
    QLIST_REMOVE (e, entries);
    g_free (e);
}

void vm_state_notify(int running, int reason)
{
    VMChangeStateEntry *e;

    for (e = vm_change_state_head.lh_first; e; e = e->entries.le_next) {
        e->cb(e->opaque, running, reason);
    }
}

void vm_start(void)
{
    if (!vm_running) {
        cpu_enable_ticks();
        vm_running = 1;
        vm_state_notify(1, 0);
        //qemu_rearm_alarm_timer(alarm_timer);
        resume_all_vcpus();
    }
}

/* reset/shutdown handler */

typedef struct QEMUResetEntry {
    QEMUResetHandler *func;
    void *opaque;
    int order;
    struct QEMUResetEntry *next;
} QEMUResetEntry;

static QEMUResetEntry *first_reset_entry;
static int reset_requested;
static int shutdown_requested, shutdown_signal = -1;
static pid_t shutdown_pid;
static int powerdown_requested;
int debug_requested;
static int vmstop_requested;

int qemu_shutdown_requested(void)
{
    int r = shutdown_requested;
    shutdown_requested = 0;
    return r;
}

int qemu_reset_requested(void)
{
    int r = reset_requested;
    reset_requested = 0;
    return r;
}

int qemu_powerdown_requested(void)
{
    int r = powerdown_requested;
    powerdown_requested = 0;
    return r;
}

int qemu_debug_requested(void)
{
    int r = debug_requested;
    debug_requested = 0;
    return r;
}

int qemu_vmstop_requested(void)
{
    int r = vmstop_requested;
    vmstop_requested = 0;
    return r;
}

void qemu_register_reset(QEMUResetHandler *func, int order, void *opaque)
{
    QEMUResetEntry **pre, *re;

    pre = &first_reset_entry;
    while (*pre != NULL && (*pre)->order >= order) {
        pre = &(*pre)->next;
    }
    re = g_malloc0(sizeof(QEMUResetEntry));
    re->func = func;
    re->opaque = opaque;
    re->order = order;
    re->next = NULL;
    *pre = re;
}

void qemu_system_reset(void)
{
    QEMUResetEntry *re;

    /* reset all devices */
    for(re = first_reset_entry; re != NULL; re = re->next) {
        re->func(re->opaque);
    }
}

void qemu_system_reset_request(void)
{
    if (no_reboot) {
        shutdown_requested = 1;
    } else {
        reset_requested = 1;
    }
    qemu_notify_event();
}

void qemu_system_killed(int signal, pid_t pid)
{
    shutdown_signal = signal;
    shutdown_pid = pid;
    qemu_system_shutdown_request();
}

void qemu_system_shutdown_request(void)
{
    shutdown_requested = 1;
    qemu_notify_event();
}

void qemu_system_powerdown_request(void)
{
    powerdown_requested = 1;
    qemu_notify_event();
}

int vm_can_run(void)
{
    if (powerdown_requested)
        return 0;
    if (reset_requested)
        return 0;
    if (shutdown_requested)
        return 0;
    if (debug_requested)
        return 0;
    return 1;
}

void version(void)
{
    printf("QEMU PC emulator version " QEMU_VERSION QEMU_PKGVERSION ", Copyright (c) 2003-2008 Fabrice Bellard\n");
}

void qemu_help(int exitcode)
{
    version();
    printf("usage: %s [options] [disk_image]\n"
           "\n"
           "'disk_image' is a raw hard image image for IDE hard disk 0\n"
           "\n"
#define DEF(option, opt_arg, opt_enum, opt_help)        \
           opt_help
#define DEFHEADING(text) stringify(text) "\n"
#include "qemu-options.def"
#undef DEF
#undef DEFHEADING
#undef GEN_DOCS
           "\n"
           "During emulation, the following keys are useful:\n"
           "ctrl-alt-f      toggle full screen\n"
           "ctrl-alt-n      switch to virtual console 'n'\n"
           "ctrl-alt        toggle mouse and keyboard grab\n"
           "\n"
           "When using -nographic, press 'ctrl-a h' to get some help.\n"
           ,
           "qemu",
           DEFAULT_RAM_SIZE,
#ifndef _WIN32
           DEFAULT_NETWORK_SCRIPT,
           DEFAULT_NETWORK_DOWN_SCRIPT,
#endif
           DEFAULT_GDBSTUB_PORT,
           "/tmp/qemu.log");
    QEMU_EXIT(exitcode);
}

#define HAS_ARG 0x0001

enum {
#define DEF(option, opt_arg, opt_enum, opt_help)        \
    opt_enum,
#define DEFHEADING(text)
#include "qemu-options.def"
#undef DEF
#undef DEFHEADING
#undef GEN_DOCS
};

typedef struct QEMUOption {
    const char *name;
    int flags;
    int index;
} QEMUOption;

static const QEMUOption qemu_options[] = {
    { "h", 0, QEMU_OPTION_h },
#define DEF(option, opt_arg, opt_enum, opt_help)        \
    { option, opt_arg, opt_enum },
#define DEFHEADING(text)
#include "qemu-options.def"
#undef DEF
#undef DEFHEADING
#undef GEN_DOCS
    { NULL, 0, 0 },
};

static void select_vgahw (const char *p)
{
    const char *opts;

    cirrus_vga_enabled = 0;
    std_vga_enabled = 0;
    vmsvga_enabled = 0;
    xenfb_enabled = 0;
    if (strstart(p, "std", &opts)) {
        std_vga_enabled = 1;
    } else if (strstart(p, "cirrus", &opts)) {
        cirrus_vga_enabled = 1;
    } else if (strstart(p, "vmware", &opts)) {
        vmsvga_enabled = 1;
    } else if (strstart(p, "xenfb", &opts)) {
        xenfb_enabled = 1;
    } else if (!strstart(p, "none", &opts)) {
    invalid_vga:
        PANIC("Unknown vga type: %s", p);
    }
    while (*opts) {
        const char *nextopt;

        if (strstart(opts, ",retrace=", &nextopt)) {
            opts = nextopt;
            if (strstart(opts, "dumb", &nextopt))
                vga_retrace_method = VGA_RETRACE_DUMB;
            else if (strstart(opts, "precise", &nextopt))
                vga_retrace_method = VGA_RETRACE_PRECISE;
            else goto invalid_vga;
        } else goto invalid_vga;
        opts = nextopt;
    }
}

#define MAX_NET_CLIENTS 32

#ifdef _WIN32
/* Look for support files in the same directory as the executable.  */
static char *find_datadir(const char *argv0)
{
    char *p;
    char buf[MAX_PATH];
    DWORD len;

    len = GetModuleFileName(NULL, buf, sizeof(buf) - 1);
    if (len == 0) {
        return NULL;
    }

    buf[len] = 0;
    p = buf + len - 1;
    while (p != buf && *p != '\\')
        p--;
    *p = 0;
    if (access(buf, R_OK) == 0) {
        return g_strdup(buf);
    }
    return NULL;
}
#else /* !_WIN32 */

/* Similarly, return the location of the executable */
static char *find_datadir(const char *argv0)
{
    char *p = NULL;
    char buf[PATH_MAX];

#if defined(__linux__)
    {
        int len;
        len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = 0;
            p = buf;
        }
    }
#elif defined(__FreeBSD__)
    {
        int len;
        len = readlink("/proc/curproc/file", buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = 0;
            p = buf;
        }
    }
#endif
    /* If we don't have any way of figuring out the actual executable
       location then try argv[0].  */
    if (!p) {
        p = realpath(argv0, buf);
        if (!p) {
            return NULL;
        }
    }

    return g_strdup(dirname(buf));
}
#endif

static char*
qemu_find_file_with_subdir(const char* data_dir, const char* subdir, const char* name)
{
    int   len = strlen(data_dir) + strlen(name) + strlen(subdir) + 2;
    char* buf = g_malloc0(len);

    snprintf(buf, len, "%s/%s%s", data_dir, subdir, name);
    VERBOSE_PRINT(init,"    trying to find: %s\n", buf);
    if (access(buf, R_OK)) {
        g_free(buf);
        return NULL;
    }
    return buf;
}

char *qemu_find_file(int type, const char *name)
{
    const char *subdir;
    char *buf;

    /* If name contains path separators then try it as a straight path.  */
    if ((strchr(name, '/') || strchr(name, '\\'))
        && access(name, R_OK) == 0) {
        return strdup(name);
    }
    switch (type) {
    case QEMU_FILE_TYPE_BIOS:
        subdir = "";
        break;
    case QEMU_FILE_TYPE_KEYMAP:
        subdir = "keymaps/";
        break;
    default:
        abort();
    }
    buf = qemu_find_file_with_subdir(data_dir, subdir, name);
#ifdef CONFIG_ANDROID
    if (type == QEMU_FILE_TYPE_BIOS) {
        /* This case corresponds to the emulator being used as part of an
         * SDK installation. NOTE: data_dir is really $bindir. */
        if (buf == NULL)
            buf = qemu_find_file_with_subdir(data_dir, "lib/pc-bios/", name);
        /* This case corresponds to platform builds. */
        if (buf == NULL)
            buf = qemu_find_file_with_subdir(data_dir, "../usr/share/pc-bios/", name);
        /* Finally, try this for standalone builds under external/qemu */
        if (buf == NULL)
            buf = qemu_find_file_with_subdir(data_dir, "../../../prebuilts/qemu-kernel/x86/pc-bios/", name);
    }
#endif
    return buf;
}

static int
add_dns_server( const char*  server_name )
{
    SockAddress   addr;

    if (sock_address_init_resolve( &addr, server_name, 55, 0 ) < 0) {
        fprintf(stdout,
                "### WARNING: can't resolve DNS server name '%s'\n",
                server_name );
        return -1;
    }

    fprintf(stderr,
            "DNS server name '%s' resolved to %s\n", server_name, sock_address_to_string(&addr) );

    if ( slirp_add_dns_server( &addr ) < 0 ) {
        fprintf(stderr,
                "### WARNING: could not add DNS server '%s' to the network stack\n", server_name);
        return -1;
    }
    return 0;
}

/* Parses an integer
 * Pararm:
 *  str      String containing a number to be parsed.
 *  result   Passes the parsed integer in this argument
 *  returns  0 if ok, -1 if failed
 */
int
parse_int(const char *str, int *result)
{
    char* r;
    *result = strtol(str, &r, 0);
    if (r == NULL || *r != '\0')
      return -1;

    return 0;
}

/* parses a null-terminated string specifying a network port (e.g., "80") or
 * port range (e.g., "[6666-7000]"). In case of a single port, lport and hport
 * are the same. Returns 0 on success, -1 on error. */

int parse_port_range(const char *str, unsigned short *lport,
                     unsigned short *hport) {

  unsigned int low = 0, high = 0;
  char *p, *arg = strdup(str);

  if ((*arg == '[') && ((p = strrchr(arg, ']')) != NULL)) {
    p = arg + 1;   /* skip '[' */
    low  = atoi(strtok(p, "-"));
    high = atoi(strtok(NULL, "-"));
    if ((low > 0) && (high > 0) && (low < high) && (high < 65535)) {
      *lport = low;
      *hport = high;
    }
  }
  else {
    low = atoi(arg);
    if ((0 < low) && (low < 65535)) {
      *lport = low;
      *hport = low;
    }
  }
  free(arg);
  if (low != 0)
    return 0;
  return -1;
}

/*
 * Implements the generic port forwarding option
 */
void
net_slirp_forward(const char *optarg)
{
    /*
     * we expect the following format:
     * dst_net:dst_mask:dst_port:redirect_ip:redirect_port OR
     * dst_net:dst_mask:[dp_range_start-dp_range_end]:redirect_ip:redirect_port
     */
    char *argument = strdup(optarg), *p = argument;
    char *dst_net, *dst_mask, *dst_port;
    char *redirect_ip, *redirect_port;
    uint32_t dnet, dmask, rip;
    unsigned short dlport = 0, dhport = 0, rport;


    dst_net = strtok(p, ":");
    dst_mask = strtok(NULL, ":");
    dst_port = strtok(NULL, ":");
    redirect_ip = strtok(NULL, ":");
    redirect_port = strtok(NULL, ":");

    if (dst_net == NULL || dst_mask == NULL || dst_port == NULL ||
        redirect_ip == NULL || redirect_port == NULL) {
        fprintf(stderr,
                "Invalid argument for -net-forward, we expect "
                "dst_net:dst_mask:dst_port:redirect_ip:redirect_port or "
                "dst_net:dst_mask:[dp_range_start-dp_range_end]"
                ":redirect_ip:redirect_port: %s\n",
                optarg);
        exit(1);
    }

    /* inet_strtoip converts dotted address to host byte order */
    if (inet_strtoip(dst_net, &dnet) == -1) {
        fprintf(stderr, "Invalid destination IP net: %s\n", dst_net);
        exit(1);
    }
    if (inet_strtoip(dst_mask, &dmask) == -1) {
        fprintf(stderr, "Invalid destination IP mask: %s\n", dst_mask);
        exit(1);
    }
    if (inet_strtoip(redirect_ip, &rip) == -1) {
        fprintf(stderr, "Invalid redirect IP address: %s\n", redirect_ip);
        exit(1);
    }

    if (parse_port_range(dst_port, &dlport, &dhport) == -1) {
        fprintf(stderr, "Invalid destination port or port range\n");
        exit(1);
    }

    rport = atoi(redirect_port);
    if (!rport) {
        fprintf(stderr, "Invalid redirect port: %s\n", redirect_port);
        exit(1);
    }

    dnet &= dmask;

    slirp_add_net_forward(dnet, dmask, dlport, dhport,
                          rip, rport);

    free(argument);
}


/* Parses an -allow-tcp or -allow-udp argument and inserts a corresponding
 * entry in the allows list */
void
slirp_allow(const char *optarg, u_int8_t proto)
{
  /*
   * we expect the following format:
   * dst_ip:dst_port OR dst_ip:[dst_lport-dst_hport]
   */
  char *argument = strdup(optarg), *p = argument;
  char *dst_ip_str, *dst_port_str;
  uint32_t dst_ip;
  unsigned short dst_lport = 0, dst_hport = 0;

  dst_ip_str = strtok(p, ":");
  dst_port_str = strtok(NULL, ":");

  if (dst_ip_str == NULL || dst_port_str == NULL) {
    fprintf(stderr,
            "Invalid argument %s for -allow. We expect "
            "dst_ip:dst_port or dst_ip:[dst_lport-dst_hport]\n",
            optarg);
    exit(1);
  }

  if (inet_strtoip(dst_ip_str, &dst_ip) == -1) {
    fprintf(stderr, "Invalid destination IP address: %s\n", dst_ip_str);
    exit(1);
  }
  if (parse_port_range(dst_port_str, &dst_lport, &dst_hport) == -1) {
    fprintf(stderr, "Invalid destination port or port range\n");
    exit(1);
  }

  slirp_add_allow(dst_ip, dst_lport, dst_hport, proto);

  free(argument);
}

/* Add a serial device at a given location in the emulated hardware table.
 * On failure, this function aborts the program with an error message.
 */
static void
serial_hds_add_at(int  index, const char* devname)
{
    char label[32];

    if (!devname || !strcmp(devname,"none"))
        return;

    if (index >= MAX_SERIAL_PORTS) {
        PANIC("qemu: invalid serial index for %s (%d >= %d)",
              devname, index, MAX_SERIAL_PORTS);
    }
    if (serial_hds[index] != NULL) {
        PANIC("qemu: invalid serial index for %s (%d: already taken!)",
              devname, index);
    }
    snprintf(label, sizeof(label), "serial%d", index);
    serial_hds[index] = qemu_chr_open(label, devname, NULL);
    if (!serial_hds[index]) {
        PANIC("qemu: could not open serial device '%s'", devname);
    }
}


/* Find a free slot in the emulated serial device table, and register
 * it. Return the allocated table index.
 */
static int
serial_hds_add(const char* devname)
{
    int  index;

    /* Find first free slot */
    for (index = 0; index < MAX_SERIAL_PORTS; index++) {
        if (serial_hds[index] == NULL) {
            serial_hds_add_at(index, devname);
            return index;
        }
    }

    PANIC("qemu: too many serial devices registered (%d)", index);
    return -1;  /* shouldn't happen */
}


// Extract the partition type/format of a given partition image
// from the content of fstab.goldfish.
// |fstab| is the address of the fstab.goldfish data in memory.
// |fstabSize| is its size in bytes.
// |partitionName| is the name of the partition for debugging
// purposes (e.g. 'userdata').
// |partitionPath| is the partition path as it appears in the
// fstab file (e.g. '/data').
// On success, sets |*partitionType| to an appropriate value,
// on failure (i.e. |partitionPath| does not appear in the fstab
// file), leave the value untouched.
void android_extractPartitionFormat(const char* fstab,
                                    size_t fstabSize,
                                    const char* partitionName,
                                    const char* partitionPath,
                                    AndroidPartitionType* partitionType) {
    char* partFormat = NULL;
    if (!android_parseFstabPartitionFormat(fstab, fstabSize, partitionPath,
                                           &partFormat)) {
        VERBOSE_PRINT(init, "Could not extract format of %s partition!",
                      partitionName);
        return;
    }
    VERBOSE_PRINT(init, "Found format of %s partition: '%s'",
                  partitionName, partFormat);
    *partitionType = androidPartitionType_fromString(partFormat);
    free(partFormat);
}


// List of value describing how to handle partition images in
// android_nand_add_image() below, when no initiali partition image
// file is provided.
//
// MUST_EXIST means that the partition image must exist, otherwise
// dump an error message and exit.
//
// CREATE_IF_NEEDED means that if the partition image doesn't exist, an
// empty partition file should be created on demand.
//
// MUST_WIPE means that the partition image should be wiped cleaned,
// even if it exists. This is useful to implement the -wipe-data option.
typedef enum {
    ANDROID_PARTITION_OPEN_MODE_MUST_EXIST,
    ANDROID_PARTITION_OPEN_MODE_CREATE_IF_NEEDED,
    ANDROID_PARTITION_OPEN_MODE_MUST_WIPE,
} AndroidPartitionOpenMode;

// Add a NAND partition image to the hardware configuration.
// |part_name| is a string indicating the type of partition, i.e. "system",
// "userdata" or "cache".
// |part_type| is an enum describing the type of partition. If it is
// DISK_PARTITION_TYPE_PROBE, then try to auto-detect the type directly
// from the content of |part_file| or |part_init_file|.
// |part_size| is the partition size in bytes.
// |part_file| is the partition file path, can be NULL if |path_init_file|
// is not NULL.
// |part_init_file| is an optional path to the initialization partition file.
// |is_ext4| is true if the partition is formatted as EXT4, false for YAFFS2.
//
// The NAND partition will be backed by |path_file|, except in the following
// cases:
//    - |part_file| is NULL, or its value is "<temp>", indicating that a
//      new temporary image file must be used instead.
//
//    - |part_file| is not NULL, but the function fails to lock the file,
//      indicating it's already used by another instance. A warning should
//      be printed to warn the user, and a new temporary image should be
//      used.
//
// If |part_file| is not NULL and can be locked, if the partition image does
// not exit, then the file must be created as an empty partition.
//
// When a new partition image is created, what happens depends on the
// value of |is_ext4|:
//
//    - If |is_ext4| is false, a simple empty file is created, since that's
//      enough to create an empty YAFFS2 partition.
//
//    - If |is_ext4| is true, an "empty ext4" partition image is created
//      instead, which will _not_ be backed by an empty file.
//
// If |part_init_file| is not NULL, its content will be used to erase
// the content of the main partition image. This is automatically handled
// by the NAND code though.
//
void android_nand_add_image(const char* part_name,
                            AndroidPartitionType part_type,
                            AndroidPartitionOpenMode part_mode,
                            uint64_t part_size,
                            const char* part_file,
                            const char* part_init_file)
{
    char tmp[PATH_MAX * 2 + 32];

    // Sanitize parameters, an empty string must be the same as NULL.
    if (part_file && !*part_file) {
        part_file = NULL;
    }
    if (part_init_file && !*part_init_file) {
        part_init_file = NULL;
    }

    // Sanity checks.
    if (part_size == 0) {
        PANIC("Invalid %s partition size 0x%" PRIx64, part_size);
    }

    if (part_init_file && !path_exists(part_init_file)) {
        PANIC("Missing initial %s image: %s", part_name, part_init_file);
    }

    // As a special case, a |part_file| of '<temp>' means a temporary
    // partition is needed.
    if (part_file && !strcmp(part_file, "<temp>")) {
        part_file = NULL;
    }

    // Verify partition type, or probe it if needed.
    {
        const char* image_file = NULL;
        if (part_file && path_exists(part_file)) {
            image_file = part_file;
        } else if (part_init_file) {
            image_file = part_init_file;
        } else if (part_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
            PANIC("Cannot determine type of %s partition: no image files!",
                  part_name);
        }

        if (part_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
            VERBOSE_PRINT(init, "Probing %s image file for partition type: %s",
                        part_name, image_file);

            part_type = androidPartitionType_probeFile(image_file);
        } else {
            // Probe the current image file to check that it is of the
            // right partition format.
            if (image_file) {
                AndroidPartitionType image_type =
                        androidPartitionType_probeFile(image_file);
                if (image_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
                    PANIC("Cannot determine %s partition type of: %s",
                          part_name,
                          image_file);
                }

                if (image_type != part_type) {
                    PANIC("Invalid %s partition image type: %s (expected %s)",
                        part_name,
                        androidPartitionType_toString(image_type),
                        androidPartitionType_toString(part_type));
                }
            }
        }
    }

    VERBOSE_PRINT(init, "%s partition format: %s", part_name,
                  androidPartitionType_toString(part_type));

    snprintf(tmp, sizeof tmp, "%s,size=0x%" PRIx64, part_name, part_size);

    bool need_temp_partition = true;
    bool need_make_empty =
            (part_mode == ANDROID_PARTITION_OPEN_MODE_MUST_WIPE);

    if (part_file) {
        if (filelock_create(part_file) == NULL) {
            fprintf(stderr,
                    "WARNING: %s image already in use, changes will not persist!\n",
                    part_name);
        } else {
            need_temp_partition = false;

            // If the partition image is missing, create it.
            if (!path_exists(part_file)) {
                if (part_mode == ANDROID_PARTITION_OPEN_MODE_MUST_EXIST) {
                    PANIC("Missing %s partition image: %s", part_name,
                          part_file);
                }
                if (path_empty_file(part_file) < 0) {
                    PANIC("Cannot create %s image file at %s: %s",
                          part_name,
                          part_file,
                          strerror(errno));
                }
                need_make_empty = true;
            }
        }
    }

    // Do we need a temporary partition image ?
    if (need_temp_partition) {
        TempFile* temp_file = tempfile_create();
        if (temp_file == NULL) {
            PANIC("Could not create temp file for %s partition image: %s\n",
                   part_name);
        }
        part_file = tempfile_path(temp_file);
        VERBOSE_PRINT(init,
                      "Mapping '%s' partition image to %s",
                      part_name,
                      part_file);

        need_make_empty = true;
    }

    pstrcat(tmp, sizeof tmp, ",file=");
    pstrcat(tmp, sizeof tmp, part_file);

    // Do we need to make the partition image empty?
    // Do not do it if there is an initial file though since it will
    // get copied directly by the NAND code into the image.
    if (need_make_empty && !part_init_file) {
        VERBOSE_PRINT(init,
                      "Creating empty %s partition image at: %s",
                      part_name,
                      part_file);
        int ret = androidPartitionType_makeEmptyFile(part_type,
                                                     part_size,
                                                     part_file);
        if (ret < 0) {
            PANIC("Could not create %s image file at %s: %s",
                  part_name,
                  part_file,
                  strerror(-ret));
        }
    }

    if (part_init_file) {
        pstrcat(tmp, sizeof tmp, ",initfile=");
        pstrcat(tmp, sizeof tmp, part_init_file);
    }

    if (part_type == ANDROID_PARTITION_TYPE_EXT4) {
        // Using a nand device to approximate a block device until full
        // support is added.
        pstrcat(tmp, sizeof tmp,",pagesize=512,extrasize=0");
    }

    nand_add_dev(tmp);
}


int main(int argc, char **argv, char **envp)
{
    const char *gdbstub_dev = NULL;
    uint32_t boot_devices_bitmap = 0;
    int i;
    int snapshot, linux_boot, __attribute__((unused)) net_boot;
    const char *icount_option = NULL;
    const char *initrd_filename;
    const char *kernel_filename, *kernel_cmdline;
    const char *boot_devices = "";
    DisplayState *ds;
    DisplayChangeListener *dcl;
    int cyls, heads, secs, translation;
    QemuOpts *hda_opts = NULL;
    QemuOpts *hdb_opts = NULL;
    const char *net_clients[MAX_NET_CLIENTS];
    int nb_net_clients;
    int optind;
    const char *r, *optarg;
    CharDriverState *monitor_hd = NULL;
    const char *monitor_device;
    const char *serial_devices[MAX_SERIAL_PORTS];
    int serial_device_index;
    const char *parallel_devices[MAX_PARALLEL_PORTS];
    int parallel_device_index;
    const char *virtio_consoles[MAX_VIRTIO_CONSOLES];
    int virtio_console_index;
    const char *loadvm = NULL;
    QEMUMachine *machine;
    const char *cpu_model;
    int tb_size;
    const char *pid_file = NULL;
    const char *incoming = NULL;
    const char* log_mask = NULL;
    const char* log_file = NULL;
    CPUState *cpu;
    int show_vnc_port = 0;
    IniFile*  hw_ini = NULL;
    STRALLOC_DEFINE(kernel_params);
    STRALLOC_DEFINE(kernel_config);
    int    dns_count = 0;

    /* Initialize sockets before anything else, so we can properly report
     * initialization failures back to the UI. */
#ifdef _WIN32
    socket_init();
#endif

    init_clocks();

    qemu_cache_utils_init();

    QLIST_INIT (&vm_change_state_head);
    os_setup_early_signal_handling();

    module_call_init(MODULE_INIT_MACHINE);
    machine = find_default_machine();
    cpu_model = NULL;
    initrd_filename = NULL;
    ram_size = 0;
    snapshot = 0;
    kernel_filename = NULL;
    kernel_cmdline = "";

    cyls = heads = secs = 0;
    translation = BIOS_ATA_TRANSLATION_AUTO;
    monitor_device = "vc:80Cx24C";

    serial_devices[0] = "vc:80Cx24C";
    for(i = 1; i < MAX_SERIAL_PORTS; i++)
        serial_devices[i] = NULL;
    serial_device_index = 0;

    parallel_devices[0] = "vc:80Cx24C";
    for(i = 1; i < MAX_PARALLEL_PORTS; i++)
        parallel_devices[i] = NULL;
    parallel_device_index = 0;

    for(i = 0; i < MAX_VIRTIO_CONSOLES; i++)
        virtio_consoles[i] = NULL;
    virtio_console_index = 0;

    for (i = 0; i < MAX_NODES; i++) {
        node_mem[i] = 0;
        node_cpumask[i] = 0;
    }

    nb_net_clients = 0;
#ifdef MAX_DRIVES
    nb_drives = 0;
    nb_drives_opt = 0;
#endif
    nb_numa_nodes = 0;

    nb_nics = 0;

    tb_size = 0;
    autostart= 1;

    register_watchdogs();

    /* Initialize boot properties. */
    boot_property_init_service();
    android_hw_control_init();
    android_net_pipes_init();

#ifdef CONFIG_KVM
    /* By default, force auto-detection for kvm */
    kvm_allowed = -1;
#endif

    optind = 1;
    for(;;) {
        if (optind >= argc)
            break;
        r = argv[optind];
        if (r[0] != '-') {
            hda_opts = drive_add(argv[optind++], HD_ALIAS, 0);
        } else {
            const QEMUOption *popt;

            optind++;
            /* Treat --foo the same as -foo.  */
            if (r[1] == '-')
                r++;
            popt = qemu_options;
            for(;;) {
                if (!popt->name) {
                    PANIC("%s: invalid option -- '%s'",
                                      argv[0], r);
                }
                if (!strcmp(popt->name, r + 1))
                    break;
                popt++;
            }
            if (popt->flags & HAS_ARG) {
                if (optind >= argc) {
                    PANIC("%s: option '%s' requires an argument",
                                      argv[0], r);
                }
                optarg = argv[optind++];
            } else {
                optarg = NULL;
            }

            switch(popt->index) {
            case QEMU_OPTION_M:
                machine = find_machine(optarg);
                if (!machine) {
                    QEMUMachine *m;
                    printf("Supported machines are:\n");
                    for(m = first_machine; m != NULL; m = m->next) {
                        printf("%-10s %s%s\n",
                               m->name, m->desc,
                               m->is_default ? " (default)" : "");
                    }
                    if (*optarg != '?') {
                        PANIC("Invalid machine parameter: %s",
                                          optarg);
                    } else {
                        QEMU_EXIT(0);
                    }
                }
                break;
            case QEMU_OPTION_cpu:
                /* hw initialization will check this */
                if (*optarg == '?') {
/* XXX: implement xxx_cpu_list for targets that still miss it */
#if defined(cpu_list)
                    cpu_list(stdout, &fprintf);
#endif
                    QEMU_EXIT(0);
                } else {
                    cpu_model = optarg;
                }
                break;
            case QEMU_OPTION_initrd:
                initrd_filename = optarg;
                break;
            case QEMU_OPTION_hda:
                if (cyls == 0)
                    hda_opts = drive_add(optarg, HD_ALIAS, 0);
                else
                    hda_opts = drive_add(optarg, HD_ALIAS
			     ",cyls=%d,heads=%d,secs=%d%s",
                             0, cyls, heads, secs,
                             translation == BIOS_ATA_TRANSLATION_LBA ?
                                 ",trans=lba" :
                             translation == BIOS_ATA_TRANSLATION_NONE ?
                                 ",trans=none" : "");
                 break;
            case QEMU_OPTION_hdb:
                hdb_opts = drive_add(optarg, HD_ALIAS, 1);
                break;

            case QEMU_OPTION_hdc:
            case QEMU_OPTION_hdd:
                drive_add(optarg, HD_ALIAS, popt->index - QEMU_OPTION_hda);
                break;
            case QEMU_OPTION_drive:
                drive_add(NULL, "%s", optarg);
	        break;
            case QEMU_OPTION_mtdblock:
                drive_add(optarg, MTD_ALIAS);
                break;
            case QEMU_OPTION_sd:
                drive_add(optarg, SD_ALIAS);
                break;
            case QEMU_OPTION_pflash:
                drive_add(optarg, PFLASH_ALIAS);
                break;
            case QEMU_OPTION_snapshot:
                snapshot = 1;
                break;
            case QEMU_OPTION_hdachs:
                {
                    const char *p;
                    p = optarg;
                    cyls = strtol(p, (char **)&p, 0);
                    if (cyls < 1 || cyls > 16383)
                        goto chs_fail;
                    if (*p != ',')
                        goto chs_fail;
                    p++;
                    heads = strtol(p, (char **)&p, 0);
                    if (heads < 1 || heads > 16)
                        goto chs_fail;
                    if (*p != ',')
                        goto chs_fail;
                    p++;
                    secs = strtol(p, (char **)&p, 0);
                    if (secs < 1 || secs > 63)
                        goto chs_fail;
                    if (*p == ',') {
                        p++;
                        if (!strcmp(p, "none"))
                            translation = BIOS_ATA_TRANSLATION_NONE;
                        else if (!strcmp(p, "lba"))
                            translation = BIOS_ATA_TRANSLATION_LBA;
                        else if (!strcmp(p, "auto"))
                            translation = BIOS_ATA_TRANSLATION_AUTO;
                        else
                            goto chs_fail;
                    } else if (*p != '\0') {
                    chs_fail:
                        PANIC("qemu: invalid physical CHS format");
                    }
		    if (hda_opts != NULL) {
                        char num[16];
                        snprintf(num, sizeof(num), "%d", cyls);
                        qemu_opt_set(hda_opts, "cyls", num);
                        snprintf(num, sizeof(num), "%d", heads);
                        qemu_opt_set(hda_opts, "heads", num);
                        snprintf(num, sizeof(num), "%d", secs);
                        qemu_opt_set(hda_opts, "secs", num);
                        if (translation == BIOS_ATA_TRANSLATION_LBA)
                            qemu_opt_set(hda_opts, "trans", "lba");
                        if (translation == BIOS_ATA_TRANSLATION_NONE)
                            qemu_opt_set(hda_opts, "trans", "none");
                    }
                }
                break;
            case QEMU_OPTION_numa:
                if (nb_numa_nodes >= MAX_NODES) {
                    PANIC("qemu: too many NUMA nodes");
                }
                numa_add(optarg);
                break;
            case QEMU_OPTION_nographic:
                display_type = DT_NOGRAPHIC;
                break;
#ifdef CONFIG_CURSES
            case QEMU_OPTION_curses:
                display_type = DT_CURSES;
                break;
#endif
            case QEMU_OPTION_portrait:
                graphic_rotate = 1;
                break;
            case QEMU_OPTION_kernel:
                kernel_filename = optarg;
                break;
            case QEMU_OPTION_append:
                kernel_cmdline = optarg;
                break;
            case QEMU_OPTION_cdrom:
                drive_add(optarg, CDROM_ALIAS);
                break;
            case QEMU_OPTION_boot:
                boot_devices = optarg;
                /* We just do some generic consistency checks */
                {
                    /* Could easily be extended to 64 devices if needed */
                    const char *p;

                    boot_devices_bitmap = 0;
                    for (p = boot_devices; *p != '\0'; p++) {
                        /* Allowed boot devices are:
                         * a b     : floppy disk drives
                         * c ... f : IDE disk drives
                         * g ... m : machine implementation dependant drives
                         * n ... p : network devices
                         * It's up to each machine implementation to check
                         * if the given boot devices match the actual hardware
                         * implementation and firmware features.
                         */
                        if (*p < 'a' || *p > 'q') {
                            PANIC("Invalid boot device '%c'", *p);
                        }
                        if (boot_devices_bitmap & (1 << (*p - 'a'))) {
                            PANIC(
                                    "Boot device '%c' was given twice",*p);
                        }
                        boot_devices_bitmap |= 1 << (*p - 'a');
                    }
                }
                break;
            case QEMU_OPTION_fda:
            case QEMU_OPTION_fdb:
                drive_add(optarg, FD_ALIAS, popt->index - QEMU_OPTION_fda);
                break;
#ifdef TARGET_I386
            case QEMU_OPTION_no_fd_bootchk:
                fd_bootchk = 0;
                break;
#endif
            case QEMU_OPTION_net:
                if (nb_net_clients >= MAX_NET_CLIENTS) {
                    PANIC("qemu: too many network clients");
                }
                net_clients[nb_net_clients] = optarg;
                nb_net_clients++;
                break;
#ifdef CONFIG_SLIRP
            case QEMU_OPTION_tftp:
		tftp_prefix = optarg;
                break;
            case QEMU_OPTION_bootp:
                bootp_filename = optarg;
                break;
            case QEMU_OPTION_redir:
                net_slirp_redir(NULL, optarg, NULL);
                break;
#endif
#ifdef HAS_AUDIO
            case QEMU_OPTION_audio_help:
                AUD_help ();
                QEMU_EXIT(0);
                break;
            case QEMU_OPTION_soundhw:
                select_soundhw (optarg);
                break;
#endif
            case QEMU_OPTION_h:
                qemu_help(0);
                break;
            case QEMU_OPTION_version:
                version();
                QEMU_EXIT(0);
                break;
            case QEMU_OPTION_m: {
                uint64_t value;
                char *ptr;

                value = strtoul(optarg, &ptr, 10);
                switch (*ptr) {
                case 0: case 'M': case 'm':
                    value <<= 20;
                    break;
                case 'G': case 'g':
                    value <<= 30;
                    break;
                default:
                    PANIC("qemu: invalid ram size: %s", optarg);
                }

                /* On 32-bit hosts, QEMU is limited by virtual address space */
                if (value > (2047 << 20) && HOST_LONG_BITS == 32) {
                    PANIC("qemu: at most 2047 MB RAM can be simulated");
                }
                if (value != (uint64_t)(ram_addr_t)value) {
                    PANIC("qemu: ram size too large");
                }
                ram_size = value;
                break;
            }
            case QEMU_OPTION_d:
                log_mask = optarg;
                break;
            case QEMU_OPTION_s:
                gdbstub_dev = "tcp::" DEFAULT_GDBSTUB_PORT;
                break;
            case QEMU_OPTION_gdb:
                gdbstub_dev = optarg;
                break;
            case QEMU_OPTION_L:
                data_dir = optarg;
                break;
            case QEMU_OPTION_bios:
                bios_name = optarg;
                break;
            case QEMU_OPTION_singlestep:
                singlestep = 1;
                break;
            case QEMU_OPTION_S:
                autostart = 0;
                break;
#ifndef _WIN32
	    case QEMU_OPTION_k:
		keyboard_layout = optarg;
		break;
#endif
            case QEMU_OPTION_localtime:
                rtc_utc = 0;
                break;
            case QEMU_OPTION_vga:
                select_vgahw (optarg);
                break;
#if defined(TARGET_PPC) || defined(TARGET_SPARC)
            case QEMU_OPTION_g:
                {
                    const char *p;
                    int w, h, depth;
                    p = optarg;
                    w = strtol(p, (char **)&p, 10);
                    if (w <= 0) {
                    graphic_error:
                        PANIC("qemu: invalid resolution or depth");
                    }
                    if (*p != 'x')
                        goto graphic_error;
                    p++;
                    h = strtol(p, (char **)&p, 10);
                    if (h <= 0)
                        goto graphic_error;
                    if (*p == 'x') {
                        p++;
                        depth = strtol(p, (char **)&p, 10);
                        if (depth != 8 && depth != 15 && depth != 16 &&
                            depth != 24 && depth != 32)
                            goto graphic_error;
                    } else if (*p == '\0') {
                        depth = graphic_depth;
                    } else {
                        goto graphic_error;
                    }

                    graphic_width = w;
                    graphic_height = h;
                    graphic_depth = depth;
                }
                break;
#endif
            case QEMU_OPTION_echr:
                {
                    char *r;
                    term_escape_char = strtol(optarg, &r, 0);
                    if (r == optarg)
                        printf("Bad argument to echr\n");
                    break;
                }
            case QEMU_OPTION_monitor:
                monitor_device = optarg;
                break;
            case QEMU_OPTION_serial:
                if (serial_device_index >= MAX_SERIAL_PORTS) {
                    PANIC("qemu: too many serial ports");
                }
                serial_devices[serial_device_index] = optarg;
                serial_device_index++;
                break;
            case QEMU_OPTION_watchdog:
                i = select_watchdog(optarg);
                if (i > 0) {
                    if (i == 1) {
                        PANIC("Invalid watchdog parameter: %s",
                                          optarg);
                    } else {
                        QEMU_EXIT(0);
                    }
                }
                break;
            case QEMU_OPTION_watchdog_action:
                if (select_watchdog_action(optarg) == -1) {
                    PANIC("Unknown -watchdog-action parameter");
                }
                break;
            case QEMU_OPTION_virtiocon:
                if (virtio_console_index >= MAX_VIRTIO_CONSOLES) {
                    PANIC("qemu: too many virtio consoles");
                }
                virtio_consoles[virtio_console_index] = optarg;
                virtio_console_index++;
                break;
            case QEMU_OPTION_parallel:
                if (parallel_device_index >= MAX_PARALLEL_PORTS) {
                    PANIC("qemu: too many parallel ports");
                }
                parallel_devices[parallel_device_index] = optarg;
                parallel_device_index++;
                break;
            case QEMU_OPTION_loadvm:
                loadvm = optarg;
                break;
            case QEMU_OPTION_savevm_on_exit:
                savevm_on_exit = optarg;
                break;
            case QEMU_OPTION_full_screen:
                full_screen = 1;
                break;
#ifdef CONFIG_SDL
            case QEMU_OPTION_no_frame:
                no_frame = 1;
                break;
            case QEMU_OPTION_alt_grab:
                alt_grab = 1;
                break;
            case QEMU_OPTION_no_quit:
                no_quit = 1;
                break;
            case QEMU_OPTION_sdl:
                display_type = DT_SDL;
                break;
#endif
            case QEMU_OPTION_pidfile:
                pid_file = optarg;
                break;
#ifdef TARGET_I386
            case QEMU_OPTION_win2k_hack:
                win2k_install_hack = 1;
                break;
            case QEMU_OPTION_rtc_td_hack:
                rtc_td_hack = 1;
                break;
#ifndef CONFIG_ANDROID
            case QEMU_OPTION_acpitable:
                if(acpi_table_add(optarg) < 0) {
                    PANIC("Wrong acpi table provided");
                }
                break;
#endif
            case QEMU_OPTION_smbios:
                do_smbios_option(optarg);
                break;
#endif
#ifdef CONFIG_KVM
            case QEMU_OPTION_enable_kvm:
                kvm_allowed = 1;
                break;
            case QEMU_OPTION_disable_kvm:
                kvm_allowed = 0;
                break;
#endif /* CONFIG_KVM */
            case QEMU_OPTION_smp:
                smp_cpus = atoi(optarg);
                if (smp_cpus < 1) {
                    PANIC("Invalid number of CPUs");
                }
                break;
	    case QEMU_OPTION_vnc:
                display_type = DT_VNC;
		vnc_display = optarg;
		break;
#ifdef TARGET_I386
            case QEMU_OPTION_no_acpi:
                acpi_enabled = 0;
                break;
            case QEMU_OPTION_no_hpet:
                no_hpet = 1;
                break;
            case QEMU_OPTION_no_virtio_balloon:
                no_virtio_balloon = 1;
                break;
#endif
            case QEMU_OPTION_no_reboot:
                no_reboot = 1;
                break;
            case QEMU_OPTION_no_shutdown:
                no_shutdown = 1;
                break;
            case QEMU_OPTION_show_cursor:
                cursor_hide = 0;
                break;
            case QEMU_OPTION_uuid:
                if(qemu_uuid_parse(optarg, qemu_uuid) < 0) {
                    PANIC("Fail to parse UUID string. Wrong format.");
                }
                break;
	    case QEMU_OPTION_option_rom:
		if (nb_option_roms >= MAX_OPTION_ROMS) {
		    PANIC("Too many option ROMs");
		}
		option_rom[nb_option_roms] = optarg;
		nb_option_roms++;
		break;
#if defined(TARGET_ARM) || defined(TARGET_M68K)
            case QEMU_OPTION_semihosting:
                semihosting_enabled = 1;
                break;
#endif
            case QEMU_OPTION_name:
                qemu_name = optarg;
                break;
#if defined(TARGET_SPARC) || defined(TARGET_PPC)
            case QEMU_OPTION_prom_env:
                if (nb_prom_envs >= MAX_PROM_ENVS) {
                    PANIC("Too many prom variables");
                }
                prom_envs[nb_prom_envs] = optarg;
                nb_prom_envs++;
                break;
#endif
#ifdef TARGET_ARM
            case QEMU_OPTION_old_param:
                old_param = 1;
                break;
#endif
            case QEMU_OPTION_clock:
                configure_alarms(optarg);
                break;
            case QEMU_OPTION_startdate:
                {
                    struct tm tm;
                    time_t rtc_start_date = 0;
                    if (!strcmp(optarg, "now")) {
                        rtc_date_offset = -1;
                    } else {
                        if (sscanf(optarg, "%d-%d-%dT%d:%d:%d",
                               &tm.tm_year,
                               &tm.tm_mon,
                               &tm.tm_mday,
                               &tm.tm_hour,
                               &tm.tm_min,
                               &tm.tm_sec) == 6) {
                            /* OK */
                        } else if (sscanf(optarg, "%d-%d-%d",
                                          &tm.tm_year,
                                          &tm.tm_mon,
                                          &tm.tm_mday) == 3) {
                            tm.tm_hour = 0;
                            tm.tm_min = 0;
                            tm.tm_sec = 0;
                        } else {
                            goto date_fail;
                        }
                        tm.tm_year -= 1900;
                        tm.tm_mon--;
                        rtc_start_date = mktimegm(&tm);
                        if (rtc_start_date == -1) {
                        date_fail:
                            PANIC("Invalid date format. Valid format are:\n"
                                    "'now' or '2006-06-17T16:01:21' or '2006-06-17'");
                        }
                        rtc_date_offset = time(NULL) - rtc_start_date;
                    }
                }
                break;

            /* -------------------------------------------------------*/
            /* User mode network stack restrictions */
            case QEMU_OPTION_drop_udp:
                slirp_drop_udp();
                break;
            case QEMU_OPTION_drop_tcp:
                slirp_drop_tcp();
                break;
            case QEMU_OPTION_allow_tcp:
                slirp_allow(optarg, IPPROTO_TCP);
                break;
            case QEMU_OPTION_allow_udp:
                slirp_allow(optarg, IPPROTO_UDP);
                break;
             case QEMU_OPTION_drop_log:
                {
                    FILE* drop_log_fd;
                    drop_log_filename = optarg;
                    drop_log_fd = fopen(optarg, "w+");

                    if (!drop_log_fd) {
                        fprintf(stderr, "Cannot open drop log: %s\n", optarg);
                        exit(1);
                    }

                    slirp_drop_log_fd(drop_log_fd);
                }
                break;

            case QEMU_OPTION_dns_log:
                {
                    FILE* dns_log_fd;
                    dns_log_filename = optarg;
                    dns_log_fd = fopen(optarg, "wb+");

                    if (dns_log_fd == NULL) {
                        fprintf(stderr, "Cannot open dns log: %s\n", optarg);
                        exit(1);
                    }

                    slirp_dns_log_fd(dns_log_fd);
                }
                break;


            case QEMU_OPTION_max_dns_conns:
                {
                    int max_dns_conns = 0;
                    if (parse_int(optarg, &max_dns_conns)) {
                      fprintf(stderr,
                              "qemu: syntax: -max-dns-conns max_connections\n");
                      exit(1);
                    }
                    if (max_dns_conns <= 0 ||  max_dns_conns == LONG_MAX) {
                      fprintf(stderr,
                              "Invalid arg for max dns connections: %s\n",
                              optarg);
                      exit(1);
                    }
                    slirp_set_max_dns_conns(max_dns_conns);
                }
                break;

            case QEMU_OPTION_net_forward:
                net_slirp_forward(optarg);
                break;
            case QEMU_OPTION_net_forward_tcp2sink:
                {
                    SockAddress saddr;

                    if (parse_host_port(&saddr, optarg)) {
                        fprintf(stderr,
                                "Invalid ip/port %s for "
                                "-forward-dropped-tcp2sink. "
                                "We expect 'sink_ip:sink_port'\n",
                                optarg);
                        exit(1);
                    }
                    slirp_forward_dropped_tcp2sink(saddr.u.inet.address,
                                                   saddr.u.inet.port);
                }
                break;
            /* -------------------------------------------------------*/

            case QEMU_OPTION_tb_size:
                tb_size = strtol(optarg, NULL, 0);
                if (tb_size < 0)
                    tb_size = 0;
                break;
            case QEMU_OPTION_icount:
                icount_option = optarg;
                break;
            case QEMU_OPTION_incoming:
                incoming = optarg;
                break;
#ifdef CONFIG_XEN
            case QEMU_OPTION_xen_domid:
                xen_domid = atoi(optarg);
                break;
            case QEMU_OPTION_xen_create:
                xen_mode = XEN_CREATE;
                break;
            case QEMU_OPTION_xen_attach:
                xen_mode = XEN_ATTACH;
                break;
#endif


            case QEMU_OPTION_mic:
                audio_input_source = (char*)optarg;
                break;
#ifdef CONFIG_NAND
            case QEMU_OPTION_nand:
                nand_add_dev(optarg);
                break;

#endif
#ifdef CONFIG_HAX
            case QEMU_OPTION_enable_hax:
                hax_disabled = 0;
                break;
            case QEMU_OPTION_disable_hax:
                hax_disabled = 1;
                break;
#endif
            case QEMU_OPTION_android_ports:
                android_op_ports = (char*)optarg;
                break;

            case QEMU_OPTION_android_port:
                android_op_port = (char*)optarg;
                break;

            case QEMU_OPTION_android_report_console:
                android_op_report_console = (char*)optarg;
                break;

            case QEMU_OPTION_http_proxy:
                op_http_proxy = (char*)optarg;
                break;

            case QEMU_OPTION_charmap:
                op_charmap_file = (char*)optarg;
                break;

            case QEMU_OPTION_android_hw:
                android_op_hwini = (char*)optarg;
                break;

            case QEMU_OPTION_dns_server:
                android_op_dns_server = (char*)optarg;
                break;

            case QEMU_OPTION_radio:
                android_op_radio = (char*)optarg;
                break;

            case QEMU_OPTION_gps:
                android_op_gps = (char*)optarg;
                break;

            case QEMU_OPTION_audio:
                android_op_audio = (char*)optarg;
                break;

            case QEMU_OPTION_cpu_delay:
                android_op_cpu_delay = (char*)optarg;
                break;

            case QEMU_OPTION_show_kernel:
                android_kmsg_init(ANDROID_KMSG_PRINT_MESSAGES);
                break;

#ifdef CONFIG_NAND_LIMITS
            case QEMU_OPTION_nand_limits:
                android_op_nand_limits = (char*)optarg;
                break;
#endif  // CONFIG_NAND_LIMITS

            case QEMU_OPTION_netspeed:
                android_op_netspeed = (char*)optarg;
                break;

            case QEMU_OPTION_netdelay:
                android_op_netdelay = (char*)optarg;
                break;

            case QEMU_OPTION_netfast:
                android_op_netfast = 1;
                break;

            case QEMU_OPTION_tcpdump:
                android_op_tcpdump = (char*)optarg;
                break;

            case QEMU_OPTION_boot_property:
                boot_property_parse_option((char*)optarg);
                break;

            case QEMU_OPTION_lcd_density:
                android_op_lcd_density = (char*)optarg;
                break;

            case QEMU_OPTION_ui_port:
                android_op_ui_port = (char*)optarg;
                break;

            case QEMU_OPTION_ui_settings:
                android_op_ui_settings = (char*)optarg;
                break;

            case QEMU_OPTION_audio_test_out:
                android_audio_test_start_out();
                break;

            case QEMU_OPTION_android_avdname:
                android_op_avd_name = (char*)optarg;
                break;

            case QEMU_OPTION_timezone:
                if (timezone_set((char*)optarg)) {
                    fprintf(stderr, "emulator: it seems the timezone '%s' is not in zoneinfo format\n",
                            (char*)optarg);
                }
                break;

            case QEMU_OPTION_snapshot_no_time_update:
                android_snapshot_update_time = 0;
                break;

            case QEMU_OPTION_list_webcam:
                android_list_web_cameras();
                exit(0);

            default:
                os_parse_cmd_args(popt->index, optarg);
            }
        }
    }

    /* Initialize character map. */
    if (android_charmap_setup(op_charmap_file)) {
        if (op_charmap_file) {
            PANIC(
                    "Unable to initialize character map from file %s.",
                    op_charmap_file);
        } else {
            PANIC(
                    "Unable to initialize default character map.");
        }
    }

    /* If no data_dir is specified then try to find it relative to the
       executable path.  */
    if (!data_dir) {
        data_dir = find_datadir(argv[0]);
    }
    /* If all else fails use the install patch specified when building.  */
    if (!data_dir) {
        data_dir = CONFIG_QEMU_SHAREDIR;
    }

    if (!android_op_hwini) {
        PANIC("Missing -android-hw <file> option!");
    }
    hw_ini = iniFile_newFromFile(android_op_hwini);
    if (hw_ini == NULL) {
        PANIC("Could not find %s file.", android_op_hwini);
    }

    androidHwConfig_init(android_hw, 0);
    androidHwConfig_read(android_hw, hw_ini);

    /* If we're loading VM from a snapshot, make sure that the current HW config
     * matches the one with which the VM has been saved. */
    if (loadvm && *loadvm && !snaphost_match_configs(hw_ini, loadvm)) {
        exit(0);
    }

    iniFile_free(hw_ini);

    const char* kernelSerialDevicePrefix =
            androidHwConfig_getKernelSerialPrefix(android_hw);
    VERBOSE_PRINT(init, "Using kernel serial device prefix: %s",
                  kernelSerialDevicePrefix);

    {
        int width  = android_hw->hw_lcd_width;
        int height = android_hw->hw_lcd_height;
        int depth  = android_hw->hw_lcd_depth;

        /* A bit of sanity checking */
        if (width <= 0 || height <= 0    ||
            (depth != 16 && depth != 32) ||
            (((width|height) & 3) != 0)  )
        {
            PANIC("Invalid display configuration (%d,%d,%d)",
                  width, height, depth);
        }
        android_display_width  = width;
        android_display_height = height;
        android_display_bpp    = depth;
    }

#ifdef CONFIG_NAND_LIMITS
    /* Init nand stuff. */
    if (android_op_nand_limits) {
        parse_nand_limits(android_op_nand_limits);
    }
#endif  // CONFIG_NAND_LIMITS

    /* Initialize AVD name from hardware configuration if needed */
    if (!android_op_avd_name) {
        if (android_hw->avd_name && *android_hw->avd_name) {
            android_op_avd_name = android_hw->avd_name;
            VERBOSE_PRINT(init,"AVD Name: %s", android_op_avd_name);
        }
    }

    // Determine format of all partition images, if possible.
    // Note that _UNKNOWN means the file, if it exists, will be probed.
    AndroidPartitionType system_partition_type =
            ANDROID_PARTITION_TYPE_UNKNOWN;
    AndroidPartitionType userdata_partition_type =
            ANDROID_PARTITION_TYPE_UNKNOWN;
    AndroidPartitionType cache_partition_type =
            ANDROID_PARTITION_TYPE_UNKNOWN;

    {
        // Starting with Android 4.4.x, the ramdisk.img contains
        // an fstab.goldfish file that lists the format of each partition.
        // If the file exists, parse it to get the appropriate values.
        char* fstab = NULL;
        size_t fstabSize = 0;

        if (android_extractRamdiskFile(android_hw->disk_ramdisk_path,
                                       "fstab.goldfish",
                                       &fstab,
                                       &fstabSize)) {
            VERBOSE_PRINT(init, "Ramdisk image contains fstab.goldfish file");

            android_extractPartitionFormat(fstab,
                                           fstabSize,
                                           "system",
                                           "/system",
                                           &system_partition_type);

            android_extractPartitionFormat(fstab,
                                           fstabSize,
                                           "userdata",
                                           "/data",
                                           &userdata_partition_type);

            android_extractPartitionFormat(fstab,
                                           fstabSize,
                                           "cache",
                                           "/cache",
                                           &cache_partition_type);

            free(fstab);
        } else {
            VERBOSE_PRINT(init, "No fstab.goldfish file in ramdisk image");
        }
    }

    /* Initialize system partition image */
    android_nand_add_image("system",
                           system_partition_type,
                           ANDROID_PARTITION_OPEN_MODE_MUST_EXIST,
                           android_hw->disk_systemPartition_size,
                           android_hw->disk_systemPartition_path,
                           android_hw->disk_systemPartition_initPath);

    /* Initialize data partition image */
    android_nand_add_image("userdata",
                           userdata_partition_type,
                           ANDROID_PARTITION_OPEN_MODE_CREATE_IF_NEEDED,
                           android_hw->disk_dataPartition_size,
                           android_hw->disk_dataPartition_path,
                           android_hw->disk_dataPartition_initPath);

    /* Initialize cache partition image, if any. Its type depends on the
     * kernel version. For anything >= 3.10, it must be EXT4, or
     * YAFFS2 otherwise.
     */
    if (android_hw->disk_cachePartition != 0) {
        if (cache_partition_type == ANDROID_PARTITION_TYPE_UNKNOWN) {
            cache_partition_type =
                (androidHwConfig_getKernelYaffs2Support(android_hw) >= 1) ?
                        ANDROID_PARTITION_TYPE_YAFFS2 :
                        ANDROID_PARTITION_TYPE_EXT4;
        }

        AndroidPartitionOpenMode cache_partition_mode =
                (android_op_wipe_data ?
                        ANDROID_PARTITION_OPEN_MODE_MUST_WIPE :
                        ANDROID_PARTITION_OPEN_MODE_CREATE_IF_NEEDED);

        android_nand_add_image("cache",
                               cache_partition_type,
                               cache_partition_mode,
                               android_hw->disk_cachePartition_size,
                               android_hw->disk_cachePartition_path,
                               NULL);
    }

    /* Init SD-Card stuff. For Android, it is always hda */
    /* If the -hda option was used, ignore the Android-provided one */
    if (hda_opts == NULL) {
        const char* sdPath = android_hw->hw_sdCard_path;
        if (sdPath && *sdPath) {
            if (!path_exists(sdPath)) {
                fprintf(stderr, "WARNING: SD Card image is missing: %s\n", sdPath);
            } else if (filelock_create(sdPath) == NULL) {
                fprintf(stderr, "WARNING: SD Card image already in use: %s\n", sdPath);
            } else {
                /* Successful locking */
                hda_opts = drive_add(sdPath, HD_ALIAS, 0);
                /* Set this property of any operation involving the SD Card
                 * will be x100 slower, due to the corresponding file being
                 * mounted as O_DIRECT. Note that this is only 'unsafe' in
                 * the context of an emulator crash. The data is already
                 * synced properly when the emulator exits (either normally or through ^C).
                 */
                qemu_opt_set(hda_opts, "cache", "unsafe");
            }
        }
    }

    if (hdb_opts == NULL) {
        const char* spath = android_hw->disk_snapStorage_path;
        if (spath && *spath) {
            if (!path_exists(spath)) {
                PANIC("Snapshot storage file does not exist: %s", spath);
            }
            if (filelock_create(spath) == NULL) {
                PANIC("Snapshot storage already in use: %s", spath);
            }
            hdb_opts = drive_add(spath, HD_ALIAS, 1);
            /* See comment above to understand why this is needed. */
            qemu_opt_set(hdb_opts, "cache", "unsafe");
        }
    }

    /* Set the VM's max heap size, passed as a boot property */
    if (android_hw->vm_heapSize > 0) {
        char  tmp[64];
        snprintf(tmp, sizeof(tmp), "%dm", android_hw->vm_heapSize);
        boot_property_add("dalvik.vm.heapsize",tmp);
    }

    /* From API 19 and above, the platform provides an explicit property for low memory devices. */
    if (android_hw->hw_ramSize <= 512) {
        boot_property_add("ro.config.low_ram", "true");
    }

    /* Initialize net speed and delays stuff. */
    if (android_parse_network_speed(android_op_netspeed) < 0 ) {
        PANIC("invalid -netspeed parameter '%s'",
                android_op_netspeed);
    }

    if ( android_parse_network_latency(android_op_netdelay) < 0 ) {
        PANIC("invalid -netdelay parameter '%s'",
                android_op_netdelay);
    }

    if (android_op_netfast) {
        qemu_net_download_speed = 0;
        qemu_net_upload_speed = 0;
        qemu_net_min_latency = 0;
        qemu_net_max_latency = 0;
    }

    /* Initialize LCD density */
    if (android_hw->hw_lcd_density) {
        long density = android_hw->hw_lcd_density;
        if (density <= 0) {
            PANIC("Invalid hw.lcd.density value: %ld", density);
        }
        hwLcd_setBootProperty(density);
    }

    /* Initialize presence of hardware nav button */
    boot_property_add("qemu.hw.mainkeys", android_hw->hw_mainKeys ? "1" : "0");

    /* Initialize TCP dump */
    if (android_op_tcpdump) {
        if (qemu_tcpdump_start(android_op_tcpdump) < 0) {
            fprintf(stdout, "could not start packet capture: %s\n", strerror(errno));
        }
    }

    /* Initialize modem */
    if (android_op_radio) {
        CharDriverState*  cs = qemu_chr_open("radio", android_op_radio, NULL);
        if (cs == NULL) {
            PANIC("unsupported character device specification: %s\n"
                        "used -help-char-devices for list of available formats",
                    android_op_radio);
        }
        android_qemud_set_channel( ANDROID_QEMUD_GSM, cs);
    } else if (android_hw->hw_gsmModem != 0 ) {
        if ( android_qemud_get_channel( ANDROID_QEMUD_GSM, &android_modem_cs ) < 0 ) {
            PANIC("could not initialize qemud 'gsm' channel");
        }
    }

    /* Initialize GPS */
    if (android_op_gps) {
        CharDriverState*  cs = qemu_chr_open("gps", android_op_gps, NULL);
        if (cs == NULL) {
            PANIC("unsupported character device specification: %s\n"
                        "used -help-char-devices for list of available formats",
                    android_op_gps);
        }
        android_qemud_set_channel( ANDROID_QEMUD_GPS, cs);
    } else if (android_hw->hw_gps != 0) {
        if ( android_qemud_get_channel( "gps", &android_gps_cs ) < 0 ) {
            PANIC("could not initialize qemud 'gps' channel");
        }
    }

    /* Initialize audio. */
    if (android_op_audio) {
        if ( !audio_check_backend_name( 0, android_op_audio ) ) {
            PANIC("'%s' is not a valid audio output backend. see -help-audio-out",
                    android_op_audio);
        }
        setenv("QEMU_AUDIO_DRV", android_op_audio, 1);
    }

    /* Initialize OpenGLES emulation */
    //android_hw_opengles_init();

    /* Initialize fake camera */
    if (strcmp(android_hw->hw_camera_back, "emulated") &&
        strcmp(android_hw->hw_camera_front, "emulated")) {
        /* Fake camera is not used for camera emulation. */
        boot_property_add("qemu.sf.fake_camera", "none");
    } else {
        /* Fake camera is used for at least one camera emulation. */
        if (!strcmp(android_hw->hw_camera_back, "emulated") &&
            !strcmp(android_hw->hw_camera_front, "emulated")) {
            /* Fake camera is used for both, front and back camera emulation. */
            boot_property_add("qemu.sf.fake_camera", "both");
        } else if (!strcmp(android_hw->hw_camera_back, "emulated")) {
            boot_property_add("qemu.sf.fake_camera", "back");
        } else {
            boot_property_add("qemu.sf.fake_camera", "front");
        }
    }

    /* Set LCD density (if required by -qemu, and AVD is missing it. */
    if (android_op_lcd_density && !android_hw->hw_lcd_density) {
        int density;
        if (parse_int(android_op_lcd_density, &density) || density <= 0) {
            PANIC("-lcd-density : %d", density);
        }
        hwLcd_setBootProperty(density);
    }

    /* Initialize camera emulation. */
    android_camera_service_init();

    if (android_op_cpu_delay) {
        char*   end;
        long    delay = strtol(android_op_cpu_delay, &end, 0);
        if (end == NULL || *end || delay < 0 || delay > 1000 ) {
            PANIC("option -cpu-delay must be an integer between 0 and 1000" );
        }
        if (delay > 0)
            delay = (1000-delay);

        qemu_cpu_delay = (int) delay;
    }

    if (android_op_dns_server) {
        char*  x = strchr(android_op_dns_server, ',');
        dns_count = 0;
        if (x == NULL)
        {
            if ( add_dns_server( android_op_dns_server ) == 0 )
                dns_count = 1;
        }
        else
        {
            x = android_op_dns_server;
            while (*x) {
                char*  y = strchr(x, ',');

                if (y != NULL) {
                    *y = 0;
                    y++;
                } else {
                    y = x + strlen(x);
                }

                if (y > x && add_dns_server( x ) == 0) {
                    dns_count += 1;
                }
                x = y;
            }
        }
        if (dns_count == 0)
            fprintf( stdout, "### WARNING: will use system default DNS server\n" );
    }

    if (dns_count == 0)
        dns_count = slirp_get_system_dns_servers();
    if (dns_count) {
        stralloc_add_format(kernel_config, " ndns=%d", dns_count);
    }

    /* qemu.gles will be read by the OpenGL ES emulation libraries.
     * If set to 0, the software GL ES renderer will be used as a fallback.
     * If the parameter is undefined, this means the system image runs
     * inside an emulator that doesn't support GPU emulation at all.
     *
     * We always start the GL ES renderer so we can gather stats on the
     * underlying GL implementation. If GL ES acceleration is disabled,
     * we just shut it down again once we have the strings. */
    {
        int qemu_gles = 0;
        if (android_initOpenglesEmulation() == 0 &&
            android_startOpenglesRenderer(android_hw->hw_lcd_width, android_hw->hw_lcd_height) == 0)
        {
            android_getOpenglesHardwareStrings(
                    android_gl_vendor, sizeof(android_gl_vendor),
                    android_gl_renderer, sizeof(android_gl_renderer),
                    android_gl_version, sizeof(android_gl_version));
            if (android_hw->hw_gpu_enabled) {
                qemu_gles = 1;
            } else {
                android_stopOpenglesRenderer();
                qemu_gles = 0;
            }
        } else {
            dwarning("Could not initialize OpenglES emulation, using software renderer.");
        }
        if (qemu_gles) {
            stralloc_add_str(kernel_params, " qemu.gles=1");
        } else {
            stralloc_add_str(kernel_params, " qemu.gles=0");
        }
    }

    /* We always force qemu=1 when running inside QEMU */
    stralloc_add_str(kernel_params, " qemu=1");

    /* We always initialize the first serial port for the android-kmsg
     * character device (used to send kernel messages) */
    serial_hds_add_at(0, "android-kmsg");
    stralloc_add_format(kernel_params,
                        " console=%s0",
                        kernelSerialDevicePrefix);

    /* We always initialize the second serial port for the android-qemud
     * character device as well */
    serial_hds_add_at(1, "android-qemud");
    stralloc_add_format(kernel_params,
                        " android.qemud=%s1",
                        kernelSerialDevicePrefix);

    if (pid_file && qemu_create_pidfile(pid_file) != 0) {
        os_pidfile_error();
        exit(1);
    }

    /* Open the logfile at this point, if necessary. We can't open the logfile
     * when encountering either of the logging options (-d or -D) because the
     * other one may be encountered later on the command line, changing the
     * location or level of logging.
     */
    if (log_mask) {
        int mask;
        if (log_file) {
            qemu_set_log_filename(log_file);
        }

        mask = qemu_str_to_log_mask(log_mask);
        if (!mask) {
            qemu_print_log_usage(stdout);
            exit(1);
        }
        qemu_set_log(mask);
    }

#if defined(CONFIG_KVM)
    if (kvm_allowed < 0) {
        kvm_allowed = kvm_check_allowed();
    }
#endif

    machine->max_cpus = machine->max_cpus ?: 1; /* Default to UP */
    if (smp_cpus > machine->max_cpus) {
        PANIC("Number of SMP cpus requested (%d), exceeds max cpus "
                "supported by machine `%s' (%d)", smp_cpus,  machine->name,
                machine->max_cpus);
    }

    if (display_type == DT_NOGRAPHIC) {
       if (serial_device_index == 0)
           serial_devices[0] = "stdio";
       if (parallel_device_index == 0)
           parallel_devices[0] = "null";
       if (strncmp(monitor_device, "vc", 2) == 0)
           monitor_device = "stdio";
    }

    if (qemu_init_main_loop()) {
        PANIC("qemu_init_main_loop failed");
    }

    if (kernel_filename == NULL) {
        kernel_filename = android_hw->kernel_path;
    }
    if (initrd_filename == NULL) {
        initrd_filename = android_hw->disk_ramdisk_path;
    }

    linux_boot = (kernel_filename != NULL);
    net_boot = (boot_devices_bitmap >> ('n' - 'a')) & 0xF;

    if (!linux_boot && *kernel_cmdline != '\0') {
        PANIC("-append only allowed with -kernel option");
    }

    if (!linux_boot && initrd_filename != NULL) {
        PANIC("-initrd only allowed with -kernel option");
    }

    /* boot to floppy or the default cd if no hard disk defined yet */
    if (!boot_devices[0]) {
        boot_devices = "cad";
    }
    os_set_line_buffering();

    if (init_timer_alarm() < 0) {
        PANIC("could not initialize alarm timer");
    }
    configure_icount(icount_option);

    /* init network clients */
    if (nb_net_clients == 0) {
        /* if no clients, we use a default config */
        net_clients[nb_net_clients++] = "nic";
#ifdef CONFIG_SLIRP
        net_clients[nb_net_clients++] = "user";
#endif
    }

    for(i = 0;i < nb_net_clients; i++) {
        if (net_client_parse(net_clients[i]) < 0) {
            PANIC("Unable to parse net clients");
        }
    }
    net_client_check();

#ifdef TARGET_I386
    /* XXX: this should be moved in the PC machine instantiation code */
    if (net_boot != 0) {
        int netroms = 0;
	for (i = 0; i < nb_nics && i < 4; i++) {
	    const char *model = nd_table[i].model;
	    char buf[1024];
            char *filename;
            if (net_boot & (1 << i)) {
                if (model == NULL)
                    model = "ne2k_pci";
                snprintf(buf, sizeof(buf), "pxe-%s.bin", model);
                filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, buf);
                if (filename && get_image_size(filename) > 0) {
                    if (nb_option_roms >= MAX_OPTION_ROMS) {
                        PANIC("Too many option ROMs");
                    }
                    option_rom[nb_option_roms] = g_strdup(buf);
                    nb_option_roms++;
                    netroms++;
                }
                if (filename) {
                    g_free(filename);
                }
            }
	}
	if (netroms == 0) {
	    PANIC("No valid PXE rom found for network device");
	}
    }
#endif

    /* init the memory */
    if (ram_size == 0) {
        ram_size = android_hw->hw_ramSize * 1024LL * 1024;
        if (ram_size == 0) {
            ram_size = DEFAULT_RAM_SIZE * 1024 * 1024;
        }
    }

    /* Quite often (especially on older XP machines) attempts to allocate large
     * VM RAM is going to fail, and crash the emulator. Since it's failing deep
     * inside QEMU, it's not really possible to provide the user with a
     * meaningful explanation for the crash. So, lets see if QEMU is going to be
     * able to allocate requested amount of RAM, and if not, lets try to come up
     * with a recomendation. */
    {
        ram_addr_t r_ram = ram_size;
        void* alloc_check = malloc(r_ram);
        while (alloc_check == NULL && r_ram > 1024 * 1024) {
        /* Make it 25% less */
            r_ram -= r_ram / 4;
            alloc_check = malloc(r_ram);
        }
        if (alloc_check != NULL) {
            free(alloc_check);
        }
        if (r_ram != ram_size) {
            /* Requested RAM is too large. Report this, as well as calculated
             * recomendation. */
            dwarning("Requested RAM size of %dMB is too large for your environment, and is reduced to %dMB.",
                     (int)(ram_size / 1024 / 1024), (int)(r_ram / 1024 / 1024));
            ram_size = r_ram;
        }
    }

#ifndef _WIN32
    qemu_log_rotation_init();
#endif

    /* init the dynamic translator */
    cpu_exec_init_all(tb_size * 1024 * 1024);

    bdrv_init();

    /* we always create the cdrom drive, even if no disk is there */
#if 0
    if (nb_drives_opt < MAX_DRIVES)
        drive_add(NULL, CDROM_ALIAS);

    /* we always create at least one floppy */

    if (nb_drives_opt < MAX_DRIVES)
        drive_add(NULL, FD_ALIAS, 0);
    /* we always create one sd slot, even if no card is in it */

    if (1) {
        drive_add(NULL, SD_ALIAS);
    }
#endif

    /* open the virtual block devices */
    if (snapshot)
        qemu_opts_foreach(qemu_find_opts("drive"), drive_enable_snapshot, NULL, 0);
    if (qemu_opts_foreach(qemu_find_opts("drive"), drive_init_func, &machine->use_scsi, 1) != 0)
        exit(1);

    //register_savevm(NULL, "timer", 0, 2, timer_save, timer_load, &timers_state);

    SaveVMHandlers* ops = g_malloc0(sizeof(*ops));
    ops->save_live_state = ram_save_live;
    ops->load_state = ram_load;

    register_savevm_live(NULL,
                         "ram",
                         0,
                         3,
                         ops,
                         NULL);

    /* must be after terminal init, SDL library changes signal handlers */
    os_setup_signal_handling();

    /* Maintain compatibility with multiple stdio monitors */
    if (!strcmp(monitor_device,"stdio")) {
        for (i = 0; i < MAX_SERIAL_PORTS; i++) {
            const char *devname = serial_devices[i];
            if (devname && !strcmp(devname,"mon:stdio")) {
                monitor_device = NULL;
                break;
            } else if (devname && !strcmp(devname,"stdio")) {
                monitor_device = NULL;
                serial_devices[i] = "mon:stdio";
                break;
            }
        }
    }

    if (nb_numa_nodes > 0) {
        int i;

        if (nb_numa_nodes > smp_cpus) {
            nb_numa_nodes = smp_cpus;
        }

        /* If no memory size if given for any node, assume the default case
         * and distribute the available memory equally across all nodes
         */
        for (i = 0; i < nb_numa_nodes; i++) {
            if (node_mem[i] != 0)
                break;
        }
        if (i == nb_numa_nodes) {
            uint64_t usedmem = 0;

            /* On Linux, the each node's border has to be 8MB aligned,
             * the final node gets the rest.
             */
            for (i = 0; i < nb_numa_nodes - 1; i++) {
                node_mem[i] = (ram_size / nb_numa_nodes) & ~((1 << 23UL) - 1);
                usedmem += node_mem[i];
            }
            node_mem[i] = ram_size - usedmem;
        }

        for (i = 0; i < nb_numa_nodes; i++) {
            if (node_cpumask[i] != 0)
                break;
        }
        /* assigning the VCPUs round-robin is easier to implement, guest OSes
         * must cope with this anyway, because there are BIOSes out there in
         * real machines which also use this scheme.
         */
        if (i == nb_numa_nodes) {
            for (i = 0; i < smp_cpus; i++) {
                node_cpumask[i % nb_numa_nodes] |= 1 << i;
            }
        }
    }

    if (kvm_enabled()) {
        int ret;

        ret = kvm_init(smp_cpus);
        if (ret < 0) {
            PANIC("failed to initialize KVM");
        }
    }

#ifdef CONFIG_HAX
    if (!hax_disabled)
    {
        int ret;

        hax_set_ramsize(ram_size);
        ret = hax_init(smp_cpus);
        fprintf(stderr, "HAX is %s and emulator runs in %s mode\n",
            !ret ? "working" :"not working", !ret ? "fast virt" : "emulation");
    }
#endif

    if (monitor_device) {
        monitor_hd = qemu_chr_open("monitor", monitor_device, NULL);
        if (!monitor_hd) {
            PANIC("qemu: could not open monitor device '%s'",
                              monitor_device);
        }
    }

    for(i = 0; i < MAX_SERIAL_PORTS; i++) {
        serial_hds_add(serial_devices[i]);
    }

    for(i = 0; i < MAX_PARALLEL_PORTS; i++) {
        const char *devname = parallel_devices[i];
        if (devname && strcmp(devname, "none")) {
            char label[32];
            snprintf(label, sizeof(label), "parallel%d", i);
            parallel_hds[i] = qemu_chr_open(label, devname, NULL);
            if (!parallel_hds[i]) {
                PANIC("qemu: could not open parallel device '%s'",
                        devname);
            }
        }
    }

    for(i = 0; i < MAX_VIRTIO_CONSOLES; i++) {
        const char *devname = virtio_consoles[i];
        if (devname && strcmp(devname, "none")) {
            char label[32];
            snprintf(label, sizeof(label), "virtcon%d", i);
            virtcon_hds[i] = qemu_chr_open(label, devname, NULL);
            if (!virtcon_hds[i]) {
                PANIC("qemu: could not open virtio console '%s'",
                        devname);
            }
        }
    }

    module_call_init(MODULE_INIT_DEVICE);


    /* Check the CPU Architecture value */
    {
        static const char* kSupportedArchs[] = {
#if defined(TARGET_ARM)
            "arm",
#endif
#if defined(TARGET_I386)
            "x86",
#endif
#if defined(TARGET_X86_64)
            "x86_64",
#endif
#if defined(TARGET_MIPS)
            "mips",
#endif
        };
        const size_t kNumSupportedArchs =
                sizeof(kSupportedArchs) / sizeof(kSupportedArchs[0]);
        bool supported_arch = false;
        size_t n;
        for (n = 0; n < kNumSupportedArchs; ++n) {
            if (!strcmp(android_hw->hw_cpu_arch, kSupportedArchs[n])) {
                supported_arch = true;
                break;
            }
        }
        if (!supported_arch) {
            fprintf(stderr, "-- Invalid CPU architecture: %s, valid values:",
                    android_hw->hw_cpu_arch);
            for (n = 0; n < kNumSupportedArchs; ++n) {
                fprintf(stderr, " %s", kSupportedArchs[n]);
            }
            fprintf(stderr, "\n");
            exit(1);
        }
    }

    /* Grab CPU model if provided in hardware.ini */
    if (    !cpu_model
         && android_hw->hw_cpu_model
         && android_hw->hw_cpu_model[0] != '\0')
    {
        cpu_model = android_hw->hw_cpu_model;
    }

    /* Combine kernel command line passed from the UI with parameters
     * collected during initialization.
     *
     * The order is the following:
     * - parameters from the hw configuration (kernel.parameters)
     * - additionnal parameters from options (e.g. -memcheck)
     * - the -append parameters.
     */
    {
        const char* kernel_parameters;

        if (android_hw->kernel_parameters) {
            stralloc_add_c(kernel_params, ' ');
            stralloc_add_str(kernel_params, android_hw->kernel_parameters);
        }

        /* If not empty, kernel_config always contains a leading space */
        stralloc_append(kernel_params, kernel_config);

        if (*kernel_cmdline) {
            stralloc_add_c(kernel_params, ' ');
            stralloc_add_str(kernel_params, kernel_cmdline);
        }

        /* Remove any leading/trailing spaces */
        stralloc_strip(kernel_params);

        kernel_parameters = stralloc_cstr(kernel_params);
        VERBOSE_PRINT(init, "Kernel parameters: %s", kernel_parameters);

        machine->init(ram_size,
                      boot_devices,
                      kernel_filename,
                      kernel_parameters,
                      initrd_filename,
                      cpu_model);

        /* Initialize multi-touch emulation. */
        if (androidHwConfig_isScreenMultiTouch(android_hw)) {
            mts_port_create(NULL);
        }

        stralloc_reset(kernel_params);
        stralloc_reset(kernel_config);
    }

    CPU_FOREACH(cpu) {
        for (i = 0; i < nb_numa_nodes; i++) {
            if (node_cpumask[i] & (1 << cpu->cpu_index)) {
                cpu->numa_node = i;
            }
        }
    }

    current_machine = machine;

    /* Set KVM's vcpu state to qemu's initial CPUOldState. */
    if (kvm_enabled()) {
        int ret;

        ret = kvm_sync_vcpus();
        if (ret < 0) {
            PANIC("failed to initialize vcpus");
        }
    }

#ifdef CONFIG_HAX
    if (hax_enabled())
        hax_sync_vcpus();
#endif

    /* just use the first displaystate for the moment */
    ds = get_displaystate();

    /* Initialize display from the command line parameters. */
    android_display_reset(ds,
                          android_display_width,
                          android_display_height,
                          android_display_bpp);

    if (display_type == DT_DEFAULT) {
#if defined(CONFIG_SDL) || defined(CONFIG_COCOA)
        display_type = DT_SDL;
#else
        display_type = DT_VNC;
        vnc_display = "localhost:0,to=99";
        show_vnc_port = 1;
#endif
    }


    switch (display_type) {
    case DT_NOGRAPHIC:
        break;
#if defined(CONFIG_CURSES)
    case DT_CURSES:
        curses_display_init(ds, full_screen);
        break;
#endif
#if defined(CONFIG_SDL) && !defined(CONFIG_STANDALONE_CORE)
    case DT_SDL:
        sdl_display_init(ds, full_screen, no_frame);
        break;
#elif defined(CONFIG_COCOA)
    case DT_SDL:
        cocoa_display_init(ds, full_screen);
        break;
#elif defined(CONFIG_STANDALONE_CORE)
    case DT_SDL:
        coredisplay_init(ds);
        break;
#endif
    case DT_VNC:
        vnc_display_init(ds);
        if (vnc_display_open(ds, vnc_display) < 0) {
            PANIC("Unable to initialize VNC display");
        }

        if (show_vnc_port) {
            printf("VNC server running on `%s'\n", vnc_display_local_addr(ds));
        }
        break;
    default:
        break;
    }
    dpy_resize(ds);

    dcl = ds->listeners;
    while (dcl != NULL) {
        if (dcl->dpy_refresh != NULL) {
            ds->gui_timer = timer_new(QEMU_CLOCK_REALTIME, SCALE_MS, gui_update, ds);
            timer_mod(ds->gui_timer, qemu_clock_get_ms(QEMU_CLOCK_REALTIME));
        }
        dcl = dcl->next;
    }

    if (display_type == DT_NOGRAPHIC || display_type == DT_VNC) {
        nographic_timer = timer_new(QEMU_CLOCK_REALTIME, SCALE_MS, nographic_update, NULL);
        timer_mod(nographic_timer, qemu_clock_get_ms(QEMU_CLOCK_REALTIME));
    }

    text_consoles_set_display(ds);
    qemu_chr_initial_reset();

    if (monitor_device && monitor_hd)
        monitor_init(monitor_hd, MONITOR_USE_READLINE | MONITOR_IS_DEFAULT);

    for(i = 0; i < MAX_SERIAL_PORTS; i++) {
        const char *devname = serial_devices[i];
        if (devname && strcmp(devname, "none")) {
            if (strstart(devname, "vc", 0))
                qemu_chr_printf(serial_hds[i], "serial%d console\r\n", i);
        }
    }

    for(i = 0; i < MAX_PARALLEL_PORTS; i++) {
        const char *devname = parallel_devices[i];
        if (devname && strcmp(devname, "none")) {
            if (strstart(devname, "vc", 0))
                qemu_chr_printf(parallel_hds[i], "parallel%d console\r\n", i);
        }
    }

    for(i = 0; i < MAX_VIRTIO_CONSOLES; i++) {
        const char *devname = virtio_consoles[i];
        if (virtcon_hds[i] && devname) {
            if (strstart(devname, "vc", 0))
                qemu_chr_printf(virtcon_hds[i], "virtio console%d\r\n", i);
        }
    }

    if (gdbstub_dev && gdbserver_start(gdbstub_dev) < 0) {
        PANIC("qemu: could not open gdbserver on device '%s'",
                gdbstub_dev);
    }

    /* call android-specific setup function */
    android_emulation_setup();

#if !defined(CONFIG_STANDALONE_CORE)
    // For the standalone emulator (UI+core in one executable) we need to
    // set the window title here.
    android_emulator_set_base_port(android_base_port);
#endif

    if (loadvm)
        do_loadvm(cur_mon, loadvm);

    if (incoming) {
        autostart = 0; /* fixme how to deal with -daemonize */
        qemu_start_incoming_migration(incoming);
    }

    if (autostart)
        vm_start();

    os_setup_post();

#ifdef CONFIG_ANDROID
    // This will notify the UI that the core is successfuly initialized
    android_core_init_completed();
#endif  // CONFIG_ANDROID

    main_loop();
    quit_timers();
    net_cleanup();
    android_emulation_teardown();
    return 0;
}

void
android_emulation_teardown(void)
{
    android_charmap_done();
}
