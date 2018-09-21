/*
 * Oscar kernel-userspace interface definitions.
 *
 * Copyright (C) 2018 Google, Inc.
 */
#ifndef __OSCAR_IOCTL_H__
#define __OSCAR_IOCTL_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

struct oscar_gate_clock_ioctl {
  /* Enter or leave clock gated state. */
  uint64_t enable;

  /* If set, enter clock gating state, regardless of custom block's
   * internal idle state
   */
  uint64_t force_idle;
};

struct oscar_abdram_alloc_ioctl {
  uint32_t flags;
  uint64_t size;
};

#define OSCAR_SYNC_TO_BUFFER 0x1
#define OSCAR_SYNC_FROM_BUFFER 0x2

struct oscar_abdram_sync_ioctl {
  uint32_t fd;
  uint32_t cmd;
  uint64_t host_address;
  uint64_t len;
};

struct oscar_abdram_map_ioctl {
  uint32_t fd;
  uint32_t flags;
  uint32_t page_table_index;
  uint64_t device_address;
};

/* Base number for all Oscar-common IOCTLs */
#define OSCAR_IOCTL_BASE 0x7F

/* Enable/Disable clock gating. */
#define OSCAR_IOCTL_GATE_CLOCK \
  _IOW(OSCAR_IOCTL_BASE, 0, struct oscar_gate_clock_ioctl)

#define OSCAR_IOCTL_ABC_ALLOC_BUFFER \
  _IOW(OSCAR_IOCTL_BASE, 1, struct oscar_abdram_alloc_ioctl)

#define OSCAR_IOCTL_ABC_SYNC_BUFFER \
  _IOW(OSCAR_IOCTL_BASE, 2, struct oscar_abdram_sync_ioctl)

#define OSCAR_IOCTL_ABC_MAP_BUFFER \
  _IOW(OSCAR_IOCTL_BASE, 3, struct oscar_abdram_map_ioctl)

#define OSCAR_IOCTL_ABC_UNMAP_BUFFER _IOW(OSCAR_IOCTL_BASE, 4, uint32_t)

#define OSCAR_IOCTL_ABC_DEALLOC_BUFFER _IOW(OSCAR_IOCTL_BASE, 5, uint32_t)

#endif /* __OSCAR_IOCTL_H__ */
