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

#pragma once

#include "android/metrics/metrics_reporter.h"
#include "android/utils/system.h"

ANDROID_BEGIN_HEADER

/* Exposed for unittests only. DO NOT USE IN PROD CODE. */

/* This is a primarily internal function definition, exposed here for testing
 * purposes
 */
const char* androidMetrics_getMetricsFilePath(void);

ABool androidMetrics_readPath(AndroidMetrics* androidMetrics, const char* path);

ABool androidMetrics_tick();

typedef ABool (*androidMetricsUploaderFunction)(const AndroidMetrics*);
void androidMetrics_injectUploader(
        androidMetricsUploaderFunction uploaderFunction);

ANDROID_END_HEADER
