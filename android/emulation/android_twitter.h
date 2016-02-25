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
** Interface to file stream of timestamped messages for use by the
** host code. Using the twitter device we can collect (nearly)
** ordered timestamps across host and guest events, to study
** latencies, corellations, etc.
**
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// #define DEBUG_TWITTER 1
#define MAX_TWEET_LEN 256

struct _tweet_t_ {
    unsigned long long tsc;  // guest's timestamp
    char msg[MAX_TWEET_LEN];
};
typedef struct _tweet_t_ tweet_t;

/*****************************************************************************/

int android_twitter_is_init();
int android_twitter_init(const char *log_path);
int android_twitter_tweet(const tweet_t* tweet);
int android_twitter_msg(const char* format, ...);
void android_twitter_fini(void);

#ifdef __cplusplus
}
#endif
