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
#include "android/utils/dirscanner.h"
#include "android/utils/filelock.h"
#include "android/utils/ini.h"
#include "android/utils/metrics/metrics_reporter.h"
#include "android/utils/path.h"
#include "android/utils/string.h"
#include "android/utils/system.h"
#ifdef _WIN32
# include <process.h>
#else
# include <unistd.h>
#endif

# define mwarning(fmt, ...) \
        dwarning("%s:%s: " fmt, __FILE__, __func__, ##__VA_ARGS__)

/* The number of metrics files to batch together when uploading metrics.
 * Tune this so that we get enough reports as well as each report consists of a
 * fair number of emulator runs.
 */
#define METRICS_REPORTING_THRESHOLD 100

/* The global variable containing the location to the metrics file in use in the
 * current emulator instance
 */
static char* metricsFilePath;

static const char metricsRelativeDir[] = "metrics";
static const char metricsFilePrefix[] = "metrics";
static const char metricsFileSuffix[] = "yogibear";

static char*
bufprint_metricsDirPath(char* buffer, char* bufend)
{
    char* pathend = bufend;
    pathend = bufprint_avd_home_path(buffer, bufend);
    if (pathend >= bufend || !path_is_dir(buffer))
    {
        mwarning("Failed to get a valid avd home directory.");
        return pathend;
    }

    pathend = bufprint(pathend, bufend, PATH_SEP "%s", metricsRelativeDir);
    if (pathend >= bufend || path_mkdir_if_needed(buffer, 0744) != 0)
    {
        mwarning("Failed to create a valid metrics home directory.");
        return pathend;
    }

    return pathend;
}

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

    pathend = bufprint_metricsDirPath(path, bufend);
    if (pathend >= bufend)
    {
        return NULL;
    }

    /* TODO(pprabhu) Deal with pid collisions. */
    pathend = bufprint(pathend, bufend, PATH_SEP "%s.%d.%s",
                       metricsFilePrefix, getpid(), metricsFileSuffix);
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
androidMetrics_tick()
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

static ABool
androidMetrics_readPath( AndroidMetrics* androidMetrics, const char* path )
{
    AndroidMetrics* am = androidMetrics;
    IniFile* ini;

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

ABool
androidMetrics_read( AndroidMetrics* androidMetrics )
{
    const char* path = androidMetrics_getMetricsFilePath();
    if (path == NULL) {
        return 0;
    }
    return androidMetrics_readPath(androidMetrics, path);
}

/* Forward declaration. */
ABool
androidMetrics_uploadMetrics( const AndroidMetrics* const* metrics_list,
                              int num_metrics );

static ABool
androidMetrics_reportPaths( const char* const* filePaths, int numFiles )
{
    const char* file_path;
    int success = 1, i;
    int num_reported = 0, num_crashes = 0;
    double system_time = 0.0, user_time = 0.0;
    AndroidMetrics* metrics;

    metrics = android_alloc0(sizeof(AndroidMetrics));
    if (metrics == NULL)
    {
        mwarning("Could not allocate metrics. OOM?");
        return 0;
    }

    VERBOSE_PRINT(init, "Attempting to report metrics for %d runs.", numFiles);
    for(i = 0; i < numFiles; ++i)
    {
        file_path = filePaths[i];
        /* Make sure that the reporting process actually did some reporting. */
        if ((success = androidMetrics_readPath(metrics, file_path)) &&
            metrics->tick > 0)
        {
            ++num_reported;
            system_time += metrics->system_time;
            user_time += metrics->user_time;
            num_crashes += !!(metrics->is_dirty);
            /* TODO(pprabhu) Collate data from multiple runs. */
            androidMetrics_uploadMetrics(&metrics, 1);
        }
    }

    VERBOSE_PRINT(init, "Uploading metrics report from %d previous runs:",
                  num_reported);
    VERBOSE_PRINT(init, "  # crashes: %d", num_crashes);
    VERBOSE_PRINT(init, "  # aggregate user time: %lf", user_time);
    VERBOSE_PRINT(init, "  # aggregate system time: %lf", system_time);
    if (user_time > 0.0)
    {
        VERBOSE_PRINT(init, "  # crashes / user time: %lf",
                      (double) num_crashes / user_time);
    }
    if (system_time > 0.0)
    {
        VERBOSE_PRINT(init, "  # crashes / system time: %lf",
                      (double) num_crashes / system_time);
    }
    if (user_time + system_time > 0.0)
    {
        VERBOSE_PRINT(init, "  # crashes / totoal time: %lf",
                      (double) num_crashes / (user_time + system_time));
    }
    AFREE(metrics);
    return success;
}

/* TODO(pprabhu) Do this asynchronously. This could be slow because it does
 * a bunch of disk and network IO, also this is not required for anything
 * that follows.
 * Consider exposing android/base/threads/Thread.h to C code.
 */
ABool
androidMetrics_tryReportAll()
{
    char path[PATH_MAX], *pathend=path, *bufend=pathend+sizeof(path);
    DirScanner* avd_dir = NULL;
    int num_max_files, num_locked_files = 0;
    const char* file_path;
    char* duped_file_path;
    char *my_absolute_path = NULL;
    char** file_paths = NULL;
    FileLock* file_lock;
    FileLock** file_locks = NULL;
    ABool success = 0, skipped = 0;

    pathend = bufprint_metricsDirPath(path, bufend);
    if (pathend >= bufend)
    {
        return 0;
    }

    avd_dir = dirScanner_new(path);
    num_max_files = dirScanner_numEntries(avd_dir);
    file_paths = android_alloc0(sizeof(const char*) * num_max_files);
    file_locks = android_alloc0(sizeof(FileLock*) * num_max_files);
    my_absolute_path = path_get_absolute(androidMetrics_getMetricsFilePath());
    if (avd_dir == NULL || file_paths == NULL || file_locks == NULL ||
        my_absolute_path == NULL)
    {
        mwarning("Failed to allocate objects. OOM?");
        dirScanner_free(avd_dir);
        AFREE(file_paths);
        AFREE(file_locks);
        AFREE(my_absolute_path);
        return 0;
    }

    while((file_path = dirScanner_nextFull(avd_dir)) != NULL)
    {
        if (!endswith(file_path, metricsFileSuffix) ||
            0 == strcmp(my_absolute_path, file_path))
        {
            continue;
        }

        /* Quickly ASTRDUP the returned string to workaround a bug in
         * dirScanner. As the contents of the scanned directory change, the
         * pointer returned by dirScanner gets changed behind our back.
         * Even taking a filelock changes the directory contents...
         */
        duped_file_path = ASTRDUP(file_path);
        if (duped_file_path == NULL)
        {
            mwarning("Failed to allocate objects. OOM?");
            break;
        }

        file_lock = filelock_create(duped_file_path);
        /* We may not get this file_lock if the emulator process that created
         * the metrics file is still running. This is by design -- we don't want
         * to process partial metrics files.
         */
        if (file_lock != NULL)
        {
            file_locks[num_locked_files] = file_lock;
            file_paths[num_locked_files] = duped_file_path;
            ++num_locked_files;
        } else {
            AFREE(duped_file_path);
        }
    }
    dirScanner_free(avd_dir);
    AFREE(my_absolute_path);

    if (num_locked_files >= METRICS_REPORTING_THRESHOLD)
    {
        success = androidMetrics_reportPaths((const char* const*) file_paths,
                                             num_locked_files);
    }
    else
    {
        VERBOSE_PRINT(init, "Metrics reports below threshold. Skipping.");
        skipped = 1;
    }

    for (;num_locked_files > 0; --num_locked_files)
    {
        if (!skipped)
        {
            success = success &
                      (path_delete_file(file_paths[num_locked_files - 1]) == 0);
        }
        filelock_release(file_locks[num_locked_files - 1]);
        AFREE(file_paths[num_locked_files]);
    }

    success = success || skipped;
    if (!success)
    {
        mwarning("Encountered an error while reporting metrics.");
    }
    AFREE(file_paths);
    AFREE(file_locks);
    return success;
}

/* The actual reporting function.
 * metrics_list contains collated data: We group metrics from fields that we
 * consider "keys", combining the data that we consider "values". None of this
 * should actually matter to the reporting function, but the backend ought to
 * care.
 */
ABool
androidMetrics_uploadMetrics( const AndroidMetrics* const* metrics_list,
                              int num_metrics )
{
    /* TODO(Actually send all the given metrics, possibly throttling the
     * maximum number of remote calls we make.
     */
    return 0;
}
