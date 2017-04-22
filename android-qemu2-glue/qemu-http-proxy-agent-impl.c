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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/http_proxy.h"

static const QAndroidHttpProxyAgent sQAndroidHttpProxyAgent = {
        .httpProxySet = android_http_proxy_set};
const QAndroidHttpProxyAgent* const gQAndroidHttpProxyAgent =
        &sQAndroidHttpProxyAgent;

AndroidProxyCB sAndroidProxyCB = {
        .ProxySet = NULL,
        .ProxyUnset = NULL
};
AndroidProxyCB* const gAndroidProxyCB = &sAndroidProxyCB;
