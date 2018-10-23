#include "qemu/osdep.h"
//#include "qemu-common.h"
#include "hw/hw.h"
#include "hw/pci/pci.h"

struct goldfish_address_space_state {
};

#define TYPE_GOLDFISH_ADDRESS_SPACE "goldfish_address_space"
#define GOLDFISH_ADDRESS_SPACE(obj) \
    OBJECT_CHECK(goldfish_address_space_state, (obj), TYPE_GOLDFISH_ADDRESS_SPACE)

static void goldfish_address_space_class_init(ObjectClass *klass, void *data) {
}

static const TypeInfo goldfish_address_space_info = {
    .name          = TYPE_GOLDFISH_ADDRESS_SPACE,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(struct goldfish_address_space_state),
    .class_init    = goldfish_address_space_class_init,
};

static void goldfish_address_space_register(void) {
    type_register_static(&goldfish_address_space_info);
}

type_init(goldfish_address_space_register);
