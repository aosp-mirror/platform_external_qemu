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

#include "android/metrics/StudioConfig.h"

#include "aemu/base/ArraySize.h"
#include "aemu/base/Optional.h"

#include "aemu/base/Uuid.h"
#include "aemu/base/Version.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "aemu/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/metrics/MetricsPaths.h"
#include "android/utils/debug.h"
#include "android/utils/dirscanner.h"
#include "android/utils/path.h"
#include "android/version.h"

#include <libxml/tree.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string_view>

#include <stdlib.h>
#include <string.h>

using android::base::LazyInstance;
using android::base::Optional;
using std::string;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

/***************************************************************************/
// These consts are replicated as macros in StudioHelper_unittest.cpp
// changes to these will require equivalent changes to the unittests
//
#ifdef __APPLE__
static const char kAndroidStudioDir[] = "AndroidStudio";
#else   // ! ___APPLE__
static const char kAndroidStudioDir[] = ".AndroidStudio";
static const char kAndroidStudioDirInfix[] = "config";
#endif  // __APPLE__

static const char kAndroidStudioDirSuffix[] = "options";
static const char kAndroidStudioDirPreview[] = "Preview";

/***************************************************************************/

namespace {
// StudioXML describes the XML parameters we are seeking for in a
// Studio preferences file. StudioXml structs are defined statically
// in functions that call parseStudioXML().
struct StudioXml {
    const char* filename;
    const char* nodename;
    const char* propname;
    const char* propvalue;
    const char* keyname;
};
}  // namespace

using android::base::c_str;
using android::base::PathUtils;
using android::base::stringLiteralLength;
using android::base::System;
using android::base::Version;
using android::studio::UpdateChannel;

// Emulator generated userId file name.
static const char kEmuUserId[] = "userid";

