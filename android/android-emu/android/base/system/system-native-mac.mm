// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Optional.h"
#include "android/base/system/System.h"

#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/Kext/KextManager.h>
#include <IOKit/storage/IOBlockStorageDevice.h>

#import <Foundation/Foundation.h>
#import <Foundation/NSProcessInfo.h>

#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <mach/mach_port.h>

#include <stdlib.h>
#include <sys/stat.h>

namespace android {
namespace base {

// From:
// https://stackoverflow.com/questions/49677034/thread-sleeps-too-long-on-os-x-when-in-background
void disableAppNap_macImpl(void) {
   if ([[NSProcessInfo processInfo]
        respondsToSelector:@selector(beginActivityWithOptions:reason:)]) {
      [[NSProcessInfo processInfo]
       beginActivityWithOptions:0x00FFFFFF
       reason:@"Not sleepy and don't want to nap"];
   }
}

struct mac_cpu_thread_info {
    uint64_t wall_time_us;
    uint64_t user_time_us;
    uint64_t sys_time_us;
};

// based on https://stackoverflow.com/questions/13893134/get-current-pthread-cpu-usage-mac-os-x
float cpuUsageCurrentThread_macImpl(void) {
    thread_local mac_cpu_thread_info prev_info;
    thread_local mac_cpu_thread_info curr_info;
    kern_return_t kr;
    mach_msg_type_number_t count;
    mach_port_t thread;
    thread_basic_info_data_t info;
    float usagePart = 0.0f;

    thread = mach_thread_self();
    curr_info.wall_time_us = System::get()->getUnixTimeUs();
    Duration elapsed = curr_info.wall_time_us - prev_info.wall_time_us;

    count = THREAD_BASIC_INFO_COUNT;
    kr = thread_info(
        thread,
        THREAD_BASIC_INFO,
        (thread_info_t)&info,
        &count);

    if (kr == KERN_SUCCESS && (info.flags & TH_FLAGS_IDLE) == 0) {
        curr_info.user_time_us =
            info.user_time.seconds * 1000000ULL +
            info.user_time.microseconds;
        curr_info.sys_time_us =
            info.system_time.seconds * 1000000ULL +
            info.system_time.microseconds;
    } else {
        curr_info.user_time_us = prev_info.user_time_us;
        curr_info.sys_time_us = prev_info.sys_time_us;
    }

    uint64_t user_time_elapsed =
        curr_info.user_time_us - prev_info.user_time_us;
    uint64_t sys_time_elapsed =
        curr_info.sys_time_us - prev_info.sys_time_us;

    usagePart = (float)(user_time_elapsed + sys_time_elapsed) / (float)elapsed;
    prev_info = curr_info;
    mach_port_deallocate(mach_task_self(), thread);

    return usagePart;
}

Optional<System::DiskKind> nativeDiskKind(int st_dev) {
    const char* devName = devname(st_dev, S_IFBLK);
    if (!devName) {
        return {};
    }
    CFMutableDictionaryRef classesToMatch =
            IOBSDNameMatching(kIOMasterPortDefault, 0, devName);
    if (!classesToMatch) {
        NSLog(@"Could not find io classes of disk");
        return {};
    }

    // get iterator of matching services
    io_iterator_t entryIterator;

    if (KERN_SUCCESS != IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                     classesToMatch,
                                                     &entryIterator)) {
        NSLog(@"Can't iterate services");
        return {};
    }

    // iterate over all found medias
    io_object_t serviceEntry, parentMedia;
    while ((serviceEntry = IOIteratorNext(entryIterator)) != 0) {
        // We assume there won't be more levels of nesting here. The limit is
        // arbitrary and can be increased if we hit it.
        int maxlevels = 8;
        do {
            kern_return_t kernResult = IORegistryEntryGetParentEntry(
                    serviceEntry, kIOServicePlane, &parentMedia);
            IOObjectRelease(serviceEntry);

            if (KERN_SUCCESS != kernResult) {
                serviceEntry = 0;
                NSLog(@"Error while getting parent service entry");
                break;
            }

            serviceEntry = parentMedia;
            if (!parentMedia) {
                break;  // finished iterator
            }

            CFTypeRef res = IORegistryEntryCreateCFProperty(
                    serviceEntry, CFSTR(kIOPropertyDeviceCharacteristicsKey),
                    kCFAllocatorDefault, 0);
            if (res) {
                NSString* type = [(NSDictionary*)res
                        objectForKey:(id)CFSTR(kIOPropertyMediumTypeKey)];
                if ([@kIOPropertyMediumTypeSolidStateKey
                            isEqualToString:type]) {
                    CFRelease(res);
                    return System::DiskKind::Ssd;
                } else if ([@kIOPropertyMediumTypeRotationalKey
                                   isEqualToString:type]) {
                    CFRelease(res);
                    return System::DiskKind::Hdd;
                }
                CFRelease(res);
            }
        } while (maxlevels--);

        if (serviceEntry) {
            IOObjectRelease(serviceEntry);
        }
    }
    IOObjectRelease(entryIterator);

    return {};
}

}  // namespace base
}  // namespace android
