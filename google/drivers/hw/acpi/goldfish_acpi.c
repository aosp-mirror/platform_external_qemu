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

#include "google//drivers/hw/acpi/goldfish_acpi.h"
#include "google/drivers/hw/acpi/goldfish_defs.h"
#include "google/drivers/hw/i386/goldfish_board.h"
#include "hw/acpi/aml-build.h"
#include "hw/boards.h"
#include "qemu/typedefs.h"

/**
 * Appends a new string property to an AML package.
 *
 * @param package The AML package to which the property will be added.
 * @param key The name of the property (as an AML string).
 * @param value The value of the property (as an AML string).
 */
static Aml *append_string_property(Aml *package, const char *key,
                                   const char *value) {
  Aml *property = aml_package(2);
  aml_append(property, aml_string("%s", key));
  aml_append(property, aml_string("%s", value));

  aml_append(package, property);
  return property;
}

/**
 * Loads ACPI properties from a specified INI file.
 *
 * @param ini_file The path to the INI file containing ACPI properties.
 *
 * @returns An AML package containing the loaded properties or an empty package
 *          if any error occurs. The caller is responsible for freeing the
 *          returned package.
 *
 * This function parses the specified INI file for keys under the "ACPI" group.
 * Each key-value pair is converted into an AML string property and added to a
 * new AML package. If any error occurs during parsing or memory allocation,
 * an empty AML package is returned.
 *
 * The caller is responsible for freeing the returned AML package using
 * `aml_free`.
 */
static Aml *load_acpi_properties_from_file(const char *ini_file) {
  GKeyFile *key_file = g_key_file_new();
  GError *error = NULL;

  if (!g_key_file_load_from_file(key_file, ini_file, G_KEY_FILE_NONE, &error)) {
    g_key_file_free(key_file);
    if (error != NULL) {
      g_error_free(error);
    }
    return aml_package(0);
  }

  // Get all keys in the group
  gsize keys_count;
  gchar **keys = g_key_file_get_keys(key_file, "ACPI", &keys_count, NULL);

  Aml *properties = aml_package(keys_count);
  // Iterate over each key
  for (gsize j = 0; j < keys_count; j++) {
    const gchar *key = keys[j];
    gchar *value = g_key_file_get_value(key_file, "ACPI", key, NULL);
    append_string_property(properties, key, value);
    g_free(value);
  }

  g_strfreev(keys);
  g_key_file_free(key_file);

  return properties;
}

/**
 * Creates an AML package representing default firmware properties.
 *
 * @param system_device_in_guest Path to the system device in the guest.
 * @param vendor_device_in_guest Path to the vendor device in the guest.
 *
 * @returns An AML package containing the firmware properties. The caller
 *          is responsible for freeing the returned package using `aml_free`.
 *
 * This function builds an AML package with properties describing the default
 * firmware, including compatible IDs and fstab entries. The number of
 * properties depends on whether system and vendor devices are specified.
 */
static Aml *add_default_firmware(const char *system_device_in_guest,
                                 const char *vendor_device_in_guest) {
  int nproperties = 2;
  if (system_device_in_guest) {
    nproperties += 5;
  }
  if (vendor_device_in_guest) {
    nproperties += 5;
  }
  Aml *properties = aml_package(nproperties);
  /* ACPI _DSD does not support nested properties (at least not in a
   * straightforward manner), so we have to use a flat layout.
   */
  append_string_property(properties, "android.compatible", "android,firmware");
  append_string_property(properties, "android.fstab.compatible",
                         "android,fstab");

  if (system_device_in_guest) {
    append_string_property(properties, "android.fstab.system.compatible",
                           "android,system");
    append_string_property(properties, "android.fstab.system.dev",
                           system_device_in_guest);
    append_string_property(properties, "android.fstab.system.type", "ext4");
    append_string_property(properties, "android.fstab.system.mnt_flags", "ro");
    append_string_property(properties, "android.fstab.system.fsmgr_flags",
                           "wait");
  }

  if (vendor_device_in_guest) {
    append_string_property(properties, "android.fstab.vendor.compatible",
                           "android,vendor");
    append_string_property(properties, "android.fstab.vendor.dev",
                           vendor_device_in_guest);
    append_string_property(properties, "android.fstab.vendor.type", "ext4");
    append_string_property(properties, "android.fstab.vendor.mnt_flags", "ro");
    append_string_property(properties, "android.fstab.vendor.fsmgr_flags",
                           "wait");
  }

  return properties;
}

/**
 * Builds an AML device representing an Android device tree node.
 *
 * @param ams Goldfish machine state containing device information.
 * @param scope The AML scope to append the device to.
 * @param dev_name The name of the device.
 * @param hid_name The ACPI HID identifier.
 * @param str_name The ACPI string identifier (optional).
 *
 * This function creates an AML device for Android DT emulation. It
 * first loads properties either from the provided ACPI INI file or
 * uses defaults if no file is specified.
 */
static void build_android_dt_aml(GoldfishMachineState *ams, Aml *scope,
                                 const char *dev_name, const char *hid_name,
                                 const char *str_name) {
  Aml *dev = aml_device("%s", dev_name);
  Aml *properties, *dsd;

  if (ams->acpi_ini) {
    properties = load_acpi_properties_from_file(ams->acpi_ini);
  } else {
    // No acpi ini file was provided, use the defaults
    properties = add_default_firmware(ams->system_device_in_guest,
                                      ams->vendor_device_in_guest);
  }

  aml_append(dev, aml_name_decl("_HID", aml_string("%s", hid_name)));
  aml_append(dev, aml_name_decl("_STR", aml_unicode(str_name)));

  dsd = aml_package(2);
  /* Device Properties UUID. Cf. https://lwn.net/Articles/612062/ */
  aml_append(dsd, aml_touuid("DAFFD814-6EBA-4D8C-8A91-BC9BBF4AA301"));
  aml_append(dsd, properties);

  aml_append(dev, aml_name_decl("_DSD", dsd));
  aml_append(scope, dev);
}

/**
 * Adds the Goldfish Device State Table (DSDT) to the AML table.
 *
 * @param machine The machine state.
 * @param table The AML table to which the DSDT will be added.
 *
 * This function builds and adds the Goldfish DSDT to the provided AML
 * table. It creates individual AML device objects for various Goldfish
 * devices like battery, events, pipe, framebuffer, audio, sync, RTC,
 * and rotary, configuring them with their respective I/O memory bases,
 * sizes, and interrupt numbers. If an ACPI INI file is provided,
 * device properties are loaded from it. Otherwise, defaults are used.
 */
void add_goldfish_dsdt(MachineState *machine, Aml *table) {
  GoldfishMachineState *ams = ANDROID_MACHINE(machine);
  Aml *scope = aml_scope("_SB");

  // TODO(jansene): Not used for fuchsia
  build_android_dt_aml(ams, scope, "ANDT", "ANDR0001", "android device tree");
  aml_append(table, scope);
}
