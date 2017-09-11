/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include <Cocoa/Cocoa.h>


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <sys/param.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/Kext/KextManager.h>


#import "android/skin/qt/mac-native-window.h"

void* getNSWindow(void* ns_view) {
    NSView *view = (NSView *)ns_view;
    if (!view) {
        return NULL;
    }
    Class viewClass = [view class];
    Class nsviewClass = [NSView class];
    if ([viewClass isSubclassOfClass:nsviewClass ]) {
        return [view window];
    } else {
        return view;
    }

}

void nsWindowHideWindowButtons(void* ns_window) {
    NSWindow *window = (NSWindow *)ns_window;
    if (!window) {
        return;
    }
    [[window standardWindowButton:NSWindowCloseButton] setHidden:true];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:true];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:true];
    [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
}

int numHeldMouseButtons() {
    return (int) [NSEvent pressedMouseButtons];
}

void nsWindowAdopt(void *ns_parent, void *ns_child) {
    NSWindow *parent = (NSWindow*)ns_parent, *child = (NSWindow*)ns_child;
    [parent addChildWindow:child ordered:NSWindowAbove];
}

BOOL isSolidState(char const *cpath)
{
	FSRef volRef;
	CFMutableDictionaryRef classesToMatch = nil;

    if (noErr == FSPathMakeRef( cpath, &volRef, NULL))
	{
		FSCatalogInfo volCatInfo;
		if (noErr == FSGetCatalogInfo(&volRef, kFSCatInfoVolume, &volCatInfo, NULL, NULL, NULL))
		{
			CFStringRef idStr = NULL;
			if (noErr == FSCopyDiskIDForVolume(volCatInfo.volume, &idStr))
			{
				NSString *str = (NSString*)idStr;
				//NSLog(@"Checking bsd disk %@",str);

				// create matching dictionary
				classesToMatch = IOBSDNameMatching(kIOMasterPortDefault,0,[str UTF8String]);

				if (idStr) CFRelease(idStr);
			}
		}
    }

	if (classesToMatch == NULL) {
		NSLog(@"Could not find io classes of disk");
		return NO;
	}

	// get iterator of matching services
	io_iterator_t entryIterator;

	if (KERN_SUCCESS != IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &entryIterator))
	{
		NSLog(@"Can't iterate services");
		return NO;
	}

	BOOL isSolidState = NO;

	// iterate over all found medias
	io_object_t serviceEntry, parentMedia;
	while (serviceEntry = IOIteratorNext(entryIterator))
	{
		int maxlevels = 8;
		do
		{
			kern_return_t kernResult = IORegistryEntryGetParentEntry(serviceEntry, kIOServicePlane, &parentMedia);
			IOObjectRelease(serviceEntry);

			if (KERN_SUCCESS != kernResult) {
				serviceEntry = 0;
				NSLog(@"Error while getting parent service entry");
				break;
			}

			serviceEntry = parentMedia;
			if (!parentMedia) break; // finished iterator

			CFTypeRef res = IORegistryEntryCreateCFProperty(serviceEntry, CFSTR(kIOPropertyDeviceCharacteristicsKey), kCFAllocatorDefault, 0);
			if (res)
			{
				NSString *type = [(NSDictionary*)res objectForKey:(id)CFSTR(kIOPropertyMediumTypeKey)];
				//NSLog(@"Found disk %@",res);
				isSolidState = [@"Solid State" isEqualToString:type]; type = nil;
				CFRelease(res);
				if (isSolidState) break;
			}
		}
		while(maxlevels--);

		if (serviceEntry) IOObjectRelease(serviceEntry);
	}
	IOObjectRelease(entryIterator);

	return isSolidState;
}
