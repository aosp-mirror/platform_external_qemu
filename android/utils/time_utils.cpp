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
#include "android/utils/timezone.h"

#include "android/base/system/System.h"
#include "android/utils/bufprint.h"

using android::base::String;
using android::base::System;

char*
bufprint_user_time( char* buffer, char* end )
{
    String user_time_s = System::getUserTime().StringifyToSeconds();
    return bufprint(buffer, end, "%s seconds", user_time_s.c_str());
}

char*
bufprint_kernel_time( char* buffer, char* end )
{
    String kernel_time_s = System::getKernelTime().StringifyToSeconds();
    return bufprint(buffer, end, "%s seconds", kernel_time_s.c_str());
}
