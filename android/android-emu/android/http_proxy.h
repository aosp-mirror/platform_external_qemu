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
#include "proxy/proxy_setup.h"
#include "android/utils/debug.h"

void
android_http_proxy_set( const char*  proxy )
{
    if (proxy == NULL)
        return;

    android_http_proxy_setup(proxy, VERBOSE_CHECK(proxy));
}
