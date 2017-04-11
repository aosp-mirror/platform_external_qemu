/* Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef struct QAndroidHttpProxyAgent {
    // Say not to use a proxy
    void (*httpProxyNone)();

    // Provide HTTP Proxy information to the AVD
    // The 'proxy' input string is of the form
    // username:password@host:port. Example:
    // "admin:passw0rd@proxx.mycorp.com:80"
    // CAUTION: The 'proxy' character string is volatile
    //          and must be copied if it is needed after
    //          httpProxySet() returns.
    void (*httpProxySet)(const char* proxy);
} QAndroidHttpProxyAgent;

ANDROID_END_HEADER
