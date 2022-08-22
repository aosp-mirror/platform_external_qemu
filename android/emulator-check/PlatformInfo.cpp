// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulator-check/PlatformInfo.h"

#ifdef __linux__
#include "android/base/system/System.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>

#include <ctype.h>
#endif

namespace android {

#ifdef __linux__

namespace {
struct XDeleter {
    void operator()(Display* d) const { XCloseDisplay(d); }
    void operator()(void* ptr) const { XFree(ptr); }
};
}

template <class T>
using XPtr = std::unique_ptr<T, XDeleter>;

// The following code is a slightly modified wmctrl code taken from
// https://github.com/geekless/wmctrl - GNU GPL 2 license

static std::vector<char> getXProperty(Display* disp,
                                      Window win,
                                      Atom xa_prop_type,
                                      const char* prop_name) {
    Atom xa_prop_name = XInternAtom(disp, prop_name, False);

    /* 1024 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     *
     * NOTE:  see
     * http://mail.gnome.org/archives/wm-spec-list/2003-March/msg00067.html
     * In particular:
     *
     * 	When the X window system was ported to 64-bit architectures, a
     * rather peculiar design decision was made. 32-bit quantities such
     * as Window IDs, atoms, etc, were kept as longs in the client side
     * APIs, even when long was changed to 64 bits.
     *
     */

    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned char* ret_prop;
    if (XGetWindowProperty(disp, win, xa_prop_name, 0, 1024, False,
                           xa_prop_type, &xa_ret_type, &ret_format, &ret_nitems,
                           &ret_bytes_after, &ret_prop) != Success) {
        // Cannot get the property.
        return {};
    }

    // Create a RAII object to make sure |ret_prop| doesn't leak.
    const XPtr<unsigned char> retPropPtr(ret_prop);

    if (xa_ret_type != xa_prop_type) {
        return {};
    }

    unsigned long tmp_size = (ret_format / 8) * ret_nitems;
    /* Correct 64 Architecture implementation of 32 bit data */
    if (ret_format == 32) {
        tmp_size *= sizeof(long) / 4;
    }
    return std::vector<char>(ret_prop, ret_prop + tmp_size);
}

static std::string getLinuxWindowManagerName() {
    const XPtr<Display> disp(XOpenDisplay(nullptr));
    if (!disp) {
        return {};
    }

    std::vector<char> supWindowPtrAsChars =
            getXProperty(disp.get(), DefaultRootWindow(disp.get()),
                         XA_WINDOW, "_NET_SUPPORTING_WM_CHECK");
    if (supWindowPtrAsChars.empty()) {
        supWindowPtrAsChars =
                getXProperty(disp.get(), DefaultRootWindow(disp.get()),
                             XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK");
        if (supWindowPtrAsChars.empty()) {
            // Cannot get window manager info properties.
            return {};
        }
    }

    if (supWindowPtrAsChars.size() < sizeof(Window)) {
        // Bad size returned, has to be at least of a Window type size
        return {};
    }

    const auto supWindow =
            reinterpret_cast<const Window*>(supWindowPtrAsChars.data());

    std::vector<char> wmName = getXProperty(
            disp.get(), *supWindow,
            XInternAtom(disp.get(), "UTF8_STRING", False), "_NET_WM_NAME");
    if (wmName.empty()) {
        wmName =
                getXProperty(disp.get(), *supWindow, XA_STRING, "_NET_WM_NAME");
        if (wmName.empty()) {
            // Cannot get name of the window manager
            return {};
        }
    }

    return std::string(wmName.begin(), wmName.end());
}

static std::string getLinuxDesktopEnvironmentName() {
    using android::base::System;

    auto xdg = System::get()->envGet("XDG_CURRENT_DESKTOP");
    std::transform(xdg.begin(), xdg.end(), xdg.begin(), tolower);
    if (xdg.empty()) {
        // try another one...
        const auto gdm = System::get()->envGet("GDMSESSION");
        if (gdm.find("kde") != std::string::npos) {
            return "kde";
        } else if (gdm.empty()) {
            return "mate";
        }
    }
    if (xdg == "x-cinnamon") {
        return "cinnamon";
    }
    // the rest of them are pretty much the same as their name
    return xdg;
}
#endif

std::string getWindowManagerName() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "Mac";
#elif defined(__linux__)
    return getLinuxWindowManagerName();
#else
    #error Unsupported platform
#endif
}


std::string getDesktopEnvironmentName() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "Mac";
#elif defined(__linux__)
    return getLinuxDesktopEnvironmentName();
#else
    #error Unsupported platform
#endif
}

}  // namespace android
