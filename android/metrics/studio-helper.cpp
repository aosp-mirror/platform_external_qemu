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

#include "android/metrics/studio-preferences.h"
#include "android/metrics/studio-helper.h"

#include "android/base/containers/StringVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/base/Version.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

#include <libxml/tree.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(__VA_ARGS__)

// StudioXML describes the XML parameters we are seeking for in
// a Studio preferences file.
typedef struct {
    const char* filename;
    const char* nodename;
    const char* propname;
    const char* propvalue;
    const char* keyname;
} StudioXml;

StudioXml optins = {
    .filename = "usage.statistics.xml",
    .nodename = "component",
    .propname = "name",
    .propvalue = "UsagesStatistic",
    .keyname = "allowed",  // assuming "true"/"false" string values
};

StudioXml uid = {
    .filename = "options.xml",
    .nodename = "property",
    .propname = "name",
    .propvalue = "installation.uid",
    .keyname = "value",  // assuming ANDROID_STUDIO_UID_HEXPATTERN
};

using namespace android::base;

/***************************************************************************/

// Extract the Android Studio version from directory name |dirName|
// If |dirName| is not formatted following the AndroidStudio
// preferences/configuration directory name pattern, an invalid
// Version will be returned. The pattern we expect is
// [.]AndroidStudio[Preview]X.Y
//  where X,Y represent a major/minor revision number
const Version extractAndroidStudioVersion(const char * const dirName) {
    if(dirName == NULL)
        return Version::Invalid();

    // get rid of ANDROID_STUDIO_DIR prefix to get to version
    const char* cVersion = strstr(dirName, ANDROID_STUDIO_DIR);
    if(cVersion == NULL || cVersion != dirName) {
        return Version::Invalid();
    }
    cVersion += (sizeof(ANDROID_STUDIO_DIR) - 1);

    // if this is a preview, get rid of ANDROID_STUDIO_DIR_PREVIEW
    // prefix too and mark preview as micro version 0
    // (assume micro version 1 for releases)
    const char *micro = "2";
    const char *previewVersion = strstr(cVersion, ANDROID_STUDIO_DIR_PREVIEW);
    if(previewVersion == cVersion) {
        cVersion += (sizeof(ANDROID_STUDIO_DIR_PREVIEW) - 1);
        micro = "1";
    }

    // cVersion should now contain at least a number; if not,
    // this is a very early AndroidStudio installation, let's
    // call it version 0
    if(strlen(cVersion) < 1) {
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

// Scan path |scanPath| for AndroidStudio preferences/configuration
// direcotires and return the path to the latest one, or
// NULL if none can be found. Caller is responsible for freeing
// returned string
//
char* latestAndroidStudioDir(const char* const scanPath) {
    if(scanPath == NULL)
        return NULL;

    DirScanner *scanner = dirScanner_new(scanPath);
    if(scanner == NULL)
        return NULL;

    System* system = System::get();
    Version latest_version("0");
    char *latest_path = NULL;
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
        AFREE(name);
        if(v.isValid() && latest_version < v) {
            latest_version = v;
            if(latest_path != NULL)
                AFREE(latest_path);
            latest_path = ASTRDUP(full_path);
        }
    }

    return latest_path;
}

// Construct and return the complete path to one of the
// predefined Android Studio XML preferences file we scan
//
char* pathToStudioXML (const char *studioPath,
                       const char* filename) {
    if(studioPath == NULL || filename == NULL)
        return NULL;

    // build /path/to/.AndroidStudio/subpath/to/file.xml
    StringVector vpath;
    vpath.append(String(studioPath));
#ifdef ANDROID_STUDIO_DIR_INFIX
    vpath.append(String(ANDROID_STUDIO_DIR_INFIX));
#endif // ANDROID_STUDIO_DIR_INFIX
    vpath.append(String(ANDROID_STUDIO_DIR_SUFFIX));
    vpath.append(String(filename));
    String path = PathUtils::recompose(vpath);

    return strdup(path.c_str());
}

// Recurse through studio xml doc and return the value described in match,
//   if one is set, NULL otherwise
//
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


// Find latest studio preferences directory and return the
// value of the xml entry described in |match|.
//
xmlChar* parseStudioXML(StudioXml* match) {
    System* sys = System::get();

    const char* home = sys->getHomeDirectory().c_str();
    if(home == NULL)
        return NULL;

    char* studio = latestAndroidStudioDir(home);
    if(studio == NULL)
        return NULL;
    char* path = pathToStudioXML(studio, match->filename);
    if(path == NULL) {
        E("Failed to find %s in %s", match->filename, studio);
        AFREE(studio);
        return NULL;
    }

    xmlDocPtr doc = xmlReadFile(path, NULL, 0);
    AFREE(studio);
    AFREE(path);
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

    xmlChar* val = parseStudioXML(&optins);
    if (val == NULL) {
        E("Failed to parse %s preferences file %s", ANDROID_STUDIO_DIR,
          optins.filename);
        E("Defaulting user crash-report opt-in to false");
        return 0;
    }

    // return 0 if user has not opted in to crash reports
    if (!xmlStrncmp(val, BAD_CAST "true", 5)) {
        retval = 1;
    }
    else if (!xmlStrncmp(val, BAD_CAST "false", 6)) {
        retval = 0;
    } else {
        E("Invalid value set in %s preferences file %s", ANDROID_STUDIO_DIR,
          optins.filename);
        retval = 0;
    }
    xmlFree(val);

    return retval;
}

/***************************************************************************/

// This class is a simple wrapper over a formatted string that
// describes an Android Studio installation ID. It is used
// to create a random ID for this emulator instance, if one
// cannot be retrieved from Studio preferences.
class StudioInstallationID {
public:
    StudioInstallationID() : siid(ANDROID_STUDIO_UID_HEXPATTERN) {
        srand(time(NULL));

        int i = 0;
        for(i = 0; siid[i] != '\0'; i++) {
            if(siid[i] == '-') {
                continue;
            }
            siid[i] += (char) (rand() % 10);
        }
    }

    const char *get() const {
        return siid.c_str();
    }

private:
    String siid;
};

// Get the installation.id reported by Android Studio (string).
// If there is not Android Studio installation or a value
// cannot be retrieved, a random installation ID will
// be generated. Caller is responsible for freeing returned
// string
//
const char *android_studio_get_installation_id() {
    const char *retval = NULL;
    xmlChar* val = parseStudioXML(&uid);

    // WARNING: In Microsoft Windows, getting Android Studio installation
    // ID requires searching in completely different path than the
    // rest of Studio preferences so random ID will be used instead

    if (val == NULL) {
        E("Failed to parse %s preferences file %s", ANDROID_STUDIO_DIR,
          uid.filename);
        E("Defaulting to new, random installation ID");
        static StudioInstallationID random_siid;
        retval = strdup(random_siid.get());
    } else {
        // this is safe, xmlChar * strings terminate with a 0
        retval = strdup((char *) val);
    }

    return retval;
}
