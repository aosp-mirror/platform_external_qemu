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
#include "android/emulation/ConfigDirs.h"
#include "android/metrics/studio-helper.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"

#include <libxml/tree.h>

#include <stdlib.h>
#include <fstream>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

/***************************************************************************/
// These consts are replicated as macros in StudioHelper_unittest.cpp
// changes to these will require equivalent changes to the unittests
//
static const char kAndroidStudioUuidDir[] = "JetBrains";
static const char kAndroidStudioUuid[] = "PermanentUserID";

#ifdef __APPLE__
static const char kAndroidStudioDir[] = "AndroidStudio";
#else   // ! ___APPLE__
static const char kAndroidStudioDir[] = ".AndroidStudio";
static const char kAndroidStudioDirInfix[] = "config";
#endif  // __APPLE__

static const char kAndroidStudioDirSuffix[] = "options";
static const char kAndroidStudioDirPreview[] = "Preview";
static const char kAndroidStudioUuidHexPattern[] =
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

using namespace android;
using namespace android::base;

// static
const Version StudioHelper::extractAndroidStudioVersion(
        const char* const dirName) {
    if (dirName == NULL) {
        return Version::Invalid();
    }

    // get rid of kAndroidStudioDir prefix to get to version
    const char* cVersion = strstr(dirName, kAndroidStudioDir);
    if (cVersion == NULL || cVersion != dirName) {
        return Version::Invalid();
    }
    cVersion += strlen(kAndroidStudioDir);

    // if this is a preview, get rid of kAndroidStudioDirPreview
    // prefix too and mark preview as micro version 0
    // (assume micro version 1 for releases)
    const char* micro = "2";
    const char* previewVersion = strstr(cVersion, kAndroidStudioDirPreview);
    if (previewVersion == cVersion) {
        cVersion += strlen(kAndroidStudioDirPreview);
        micro = "1";
    }

    // cVersion should now contain at least a number; if not,
    // this is a very early AndroidStudio installation, let's
    // call it version 0
    if (cVersion[0] == '\0') {
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
String StudioHelper::latestAndroidStudioDir(const String& scanPath) {
    String latest_path;

    if (scanPath.empty()) {
        return latest_path;
    }

    DirScanner* scanner = dirScanner_new(scanPath.c_str());
    if (scanner == NULL) {
        return latest_path;
    }

    System* system = System::get();
    Version latest_version(0, 0, 0);

    for (;;) {
        const char* full_path = dirScanner_nextFull(scanner);
        if (full_path == NULL) {
            // End of enumeration.
            break;
        }
        // ignore files, only interested in subDirs
        if (!system->pathIsDir(full_path)) {
            continue;
        }

        char* name = path_basename(full_path);
        if (name == NULL) {
            continue;
        }
        Version v = extractAndroidStudioVersion(name);
        free(name);
        if (v.isValid() && latest_version < v) {
            latest_version = v;
            latest_path = String(full_path);
        }
    }

    return latest_path;
}

// static
String StudioHelper::pathToStudioXML(const String& studioPath,
                                     const String& filename) {
    if (studioPath.empty() || filename.empty())
        return String();

    // build /path/to/.AndroidStudio/subpath/to/file.xml
    StringVector vpath;
    vpath.append(studioPath);
#ifndef __APPLE__
    vpath.append(String(kAndroidStudioDirInfix));
#endif  // !__APPLE__
    vpath.append(String(kAndroidStudioDirSuffix));
    vpath.append(filename);
    return PathUtils::recompose(vpath);
}

#ifdef _WIN32
// static
String StudioHelper::pathToStudioUUIDWindows() {
    System* sys = System::get();
    String appDataPath = sys->getAppDataDirectory();

    String retval;
    if (!appDataPath.empty()) {
        // build /path/to/APPDATA/subpath/to/StudioUUID file
        StringVector vpath;
        vpath.append(appDataPath);
        vpath.append(String(kAndroidStudioUuidDir));
        vpath.append(String(kAndroidStudioUuid));

        retval = PathUtils::recompose(vpath);
    }

    return retval;
}
#endif

/*****************************************************************************/

// Recurse through studio xml doc and return the value described in match
// as a String, if one is set. Otherwise, return an empty string
//
static String eval_studio_config_xml(xmlDocPtr doc,
                                     xmlNodePtr root,
                                     const StudioXml* const match) {
    xmlNodePtr current = NULL;
    xmlChar* xmlval = NULL;
    String retVal;

    for (current = root; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(current->name, BAD_CAST match->nodename))) {
                xmlChar* propvalue =
                        xmlGetProp(current, BAD_CAST match->propname);
                int nomatch = xmlStrcmp(propvalue, BAD_CAST match->propvalue);
                xmlFree(propvalue);
                if (!nomatch) {
                    xmlval = xmlGetProp(current, BAD_CAST match->keyname);
                    if (xmlval != NULL) {
                        // xmlChar* is defined as unsigned char
                        // we are simply reading the result and don't
                        // operate on it, soe simply reinterpret as
                        // as char * array
                        retVal = String(reinterpret_cast<char*>(xmlval));
                        xmlFree(xmlval);
                        break;
                    }
                }
            }
        }
        retVal = eval_studio_config_xml(doc, current->children, match);
        if (!retVal.empty()) {
            break;
        }
    }

    return retVal;
}

