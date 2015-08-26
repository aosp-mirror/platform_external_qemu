/* Copyright (C) 20015 The Android Open Source Project
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

#ifndef _ANDROID_METRICS_REPOTER_H
#define _ANDROID_METRICS_REPOTER_H

#include "android/utils/compiler.h"
#include "android/utils/system.h"

ANDROID_BEGIN_HEADER

/* These macros are used to define the fields of AndroidMetrics */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DOUBLE
#define METRICS_INT(n, s, d)        int n;
#define METRICS_STRING(n, s, d)     char* n;
#define METRICS_DOUBLE(n, s, d)     double n;

typedef struct {
#include "android/utils/metrics/metrics_fields.h"
} AndroidMetrics;

/* Set all default values. */
extern void androidMetrics_init( AndroidMetrics* androidMetrics );

/* Frees memory allocated for fields within androidMetrics.
 * This *does not* free the memory allocated for the object itself. If you
 * allocated the object on heap, you should:
 *   AndroidMetrics *my_metrics = android_alloc(sizeof(AndroidMetrics));
 *   ...
 *   androidMetrics_free(my_metrics);
 *   AFREE(my_metrics);
 */
extern void androidMetrics_free( AndroidMetrics* androidMetrics );

/* Dumps the given androidMetrics to metricsFile, updating as necessary.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_write( const AndroidMetrics* androidMetrics );

/* Updates some time measurements for the current emulator file.
 * This is used to semi-regularly dump and updated view of the time this
 * emulator process has been running.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_tick();

/* Helper to easily replace string fields */
#define ANDROID_METRICS_STRASSIGN(name, val) \
        do { AFREE(name); name = ASTRDUP(val); } while(0)

/* This is the last function any emulator process should call on a metrics file
 * to indicate that the process exited cleanly.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_seal();

/* Reads the android metrics file for this simulator instance.
 *
 * Note that default values are written to androidMetrics if metricsFile
 * doesn't have the corresponding hardware key.
 *
 * Returns: 1 if successful, 0 otherwise.
 */
extern ABool androidMetrics_read( AndroidMetrics* androidMetrics );

/* Run through all completed metrics drops and upload them to backend.
 *
 * Returns: 1 if successful (no need to report / reported successfully), else 0.
 */
extern ABool androidMetrics_tryReportAll();


/* Exposed for unittests only. DO NOT USE IN PROD CODE. */

/* This is a primarily internal function definition, exposed here for testing
 * purposes
 */
typedef ABool (*androidMetricsUploaderFunction) ( const AndroidMetrics* );
const char* androidMetrics_getMetricsFilePath();
ABool androidMetrics_readPath( AndroidMetrics* androidMetrics,
                               const char* path );
void androidMetrics_injectUploader(
        androidMetricsUploaderFunction uploaderFunction );

ANDROID_END_HEADER

#endif  /* _ANDROID_METRICS_REPOTER_H */
