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
**
**
** Description
**
** A virtual device to exchange short, fixed size messages with timestamps
** between the guest and host. This device can be used for event logging
** across the host and guest with (nearly) proper ordering.
**
*/

#include "android_twitter.h"

#include "android/base/synchronization/Lock.h"
#include "android/utils/debug.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/time.h>  // for gettimeofday
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#define  DEBUG  1

#define  E(...)  derror(__VA_ARGS__)
#if DEBUG
#define  D(...)   VERBOSE_PRINT(twitter,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

/*****************************************************************************/

#define MAX_PATH_LEN 256

static FILE* twitter_stream = NULL;
#ifdef _WIN32
static LARGE_INTEGER cpu_freq = {0};
#endif
static android::base::Lock lock;

/******************************************************************************/

// emulator is x86 target only
unsigned long long time_stamp_cpu(void) {
    unsigned long low = 0;
    unsigned long hi = 0;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(hi));
    return ((unsigned long long)low) | (((unsigned long long)hi) << 32);
}

#ifdef __linux__
unsigned long long time_stamp_mono(void)  // in usec
{
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    unsigned long long t = (unsigned long long)tm.tv_nsec / 1000;
    t += (unsigned long long)tm.tv_sec * 1e6;
    return t;
}
#elif defined(_WIN32)
unsigned long long time_stamp_mono(void)  // in usec
{
    LARGE_INTEGER pfc = {0};

    QueryPerformanceCounter(&pfc);

    unsigned long long t = pfc.QuadPart * 1000000;
    return t / cpu_freq.QuadPart;
}
#elif defined(__APPLE__)
unsigned long long time_stamp_mono(void)  // in usec
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1e6 + t.tv_usec;
}
#endif  // _WIN32

/*****************************************************************************/
/* Close timestamped log stream */
void android_twitter_fini(void) {
    if (twitter_stream == NULL) {
        return;
    }

    D("Closing and saving twitter log");
    fclose(twitter_stream);

    return;
}

/*****************************************************************************/
/* Check if timestamped log stream has been opened */
int android_twitter_is_init() {
    if (twitter_stream != NULL) {
        D("Twitter file stream has been initialized");
        return 1;
    }

    D("Twitter file stream has not been initialized");
    return 0;
}

/*****************************************************************************/
/* Open timestamped log stream */
int android_twitter_init(const char *log_path) {
    if (twitter_stream != NULL) {
        D("Twitter log already open; can't reopen at %s", log_path);
        return 0;
    }

#ifdef _WIN32
    if (QueryPerformanceFrequency(&cpu_freq) == 0) {
        E("Timing failure; could not query CPU frequency");
        return -1;
    }
#endif

    twitter_stream = fopen(log_path, "w");
    if (twitter_stream == NULL) {
        E("Failed to open twitter log file @ %s", log_path);
        return -1;
    } else {
        atexit(android_twitter_fini);
        D("New twitter log @ %s", log_path);
    }

    D("Twitter log opened @ %s", log_path);
    // TODO: mmap file to lower writes overhead

    return 0;
}

/*****************************************************************************/
/* Timestamp and save a short log message */
int android_twitter_tweet(const tweet_t* tweet) {
    // Note that this synchronization point might add non-trivial overhead
    // if twitter use is contended. These concerns have not been realized
    // in the end-to-end (input to rendering) experiments conducted with this
    // device, so simplicity wins.
    android::base::AutoLock _lock(lock);

    unsigned long long now_cpu = time_stamp_cpu();
    unsigned long long now_mono = time_stamp_mono();
    int ret = fprintf(twitter_stream,
                  "T: %-16llu H: %-16llu G: %-16llu %s\n",
                  now_mono, now_cpu, tweet->tsc, tweet->msg);

    D("T: %-16llu H: %-16llu G: %-16llu %s", now_mono, now_cpu,
      tweet->tsc, tweet->msg);

    return ret;
}

/*****************************************************************************/
/* timestamp and save a short message */
int android_twitter_msg(const char* format, ...) {
    tweet_t tweet = {.tsc = 0, .msg = {0}};
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(tweet.msg, MAX_TWEET_LEN - 1, format, argptr);
    va_end(argptr);

    return android_twitter_tweet(&tweet);
}
