/*
* Copyright (C) 2011 The Android Open Source Project
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
#include "X11ErrorHandler.h"

// static
int X11ErrorHandler::s_lastErrorCode = 0;

// static
android::base::Lock X11ErrorHandler::s_lock;

X11ErrorHandler::X11ErrorHandler(EGLNativeDisplayType dpy):
    m_dpy(dpy) {
    android::base::AutoLock mutex(s_lock);
    XSync(dpy,False);
    s_lastErrorCode = 0;
    m_oldErrorHandler = XSetErrorHandler(errorHandlerProc);
}

X11ErrorHandler::~X11ErrorHandler() {
    android::base::AutoLock mutex(s_lock);
    XSync(m_dpy, False);
    XSetErrorHandler(m_oldErrorHandler);
    s_lastErrorCode = 0;
}

// static
int X11ErrorHandler::errorHandlerProc(EGLNativeDisplayType dpy,
                                      XErrorEvent* event) {
    s_lastErrorCode = event->error_code;
    return 0;
}
