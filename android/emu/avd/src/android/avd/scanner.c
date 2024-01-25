/* Copyright 2014 The Android Open Source Project
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
#include "android/avd/scanner.h"

#include "android/avd/util.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"
#include "android/utils/system.h"

#include <string.h>

#define D(...) VERBOSE_PRINT(init,__VA_ARGS__)
#define SPACE_LEFT(S) (sizeof(S) - (strlen(S) + 1))

struct AvdScanner {
    DirScanner* scanner;
    char avd_path[PATH_MAX];
    char avd_name[PATH_MAX];
    char snapshots_path[PATH_MAX];
};

void append_snapshot_names(AvdScanner* s);

AvdScanner* avdScanner_new(const char* sdk_home) {
    AvdScanner* s;

    ANEW0(s);

    char* p = s->avd_path;
    char* end = p + sizeof(s->avd_path);

    if (!sdk_home) {
        p = bufprint_avd_home_path(p, end);
    } else {
        p = bufprint(p, end, "%s" PATH_SEP ANDROID_AVD_DIR, sdk_home);
    }

    if (p >= end) {
        // Path is too long, no search will be performed.
        D("Path too long: %s\n", s->avd_path);
        return s;
    }
    if (!path_is_dir(s->avd_path)) {
        // Path does not exist, no search will be performed.
        D("Path is not a directory: %s\n", s->avd_path);
        return s;
    }
    s->scanner = dirScanner_new(s->avd_path);
    return s;
}

const char* avdScanner_next(AvdScanner* s, int list_snapshots) {
    if (s->scanner) {
        for (;;) {
            const char* entry = dirScanner_next(s->scanner);
            if (!entry) {
                // End of enumeration.
                break;
            }
            size_t entry_len = strlen(entry);
            if (entry_len < 4 || 
                memcmp(entry + entry_len - 4U, ".ini", 4) != 0) {
                // Can't possibly be a <name>.ini file.
                continue;
            }

            // It's a match, get rid of the .ini suffix and return it.
            entry_len -= 4U;
            if (entry_len >= sizeof(s->avd_name)) {
                D("Name too long: %s\n", entry);
                continue;
            }
            memcpy(s->avd_name, entry, entry_len);
            s->avd_name[entry_len] = '\0';

            if (list_snapshots) {
                append_snapshot_names(s);
            }

            return s->avd_name;
        }
    }
    return NULL;
}

void avdScanner_free(AvdScanner* s) {
    if (s) {
        if (s->scanner) {
            dirScanner_free(s->scanner);
            s->scanner = NULL;
        }
        AFREE(s);
    }
}

void append_snapshot_names(AvdScanner* s) {
    char* p = s->snapshots_path;
    char* end = p + sizeof(s->snapshots_path);

    p = bufprint(p, end, "%s" PATH_SEP "%s.avd" PATH_SEP
                 ANDROID_AVD_SNAPSHOTS_DIR, s->avd_path, s->avd_name);
    if (p >= end) {
        // Path is too long, no search will be performed.
        D("Path too long: %s\n", s->snapshots_path);
        return;
    }
    if (!path_is_dir(s->snapshots_path)) {
        // Path does not exist, no search will be performed.
        D("Path is not a directory: %s\n", s->snapshots_path);
        return;
    }

    // Found the snapshots path, now lets append the snapshot names.
    DirScanner *snap_scanner = dirScanner_new(s->snapshots_path);
    if (snap_scanner) {
        strncat(s->avd_name, "\t: ", SPACE_LEFT(s->avd_name));

        for (;;) {
            const char* snap_name = dirScanner_next(snap_scanner);
            if (!snap_name) {
                // End of enumeration.
                break;
            }

            strncat(s->avd_name, snap_name, SPACE_LEFT(s->avd_name));
            strncat(s->avd_name, ", ", SPACE_LEFT(s->avd_name));
        }

        dirScanner_free(snap_scanner);
    }
}