namespace android {
namespace studio {

// Find latest studio preferences directory and return the
// value of the xml entry described in |match|.
static Optional<string> parseStudioXML(const StudioXml* const match);

static std::string findLatestAndroidStudioDir() {
    System* sys = System::get();
    // Get path to .AndroidStudio
    std::string studio = sys->envGet("ANDROID_STUDIO_PREFERENCES");
    if (!studio.empty()) {
        return studio;
    }

    const std::string appDataPath =
#ifdef __APPLE__
            sys->getAppDataDirectory();
#else
            sys->getHomeDirectory();
#endif  // __APPLE__
    if (appDataPath.empty()) {
        return {};
    }
    return latestAndroidStudioDir(appDataPath);
}

Version extractAndroidStudioVersion(const char* const dirName) {
    if (dirName == nullptr) {
        return Version::invalid();
    }

    // get rid of kAndroidStudioDir prefix to get to version
    if (strncmp(dirName, kAndroidStudioDir,
                stringLiteralLength(kAndroidStudioDir)) != 0) {
        return Version::invalid();
    }

    const char* cVersion = dirName + stringLiteralLength(kAndroidStudioDir);

    // if this is a preview, get rid of kAndroidStudioDirPreview
    // prefix too and mark preview as build #1
    // (assume build #2 for releases)
    auto build = 2;
    if (strncmp(cVersion, kAndroidStudioDirPreview,
                stringLiteralLength(kAndroidStudioDirPreview)) == 0) {
        cVersion += stringLiteralLength(kAndroidStudioDirPreview);
        build = 1;
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
    if (rawVersion == Version::invalid()) {
        return Version::invalid();
    }

    return Version(rawVersion.component<Version::kMajor>(),
                   rawVersion.component<Version::kMinor>(),
                   rawVersion.component<Version::kMicro>(), build);
}

std::string latestAndroidStudioDir(const std::string& scanPath) {
    if (scanPath.empty()) {
        return {};
    }

    const auto scanner = android::base::makeCustomScopedPtr(
            dirScanner_new(scanPath.c_str()), dirScanner_free);
    if (!scanner) {
        return {};
    }

    std::string latest_path;
    System* system = System::get();
    Version latest_version(0, 0, 0);

    for (;;) {
        const char* full_path = dirScanner_nextFull(scanner.get());
        if (full_path == nullptr) {
            // End of enumeration.
            break;
        }

        char* name = path_basename(full_path);
        if (name == nullptr) {
            continue;
        }

        Version v = extractAndroidStudioVersion(name);
        free(name);

        // ignore files, only interested in subDirs
        if (!v.isValid() || !system->pathIsDir(full_path)) {
            continue;
        }

        if (latest_version < v) {
            latest_version = v;
            latest_path = full_path;
        }
    }

    return latest_path;
}

std::string pathToStudioXML(const std::string& studioPath,
                            const std::string& filename) {
    if (studioPath.empty() || filename.empty()) {
        return std::string();
    }

    // build /path/to/.AndroidStudio/subpath/to/file.xml
    std::vector<std::string> vpath;
    vpath.push_back(studioPath);
#ifndef __APPLE__
    vpath.push_back(std::string(kAndroidStudioDirInfix));
#endif  // !__APPLE__
    vpath.push_back(std::string(kAndroidStudioDirSuffix));
    vpath.push_back(filename);
    return PathUtils::recompose(vpath);
}

base::Version lastestAndroidStudioVersion() {
    const auto studio = findLatestAndroidStudioDir();
    if (studio.empty()) {
        return {};
    }
    std::string basename;
    PathUtils::split(studio.data(), nullptr, &basename);
    return extractAndroidStudioVersion(c_str(basename));
}

struct UpdateChannelInfo {
    UpdateChannel updateChannel = UpdateChannel::Unknown;
    bool initialized = false;
};

static LazyInstance<UpdateChannelInfo> sUpdateChannelInfo = LAZY_INSTANCE_INIT;

UpdateChannel updateChannel() {
    // Only interact with other files if
    // a) this is the first time calling updateChannel()
    // b) a previous call to updateChannel() did not succeed
    if (!sUpdateChannelInfo->initialized) {
        static const StudioXml channelInfo = {
                .filename = "updates.xml",
                .nodename = "option",
                .propname = "name",
                .propvalue = "UPDATE_CHANNEL_TYPE",
                .keyname = "value",
        };
        const auto channelStr = parseStudioXML(&channelInfo);
        if (!channelStr) {
            return UpdateChannel::Unknown;
        }

        static const struct NamedChannel {
            std::string_view name;
            UpdateChannel channel;
        } channelNames[] = {
                {"", UpdateChannel::Canary},  // this is the current default
                {"eap", UpdateChannel::Canary},
                {"release", UpdateChannel::Stable},
                {"beta", UpdateChannel::Beta},
                {"milestone", UpdateChannel::Dev}};

        const auto channelIt =
                std::find_if(std::begin(channelNames), std::end(channelNames),
                             [&channelStr](const NamedChannel& channel) {
                                 return channel.name == *channelStr;
                             });
        if (channelIt != std::end(channelNames)) {
            sUpdateChannelInfo->initialized = true;
            sUpdateChannelInfo->updateChannel = channelIt->channel;
            return channelIt->channel;
        }
        return UpdateChannel::Unknown;
    } else {
        return sUpdateChannelInfo->updateChannel;
    }
}

/*****************************************************************************/

// Recurse through studio xml doc and return the value described in match
// as a string, if one is set. Otherwise, return an empty string
//
static std::string eval_studio_config_xml(xmlDocPtr doc,
                                          xmlNodePtr root,
                                          const StudioXml* const match) {
    xmlNodePtr current = nullptr;
    xmlChar* xmlval = nullptr;
    std::string retVal;

    for (current = root; current; current = current->next) {
        if (current->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(current->name, BAD_CAST match->nodename))) {
                xmlChar* propvalue =
                        xmlGetProp(current, BAD_CAST match->propname);
                int nomatch = xmlStrcmp(propvalue, BAD_CAST match->propvalue);
                xmlFree(propvalue);
                if (!nomatch) {
                    xmlval = xmlGetProp(current, BAD_CAST match->keyname);
                    if (xmlval != nullptr) {
                        // xmlChar* is defined as unsigned char
                        // we are simply reading the result and don't
                        // operate on it, soe simply reinterpret as
                        // as char * array
                        retVal = std::string(reinterpret_cast<char*>(xmlval));
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

// Returns:
//   - An empty Optional to indicate that parsing failed.
//   - Optional("") to indicate that we found the file, but |match| failed.
//   - Optional(matched_vaule) in case of success.
static Optional<string> parseStudioXML(const StudioXml* const match) {
    std::string studio = findLatestAndroidStudioDir();
    if (studio.empty()) {
        return {};
    }

    // Find match->filename xml file under .AndroidStudio
    std::string xml_path = pathToStudioXML(studio, match->filename);
    if (xml_path.empty()) {
        D("Failed to find %s in %s", match->filename, studio.c_str());
        return {};
    }

    xmlDocPtr doc = xmlReadFile(xml_path.c_str(), nullptr, 0);
    if (doc == nullptr) {
        return {};
    }

    // At this point, we assume that we have found the correct xml file.
    // If we fail to match within this file, we'll return an empty string
    // indicating that nothing matches.
    xmlNodePtr root = xmlDocGetRootElement(doc);
    Optional<string> retval = eval_studio_config_xml(doc, root, match);
    xmlFreeDoc(doc);
    return retval;
}

//
// This is a simple ad-hoc JSON parser for the currently present values of the
// Android Studio metrics configuration file.
// Function doesn't try to be format-compliant or even implement more than we
// currently need, it just allows one to parse basic name:value pairs and pass
// a callback to handle the "value" part
// |name| - the key to extract value for, without quotes
// |extractValue| - a functor that's called with a whole rest of the current
//  line as an argument, starting with the first non-whitespace character after
//  ':'. E.g.:
//
//      "key" :   "value","otherkey":11
//                ^-------------------^
//                  |linePart|, inclusive.
//
template <class ValueType, class ValueExtractor>
static Optional<ValueType> getStudioConfigJsonValue(
        std::string_view name,
        ValueExtractor extractValue) {
    const std::string configPath = android::metrics::getSettingsFilePath();
    std::ifstream in(PathUtils::asUnicodePath(configPath.data()).c_str());
    if (!in) {
        return {};
    }

    // Parse JSON to extract the needed option
    std::string line;
    while (std::getline(in, line)) {
        // All names start from a double quote character, so start with finding
        // one.
        for (auto it = std::find(line.begin(), line.end(), '\"');
             it != line.end(); it = std::find(it + 1, line.end(), '\"')) {
            auto itName = it + 1;
            if (itName == line.end()) {
                continue;
            }
            // Check if this is the right key...
            if (strncmp(&*itName, name.data(), name.size()) != 0) {
                continue;
            }
            auto itNameEnd = itName + name.size();
            if (itNameEnd == line.end() || *itNameEnd != '\"') {
                continue;
            }

            auto itNext = itNameEnd + 1;
            // Skip the spaces before the colon...
            while (itNext < line.end() && isspace(*itNext)) {
                ++itNext;
            }
            // ... the colon itself...
            if (itNext == line.end() || *itNext != ':') {
                continue;
            }
            ++itNext;
            // ... and spaces after it.
            while (itNext < line.end() && isspace(*itNext)) {
                ++itNext;
            }
            if (itNext == line.end()) {
                continue;
            }
            // The value has to be somewhere in the remaining part of the line,
            // pass it to someone who knows how to extract it.
            auto val = extractValue(
                    std::string_view(&*itNext, line.end() - itNext));
            return val;
        }
    }
    return {};
}

bool getUserMetricsOptIn() {
    return getStudioConfigJsonValue<bool>(
                   "hasOptedIn",
                   [](std::string_view linePart) -> bool {
                       static constexpr std::string_view trueValues[] = {"true",
                                                                         "1"};
                       auto it = linePart.begin();
                       for (const auto& trueVal : trueValues) {
                           if (strncmp(trueVal.data(), &*it, trueVal.size()) !=
                               0) {
                               continue;
                           }
                           it += trueVal.size();
                           // make sure the value ends here
                           if (it == linePart.end()) {
                               return true;
                           }
                           if (it < linePart.end() &&
                               (isspace(*it) || ispunct(*it))) {
                               return true;
                           }
                       }
                       return false;
                   })
            .valueOr(false);
}

bool userMetricsOptInExists() {
    return getStudioConfigJsonValue<bool>(
                   "hasOptedIn",
                   [](std::string_view linePart) -> bool {
                       // The field exists and we don't care about its value.
                       return true;
                   })
            .valueOr(false);
}

std::string getAnonymizationSalt() {
    return getStudioConfigJsonValue<std::string>(
                   "saltValue",
                   [](std::string_view linePart) -> std::string {
                       if (linePart.empty()) {
                           return {};
                       }
                       auto it = linePart.begin();
                       if (*it == '\"') {
                           // This is a quoted string.
                           // BTW, it doesn't support escaping as there was no
                           // need yet.
                           ++it;
                           auto itEnd = it;
                           while (itEnd != linePart.end() && *itEnd != '\"') {
                               ++itEnd;
                           }
                           return std::string(it, itEnd);
                       } else {
                           // No quotes, just parse till the end of the value.
                           auto itEnd = it;
                           while (itEnd != linePart.end() && !isspace(*itEnd) &&
                                  *itEnd != ',') {
                               ++itEnd;
                           }
                           return std::string(it, itEnd);
                       }
                   })
            .valueOr(EMULATOR_FULL_VERSION_STRING "-" EMULATOR_CL_SHA1);
}

// Forward-declare functions to use here and in the unit test.
std::string extractInstallationId();
std::string persistAndLoadEmuUserId();

namespace {

// A collection of cached configuration values - these aren't supposed to change
// during the emulator run time, so we don't need to re-parse them on every
// call.
struct StaticValues {
    std::string installationId;

    StaticValues() {
        installationId = extractInstallationId();

        if (installationId.empty()) {
            // This can happen if Android Studio is not installed, or was never
            // launched to create the configuration file. In this case we
            // generate a new ID and persist it on the disk to reuse next time.
            installationId = persistAndLoadEmuUserId();
            D("Failed to extract userId from Android Studio. Using emulator generated id=%s",
              installationId.c_str());
        } else {
            D("Using the same userId as Android Studio for metrics. id=%s", installationId.c_str());
        }
    }
};

LazyInstance<StaticValues> sStaticValues = {};

}  // namespace

// This is to be able to unit-test the implementation without hacking into the
// LazyInstance<> code.
std::string extractInstallationId() {
    auto optionalId = getStudioConfigJsonValue<std::string>(
            "userId", [](std::string_view linePart) -> std::string {
                if (linePart.empty()) {
                    return {};
                }
                auto it = linePart.begin();
                if (*it == '\"') {
                    // This is a quoted string.
                    // BTW, it doesn't support escaping as there was no need
                    // yet.
                    ++it;
                    auto itEnd = it;
                    while (itEnd != linePart.end() && *itEnd != '\"') {
                        ++itEnd;
                    }
                    return std::string(it, itEnd);
                } else {
                    // No quotes, just parse till the end of the value.
                    auto itEnd = it;
                    while (itEnd != linePart.end() && !isspace(*itEnd) &&
                           *itEnd != ',') {
                        ++itEnd;
                    }
                    return std::string(it, itEnd);
                }
            });
    return optionalId.valueOr({});
}

// This is to be able to unit-test the implementation without hacking into the
// LazyInstance<> code.
std::string persistAndLoadEmuUserId() {
    std::string userId;
    auto idPath = PathUtils::join(ConfigDirs::getUserDirectory(), kEmuUserId);

    std::ifstream idFile;
    // First, try to reuse the existing ID if possible.
    idFile.open(PathUtils::asUnicodePath(idPath.data()).c_str(), std::ifstream::in);
    if (idFile.good()) {
        std::stringstream idBuffer;
        idBuffer << idFile.rdbuf();
        idFile.close();

        userId = idBuffer.str();
    } else {
        idFile.close();

        // The ID file doesn't exist, generate a new id and save to disk.
        userId = base::Uuid::generate().toString();

        std::ofstream newIdFile;
        newIdFile.open(PathUtils::asUnicodePath(idPath.data()).c_str(), std::ofstream::trunc);
        newIdFile << userId;
        newIdFile.close();
    }
    return userId;
}

const std::string& getInstallationId() {
    return sStaticValues->installationId;
}

}  // namespace studio
}  // namespace android
