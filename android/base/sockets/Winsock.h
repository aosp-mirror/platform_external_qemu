// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_BASE_SOCKETS_WINSOCKS_H
#define ANDROID_BASE_SOCKETS_WINSOCKS_H

#ifndef _WIN32
#error "Only include this header when building for Windows."
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

// Default FD_SETSIZE is 64 which is not enough for us.
#  define FD_SETSIZE 1024

// Order of inclusion of winsock2.h and windows.h depends on the version
// of Mingw32 being used.
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#undef ERROR  // necessary to compile LOG(ERROR) statements.

#endif  // ANDROID_BASE_SOCKETS_WINSOCKS_H
