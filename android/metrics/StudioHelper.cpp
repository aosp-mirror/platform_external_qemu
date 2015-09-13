// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/metrics/StudioHelper.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/String.h"
#include "android/base/Version.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"

#include <libxml/tree.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(init, __VA_ARGS__)

/***************************************************************************/
// These macros are replicated in the respective unittests file;
// changes to these will require equivalent changes to the unittests
//
#ifdef __APPLE__
static const char * const  ANDROID_STUDIO_DIR_PREFIX = "Library/Preferences";
static const char * const  ANDROID_STUDIO_DIR = "AndroidStudio";
#else  // ! ___APPLE__
static const char * const  ANDROID_STUDIO_DIR = ".AndroidStudio";
static const char * const  ANDROID_STUDIO_DIR_INFIX = "config";
#endif  // __APPLE__
static const char * const  ANDROID_STUDIO_DIR_SUFFIX = "options";
static const char * const  ANDROID_STUDIO_DIR_PREVIEW = "Preview";

static const char* const ANDROID_STUDIO_UUID_HEXPATTERN =
        "00000000-0000-0000-0000-000000000000";

// StudioXML describes the XML parameters we are seeking for in a
// Studio preferences file. StudioXml structs are statically
// defined and used in  android_studio_get_installation_id()
// and android_studio_get_optins().
typedef struct {
    const char* filename;
    const char* nodename;
    const char* propname;
    const char* propvalue;
    const char* keyname;
} StudioXml;

/***************************************************************************/

using namespace android::base;

// static
const Version android::StudioHelper::extractAndroidStudioVersion(const char * const dirName) {
    if(dirName == NULL)
        return Version::Invalid();

    // get rid of ANDROID_STUDIO_DIR prefix to get to version
    const char* cVersion = strstr(dirName, ANDROID_STUDIO_DIR);
    if(cVersion == NULL || cVersion != dirName) {
        return Version::Invalid();
    }
    cVersion += strlen(ANDROID_STUDIO_DIR);

    // if this is a preview, get rid of ANDROID_STUDIO_DIR_PREVIEW
    // prefix too and mark preview as micro version 0
    // (assume micro version 1 for releases)
    const char *micro = "2";
    const char *previewVersion = strstr(cVersion, ANDROID_STUDIO_DIR_PREVIEW);
    if(previewVersion == cVersion) {
        cVersion += strlen(ANDROID_STUDIO_DIR_PREVIEW);
        micro = "1";
    }

    // cVersion should now contain at least a number; if not,
    // this is a very early AndroidStudio installation, let's
    // call it version 0
    if(cVersion[0] == '\0') {
        cVersion = "0";
    }

    // Create a major.micro.minor version string and append
    // a "2" for release and a "1" for previews; this will
    // allow proper sorting
    Version rawVersion(cVersion);
    String adjustedVersion(rawVersion.toString());
    adjustedVersion.append(micro);

    return Version(adjustedVersion.c_str());
}

// static
String android::StudioHelper::latestAndroidStudioDir(const String& scanPath) {
    String latest_path;

    if(scanPath.empty())
        return latest_path;

    DirScanner *scanner = dirScanner_new(scanPath.c_str());
    if(scanner == NULL)
        return latest_path;

    System* system = System::get();
    Version latest_version("0");

    for (;;) {
        const char* full_path = dirScanner_nextFull(scanner);
        if (full_path == NULL) {
            // End of enumeration.
            break;
        }
        // ignore files, only interested in subDirs
        if (!system->pathIsDir(full_path))
            continue;

        char *name = path_basename(full_path);
        if(name == NULL)
            continue;
        Version v = extractAndroidStudioVersion(name);
        free(name);
        if(v.isValid() && latest_version < v) {
            latest_version = v;
            latest_path = String(full_path);
        }
    }

    return latest_path;
}

