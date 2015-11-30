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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/time.h>  // for gettimeofday
#endif

#include "android_twitter.h"

#define E(fmt, ...)                                         \
    do {                                                    \
        fprintf(stderr, "error: " fmt "\n", ##__VA_ARGS__); \
    } while (0)

#ifdef DEBUG_TWITTER
#define D(fmt, ...)                                         \
    do {                                                    \
        fprintf(stdout, "tweet: " fmt "\n", ##__VA_ARGS__); \
    } while (0)
#else
#define D(fmt, ...)    \
    do { /* nothing */ \
    } while (0)
#endif  // DEBUG_TWITTER

/*****************************************************************************/

#define MAX_PATH_LEN 256

static FILE* twitter_stream = NULL;
static unsigned long long event_mono = 0;
static unsigned long event_gid = 0;
#ifdef _WIN32
const char* pathsep = "\\";
static LARGE_INTEGER cpu_freq = {0};
#else
const char* pathsep = "/";
#endif

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

void android_twitter_fini(void) {
    if (twitter_stream == NULL)
        return;

    D("Closing twitter log");
    fclose(twitter_stream);

    return;
}

/*****************************************************************************/
/*
** Initialize timestamped log stream
**/
int android_twitter_init(void) {
    if (twitter_stream != NULL) {
        D("Twitter log already open");
        return 0;
    }

#ifdef _WIN32
    if (QueryPerformanceFrequency(&cpu_freq) == 0) {
        E("Timing failure; could not query CPU frequency");
        return -1;
    }
#endif

    char cwd[MAX_PATH_LEN] = {0};
    if (getcwd(cwd, MAX_PATH_LEN) == NULL) {
        E("Could not get path to current working directory\n");
        return -1;
    }
    std::string path(cwd);

    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);
    char now_str[32] = {0};
    strftime(now_str, 31, "_%Y_%m_%d_%H_%M_%S", now_tm);

    path.append(pathsep);
    path.append("eventlog");
    path.append(now_str);
    path.append(".log");

    const char* log_path = path.c_str();

    twitter_stream = fopen(log_path, "w");
    if (twitter_stream == NULL) {
        E("Failed to open twitter temp log file @ %s", log_path);
        return -1;
    } else {
        atexit(android_twitter_fini);
        D("New twitter log @ %s", log_path);
    }

    return 0;
}

/*****************************************************************************/
/* save a short timestamped log message */
int android_twitter_tweet(const tweet_t* tweet) {
    int ret = 0;
    unsigned long long now_cpu = time_stamp_cpu();
    unsigned long long now_mono = time_stamp_mono();
    if (now_mono - event_mono > 1e9 && tweet->tsc == 0) {
        // only host-inititaed events can mark new qualifying events
        event_gid++;
    }
    event_mono = now_mono;

    // TODO: synchronization
    ret = fprintf(twitter_stream,
                  "T: %-16llu H: %-16llu G: %-16llu Group: %-8ld %s\n",
                  now_mono, now_cpu, tweet->tsc, event_gid, tweet->msg);
    D("T: %-16llu H: %-16llu G: %-16llu Group: %-8ld %s", now_mono, now_cpu,
      tweet->tsc, event_gid, tweet->msg);

    return ret;
}

/*****************************************************************************/
/* timestamp and save a short message */
int android_twitter_sms(const char* format, ...) {
    tweet_t tweet = {.tsc = 0, .msg = {0}};
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(tweet.msg, MAX_TWEET_LEN - 1, format, argptr);
    va_end(argptr);

    return android_twitter_tweet(&tweet);
}
