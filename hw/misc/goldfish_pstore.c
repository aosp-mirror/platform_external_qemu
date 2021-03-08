/* Copyright (C) 2017 The Android Open Source Project
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
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/misc/goldfish_pstore.h"
#include "exec/address-spaces.h"
#include "io/channel-file.h"
#include "migration/qemu-file.h"
#include "migration/qemu-file-channel.h"

#define GOLDFISH_PSTORE(obj) OBJECT_CHECK(goldfish_pstore, (obj), TYPE_GOLDFISH_PSTORE)

/**
  A goldfish pstore is currently an area of reservered memory that survives reboots. The memory
  is persisted to disk if the filename parameter has been given. We persist the memory region
  when the device is unrealized.
*/
typedef struct goldfish_pstore {
    DeviceState parent;

    MemoryRegion memory;
    uint64_t size;   /* Size of the region in bytes, keep in mind the larger this
                        gets, the longer writes on exit take */
    hwaddr   addr;   /* Physical guest address where this memory shows up */
    char*    fname;  /* File where we will (re)store the bytes on entry/exit */
} goldfish_pstore;

static Property goldfish_pstore_properties[] = {
    DEFINE_PROP_UINT64(GOLDFISH_PSTORE_ADDR_PROP, goldfish_pstore, addr, 0),
    DEFINE_PROP_SIZE(GOLDFISH_PSTORE_SIZE_PROP, goldfish_pstore, size, 0),
    DEFINE_PROP_STRING(GOLDFISH_PSTORE_FILE_PROP, goldfish_pstore, fname),
    DEFINE_PROP_END_OF_LIST(),
};

/* Construct a QEMUFile object from the given fd. */
static QEMUFile* qemu_file_from_fd(int fd, bool write)
{
    QIOChannel *ioc;
    QEMUFile *f;
    assert(fd > 0);

    lseek(fd, 0, SEEK_SET);
    ioc = QIO_CHANNEL(qio_channel_file_new_fd(fd));
    if (write) {
        f = qemu_fopen_channel_output(ioc);
    } else {
        f = qemu_fopen_channel_input(ioc);
    }
    object_unref(OBJECT(ioc));
    return f;
}

static void goldfish_pstore_save_restore(goldfish_pstore *store, bool write) {
  Error *local_err = NULL;
  QEMUFile *file = NULL;
  int fd = -1;
  if (!store->fname) {
    error_setg(&local_err, "No storage file has been specified");
    goto Error;
  }

  // Let's get the file handle
  if (write) {
    fd = OPEN_WRITE(store->fname);
  } else {
    fd = OPEN_READ(store->fname);
  }
  if (fd == -1) {
    if (write) {
      error_setg_errno(&local_err, errno, "Unable to open %s", store->fname);
    }
    goto Error;
  }

  // And just read/write the memory contents.
  file = qemu_file_from_fd(fd, write);
  if (write) {
    qemu_put_buffer(file, (uint8_t *)memory_region_get_ram_ptr(&store->memory),
                    memory_region_size(&store->memory));
  } else {
    size_t read = qemu_get_buffer(
        file, (uint8_t *)memory_region_get_ram_ptr(&store->memory),
        memory_region_size(&store->memory));
  }

  if (qemu_file_get_error(file)) {
    const char *flag_msg = write ? "write" : "read";
    error_setg_errno(&local_err, errno, "Unable to %s memory using file: %s",
                     flag_msg, store->fname);
    goto Error;
  }

Error:
  if (local_err && qemu_loglevel_mask(LOG_TRACE)) {
      // pstore is best effort, users frequently see warnings which is confusing.
      // pstore support is usually only needed for those that do low-level development
      // and need the reporting upon reboot. Those developers should enable tracing to
      // see the error messages
      error_report_err(local_err);
  }

  if (file) {
    qemu_fclose(file);
  }
#ifndef _MSC_VER
  close(fd);
#endif
}

static void goldfish_pstore_realize(DeviceState *dev, Error **errp) {
  goldfish_pstore *s = GOLDFISH_PSTORE(dev);

  // Reserve a slot of memory that we will use for our pstore.
  memory_region_init_ram_nomigrate(&s->memory, OBJECT(s), GOLDFISH_PSTORE_DEV_ID, s->size,
                         errp);

  // Ok, now we just need to move it to the right physical address.
  memory_region_add_subregion(get_system_memory(), s->addr, &s->memory);

  // And restore the contents from disk
  goldfish_pstore_save_restore(s, false);
}

static void goldfish_pstore_unrealize(DeviceState *dev, Error **errp) {
  goldfish_pstore *s = GOLDFISH_PSTORE(dev);
  goldfish_pstore_save_restore(s, true);
}

static void goldfish_dev_init(Object *obj) {
  // ID registration needs to happen before realization, otherwise
  // the device will show up with some unknown name in the dev tree
  // or with a name that has been handed through command line parameters.
  // We don't want any of that, so we set the device-id right here.
  goldfish_pstore *s = GOLDFISH_PSTORE(obj);
  s->parent.id = GOLDFISH_PSTORE_DEV_ID;
}

static void goldfish_pstore_class_init(ObjectClass *klass, void *data) {
  DeviceClass *dc = DEVICE_CLASS(klass);

  dc->realize = goldfish_pstore_realize;
  dc->unrealize = goldfish_pstore_unrealize;
  dc->desc = "goldfish emulated persistent storage ram";
  dc->props = goldfish_pstore_properties;
}

static const TypeInfo goldfish_pstore_info = {
    .name = TYPE_GOLDFISH_PSTORE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(goldfish_pstore),
    .instance_init = goldfish_dev_init,
    .class_init = goldfish_pstore_class_init,
};

static void goldfish_pstore_register(void) {
  type_register_static(&goldfish_pstore_info);
}

type_init(goldfish_pstore_register);

