/* Copyright (C) 2020 The Android Open Source Project
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
#ifndef _HW_ANDROID_PIPE_H
#define _HW_ANDROID_PIPE_H

// vtbl in Android pipe.
typedef struct AndroidPipeHwFuncs {
    void (*closeFromHost)(void* hwpipe);
    void (*signalWake)(void* hwpipe, unsigned flags);
    int (*getPipeId)(void* hwpipe);
} AndroidPipeHwFuncs;

#endif /* _HW_ANDROID_PIPE_H */
