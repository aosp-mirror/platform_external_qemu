/* Copyright 2014 The Android Open Source Project

** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/metrics/studio_helper.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"
#include "android/utils/system.h"
#include <ctype.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>

#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

#ifdef __APPLE__
#define ANDROID_STUDIO_DIR "AndroidStudio"
#define ANDROID_STUDIO_DIR_PREFIX "Library/Preferences"
#else  // ! ___APPLE__
#define ANDROID_STUDIO_DIR_SUFFIX_PRE "config"
#define ANDROID_STUDIO_DIR ".AndroidStudio"
#define ANDROID_STUDIO_DIR_PREFIX ""
#endif  // __APPLE__
#define ANDROID_STUDIO_DIR_SUFFIX "options"
#define ANDROID_STUDIO_DIR_PREVIEW "Preview"

typedef struct {
    const char* filename;
    const char* nodename;
    const char* propname;
    const char* propvalue;
    const char* keyname;
} StudioXml;

static StudioXml optins = {
        .filename = "usage.statistics.xml",
        .nodename = "component",
        .propname = "name",
        .propvalue = "UsagesStatistic",
        .keyname = "allowed",  // assuming "true"/"false" string values
};

static StudioXml uid = {
        .filename = "options.xml",
        .nodename = "property",
        .propname = "name",
        .propvalue = "installation.uid",
        .keyname = "value",  // assuming hex string format
                             // "00000000-0000-0000-0000-000000000000"
};

/******************************************************************************/

// Opaque type to an object used to scan all available .AndroidStudio dirs
// and assign a version ID to each
typedef struct {
    DirScanner* scanner;
    char temp[PATH_MAX];
    char name[PATH_MAX];
    int cmp;
} StudioScanner;

// Translate the name of an Android Studio preferences directory to
// an integer that can be used for finding the most up-to-date
// preferences installation; this value gets assigned to
// StudioScanner.cmp
// e.g. .AndroidStudio           --> 0     # old releases
//      .AndroidStudioWithBlaze  --> 0     # Google only
//      .AndroidStudioPreview1.2 --> 121   # previews get lower priority
//      .AndroidStudio1.2        --> 122
//      .AndroidStudio1.3        --> 132   # releases get higher
//      priority
int tr_studio_revision(const char* name) {
    // version: major, minor, release
    unsigned char version[3] = {0, 0, 0};

    if (name == NULL || strlen(name) < (sizeof(ANDROID_STUDIO_DIR) - 1))
        return -1;

    // mark end of ANDROID_STUDIO_DIR and beginning of revision string
    const char* revstr = &name[sizeof(ANDROID_STUDIO_DIR) - 1];

    // Detect "Preview" note
    if (strncmp(revstr, ANDROID_STUDIO_DIR_PREVIEW,
                sizeof(ANDROID_STUDIO_DIR_PREVIEW) - 1) == 0) {
        version[2] = 1;  // preview
        // update revstr to point past "Preview" suffix
        revstr = &revstr[sizeof(ANDROID_STUDIO_DIR_PREVIEW) - 1];
    } else {
        version[2] = 2;  // release
    }

    // Assuming *revstr = X.Y, where X, Y in [0,9]
    // Non standard naming will be de-prioritized as version 0
    if (strlen(revstr) != 3 || !isdigit(revstr[0]) || revstr[1] != '.' ||
        !isdigit(revstr[2])) {
        return 0;
    }

    version[0] = ((int)revstr[0]) - ((int)'0');
    version[1] = ((int)revstr[2]) - ((int)'0');

    return (version[0] << 16) | (version[1] << 8) | version[2];
}

/* Create a new .AndroidStudio scanner instance, used to parse the
 * directory
 * at |studio_preferences_home|. If this parameter is NULL, this will
 * search
 * for the default .AndroidStudio directory. Roughly
 * studio_preferences_home is
 *     $HOME/.AndroidStudio<Preview>X.Y in Linux
 *     $HOME/Library/Preferences/AndroidStudio<Preview>X.Y in MacOS
 *     $HOME/.AndroidStudio<Preview>X.Y in Windows in Windows
 * where X.Y is the AndroidStudio version
 */
