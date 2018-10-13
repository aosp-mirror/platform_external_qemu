/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "qemu/osdep.h"
#include "qemu/timer.h"

/***********************************************************/
/* real time host monotonic timer */

#ifdef _WIN32

int64_t clock_freq;

static void __attribute__((constructor)) init_get_clock(void)
{
    LARGE_INTEGER freq;
    int ret;
    ret = QueryPerformanceFrequency(&freq);
    if (ret == 0) {
        fprintf(stderr, "Could not calibrate ticks\n");
        exit(1);
    }
    clock_freq = freq.QuadPart;
}

#else

int use_rt_clock;

#ifdef __APPLE__

// Inlined version of android/android-emu/android/base/System.cpp
#import <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <string.h>
// Instead of including this private header let's copy its important
// definitions in.
// #include <CoreFoundation/CFPriv.h>
/* System Version file access */
CF_EXPORT CFDictionaryRef _CFCopySystemVersionDictionary(void);
CF_EXPORT CFDictionaryRef _CFCopyServerVersionDictionary(void);
CF_EXPORT const CFStringRef _kCFSystemVersionProductNameKey;
CF_EXPORT const CFStringRef _kCFSystemVersionProductVersionKey;

static bool macos_use_clock_gettime()
{
    // Taken from https://opensource.apple.com/source/DarwinTools/DarwinTools-1/sw_vers.c
    /*
         * Copyright (c) 2005 Finlay Dobbie
         * All rights reserved.
         *
         * Redistribution and use in source and binary forms, with or without
         * modification, are permitted provided that the following conditions
         * are met:
         * 1. Redistributions of source code must retain the above copyright
         *    notice, this list of conditions and the following disclaimer.
         * 2. Redistributions in binary form must reproduce the above copyright
         *    notice, this list of conditions and the following disclaimer in the
         *    documentation and/or other materials provided with the distribution.
         * 3. Neither the name of Finlay Dobbie nor the names of his contributors
         *    may be used to endorse or promote products derived from this software
         *    without specific prior written permission.
         *
         * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
         * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
         * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
         * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
         * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
         * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
         * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
         * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
         * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
         * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
         * POSSIBILITY OF SUCH DAMAGE.
         */

    CFDictionaryRef dict = _CFCopyServerVersionDictionary();
    if (!dict)
    {
        dict = _CFCopySystemVersionDictionary();
    }

    if (!dict)
    {
        return false;
    }

    CFStringRef str =
        CFStringCreateWithFormat(
            NULL, NULL,
            CFSTR("%@ %@"),
            CFDictionaryGetValue(dict, _kCFSystemVersionProductNameKey),
            CFDictionaryGetValue(dict, _kCFSystemVersionProductVersionKey));

    if (!str)
    {
        CFRelease(dict);
        return false;
    }

    int length = CFStringGetLength(str);
    if (!length)
    {
        CFRelease(str);
        CFRelease(dict);
        return false;
    }

    char* version = (char*)malloc(length + 1);
    memset(version, 0x0, length + 1);
    if (!CFStringGetCString(str, version, length + 1, CFStringGetSystemEncoding()))
    {
        CFRelease(str);
        CFRelease(dict);
        free(version);
        return false;
    }

    // clock_gettime is not available on macOS 10.11
    // and below. Check against this blacklist:
    const char* gettime_blacklist[] = {
        // Start at 10.8 because we really do not
        // support anything lower.
        "10.8",
        "10.9",
        "10.10",
        "10.11",
        // 10.12 and over are assumed to have clock_gettime.
    };

    int gettime_blacklist_num_items =
        sizeof(gettime_blacklist) / sizeof(gettime_blacklist[0]);
    
    for (int i = 0; i < gettime_blacklist_num_items; i++)
    {
        if (strstr(version, gettime_blacklist[i]))
        {
            free(version);
            return false;
        }
    }

    free(version);
    return true;
}
#endif

static void __attribute__((constructor)) init_get_clock(void)
{
    use_rt_clock = 0;
#ifdef CLOCK_MONOTONIC
    {
        struct timespec ts;
#ifdef __APPLE__
        // bug: 117641619
        // Query macOS version to see if it is OK
        // to use clock_gettime.
        if (!macos_use_clock_gettime()) {
            return;
        }
#endif
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            use_rt_clock = 1;
        }
    }
#endif
}
#endif
