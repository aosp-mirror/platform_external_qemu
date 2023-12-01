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
#ifndef HW_I386_GOLDFISH_BOARD_H
#define HW_I386_GOLDFISH_BOARD_H

#include "qemu/osdep.h"
#include "qemu/units.h"

#include "hw/boards.h"
#include "hw/i386/pc.h"

typedef struct GoldfishMachineState {
  /*< private >*/
  PCMachineState pc;

  /* <public> */

  /* Ini file containing acpi descriptions */
  char *acpi_ini;

  /* The path of system.img in the guest
   * This is the location where the OS is expecting the physical drive to be.
   * For example: /dev/block/pci/pci0000:00/0000:00:07.0/by-name/vendor
   */
  char *system_device_in_guest;

  /* The path of the vendor.img in the guest (if any),
   * This is the location where the OS is expecting the physical drive to be.
   * For example: /dev/block/pci/pci0000:00/0000:00:07.0/by-name/vendor
   */
  char *vendor_device_in_guest;

} GoldfishMachineState;

#define TYPE_ANDROID_MACHINE "goldfish-machine"
OBJECT_DECLARE_TYPE(GoldfishMachineState, AndroidMachineClass, ANDROID_MACHINE)

#endif