StudioScanner* studio_scanner_new(const char* studio_preferences_home) {
    StudioScanner* s;
    ANEW0(s);

    char* p = s->temp;
    char* end = p + sizeof(s->temp);

    if (!studio_preferences_home) {
        p = bufprint_home_dir(p, end);
    } else {
        p = bufprint(p, end, "%s", studio_preferences_home);
    }

    // Build the path to AndroidStudio
    p = bufprint(p, end, "%s", PATH_SEP ANDROID_STUDIO_DIR_PREFIX);
    if (p >= end) {
        // Path is too long, no search will be performed.
        D("Alleged path to %s too long: \"%s\"\n", ANDROID_STUDIO_DIR, s->temp);
        return s;
    }

    if (!path_exists(s->temp)) {
        // Path does not exist, no search will be performed.
        D("Path to %s does not exist: %s\n", ANDROID_STUDIO_DIR, s->temp);
        return s;
    }
    s->scanner = dirScanner_new(s->temp);
    return s;
}

/* Return the name of the next .AndroidStudio dir detected by the
 * scanner.
 * This will be NULL at the end of the scan.
 * Assumes PATH_MAX path strings
 */
StudioScanner* studio_scanner_next(StudioScanner* s) {
    if (s == NULL || s->scanner == NULL)
        return NULL;
    if (s->scanner) {
        for (;;) {
            const char* path = dirScanner_nextFull(s->scanner);
            if (!path) {
                // End of enumeration.
                break;
            }
            const char* entry = path_basename(path);
            if (strncmp(entry, ANDROID_STUDIO_DIR,
                        sizeof(ANDROID_STUDIO_DIR) - 1) != 0) {
                // Can't possibly be a .AndroidStudio directory
                // fprintf(stderr, "??? \t %s \n", entry);
                continue;
            } else {
                s->cmp = tr_studio_revision(entry);
                // fprintf(stderr, "%s --> %d", entry, s->cmp);
            }
            // It's a match, return it
            size_t path_len = strlen(path);
            if (path_len >= PATH_MAX) {
                D("Cannot reach %s preferences in path %s", ANDROID_STUDIO_DIR,
                  path);
                continue;
            }
            memcpy(s->name, path, path_len);
            s->name[path_len] = '\0';
            return s;
        }
    }
    return NULL;
}

// Release an StudioScanner object and associated resources.
// This can be called before the end of a scan.
void studio_scanner_free(StudioScanner* s) {
    if (s) {
        if (s->scanner) {
            dirScanner_free(s->scanner);
            s->scanner = NULL;
        }
        AFREE(s);
    }
}

/******************************************************************************/

/* Scan through all possible android studio dirs and return path to
 * latest one
 * Assumes PATH_MAX path strings
 */
int path_to_latest_studio_dir(char* path) {
    StudioScanner* head = studio_scanner_new(NULL);
    StudioScanner* current = NULL;
    int last = -1;

    while ((current = studio_scanner_next(head)) != NULL) {
        if (last < current->cmp) {
            last = current->cmp;
        }
    }
    studio_scanner_free(head);

    if (last == -1)
        return -1;

    // repeat search for actual selection
    head = studio_scanner_new(NULL);
    while ((current = studio_scanner_next(head)) != NULL) {
        if (current->cmp == last)
            break;
    }

    if (strncpy(path, current->name, PATH_MAX) != path) {
        D("%s: memcpy error", __FUNCTION__);
        return -1;
    }
    studio_scanner_free(head);

    return 0;
}

/* Get the path to one of AndroidStudio's xml config files under path.
 * Assumes PATH_MAX path strings.
 * Following AndroidStudio and IntelliJ conventions, it is assumed that
 * studio xml config files will be found under
 *  AndroidStudio/options in Linux and Windows and
 *  AndroidStudio/config/options in MacOS
 */