// static
String android::StudioHelper::pathToStudioXML (const String& studioPath,
                                               const String& filename) {
    if(studioPath.empty() || filename.empty())
        return String();

    // build /path/to/.AndroidStudio/subpath/to/file.xml
    StringVector vpath;
    vpath.append(studioPath);
#ifndef __APPLE__
    vpath.append(String(ANDROID_STUDIO_DIR_INFIX));
#endif // !__APPLE__
    vpath.append(String(ANDROID_STUDIO_DIR_SUFFIX));
    vpath.append(filename);
    return PathUtils::recompose(vpath);
}

// Recurse through studio xml doc and return the value described in match,
// if one is set, NULL otherwise
//
static xmlChar* eval_studio_config_xml(xmlDocPtr doc,
                                       xmlNodePtr root,
                                       const StudioXml* const match) {
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


// Find latest studio preferences directory and return the
// value of the xml entry described in |match|.
//
static xmlChar* parseStudioXML(const StudioXml* const match) {
    System* sys = System::get();

    String home = sys->getHomeDirectory();
    if(home.empty())
        return NULL;

    String studio = android::StudioHelper::latestAndroidStudioDir(home);
    if (studio.empty())
        return NULL;
    String path = android::StudioHelper::pathToStudioXML(studio, String(match->filename));
    if(path.empty()) {
        D("Failed to find %s in %s", match->filename, studio.c_str());
        return NULL;
    }

    xmlDocPtr doc = xmlReadFile(path.c_str(), NULL, 0);
    if (doc == NULL)
        return NULL;

    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlChar* retval = eval_studio_config_xml(doc, root, match);

    xmlFreeDoc(doc);

    return retval;
}

/*****************************************************************************/


// Get the status of user opt-in to crash reporting in AndroidStudio
// preferences. Returns 1 only if the user has opted-in, 0 otherwise.
//
int android_studio_get_optins() {
    int retval = 0;

    static const StudioXml optins = {
        .filename = "usage.statistics.xml",
        .nodename = "component",
        .propname = "name",
        .propvalue = "UsagesStatistic",
        .keyname = "allowed",  // assuming "true"/"false" string values
    };

    xmlChar* val = parseStudioXML(&optins);
    if (val == NULL) {
        D("Failed to parse %s preferences file %s", ANDROID_STUDIO_DIR,
          optins.filename);
        D("Defaulting user crash-report opt-in to false");
        return 0;
    }

    // return 0 if user has not opted in to crash reports
    if (!xmlStrncmp(val, BAD_CAST "true", 5)) {
        retval = 1;
    }
    else if (!xmlStrncmp(val, BAD_CAST "false", 6)) {
        retval = 0;
    } else {
        D("Invalid value set in %s preferences file %s", ANDROID_STUDIO_DIR,
          optins.filename);
        retval = 0;
    }
    xmlFree(val);

    return retval;
}

// Get the installation.id reported by Android Studio (string).
// If there is not Android Studio installation or a value
// cannot be retrieved, a random installation ID will
// be generated. Caller is responsible for freeing returned
// string
//
char *android_studio_get_installation_id() {
    char *retval = NULL;
    static const StudioXml uuid = {
        .filename = "options.xml",
        .nodename = "property",
        .propname = "name",
        .propvalue = "installation.uid",
        .keyname = "value",  // assuming ANDROID_STUDIO_UUID_HEXPATTERN
    };

    xmlChar* val = parseStudioXML(&uuid);

    // WARNING: In Microsoft Windows, getting Android Studio installation
    // ID requires searching in completely different path than the
    // rest of Studio preferences so random ID will be used instead

    if (val == NULL) {
        D("Failed to parse %s preferences file %s", ANDROID_STUDIO_DIR,
          uuid.filename);
        D("Defaulting to zero installation ID");
        retval = strdup(ANDROID_STUDIO_UUID_HEXPATTERN);
    } else {
        // this is safe, xmlChar * strings terminate with a 0
        retval = strdup((char *) val);
    }

    return retval;
}
