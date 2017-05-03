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
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/misc/goldfish_pstore.h"
#include "exec/address-spaces.h"

#define TYPE_GOLDFISH_PSTORE "goldfish_pstore"
#define GOLDFISH_PSTORE(obj) OBJECT_CHECK(struct goldfish_pstore, (obj), TYPE_GOLDFISH_PSTORE)


/**
  A goldfish pstore is currently an area of reservered memory that survives reboots. In the future
  this memory can be stored on disk, so that it will be there on emulator restarts as well.

  It takes up 64kb that lives at the end of the goldfish reserved memory area.
*/

struct goldfish_pstore {
    DeviceState parent;

    MemoryRegion memory;
};

static void goldfish_pstore_realize(DeviceState *dev, Error **errp)
{
    struct goldfish_pstore *s = GOLDFISH_PSTORE(dev);

    // Reserve a slot of memory that we will use for our pstore.
    // TODO(jansene): We should probably load it from disk..
    memory_region_init_ram(&s->memory, OBJECT(s), "goldfish_pstore", GOLDFISH_PSTORE_MEM_SIZE, errp);

    // Ok, now we just need to move it to the right physical address.
    memory_region_add_subregion(get_system_memory(), (hwaddr) GOLDFISH_PSTORE_MEM_BASE, &s->memory);
}


static void goldfish_pstore_unrealize(DeviceState *dev, Error **errp)
{
    // TODO(jansene): Here we want to write the memory contents
    // to a file. 
    struct goldfish_pstore *s = GOLDFISH_PSTORE(dev);
}

static void goldfish_pstore_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_pstore_realize;
    dc->unrealize = goldfish_pstore_unrealize;
    dc->desc = "goldfish pstore";
}

static const TypeInfo goldfish_pstore_info = {
    .name              = TYPE_GOLDFISH_PSTORE,
    .parent            = TYPE_DEVICE,
    .instance_size     = sizeof(struct goldfish_pstore),
    .class_init        = goldfish_pstore_class_init,
};

static void goldfish_pstore_register(void)
{
    type_register_static(&goldfish_pstore_info);
}

type_init(goldfish_pstore_register);

