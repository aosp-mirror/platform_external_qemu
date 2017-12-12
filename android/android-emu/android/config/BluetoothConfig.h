// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <stdio.h>
#include <stdint.h>
#include <string>
#include "android/emulation/ParameterList.h"

namespace android {

/**
 * A Simple class that parses out the bluetooth configuration given the device
 * information.
 */
class BluetoothConfig {
 public:

  BluetoothConfig(const char* device) {
     uint16_t vendorid = 0, productid = 0;
     if (device && sscanf(device, "%hx:%hx", &vendorid, &productid) == 2) {
       preqemu_.add2("-boot-property", "qemu.bluetooth=enabled");
       preqemu_.add("-boot-property");
       preqemu_.addFormat("qemu.bluetooth.vendorid=0x%x", vendorid);
       preqemu_.add("-boot-property");
       preqemu_.addFormat("qemu.bluetooth.productid=0x%x", productid);
       postqemu_.add2("-usb", "-device");
       postqemu_.addFormat("usb-host,vendorid=0x%x,productid=0x%x", vendorid, productid);
     }
  }

  ParameterList getParameters() { return preqemu_; }

  ParameterList getQemuParameters() { return postqemu_; }

  private:
      ParameterList postqemu_;
      ParameterList preqemu_;
};
}  // namespace android

