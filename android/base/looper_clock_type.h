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

#pragma once

// Type of the clock used in a Looper object
//  LOOPER_CLOCK_REALTIME - realtime monotinic host clock
//  LOOPER_CLOCK_VIRTUAL - virtual machine guest time, stops when guest is
//      suspended
//  LOOPER_CLOCK_HOST - host time, mimics the current time set in the OS, so
//      is may go back when OS time is set back
//
//  Note: the only time which is always supported is LOOPER_CLOCK_HOST;
//        other types may be not implemented by different times and fall back
//        to LOOPER_CLOCK_HOST

typedef enum {
    LOOPER_CLOCK_REALTIME,
    LOOPER_CLOCK_VIRTUAL,
    LOOPER_CLOCK_HOST
} LooperClockType;