int path_to_latest_studio_config_xml(char* path, const char* filename) {
    char* p = path;
    char* end = &p[PATH_MAX];

    // append path to config filename (subdirs after ANDROID_STUDIO_DIR
    // are
    // fixed)
    p = &path[strlen(path)];
#ifdef ANDROID_STUDIO_DIR_SUFFIX_PRE
    p = bufprint(p, end, PATH_SEP ANDROID_STUDIO_DIR_SUFFIX_PRE);
#endif
    p = bufprint(p, end, PATH_SEP ANDROID_STUDIO_DIR_SUFFIX);
    p = bufprint(p, end, "%s%s", PATH_SEP, filename);

    if (p >= end) {
        // Path is too long, can't reach file
        D("Alleged path to %s too long: \"%s\"\n", filename, path);
        return -1;
    }

    return 0;
}

/******************************************************************************/

/* Recurse through studio xml doc and return the value described in match,
   if one is set, or NULL otherwise
 */
xmlChar* eval_studio_config_xml(xmlDocPtr doc,
                                xmlNodePtr root,
                                StudioXml* match) {
    xmlNodePtr current = NULL;
    xmlChar* retval = NULL;

    for (current = root; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(current->name, BAD_CAST match->nodename))) {
                xmlChar* propvalue =
                        xmlGetProp(current, BAD_CAST match->propname);
                int nomatch = xmlStrcmp(propvalue, BAD_CAST match->propvalue);
                xmlFree(propvalue);
                if (!nomatch) {
                    retval = xmlGetProp(current, BAD_CAST match->keyname);
                    if (retval != NULL)
                        break;
                }
            }
        }
        retval = eval_studio_config_xml(doc, current->children, match);
        if (retval != NULL)
            break;
    }

    // will be freed by caller
    return retval;
}

/* Find latest studio preferences directory and return the
 * value of the xml entry described in |match|
 */
xmlChar* parse_studio_config_xml(StudioXml* match) {
    char path[PATH_MAX] = {0};

    if (path_to_latest_studio_dir(path) < 0) {
        D("Failed to find latest %s preferences directory", ANDROID_STUDIO_DIR);
        return NULL;
    }

    if (path_to_latest_studio_config_xml(path, match->filename) < 0) {
        D("Failed to find %s in %s", match->filename, path);
        return NULL;
    }

    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadFile(path, NULL, 0);
    if (doc == NULL) {
        D("Failed to parse %s\n", path);
        return NULL;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlChar* retval = eval_studio_config_xml(doc, root, match);

    xmlFreeDoc(doc);

    return retval;
}

/******************************************************************************/

/* Get the status of user opt-in to crash reporting in AndroidStudio
 * preferences. Returns 0 if the user has not agreed, or 1 otherwise.
 */
int get_android_studio_optins(char* retval) {
    if (retval == NULL) {
        D("Invalid use of %s", __FUNCTION__);
    }
    xmlChar* val = parse_studio_config_xml(&optins);

    if (val == NULL) {
        D("Failed to parse %s preferences file %s", ANDROID_STUDIO_DIR,
          optins.filename);
        return -1;
    }

    // return 0 if user has not opted in to crash reports etc
    char* rv = NULL;
    if (!xmlStrncmp(val, BAD_CAST "false", 6)) {
        rv = strncpy(retval, "false", 6);
    } else {
        rv = strncpy(retval, "true", 5);
    }
    if (rv != retval) {
        D("Stuio crash-report preferences could not be copied");
        return -1;
    }

    xmlFree(val);

    return 0;
}

/* Get the installation.id reported by Android Studio.
 * retval should be at least STUDIO_UID_LEN chars long
 * (STUDIO_UID_LEN defined in studio_helper.h)
 */
int get_android_studio_installation_id(char* retval) {
    if (retval == NULL) {
        D("Illegal use of %s", __FUNCTION__);
        return -1;
    }

    xmlChar* val = parse_studio_config_xml(&uid);
    if (val == NULL) {
        D("Failed to parse %s preferences file %s", ANDROID_STUDIO_DIR,
          uid.filename);
        return -1;
    }

    int len = strlen((char*)val);
    if (len >= STUDIO_UID_LEN_MAX) {
        D("Stuio UID unexpectedly long");
        return -1;
    }
    if (strncpy(retval, (char*)val, len) != retval) {
        D("Stuio UID could not be copied");
        return -1;
    }

    xmlFree(val);

    return 0;
}
