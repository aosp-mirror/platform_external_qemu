/*
 * Virtio video device
 *
 * Copyright Red Hat
 *
 * Authors:
 *  Dave Airlie
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/pci/pci.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-bus.h"
#include "hw/virtio/virtio-pci.h"
#include "hw/virtio/virtio-gpu.h"
#include "standard-headers/linux/virtio_pci.h"

static Property virtio_gpu_pci_properties[] = {
    DEFINE_VIRTIO_GPU_PCI_PROPERTIES(VirtIOPCIProxy),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_gpu_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VirtIOGPUPCI *vgpu = VIRTIO_GPU_PCI(vpci_dev);
    fprintf(stderr, "%s: proxy: %p\n", __func__, vpci_dev);
    VirtIOGPU *g = &vgpu->vdev;
    DeviceState *vdev = DEVICE(&vgpu->vdev);
    int i;
    Error *local_error = NULL;

    // | 0 1 : modern | 2 msix | 
    vpci_dev->modern_mem_bar_idx = 0;
    vpci_dev->msix_bar_idx = 2; 
    vpci_dev->hostshm_mem_bar_idx = 4;

    memory_region_init(
            &g->host_coherent_memory, OBJECT(g),
            "virtio-gpu-host-coherent", 512 * 1048576);

    struct virtio_pci_shm_cap shm_cap;
    uint32_t mask32 = ~0;

    shm_cap.cap.cap_len = sizeof(shm_cap);
    shm_cap.cap.cfg_type = 8; // PciCapabilityType::SharedMemoryConfig
    shm_cap.cap.bar = vpci_dev->hostshm_mem_bar_idx;

    shm_cap.cap.length = cpu_to_le32(mask32 & (g->host_coherent_memory.size));
    shm_cap.length_hi = cpu_to_le32(mask32 & (g->host_coherent_memory.size >> 32ULL));

    shm_cap.cap.offset = mask32 & 0;
    shm_cap.offset_hi = mask32 & 0;

    shm_cap.id = 1;

    fprintf(stderr, "%s: memory region ram addr: 0x%llx\n", __func__,
            (unsigned long long)(memory_region_get_ram_addr(&g->host_coherent_memory)));

    fprintf(stderr, "%s: adding shm mem cap. type 0x%x bar %d off 0x%llx length 0x%llx\n", __func__,
            (uint32_t)(shm_cap.cap.cfg_type),
            (int)(shm_cap.cap.bar),
            (unsigned long long)(shm_cap.cap.offset),
            (unsigned long long)(shm_cap.cap.length));

    {


        fprintf(stderr, "%s: memory region ram addr: 0x%llx\n", __func__,
                (unsigned long long)(memory_region_get_ram_addr(&g->host_coherent_memory)));


        pci_register_bar(&vpci_dev->pci_dev, vpci_dev->hostshm_mem_bar_idx,
                PCI_BASE_ADDRESS_SPACE_MEMORY |
                PCI_BASE_ADDRESS_MEM_TYPE_64, &g->host_coherent_memory);
        fprintf(stderr, "%s: do hostshm init done\n", __func__);
        fprintf(stderr, "%s: memory region ram addr: 0x%llx\n", __func__,
                (unsigned long long)(memory_region_get_ram_addr(&g->host_coherent_memory)));

        int offset = pci_add_capability(&vpci_dev->pci_dev, PCI_CAP_ID_VNDR, 0, shm_cap.cap.cap_len, &error_abort);
        fprintf(stderr, "%s: offset of host shm: %d\n", __func__, offset);
        memcpy(vpci_dev->pci_dev.config + offset + PCI_CAP_FLAGS, &shm_cap.cap.cap_len,
                shm_cap.cap.cap_len - PCI_CAP_FLAGS);
    }

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus));
    virtio_pci_force_virtio_1(vpci_dev);

    object_property_set_bool(OBJECT(vdev), true, "realized", &local_error);
    fprintf(stderr, "%s: proxy: %p (after)\n", __func__, vpci_dev);

    fprintf(stderr, "%s: memory region ram addr: 0x%llx (right after register bar)\n", __func__,
            (unsigned long long)(memory_region_get_ram_addr(&g->host_coherent_memory)));
    if (local_error) {
        error_propagate(errp, local_error);
        return;
    }

    for (i = 0; i < g->conf.max_outputs; i++) {
        object_property_set_link(OBJECT(g->scanout[i].con),
                                 OBJECT(vpci_dev),
                                 "device", errp);
    }
}

static void virtio_gpu_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
    dc->props = virtio_gpu_pci_properties;
    dc->hotpluggable = false;
    k->realize = virtio_gpu_pci_realize;
    pcidev_k->class_id = PCI_CLASS_DISPLAY_OTHER;
}

static void virtio_gpu_initfn(Object *obj)
{
    VirtIOGPUPCI *dev = VIRTIO_GPU_PCI(obj);

    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_GPU);
}

static const TypeInfo virtio_gpu_pci_info = {
    .name = TYPE_VIRTIO_GPU_PCI,
    .parent = TYPE_VIRTIO_PCI,
    .instance_size = sizeof(VirtIOGPUPCI),
    .instance_init = virtio_gpu_initfn,
    .class_init = virtio_gpu_pci_class_init,
};

static void virtio_gpu_pci_register_types(void)
{
    type_register_static(&virtio_gpu_pci_info);
}
type_init(virtio_gpu_pci_register_types)
