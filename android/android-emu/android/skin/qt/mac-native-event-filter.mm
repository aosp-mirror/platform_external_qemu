/*
* Copyright (C) 2019 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "android/skin/qt/mac-native-event-filter.h"

#import <AppKit/AppKit.h>

MacNativeEventFilter::MacNativeEventFilter(EmulatorQtWindow* emulatorQtWindow) : 
	QAbstractNativeEventFilter(), mEmuQtWindow(emulatorQtWindow) {
} 

bool MacNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *)
{
    if (eventType == "mac_generic_NSEvent") {
        NSEvent *event = static_cast<NSEvent *>(message);
        if ([event type] == NSKeyDown) {
            // Handle key event
            NSLog(@"mac_generic_NSEvent keydown registered! %@ length %d \n", [event characters], [[event characters] length]);
            if (mEmuQtWindow != nullptr)
            	mEmuQtWindow->handleNativeKeyEvent([event keyCode], [event modifierFlags], kEventKeyDown);
        }
    }
    return false;
}






