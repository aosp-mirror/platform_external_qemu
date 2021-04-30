// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/userspace-boot-properties.h"
#include "android/base/misc/StringUtils.h"

#include <map>
#include <gtest/gtest.h>

namespace android {

namespace {
AndroidOptions makeAndroidOptions() {
    AndroidOptions opts;
    memset(&opts, 0, sizeof(opts));
    return opts;
}
} // namespace

using android::base::join;

TEST(getUserspaceBootProperties, Simple) {
    const std::vector<std::string> verifiedBootParameters = {
        "verifiedBootParameters.foo=value"
    };

    AndroidOptions opts = makeAndroidOptions();

    opts.logcat = (char*)"logcat";
    opts.bootchart = (char*)"bootchart";
    opts.selinux = (char*)"selinux";

    const auto params = getUserspaceBootProperties(
        &opts,
        "x86_64",                   // targetArch
        "serialno",                 // serialno
        true,                       // isQemu2
        600,                        // lcd_width
        800,                        // lcd_height
        60,                         // lcd_vsync
        kAndroidGlesEmulationHost,  // glesMode
        123,                        // bootPropOpenglesVersion
        64,                         // vm_heapSize
        29,                         // apiLevel
        "kernelSerialPrefix",       // kernelSerialPrefix
        &verifiedBootParameters,    // verifiedBootParameters
        "gltransport",              // gltransport
        77,                         // gltransport_drawFlushInterval
        "displaySettingsXml");      // displaySettingsXml

    std::map<std::string, std::string> paramsMap;
    for (const auto& kv : params) {
        EXPECT_TRUE(paramsMap.insert(kv).second);
    }

    EXPECT_STREQ(paramsMap["qemu"].c_str(), "1");
    EXPECT_STREQ(paramsMap["androidboot.hardware"].c_str(), "ranchu");
    EXPECT_STREQ(paramsMap["androidboot.serialno"].c_str(), "serialno");
    EXPECT_STREQ(paramsMap["android.checkjni"].c_str(), "1");
    EXPECT_STREQ(paramsMap["qemu.gles"].c_str(), "1");
    EXPECT_STREQ(paramsMap["qemu.settings.system.screen_off_timeout"].c_str(), "2147483647");
    EXPECT_STREQ(paramsMap["qemu.vsync"].c_str(), "60");
    EXPECT_STREQ(paramsMap["qemu.gltransport"].c_str(), "gltransport");
    EXPECT_STREQ(paramsMap["qemu.gltransport.drawFlushInterval"].c_str(), "77");
    EXPECT_STREQ(paramsMap["qemu.opengles.version"].c_str(), "123");
    EXPECT_STREQ(paramsMap["androidboot.logcat"].c_str(), "logcat");
    EXPECT_STREQ(paramsMap["androidboot.bootchart"].c_str(), "bootchart");
    EXPECT_STREQ(paramsMap["androidboot.selinux"].c_str(), "selinux");
    EXPECT_STREQ(paramsMap["qemu.dalvik.vm.heapsize"].c_str(), "64m");
    EXPECT_STREQ(paramsMap["verifiedBootParameters.foo"].c_str(), "value");
    EXPECT_STREQ(paramsMap["qemu.display.settings.xml"].c_str(), "displaySettingsXml");
}

}  // namespace android
