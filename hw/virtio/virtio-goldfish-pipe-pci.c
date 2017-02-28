/* Copyright (c) 2017 The Android Open Source Project
**
** Authors:
**    Huihong Luo huisinro@google.com
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
#include "hw/pci/pci.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-bus.h"
#include "hw/virtio/virtio-pci.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
#include "qapi/error.h"

/* Set to > 0 for debug output */
#define PIPE_DEBUG 2

/* Set to 1 to debug i/o register reads/writes */
#define PIPE_DEBUG_REGS 0

#if PIPE_DEBUG >= 1
#define D(fmt, ...) \
    do { fprintf(stdout, "virtio_goldfish_pipe_pci: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define D(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG >= 2
#define DD(fmt, ...) \
    do { fprintf(stdout, "virtio_goldfish_pipe_pci: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define DD(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG_REGS >= 1
#  define DR(...)   D(__VA_ARGS__)
#else
#  define DR(...)   do { /* nothing */ } while (0)
#endif

#define E(fmt, ...)  \
    do { fprintf(stdout, "ERROR:" fmt "\n", ## __VA_ARGS__); } while (0)

#define APANIC(...)                     \
    do {                                \
        error_report(__VA_ARGS__);      \
        exit(1);                        \
    } while (0)


static void virtio_goldfish_pipe_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VirtIOGoldfishPipePCI *vgp = VIRTIO_GOLDFISH_PIPE_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&vgp->vdev);
    Error *err = NULL;

    DD("%s():", __func__);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    object_property_set_bool(OBJECT(vdev), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    //object_property_set_link(OBJECT(vgp),
    //             OBJECT(vgp->vdev.conf.vgpdev), "goldfishpipedev",
    //             NULL);
}

static void virtio_goldfish_pipe_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    DD("%s():", __func__);
		
    k->realize = virtio_goldfish_pipe_pci_realize;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_GOLDFISH_PIPE;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_goldfish_pipe_initfn(Object *obj)
{
    VirtIOGoldfishPipePCI *dev = VIRTIO_GOLDFISH_PIPE_PCI(obj);

    DD("%s():", __func__);

    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_GOLDFISH_PIPE);
    //object_property_add_alias(obj, "goldfishpipedev", OBJECT(&dev->vdev),
    //                          "goldfishpipedev", &error_abort);
}

static const TypeInfo virtio_goldfish_pipe_pci_info = {
    .name          = TYPE_VIRTIO_GOLDFISH_PIPE_PCI,
    .parent        = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOGoldfishPipePCI),
    .instance_init = virtio_goldfish_pipe_initfn,
    .class_init    = virtio_goldfish_pipe_pci_class_init,
};

static void virtio_goldfish_pipe_pci_register_types(void)
{
    type_register_static(&virtio_goldfish_pipe_pci_info);
}

type_init(virtio_goldfish_pipe_pci_register_types)
