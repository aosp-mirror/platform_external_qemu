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
#include "android/utils/debug.h"
#include "android/utils/stralloc.h"
#include "android/proxy/proxy_common.h"
#include "android/proxy/proxy_setup.h"
#include "android/emulation/control/http_proxy_agent.h"
#include <stdio.h>
#include <string.h>
#define  D(...)  VERBOSE_PRINT(proxy,__VA_ARGS__)

extern AndroidProxyCB *const gAndroidProxyCB;
extern void proxy_manager_atexit(void);
STRALLOC_DEFINE(UIProxy);

void
android_http_proxy_set( const char*  proxy )
{
    D("android_http_proxy_set %s proxy \n",
           proxy? "add":"remove");
    // remove proxy
    if (proxy == NULL) {
        if (stralloc_cstr(UIProxy)) {
            stralloc_reset(UIProxy);
        }
        if (gAndroidProxyCB->ProxyUnset) {
            gAndroidProxyCB->ProxyUnset();
        }
        proxy_manager_atexit();
        return;
    }
    // set proxy
    if (stralloc_cstr(UIProxy)) {
        stralloc_reset(UIProxy);
        if (gAndroidProxyCB->ProxyUnset) {
            gAndroidProxyCB->ProxyUnset();
        }
        proxy_manager_atexit();
    }
    stralloc_add_str(UIProxy, proxy);
    if(gAndroidProxyCB->ProxySet) {
        gAndroidProxyCB->ProxySet(stralloc_cstr(UIProxy));
    }
    android_http_proxy_setup(stralloc_cstr(UIProxy), VERBOSE_CHECK(proxy));
}

bool
android_http_proxy_check(const char* proxy, const int port)
{
    if (proxy_check_connection(proxy, strlen(proxy), port, 1000) < 0) {
        D("Could not connect to proxy at %s:%d: %s !", proxy,
            port, strerror(errno));
        return false;
    }
    return true;
}
