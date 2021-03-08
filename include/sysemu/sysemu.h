#ifndef SYSEMU_H
#define SYSEMU_H
/* Misc. things related to the system emulator.  */

#include "qapi/qapi-types-run-state.h"
#include "qemu/queue.h"
#include "qemu/timer.h"
#include "qemu/notify.h"
#include "qemu/main-loop.h"
#include "qemu/bitmap.h"
#include "qemu/uuid.h"
#include "qom/object.h"

/* vl.c */

extern const char *bios_name;
extern const char *qemu_name;
extern QemuUUID qemu_uuid;
extern bool qemu_uuid_set;

bool runstate_check(RunState state);
RunState get_runstate();
void runstate_set(RunState new_state);
int runstate_is_running(void);
bool runstate_needs_reset(void);
bool runstate_store(char *str, size_t size);
typedef struct vm_change_state_entry VMChangeStateEntry;
typedef void VMChangeStateHandler(void *opaque, int running, RunState state);

VMChangeStateEntry *qemu_add_vm_change_state_handler(VMChangeStateHandler *cb,
                                                     void *opaque);
void qemu_del_vm_change_state_handler(VMChangeStateEntry *e);
void vm_state_notify(int running, RunState state);

/* Enumeration of various causes for shutdown. */
typedef enum ShutdownCause {
    SHUTDOWN_CAUSE_NONE,          /* No shutdown request pending */
    SHUTDOWN_CAUSE_HOST_ERROR,    /* An error prevents further use of guest */
    SHUTDOWN_CAUSE_HOST_QMP,      /* Reaction to a QMP command, like 'quit' */
    SHUTDOWN_CAUSE_HOST_SIGNAL,   /* Reaction to a signal, such as SIGINT */
    SHUTDOWN_CAUSE_HOST_UI,       /* Reaction to UI event, like window close */
    SHUTDOWN_CAUSE_GUEST_SHUTDOWN,/* Guest shutdown/suspend request, via
                                     ACPI or other hardware-specific means */
    SHUTDOWN_CAUSE_GUEST_RESET,   /* Guest reset request, and command line
                                     turns that into a shutdown */
    SHUTDOWN_CAUSE_GUEST_PANIC,   /* Guest panicked, and command line turns
                                     that into a shutdown */
    SHUTDOWN_CAUSE__MAX,
} ShutdownCause;

static inline bool shutdown_caused_by_guest(ShutdownCause cause)
{
    return cause >= SHUTDOWN_CAUSE_GUEST_SHUTDOWN;
}

void vm_start(void);
int vm_prepare_start(void);
int vm_stop(RunState state);
int vm_stop_force_state(RunState state);
int vm_shutdown(void);

typedef enum WakeupReason {
    /* Always keep QEMU_WAKEUP_REASON_NONE = 0 */
    QEMU_WAKEUP_REASON_NONE = 0,
    QEMU_WAKEUP_REASON_RTC,
    QEMU_WAKEUP_REASON_PMTIMER,
    QEMU_WAKEUP_REASON_OTHER,
} WakeupReason;

void qemu_system_reset_request(ShutdownCause reason);
void qemu_system_suspend_request(void);
void qemu_register_suspend_notifier(Notifier *notifier);
void qemu_system_wakeup_request(WakeupReason reason);
void qemu_system_wakeup_enable(WakeupReason reason, bool enabled);
void qemu_register_wakeup_notifier(Notifier *notifier);
void qemu_system_invalidate_exit_snapshot(void);
void qemu_system_shutdown_request(ShutdownCause reason);
void qemu_system_powerdown_request(void);
void qemu_register_powerdown_notifier(Notifier *notifier);
void qemu_system_debug_request(void);
void qemu_system_vmstop_request(RunState reason);
void qemu_system_vmstop_request_prepare(void);
bool qemu_vmstop_requested(RunState *r);
ShutdownCause qemu_shutdown_requested_get(void);
ShutdownCause qemu_reset_requested_get(void);
void qemu_system_killed(int signal, pid_t pid);
void qemu_system_reset(ShutdownCause reason);
void qemu_system_guest_panicked(GuestPanicInformation *info);

void qemu_add_exit_notifier(Notifier *notify);
void qemu_remove_exit_notifier(Notifier *notify);
void qemu_exit_notifiers_notify(void);

extern bool machine_init_done;

void qemu_add_machine_init_done_notifier(Notifier *notify);
void qemu_remove_machine_init_done_notifier(Notifier *notify);

