#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/acpi/goldfish_defs.h"
#include "hw/sysbus.h"
#include "hw/pci/pci.h"

#define D0() fprintf(stderr, "rkir %s:%d\n", __FUNCTION__, __LINE__)
#define DS(S) fprintf(stderr, "rkir %s:%d " S "\n", __FUNCTION__, __LINE__)
#define DF(FMT, ...) fprintf(stderr, "rkir %s:%d " FMT "\n", __FUNCTION__, __LINE__, __VA_ARGS__)

/*

We need to be able to
- communicate which guest physical address is associated with which host virtual address, and when the mapping changes
- guest needs to know when the host is definitely done preparing the mapped buffer pointer
- the driver needs to communicate with guest userspace on what is the resulting physical address, mmap will hide it

*/

enum CMD_ID {
    CMD_ID_DUNNO
};

enum REGISTER_ID {
    REGISTER_ID_TRIGGER = 0,
};

struct address_space_cmd {
    enum CMD_ID cmd_id;
};

struct address_space_cmd_node {
    struct address_space_cmd_node *prev;
    struct address_space_cmd_node *next;
    struct address_space_cmd cmd;
};

struct goldfish_address_space_state {
    PCIDevice parent_obj;
    SysBusDevice parent;
    MemoryRegion control_iomem;
    MemoryRegion area_iomem;
    qemu_irq irq;

    struct address_space_cmd pending;
    struct address_space_cmd_node *head;
    struct address_space_cmd_node *tail;
};

#define GOLDFISH_ADDRESS_SPACE(obj) \
    OBJECT_CHECK(struct goldfish_address_space_state, (obj), \
                 GOLDFISH_ADDRESS_SPACE_NAME)

static uint64_t goldfish_address_space_control_read(void *opaque,
                                                    hwaddr offset,
                                                    unsigned size) {
    return 0; /* do nothing here */
}

static void goldfish_address_space_control_write(void *opaque,
                                                 hwaddr offset,
                                                 uint64_t val,
                                                 unsigned size) {
    /* do nothing here */
}

static uint64_t goldfish_address_space_area_read(void *opaque,
                                                 hwaddr offset,
                                                 unsigned size) {
    return 0; /* do nothing here */
}

static void goldfish_address_space_area_write(void *opaque,
                                              hwaddr offset,
                                              uint64_t val,
                                              unsigned size) {
    /* do nothing here */
}

static const MemoryRegionOps goldfish_address_space_control_ops = {
    .read = &goldfish_address_space_control_read,
    .write = &goldfish_address_space_control_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static const MemoryRegionOps goldfish_address_space_area_ops = {
    .read = &goldfish_address_space_area_read,
    .write = &goldfish_address_space_area_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void goldfish_address_space_pci_reset(DeviceState *dev)
{
    struct goldfish_address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);

    DF("address_space=%p", address_space);
}

static void goldfish_address_space_pci_realize(PCIDevice *dev, Error **errp) {
    struct goldfish_address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);

    DF("address_space=%p", address_space);

    memory_region_init_io(&address_space->control_iomem,
                          OBJECT(address_space),
                          &goldfish_address_space_control_ops,
                          address_space,
                          GOLDFISH_ADDRESS_SPACE_CONTROL_NAME,
                          GOLDFISH_ADDRESS_SPACE_CONTROL_SIZE);
    memory_region_init_io(&address_space->area_iomem,
                          OBJECT(address_space),
                          &goldfish_address_space_area_ops,
                          address_space,
                          GOLDFISH_ADDRESS_SPACE_AREA_NAME,
                          GOLDFISH_ADDRESS_SPACE_AREA_SIZE);

    pci_register_bar(dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &address_space->control_iomem);
    pci_register_bar(dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &address_space->area_iomem);

    DF("control_iomem={ size=%lu, addr=%08lX, name='%s' }",
        (unsigned long) address_space->control_iomem.size,
        (unsigned long) address_space->control_iomem.addr,
        address_space->control_iomem.name);

    DF("area_iomem={ size=%lu, addr=%08lX, name='%s' }",
        (unsigned long) address_space->area_iomem.size,
        (unsigned long) address_space->area_iomem.addr,
        address_space->area_iomem.name);
}

static void goldfish_address_space_pci_unrealize(PCIDevice* dev) {
    struct goldfish_address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);

    DF("address_space=%p", address_space);
}

static Property goldfish_address_space_pci_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_goldfish_address_space_pci = {
    .name = GOLDFISH_ADDRESS_SPACE_NAME,
    .version_id = 1,
    .minimum_version_id = 1,
};

static void goldfish_address_space_class_init(ObjectClass *klass, void *data) {
    DF("data=%p", data);

    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->desc = GOLDFISH_ADDRESS_SPACE_NAME;
    dc->hotpluggable = false;
    dc->reset = &goldfish_address_space_pci_reset;
    dc->props = goldfish_address_space_pci_properties;
    dc->vmsd = &vmstate_goldfish_address_space_pci;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    PCIDeviceClass* pci = PCI_DEVICE_CLASS(klass);

    pci->realize = &goldfish_address_space_pci_realize;
    pci->exit = &goldfish_address_space_pci_unrealize;
    pci->vendor_id = GOLDFISH_ADDRESS_SPACE_PCI_VENDOR_ID;
    pci->device_id = GOLDFISH_ADDRESS_SPACE_PCI_DEVICE_ID;
    pci->revision = 0x00;
    pci->class_id = PCI_CLASS_OTHERS;
}

static const TypeInfo goldfish_address_space_pci_info = {
    .name          = GOLDFISH_ADDRESS_SPACE_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(struct goldfish_address_space_state),
    .class_init    = goldfish_address_space_class_init,
};

static void goldfish_address_space_register(void) {
    type_register_static(&goldfish_address_space_pci_info);
}

type_init(goldfish_address_space_register);
