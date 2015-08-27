// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_BASE_TYPES_H
#define ANDROID_BASE_TYPES_H

#include <stdint.h>
#include <limits.h>

/**********************************************************************
 **********************************************************************
 *****
 *****  T I M E   R E P R E S E N T A T I O N
 *****
 **********************************************************************
 **********************************************************************/

/* A Duration represents a duration in milliseconds */
typedef int64_t   Duration;

/* A special Duration value used to mean "infinite" */
#define  DURATION_INFINITE       ((Duration)INT64_MAX)


#endif //ANDROID_BASE_TYPES_H
