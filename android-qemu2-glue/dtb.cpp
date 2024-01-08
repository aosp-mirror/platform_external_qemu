// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/dtb.h"
#include <vector>
#include <cstring>
#include "aemu/base/files/ScopedStdioFile.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"

#include "libdtb.h"

namespace {
// These functions acceps char* (instead of const char*) because libdtb
// structures have no const in them.

void initProperty(char* name, char* val, property* prop, property* next) {
    prop->deleted = false;
    prop->name = name;
    prop->val.len = strlen(val) + 1;  // '+1' is required, I used dtc as the reference
    prop->val.val = val;
    prop->next = next;
}

void initNode(char* name, struct property *proplist, node* n, node* children) {
    n->deleted = false;
    n->name = name;
    n->proplist = proplist;
    n->children = children;
    n->next_sibling = nullptr;
    n->basenamelen = strlen(name);  // no '+1' here, I used dtc as the reference
}

}  // namsepace

namespace dtb {

int createDtbFile(const Params& params, const std::string &dtbFilename) {
    struct property android_properties[1];
    struct property fstab_properties[1];
    struct property vendor_properties[5];

    char lit_compatible[] = "compatible";
    char lit_dev[] = "dev";
    char lit_type[] = "type";
    char lit_mnt_flags[] = "mnt_flags";
    char lit_fsmgr_flags[] = "fsmgr_flags";

    char lit_vendor_compatible_value[] = "android,vendor";
    char lit_fstab_compatibe_value[] = "android,fstab";
    char lit_android_compatibe_value[] = "android,firmware";

    char lit_type_value[] = "ext4";
    char lit_mnt_flags_value[] = "noatime,ro,errors=panic";
    char lit_fsmgr_flags_value[] = "wait";

    std::vector<char> vendor_device_location(
        params.vendor_device_location.begin(), params.vendor_device_location.end());
    vendor_device_location.push_back(0);

    initProperty(lit_compatible, lit_android_compatibe_value,
        &android_properties[0], nullptr);

    initProperty(lit_compatible, lit_fstab_compatibe_value,
        &fstab_properties[0], nullptr);

    initProperty(lit_compatible, lit_vendor_compatible_value,
        &vendor_properties[0], &vendor_properties[1]);

    initProperty(lit_dev, vendor_device_location.data(),
        &vendor_properties[1], &vendor_properties[2]);

    initProperty(lit_type, lit_type_value,
        &vendor_properties[2], &vendor_properties[3]);

    initProperty(lit_mnt_flags, lit_mnt_flags_value,
        &vendor_properties[3], &vendor_properties[4]);

    initProperty(lit_fsmgr_flags, lit_fsmgr_flags_value,
        &vendor_properties[4], NULL);

    struct node root;
    struct node firmware;
    struct node android;
    struct node fstab;
    struct node vendor;

    char lit_root[] = "";
    char lit_firmware[] = "firmware";
    char lit_android[] = "android";
    char lit_fstab[] = "fstab";
    char lit_vendor[] = "vendor";

    initNode(lit_root, nullptr, &root, &firmware);
    initNode(lit_firmware, nullptr, &firmware, &android);
    initNode(lit_android, android_properties, &android, &fstab);
    initNode(lit_fstab, fstab_properties, &fstab, &vendor);
    initNode(lit_vendor, vendor_properties, &vendor, nullptr);

    android::base::ScopedStdioFile file(::android_fopen(dtbFilename.c_str(), "wb"));
    if (file.get()) {
        struct dt_info dt;
        dt.dt = &root;
        dt_to_blob(file.get(), &dt, DEFAULT_FDT_VERSION);
        return 0;
    } else {
        derror("Could not open '%s' for writing", dtbFilename);
        return 1;
    }
}

}  // namespace dtb
