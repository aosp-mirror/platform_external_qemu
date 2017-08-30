/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <keymaster/keymaster_configuration.h>

#include <regex>
#include <string>

#include <regex.h>

#define LOG_TAG "keymaster"
#include <cutils/log.h>

#ifndef KEYMASTER_UNIT_TEST_BUILD
#include <cutils/properties.h>
#else
#define PROPERTY_VALUE_MAX 80 /* Value doesn't matter */
void property_get(const char* /* prop_name */, char* /* prop */, const char* /* default */) {}
#endif

#include <keymaster/authorization_set.h>

namespace keymaster {

namespace {

constexpr char kPlatformVersionProp[] = "ro.build.version.release";
constexpr char kPlatformVersionRegex[] = "^([0-9]{1,2})(\\.([0-9]{1,2}))?(\\.([0-9]{1,2}))?";
constexpr size_t kMajorVersionMatch = 1;
constexpr size_t kMinorVersionMatch = 3;
constexpr size_t kSubminorVersionMatch = 5;
constexpr size_t kPlatformVersionMatchCount = kSubminorVersionMatch + 1;

constexpr char kPlatformPatchlevelProp[] = "ro.build.version.security_patch";
constexpr char kPlatformPatchlevelRegex[] = "^([0-9]{4})-([0-9]{2})-[0-9]{2}$";
constexpr size_t kYearMatch = 1;
constexpr size_t kMonthMatch = 2;
constexpr size_t kPlatformPatchlevelMatchCount = kMonthMatch + 1;

uint32_t match_to_uint32(const char* expression, const regmatch_t& match) {
    if (match.rm_so == -1)
        return 0;

    size_t len = match.rm_eo - match.rm_so;
    std::string s(expression + match.rm_so, len);
    return std::stoul(s);
}

}  // anonymous namespace

keymaster_error_t ConfigureDevice(keymaster2_device_t* dev, uint32_t os_version,
                                  uint32_t os_patchlevel) {
    AuthorizationSet config_params(AuthorizationSetBuilder()
                                       .Authorization(keymaster::TAG_OS_VERSION, os_version)
                                       .Authorization(keymaster::TAG_OS_PATCHLEVEL, os_patchlevel));
    return dev->configure(dev, &config_params);
}

keymaster_error_t ConfigureDevice(keymaster2_device_t* dev) {
    return ConfigureDevice(dev, GetOsVersion(), GetOsPatchlevel());
}

uint32_t GetOsVersion(const char* version_str) {
    regex_t regex;
    if (regcomp(&regex, kPlatformVersionRegex, REG_EXTENDED)) {
        ALOGE("Failed to compile version regex! (%s)", kPlatformVersionRegex);
        return 0;
    }

    regmatch_t matches[kPlatformVersionMatchCount];
    int not_match =
        regexec(&regex, version_str, kPlatformVersionMatchCount, matches, 0 /* flags */);
    regfree(&regex);
    if (not_match) {
        ALOGI("Platform version string does not match expected format.  Using version 0.");
        return 0;
    }

    uint32_t major = match_to_uint32(version_str, matches[kMajorVersionMatch]);
    uint32_t minor = match_to_uint32(version_str, matches[kMinorVersionMatch]);
    uint32_t subminor = match_to_uint32(version_str, matches[kSubminorVersionMatch]);

    return (major * 100 + minor) * 100 + subminor;
}

uint32_t GetOsVersion() {
    char version_str[PROPERTY_VALUE_MAX];
    property_get(kPlatformVersionProp, version_str, "" /* default */);
    return GetOsVersion(version_str);
}

uint32_t GetOsPatchlevel(const char* patchlevel_str) {
    regex_t regex;
    if (regcomp(&regex, kPlatformPatchlevelRegex, REG_EXTENDED) != 0) {
        ALOGE("Failed to compile platform patchlevel regex! (%s)", kPlatformPatchlevelRegex);
        return 0;
    }

    regmatch_t matches[kPlatformPatchlevelMatchCount];
    int not_match =
        regexec(&regex, patchlevel_str, kPlatformPatchlevelMatchCount, matches, 0 /* flags */);
    regfree(&regex);
    if (not_match) {
        ALOGI("Platform patchlevel string does not match expected format.  Using patchlevel 0");
        return 0;
    }

    uint32_t year = match_to_uint32(patchlevel_str, matches[kYearMatch]);
    uint32_t month = match_to_uint32(patchlevel_str, matches[kMonthMatch]);

    if (month < 1 || month > 12) {
        ALOGE("Invalid patch month %d", month);
        return 0;
    }
    return year * 100 + month;
}

uint32_t GetOsPatchlevel() {
    char patchlevel_str[PROPERTY_VALUE_MAX];
    property_get(kPlatformPatchlevelProp, patchlevel_str, "" /* default */);
    return GetOsPatchlevel(patchlevel_str);
}

}  // namespace keymaster
