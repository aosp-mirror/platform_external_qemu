// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "qemu/osdep.h"

#include "google/drivers/hw/i386/goldfish_board.h"

#include "google/drivers/hw/acpi/goldfish_acpi.h"
#include "hw/boards.h"
#include "hw/char/parallel-isa.h"
#include "hw/cxl/cxl_host.h"
#include "hw/display/ramfb.h"
#include "hw/hotplug.h"
#include "hw/hyperv/vmbus-bridge.h"
#include "hw/i386/pc.h"
#include "hw/southbridge/piix.h"
#include "qemu/module.h"
#include "qemu/typedefs.h"

static void android_machine_options(MachineClass *m) {
  m->desc = "Android x86_64 (i440fx + PIIX)";
  m->default_machine_opts = "firmware=bios-256k.bin";
  m->default_machine_opts = "firmware=bios-256k.bin,suppress-vmdesc=on";
  m->default_display = "std";
  m->default_nic = "e1000";
  m->no_parallel = !module_object_class_by_name(TYPE_ISA_PARALLEL);
  machine_class_allow_dynamic_sysbus_dev(m, TYPE_RAMFB_DEVICE);
  machine_class_allow_dynamic_sysbus_dev(m, TYPE_VMBUS_BRIDGE);

  PCMachineClass *pcmc = PC_MACHINE_CLASS(m);
  pcmc->default_south_bridge = TYPE_PIIX3_DEVICE;
  pcmc->rsdp_in_ram = false;
  pcmc->resizable_acpi_blob = false;
  pcmc->broken_32bit_mem_addr_check = true;
  pcmc->pci_root_uid = 0;
  pcmc->default_cpu_version = 1;
}

static MachineClass *find_machine(const char *name, GSList *machines) {
  GSList *el;

  for (el = machines; el; el = el->next) {
    MachineClass *mc = el->data;

    if (!strcmp(mc->name, name) || !g_strcmp0(mc->alias, name)) {
      return mc;
    }
  }

  return NULL;
}

static void pc_init_goldfish(MachineState *machine) {
  // The goldfish board is basically a modified pc-i440fx board with a custom
  // acpi. So we are going to find the pc-i440fx machine, and call its
  // initialize function. This will give us a good baseline.
  GSList *machines = object_class_get_list(TYPE_MACHINE, false);
  MachineClass *machine_class;

  machine_class = find_machine("pc", machines);
  machine_class->init(machine);

  // Make sure we override dsdt function, as we want to configure the board
  // properly.
  PCMachineState *pcms = PC_MACHINE(machine);
  pcms->add_goldfish_dsdt = add_goldfish_dsdt;
}

/** Various property setters */

#define goldfish_set_str(member)                                               \
  static void goldfish_set_##member(Object *obj, const char *value,            \
                                    Error **errp) {                            \
    GoldfishMachineState *ams = ANDROID_MACHINE(obj);                          \
    g_free(ams->member);                                                       \
    ams->member = g_strdup(value);                                             \
  }

goldfish_set_str(acpi_ini);
goldfish_set_str(system_device_in_guest);
goldfish_set_str(vendor_device_in_guest);

static void goldfish_machine_std_class_init(ObjectClass *oc, void *data) {
  MachineClass *mc = MACHINE_CLASS(oc);

  // Enable the loading of the acpi ini file.
  object_class_property_add_str(oc, "acpi_ini", NULL, goldfish_set_acpi_ini);
  object_class_property_add_str(oc, "system", NULL,
                                goldfish_set_system_device_in_guest);
  object_class_property_add_str(oc, "vendor", NULL,
                                goldfish_set_vendor_device_in_guest);

  android_machine_options(mc);
  mc->init = pc_init_goldfish;
  mc->kvm_type = pc_machine_kvm_type;
}

static const TypeInfo goldfish_machine_type_std = {
    .name = "goldfish" TYPE_MACHINE_SUFFIX,
    .parent = TYPE_PC_MACHINE,
    .class_init = goldfish_machine_std_class_init,
    .instance_size = sizeof(GoldfishMachineState),
    .interfaces = (InterfaceInfo[]){{TYPE_HOTPLUG_HANDLER}, {}},
};

static void goldfish_machine_init_std(void) {
  type_register(&goldfish_machine_type_std);
}
type_init(goldfish_machine_init_std)