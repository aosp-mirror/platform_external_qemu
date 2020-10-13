#include "StubXlib.h"

// this is C linkage
extern "C" {

int XCloseDisplay(Display* disp) {
    return 0;
}

Display* XOpenDisplay(_Xconst char* display_name) {
    (void)display_name;
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
    Window wd{};
    return wd;
}

int XDestroyWindow(Display* /* display */, Window /* w */
) {
    return 0;
}

int XFree(void* /* data */
) {
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
    Status st{};
    return st;
}

Status XGetWindowAttributes(Display* /* display */,
                            Window /* w */,
                            XWindowAttributes* /* window_attributes_return */
) {
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
    return 0;
}

int XMapWindow(Display* /* display */, Window /* w */
) {
    return 0;
}

int XMoveResizeWindow(Display* /* display */,
                      Window /* w */,
                      int /* x */,
                      int /* y */,
                      unsigned int /* width */,
                      unsigned int /* height */
) {
    return 0;
}

XErrorHandler XSetErrorHandler(XErrorHandler /* handler */
) {
    return XErrorHandler{};
}

int XSetWindowBackground(Display* /* display */,
                         Window /* w */,
                         unsigned long /* background_pixel */
) {
    return 0;
}

int XSetWindowBackgroundPixmap(Display* /* display */,
                               Window /* w */,
                               Pixmap /* background_pixmap */
) {
    return 0;
}

int XSync(Display* /* display */, Bool /* discard */
) {
    return 0;
}
}
