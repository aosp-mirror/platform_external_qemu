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

#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/ini.h"
#include "android/utils/metrics/metrics_reporter.h"
#include "android/utils/path.h"
#include "android/utils/system.h"
#ifdef _WIN32
# include <process.h>
#else
# include <unistd.h>
#endif

# define mwarning(fmt, ...) \
        dwarning("%s:%s: " fmt, __FILE__, __func__, ##__VA_ARGS__)

/* The global variable containing the location to the metrics file in use in the
 * current emulator instance
 */
static char* metricsFilePath;

static const char metricsRelativeDir[] = "metrics";
static const char metricsFilePrefix[] = "metrics";

/* Generate the path to the metrics file to use in the emulator instance.
 * - Ensures that the path is sane, unused so far.
 * - Locks the file path so that no other emulator instance can claim it. This
 *   lock should be held as long as we intend to update the metrics file.
 * - Stashes away the path to a static global var, so that subsequent calls are
 *   guaranteed to return the same value.
 *
 *   Returns NULL on failure to initialize the path.
 */
static const char*
androidMetrics_getMetricsFilePath()
{
    char path[PATH_MAX], *pathend=path, *bufend=pathend+sizeof(path);

    if (metricsFilePath != NULL)
    {
        return metricsFilePath;
    }

    pathend = bufprint_avd_home_path(path, bufend);
    if (pathend >= bufend || !path_is_dir(path))
    {
        mwarning("Failed to get a valid avd home directory.");
        return NULL;
    }

    pathend = bufprint(pathend, bufend, PATH_SEP "%s", metricsRelativeDir);
    if (pathend >= bufend || path_mkdir_if_needed(path, 0744) != 0)
    {
        mwarning("Failed to create a valid metrics home directory.");
        return NULL;
    }

    /* TODO(pprabhu) Deal with pid collisions. */
    pathend = bufprint(pathend, bufend, PATH_SEP "%s", metricsFilePrefix);
    pathend = bufprint(pathend, bufend, ".%d", getpid());
    if (pathend >= bufend || path_exists(path))
    {
        mwarning("Failed to get a writable, unused path for metrics. Tried: %s",
                 path);
        return NULL;
    }
    /* We ignore the returned FileLock, it will be released when the process
     * exits.
     */
    if (filelock_create(path) == 0)
    {
        mwarning("Failed to lock file at %s. "
                 "This indicates metric file name collission.",
                 path);
        return NULL;
    }

    /* This string will be freed by androidMetrics_seal. */
    metricsFilePath = ASTRDUP(path);
    return metricsFilePath;
}

void
androidMetrics_init( AndroidMetrics *androidMetrics )
{
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DOUBLE
#define METRICS_INT(n, s, d)        androidMetrics->n = d;
#define METRICS_STRING(n, s, d)     androidMetrics->n = ASTRDUP(d);
#define METRICS_DOUBLE(n, s, d)     androidMetrics->n = d;

#include "android/utils/metrics/metrics_fields.h"
}

ABool
androidMetrics_write( const AndroidMetrics* androidMetrics )
{
    const char* path = androidMetrics_getMetricsFilePath();
    const AndroidMetrics* am = androidMetrics;
    IniFile* ini;

    if (path == NULL) {
        return 0;
    }

    ini = iniFile_newFromMemory("", path);
    if (ini == NULL) {
        mwarning("Failed to malloc ini file.");
        return 0;
    }

    /* Use magic macros to write all fields to the ini file. */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DOUBLE
#define METRICS_INT(n, s, d)        iniFile_setInteger(ini, s, am->n);
#define METRICS_STRING(n, s, d)     iniFile_setValue(ini, s, am->n);
#define METRICS_DOUBLE(n, s, d)     iniFile_setDouble(ini, s, am->n);

#include "android/utils/metrics/metrics_fields.h"

    if (iniFile_saveToFile(ini, path) != 0)
    {
        mwarning("Failed to save to file at %s", path);
        iniFile_free(ini);
        return 0;
    }
    iniFile_free(ini);
    return 1;
}

ABool
androidMetrics_timestamp()
{
    return 0;
}

ABool
androidMetrics_seal()
{
    AndroidMetrics* metrics;
    int retval;

    metrics = android_alloc0(sizeof(AndroidMetrics));
    androidMetrics_read(metrics);
    metrics->is_dirty = 0;
    retval = androidMetrics_write(metrics);
    AFREE(metrics);

    AFREE(metricsFilePath);
    metricsFilePath = NULL;
    return retval;
}

ABool
androidMetrics_read( AndroidMetrics* androidMetrics )
{
    const char* path = androidMetrics_getMetricsFilePath();
    AndroidMetrics* am = androidMetrics;
    IniFile* ini;

    if (path == NULL) {
        return 0;
    }
    ini = iniFile_newFromFile(path);
    if (ini == NULL) {
        mwarning("Could not open metrics file %s for reading", path);
        return 0;
    }

    /* Use magic macros to read all fields from the ini file.
     * Set to default for the missing fields.
     */
#undef METRICS_INT
#undef METRICS_STRING
#undef METRICS_DOUBLE
#define METRICS_INT(n, s, d)        am->n = iniFile_getInteger(ini, s, d);
#define METRICS_STRING(n, s, d) \
    if (iniFile_getValue(ini, s)) { \
        AFREE(am->n); \
        am->n = iniFile_getString(ini, s, d); \
    }
#define METRICS_DOUBLE(n, s, d)     am->n = iniFile_getDouble(ini, s, d);

#include "android/utils/metrics/metrics_fields.h"

    iniFile_free(ini);
    return 1;
}
