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

#include "android/utils/looper.h"
#include "android/utils/compiler.h"
#include "android/utils/system.h"

ANDROID_BEGIN_HEADER

/*
 * Before all else, initialize this module:
 *   androidMetrics_moduleInit("avd/home/of/your/holiness");
 * When you're done, finialize this module:
 *   andoirdMetrics_moduleFini();
 *
 * The main object exported by this header is AndroidMetrics. Always initialize
 * and finalize the object so:
 *
 *   AndroidMetrics my_metrics;
 *   androidMetrics_init(&my_metrics);
 *   // Do stuff now... androidMetrics_read / androidMetrics_write etc.
 *   androidMetrics_fini(&my_metrics);
 *
 * There are two parts to reporting metrics from the emulator.
 * [1] Dropping metrics from an emulator process: There are three main steps:
 *
 *   // [A] First write important guest information for the current run:
 *   AndroidMetrics my_metrics;
 *   androidMetrics_init(&my_metrics);
 *   my_metrics.guest_arch = get_my_arch_somehow(); // etc.
 *   androidMetrics_write(&my_metrics);
 *
 *   // [B] Liveness ping: Call AndroidMetrics_keepAlive(...).
 *   This sets up a regular update of system/user time metrics.
 *
 *   // [C] Finally, notify metrics of a clean exit by calling
 *   androidMetrics_seal();
 *   This stops the liveness ping and marks the run to have exited cleanly.
 *   You should no longer reference androidMetrics at all.
 *
 * [2] Reporting metrics from previous runs.
 *   androidMetrics_tryReportAll();
 */

/* These macros are used to define the fields of AndroidMetrics */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION
#define METRICS_INT(n, s, d) int n;
#define METRICS_STRING(n, s, d) char* n;
#define METRICS_DURATION(n, s, d) int64_t n;

typedef struct {
#include "android/metrics/metrics_fields.h"
} AndroidMetrics;

/* Module initialization and finalization functions. */
extern ABool androidMetrics_moduleInit(const char* avdHome);
extern void androidMetrics_moduleFini(void);

/* Set all default values. */
extern void androidMetrics_init(AndroidMetrics* androidMetrics);

/* Frees memory allocated for fields within androidMetrics.
 * This *does not* free the memory allocated for the object itself. If you
 * allocated the object on heap, you should:
 *   AndroidMetrics *my_metrics = android_alloc(sizeof(AndroidMetrics));
 *   ...
 *   androidMetrics_fini(my_metrics);
 *   AFREE(my_metrics);
 */
extern void androidMetrics_fini(AndroidMetrics* androidMetrics);

/* Dumps the given androidMetrics to a file on disk, updating as necessary.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_write(const AndroidMetrics* androidMetrics);

/* Updates some time measurements for the current emulator file.
 * This is used to semi-regularly dump and updated view of the time this
 * emulator process has been running.
 *
 * You should call |androidMetrics_seal| when done to cleanup.
 *
 * Returns: 1 if a regular update was successfullly setup, 0 otherwise.
 */
extern ABool androidMetrics_keepAlive(Looper* metrics_looper);

/* Helper to easily replace string fields */
#define ANDROID_METRICS_STRASSIGN(name, val) \
    do {                                     \
        AFREE(name);                         \
        name = ASTRDUP(val);                 \
    } while (0)

/* This is the last function any emulator process should call on a metrics file
 * to indicate that the process exited cleanly.
 * Calling this before initializing the module trivially succeeds.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_seal(void);

/* Reads the android metrics file for this simulator instance.
 *
 * Note that default values are written to androidMetrics if metricsFile
 * doesn't have the corresponding hardware key.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_read(AndroidMetrics* androidMetrics);

/* Run through all completed metrics drops and upload them to backend.
 *
 * Returns: 1 if successful (no need to report / reported successfully), else 0.
 */
extern ABool androidMetrics_tryReportAll(void);

#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DURATION

ANDROID_END_HEADER
