#include <Cocoa/Cocoa.h>
#import "mac-native-window.h"

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
