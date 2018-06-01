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

#pragma once

#include <string>

namespace dtb {

struct Params {
    std::string vendor_device_location;  // location of the vendor partition.
};

/*
The function creates a device tree blob file containing the following nodes:

node(name='' basenamelen=0)
  node(name='firmware' basenamelen=8)
    node(name='android' basenamelen=7)
        property(name='compatible' val=(len=17, val='android,firmware'))
      node(name='fstab' basenamelen=5)
          property(name='compatible' val=(len=14, val='android,fstab'))
        node(name='vendor' basenamelen=6)
            property(name='compatible' val=(len=15, val='android,vendor'))
            property(name='dev' val=(len=54, val=params.vendor_device_location))
            property(name='type' val=(len=5, val='ext4'))
            property(name='mnt_flags' val=(len=24, val='noatime,ro,errors=panic'))
            property(name='fsmgr_flags' val=(len=5, val='wait'))

Android Emulator uses this file to find the vendor partition.

The function returns 0 on success or an error code otherwise.
*/

int createDtbFile(const Params& params, const std::string& dtbFilename);

}  // namespace dtb