typedef struct {
    int (*on_start)(const char* name);
    void (*on_end)(const char* name, int res);
    void (*on_quick_fail)(const char* name, int res);
    bool (*is_canceled)(const char* name);
} QEMUCallbackSet;

typedef struct {
    QEMUCallbackSet savevm;
    QEMUCallbackSet loadvm;
    QEMUCallbackSet delvm;
} QEMUSnapshotCallbacks;

typedef uint32_t (*qemu_address_space_device_gen_handle_t)(void);
typedef void (*qemu_address_space_device_destroy_handle_t)(uint32_t);
typedef void (*qemu_address_space_device_tell_ping_info_t)(uint32_t handle, uint64_t gpa);
typedef void (*qemu_address_space_device_ping_t)(uint32_t handle);
typedef int (*qemu_address_space_device_add_memory_mapping_t)(uint64_t gpa, void *ptr, uint64_t size);
typedef int (*qemu_address_space_device_remove_memory_mapping_t)(uint64_t gpa, void *ptr, uint64_t size);
typedef void* (*qemu_address_space_device_get_host_ptr_t)(uint64_t gpa);
typedef void* (*qemu_address_space_device_handle_to_context_t)(uint32_t handle);
typedef void (*qemu_address_space_device_clear_t)(void);
// virtio-gpu-next
typedef uint64_t (*qemu_address_space_device_hostmem_register_t)(uint64_t hva, uint64_t size);
typedef void (*qemu_address_space_device_hostmem_unregister_t)(uint64_t id);
typedef void (*qemu_address_space_device_ping_at_hva_t)(uint32_t handle, void* hva);
// deallocation callbacks
typedef void (*qemu_address_space_device_deallocation_callback_t)(void* context, uint64_t gpa);
typedef void (*qemu_address_space_device_register_deallocation_callback_t)(void* context, uint64_t gpa, qemu_address_space_device_deallocation_callback_t);
typedef void (*qemu_address_space_device_run_deallocation_callbacks_t)(uint64_t gpa);

struct qemu_address_space_device_control_ops {
    qemu_address_space_device_gen_handle_t gen_handle;
    qemu_address_space_device_destroy_handle_t destroy_handle;
    qemu_address_space_device_tell_ping_info_t tell_ping_info;
    qemu_address_space_device_ping_t ping;
    qemu_address_space_device_add_memory_mapping_t add_memory_mapping;
    qemu_address_space_device_remove_memory_mapping_t remove_memory_mapping;
    qemu_address_space_device_get_host_ptr_t get_host_ptr;
    qemu_address_space_device_handle_to_context_t handle_to_context;
    qemu_address_space_device_clear_t clear;
    qemu_address_space_device_hostmem_register_t hostmem_register;
    qemu_address_space_device_hostmem_unregister_t hostmem_unregister;
    qemu_address_space_device_ping_at_hva_t ping_at_hva;
    qemu_address_space_device_register_deallocation_callback_t register_deallocation_callback;
    qemu_address_space_device_run_deallocation_callbacks_t run_deallocation_callbacks;
};

void qemu_set_address_space_device_control_ops(struct qemu_address_space_device_control_ops* ops);
struct qemu_address_space_device_control_ops*
qemu_get_address_space_device_control_ops(void);

void qemu_set_snapshot_callbacks(const QEMUSnapshotCallbacks* callbacks);

typedef void (*QEMURamLoadCallback)(void*, uint64_t);

void qemu_set_ram_load_callback(QEMURamLoadCallback f);

/* A callback for regular and error message redirection */
typedef struct {
    void* opaque;
    void (*out)(void* opaque, const char* fmt, ...);
    void (*err)(void* opaque, Error* err, const char* fmt, ...);
} QEMUMessageCallback;

int qemu_savevm(const char* name, const QEMUMessageCallback* messages);
int qemu_loadvm(const char* name, const QEMUMessageCallback* messages);
int qemu_delvm(const char* name, const QEMUMessageCallback* messages);

// Callback for lazy loading of RAM for snapshots.
void qemu_ram_load(void* hostRam, uint64_t size);

// Disable or enable real audio input.
// TODO: Also a potential way to pipe fake audio input
void qemu_allow_real_audio(bool allow);
bool qemu_is_real_audio_allowed(void);

/* Populates the passed array of strings with the snapshot names.
 * If |names_count| is not NULL, it must be the size of |names| array.
 * On exit, it is set to the total snapshot count, regardless of its
 * input value.
 * Function also prints the formatted list of snapshots into the |messages|'s
 * |out| callback. */
