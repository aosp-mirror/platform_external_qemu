// Copyright 2017 The Android Open Source Project
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

#include "android/opengl/NativeGpuInfo.h"

#include "android/opengl/macTouchOpenGL.h"

#include <CoreGraphics/CGDirectDisplay.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOKitLib.h>

#include <string>
#include <vector>

#include <stdio.h>

typedef std::pair<uint32_t, uint32_t> GpuVendorDeviceId;
typedef std::vector<GpuVendorDeviceId> GpuVendorDeviceIdList;

// Based on code from:
// https://chromium.googlesource.com/chromium/src/gpu/+/e626016b34c63b7ff51bf9a6c20b37bcc18150c4/config/gpu_info_collector_mac.mm
// https://github.com/glfw/glfw/blob/e0a6772e5e4c672179fc69a90bcda3369792ed1f/src/cocoa_monitor.m

// GetEntryProperty():
// Return 0 if we couldn't find the property.
// The property values we use should not be 0, so it's OK to use 0 as failure.
uint32_t GetEntryProperty(io_registry_entry_t entry, CFStringRef property_name) {
    CFDataRef ref(
        static_cast<CFDataRef>(IORegistryEntrySearchCFProperty(
            entry,
            kIOServicePlane,
            property_name,
            kCFAllocatorDefault,
            kIORegistryIterateRecursively | kIORegistryIterateParents)));

    if (!ref)
        return 0;

    uint32_t value = 0;
    const uint32_t* value_pointer =
        reinterpret_cast<const uint32_t*>(CFDataGetBytePtr(ref));

    if (value_pointer != NULL)
        value = *value_pointer;

    CFRelease(ref);
    return value;
}

static auto GetGPUInfoMac() {

    GpuVendorDeviceIdList res;

    io_iterator_t iter;
    io_service_t serv = 0;

    CFMutableDictionaryRef matching = IOServiceMatching("IODisplayConnect");
    kern_return_t err = IOServiceGetMatchingServices(kIOMasterPortDefault,
                             matching,
                             &iter);

    if (err) return res;

    while ((serv = IOIteratorNext(iter)) != 0) {
         uint32_t vendor_id = GetEntryProperty(serv, CFSTR("vendor-id"));
         if (vendor_id) {
             uint32_t device_id = GetEntryProperty(serv, CFSTR("device-id"));
             if (device_id) {
                res.push_back(std::make_pair(vendor_id, device_id));
             }
         }
     }

    return res;
}

void getGpuInfoListNative(GpuInfoList* out) {
    // This call initializes a pixel format,
    // which should update the IOKit stuff to the
    // correct GPU that will actually be used
    // while the emulator is running.
    macTouchOpenGL();

    auto gpulist = GetGPUInfoMac();

    char vendoridbuf[64] = {};
    char deviceidbuf[64] = {};

    for (const auto& elt : gpulist) {
        snprintf(vendoridbuf, sizeof(vendoridbuf), "%04x", elt.first);
        snprintf(deviceidbuf, sizeof(deviceidbuf), "%04x", elt.second);
        out->infos.emplace_back(
                std::string(vendoridbuf), // make -> vendorid
                std::string(deviceidbuf), // model -> deviceid
                std::string(deviceidbuf), // device_id -> deviceid
                "", "", "" // revision, version, renderer blank
                );
    }
}
