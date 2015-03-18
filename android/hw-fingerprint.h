/* Copyright (C) 2015 The Android Open Source Project
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
#ifndef _android_fingerprint_h
#define _android_fingerprint_h

#include "qemu-common.h"

/* initialize */
extern void  android_hw_fingerprint_init( void );

/* send data to guest system */
void android_hw_fingerprint_send(const char* msg, int msglen);

#endif /* _android_fingerprint_h */