int qemu_listvms(char (*names)[256], int* names_count,
                 const QEMUMessageCallback* messages);

void hmp_savevm(Monitor *mon, const QDict *qdict);
int save_vmstate(Monitor *mon, const char *name);
int load_vmstate(const char *name);

void qemu_announce_self(void);

extern int autostart;

typedef enum {
    VGA_NONE, VGA_STD, VGA_CIRRUS, VGA_VMWARE, VGA_XENFB, VGA_QXL,
    VGA_TCX, VGA_CG3, VGA_DEVICE, VGA_VIRTIO,
    VGA_TYPE_MAX,
} VGAInterfaceType;

extern int vga_interface_type;
#define xenfb_enabled (vga_interface_type == VGA_XENFB)

extern int graphic_width;
extern int graphic_height;
extern int graphic_depth;
extern int display_opengl;
extern const char *keyboard_layout;
extern int win2k_install_hack;
extern int alt_grab;
extern int ctrl_grab;
extern int no_frame;
extern int smp_cpus;
extern unsigned int max_cpus;
extern int cursor_hide;
extern int graphic_rotate;
extern int no_quit;
extern int no_shutdown;
extern int old_param;
extern int boot_menu;
extern bool boot_strict;
extern uint8_t *boot_splash_filedata;
extern size_t boot_splash_filedata_size;
extern bool enable_mlock;
extern uint8_t qemu_extra_params_fw[2];
extern QEMUClockType rtc_clock;
extern const char *mem_path;
extern int mem_prealloc;
extern int mem_file_shared;

#define MAX_NODES 128
#define NUMA_NODE_UNASSIGNED MAX_NODES
#define NUMA_DISTANCE_MIN         10
#define NUMA_DISTANCE_DEFAULT     20
#define NUMA_DISTANCE_MAX         254
#define NUMA_DISTANCE_UNREACHABLE 255

#define MAX_OPTION_ROMS 16
typedef struct QEMUOptionRom {
    const char *name;
    int32_t bootindex;
} QEMUOptionRom;
extern QEMUOptionRom option_rom[MAX_OPTION_ROMS];
extern int nb_option_roms;

#define MAX_PROM_ENVS 128
extern const char *prom_envs[MAX_PROM_ENVS];
extern unsigned int nb_prom_envs;

/* generic hotplug */
void hmp_drive_add(Monitor *mon, const QDict *qdict);

/* pcie aer error injection */
void hmp_pcie_aer_inject_error(Monitor *mon, const QDict *qdict);

/* serial ports */

#define MAX_SERIAL_PORTS 4

extern Chardev *serial_hds[MAX_SERIAL_PORTS];

/* parallel ports */

#define MAX_PARALLEL_PORTS 3

extern Chardev *parallel_hds[MAX_PARALLEL_PORTS];

void hmp_info_usb(Monitor *mon, const QDict *qdict);

void add_boot_device_path(int32_t bootindex, DeviceState *dev,
                          const char *suffix);
char *get_boot_devices_list(size_t *size, bool ignore_suffixes);

DeviceState *get_boot_device(uint32_t position);
void check_boot_index(int32_t bootindex, Error **errp);
void del_boot_device_path(DeviceState *dev, const char *suffix);
void device_add_bootindex_property(Object *obj, int32_t *bootindex,
                                   const char *name, const char *suffix,
                                   DeviceState *dev, Error **errp);
void restore_boot_order(void *opaque);
void validate_bootdevices(const char *devices, Error **errp);

/* handler to set the boot_device order for a specific type of MachineClass */
typedef void QEMUBootSetHandler(void *opaque, const char *boot_order,
                                Error **errp);
void qemu_register_boot_set(QEMUBootSetHandler *func, void *opaque);
void qemu_boot_set(const char *boot_order, Error **errp);

QemuOpts *qemu_get_machine_opts(void);

bool defaults_enabled(void);

extern QemuOptsList qemu_legacy_drive_opts;
extern QemuOptsList qemu_common_drive_opts;
extern QemuOptsList qemu_drive_opts;
extern QemuOptsList bdrv_runtime_opts;
extern QemuOptsList qemu_chardev_opts;
extern QemuOptsList qemu_device_opts;
extern QemuOptsList qemu_netdev_opts;
extern QemuOptsList qemu_nic_opts;
extern QemuOptsList qemu_net_opts;
extern QemuOptsList qemu_global_opts;
extern QemuOptsList qemu_mon_opts;

#endif
