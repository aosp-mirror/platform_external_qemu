/* Copyright (C) 2007-2008 The Android Open Source Project
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
#ifndef ANDROID_PROXY_PROXY_HTTP_H
#define ANDROID_PROXY_PROXY_HTTP_H

#include "android/proxy/proxy_common.h"

extern int
proxy_http_setup( const char*         servername,
                  int                 servernamelen,
                  int                 serverport,
                  int                 num_options,
                  const ProxyOption*  options );

#endif  // ANDROID_PROXY_PROXY_HTTP_H
