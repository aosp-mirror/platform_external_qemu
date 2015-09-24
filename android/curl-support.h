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

#include "android/utils/compiler.h"

#include <curl/curl.h>

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// CURL library initialization/cleanup helpers
// they make sure it is initialized properly, and only once
// NOTE: not thread-safe, run only on a main thread!

// Args: |ca_info| is path to the SSL Certificate Authority bundle. May be NULL.
extern bool curl_init(const char* ca_info);
extern void curl_cleanup();

// Use this function instead of curl_easy_init to initialize a new |CURL|
// object. This calls libcurl's curl_easy_init, and then sets some default
// library-wide options on it.
// Returns NULL on failure.
extern CURL* curl_easy_default_init();

ANDROID_END_HEADER
