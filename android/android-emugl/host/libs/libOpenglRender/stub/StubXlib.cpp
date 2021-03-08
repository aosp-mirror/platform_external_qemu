/* Copyright (C) 2021 The Android Open Source Project
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

#include "StubXlib.h"

#include <stdio.h>

#define STUBXLIB_DEBUG 0

#if STUBXLIB_DEBUG
#define DD(fmt, ...)                                                  \
    fprintf(stderr, "stub-xlib: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define DD(fmt, ...)
#endif

// this is C linkage
extern "C" {

int XCloseDisplay(Display* disp) {
    DD("here");
    return 0;
}

Display* XOpenDisplay(_Xconst char* display_name) {
    (void)display_name;
    DD("here");
    return (Display*)(NULL);
}

Window XCreateWindow(Display* /* display */,
                     Window /* parent */,
                     int /* x */,
                     int /* y */,
                     unsigned int /* width */,
                     unsigned int /* height */,
                     unsigned int /* border_width */,
                     int /* depth */,
                     unsigned int /* class */,
                     Visual* /* visual */,
                     unsigned long /* valuemask */,
                     XSetWindowAttributes* /* attributes */
) {
    DD("here");
    Window wd{};
    return wd;
}

int XDestroyWindow(Display* /* display */, Window /* w */
) {
    DD("here");
    return 0;
}

int XFree(void* /* data */
) {
    DD("here");
    return 0;
}

Status XGetGeometry(Display* /* display */,
                    Drawable /* d */,
                    Window* /* root_return */,
                    int* /* x_return */,
                    int* /* y_return */,
                    unsigned int* /* width_return */,
                    unsigned int* /* height_return */,
                    unsigned int* /* border_width_return */,
                    unsigned int* /* depth_return */
) {
    DD("here");
    Status st{};
    return st;
}

Status XGetWindowAttributes(Display* /* display */,
                            Window /* w */,
                            XWindowAttributes* /* window_attributes_return */
) {
    DD("here");
    return Status{};
}

int XIfEvent(Display* /* display */,
             XEvent* /* event_return */,
             Bool (*)(Display* /* display */,
                      XEvent* /* event */,
                      XPointer /* arg */
                      ) /* predicate */,
             XPointer /* arg */
) {
    DD("here");
    return 0;
}

int XMapWindow(Display* /* display */, Window /* w */
) {
    DD("here");
    return 0;
}

int XMoveResizeWindow(Display* /* display */,
                      Window /* w */,
                      int /* x */,
                      int /* y */,
                      unsigned int /* width */,
                      unsigned int /* height */
) {
    DD("here");
    return 0;
}

XErrorHandler XSetErrorHandler(XErrorHandler /* handler */
) {
    DD("here");
    return XErrorHandler{};
}

int XSetWindowBackground(Display* /* display */,
                         Window /* w */,
                         unsigned long /* background_pixel */
) {
    DD("here");
    return 0;
}

int XSetWindowBackgroundPixmap(Display* /* display */,
                               Window /* w */,
                               Pixmap /* background_pixmap */
) {
    DD("here");
    return 0;
}

int XSync(Display* /* display */, Bool /* discard */
) {
    DD("here");
    return 0;
}
}