// Find latest studio preferences directory and return the
// value of the xml entry described in |match|.
//
static String parseStudioXML(const StudioXml* const match) {
    String retval;

    System* sys = System::get();
    // Get path to .AndroidStudio
    String appDataPath;
    String studio = sys->envGet("ANDROID_STUDIO_PREFERENCES");
    if (studio.empty()) {
#ifdef __APPLE__
        appDataPath = sys->getAppDataDirectory();
#else
        appDataPath = sys->getHomeDirectory();
#endif  // __APPLE__
        if (appDataPath.empty()) {
            return retval;
        }
        studio = StudioHelper::latestAndroidStudioDir(appDataPath);
    }
    if (studio.empty()) {
        return retval;
    }

    // Find match->filename xml file under .AndroidStudio
    String xml_path =
            StudioHelper::pathToStudioXML(studio, String(match->filename));
    if (xml_path.empty()) {
        D("Failed to find %s in %s", match->filename, studio.c_str());
        return retval;
    }

    xmlDocPtr doc = xmlReadFile(xml_path.c_str(), NULL, 0);
    if (doc == NULL)
        return retval;

    xmlNodePtr root = xmlDocGetRootElement(doc);
    retval = eval_studio_config_xml(doc, root, match);

    xmlFreeDoc(doc);

    return retval;
}

#ifdef _WIN32
static String android_studio_get_Windows_uuid() {
    // Appropriately sized container for UUID
    String uuid_file_path = StudioHelper::pathToStudioUUIDWindows();
    String retval;

    // Read UUID from file
    std::ifstream uuid_file(uuid_file_path.c_str());
    if (uuid_file) {
        std::string line;
        std::getline(uuid_file, line);
        retval = String(line.c_str());
    }

    return retval;
}
#endif  // _WIN32

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

    String xmlVal = parseStudioXML(&optins);
    if (xmlVal.empty()) {
        D("Failed to parse %s preferences file %s", kAndroidStudioDir,
          optins.filename);
        D("Defaulting user crash-report opt-in to false");
        return 0;
    }

    // return 0 if user has not opted in to crash reports
    if (xmlVal == "true") {
        retval = 1;
    } else if (xmlVal == "false") {
        retval = 0;
    } else {
        D("Invalid value set in %s preferences file %s", kAndroidStudioDir,
          optins.filename);
    }

    return retval;
}

static String android_studio_get_installation_id_legacy() {
    String retval;
#ifndef __WIN32
    static const StudioXml uuid = {
            .filename = "options.xml",
            .nodename = "property",
            .propname = "name",
            .propvalue = "installation.uid",
            .keyname = "value",  // assuming kAndroidStudioUuidHexPattern
    };
    retval = parseStudioXML(&uuid);

    if (retval.empty()) {
        D("Failed to parse %s preferences file %s", kAndroidStudioDir,
          uuid.filename);
    }
#else
    // In Microsoft Windows, getting Android Studio installation
    // ID requires searching in completely different path than the
    // rest of Studio preferences ...
    retval = android_studio_get_Windows_uuid();
    if (retval.empty()) {
        D("Failed to parse %s preferences file %s", kAndroidStudioDir,
          kAndroidStudioUuid);
    }
#endif
    return retval;
}

// Get the installation.id reported by Android Studio (string).
// If there is no Android Studio installation or a value
// cannot be retrieved, a fixed dummy UUID will be returned.  Caller is
// responsible for freeing returned string.
char* android_studio_get_installation_id() {
    String uuid_path =
            PathUtils::join(ConfigDirs::getUserDirectory(), "uid.txt");
    std::ifstream uuid_file(uuid_path.c_str());
    if (uuid_file) {
        std::string line;
        std::getline(uuid_file, line);
        if (!line.empty()) {
            return strdup(line.c_str());
        }
    }

    // Couldn't find uuid in the android specific location. Try legacy uuid
    // locations.
    auto uuid = android_studio_get_installation_id_legacy();
    if (!uuid.empty()) {
        return uuid.release();
    }

    D("Defaulting to zero installation ID");
    return strdup(kAndroidStudioUuidHexPattern);
}
