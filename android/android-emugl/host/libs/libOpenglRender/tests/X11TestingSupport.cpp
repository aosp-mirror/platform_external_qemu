// Copyright (C) 2021 The Android Open Source Project
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
#include <X11/Xlib.h>
#include <stdio.h>

struct X11State {
    X11State() :
        threadsInitResult(XInitThreads()),
        display(XOpenDisplay(NULL)),
        window(DefaultRootWindow(display)) {
        fprintf(stderr, "%s: Are we testing x11? Just initialized x11 "
                "with threads init status %u display %p window %p\n",
                __func__, threadsInitResult, display, (void*)window);
    }
    Status threadsInitResult;
    Display* display;
    Window window;
};

// static X11State sX11State;
static X11State* x11() {
    static X11State* res = new X11State;
    return res;
}

void* createNativePixmap(int width, int height, int bytesPerPixel) {
    auto x = x11();

    auto res = XCreatePixmap(x->display, x->window, width, height, bytesPerPixel * 8);
    if (!res) {
        fprintf(stderr, "%s: Failed to create pixmap.\n", __func__);
    }

    XFlush(x->display);
    return (void*)res;
}

void freeNativePixmap(void* pixmap) { XFreePixmap(x11()->display, (Pixmap)pixmap); }
