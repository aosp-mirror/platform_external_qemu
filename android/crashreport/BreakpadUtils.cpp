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

#pragma once

#include "android/crashreport/BreakpadUtils.h"

#include "android/base/files/IniFile.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

#ifdef ANDROID_SDK_TOOLS_REVISION
#define VERSION_STRING STRINGIFY(ANDROID_SDK_TOOLS_REVISION) ".0"
#else
#define VERSION_STRING "standalone"
#endif

#ifdef ANDROID_BUILD_NUMBER
#define BUILD_STRING STRINGIFY(ANDROID_BUILD_NUMBER)
#else
#define BUILD_STRING "0"
#endif

#define PRODUCT_VERSION VERSION_STRING "-" BUILD_STRING

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define PROD 1
#define STAGING 2

using namespace android::base;

BreakpadUtils::BreakpadUtils() {
    if (mEmulatorPath.empty()) {
        mEmulatorPath.assign(System::get()->getProgramDirectory().c_str());
        mEmulatorPath += System::kDirSeparator;
        mEmulatorPath += kEmulatorBinary;
    }
    if (mCrashCmdLine.empty()) {
        mCrashCmdLine.append(String(mEmulatorPath.c_str()));
        mCrashCmdLine.append(String(kEmulatorCrashArg));
    }
    if (mCrashDir.empty()) {
        mCrashDir.assign(::android::ConfigDirs::getUserDirectory().c_str());
        mCrashDir += System::kDirSeparator;
        mCrashDir += kCrashSubDir;
    }
#ifdef _WIN32
    if (mWCrashDir.empty()) {
        mWCrashDir.assign(getCrashDirectory().begin(), getCrashDirectory().end());
    }
#endif
    if (mCaBundlePath.empty()) {
        mCaBundlePath.assign(System::get()->getProgramDirectory().c_str());
        mCaBundlePath += System::kDirSeparator;
        mCaBundlePath += "lib";
        mCaBundlePath += System::kDirSeparator;
        mCaBundlePath += "ca-bundle.pem";
    }
}

bool BreakpadUtils::validatePaths(void) {
    bool valid = true;
    if (!System::get()->pathExists(mCrashDir.c_str())) {
        if (path_mkdir_if_needed(mCrashDir.c_str(), 0744) == -1) {
            E("Couldn't create crash dir %s\n", mCrashDir.c_str());
            valid = false;
        }
    } else if (System::get()->pathIsFile(mCrashDir.c_str())) {
        E("Crash dir is a file! %s\n", mCrashDir.c_str());
        valid = false;
    }
    if (!System::get()->pathIsFile(mCaBundlePath.c_str())) {
        E("Couldn't find file %s\n", mCaBundlePath.c_str());
        valid = false;
    }
    if (!System::get()->pathIsFile(mEmulatorPath.c_str())) {
        E("Couldn't find emulator executable %s\n", mEmulatorPath.c_str());
        valid = false;
    }
    return valid;
}

const std::string& BreakpadUtils::getEmulatorPath(void) {
    return mEmulatorPath;
}

const std::string& BreakpadUtils::getCrashDirectory(void) {
    return mCrashDir;
}

#ifdef _WIN32
const std::wstring& BreakpadUtils::getWCrashDirectory(void) {
    return mWCrashDir;
}
#endif

const StringVector BreakpadUtils::getCrashCmdLine(String dumpPath) {
    StringVector cmdline (mCrashCmdLine);
    cmdline.append(dumpPath);
    return cmdline;
}

bool BreakpadUtils::isDump(const std::string &path) {
    return System::get()->pathIsFile(path.c_str()) &&
              path.size() >= kDumpSuffix.size() &&
              (path.compare(path.size() - kDumpSuffix.size(),
                          kDumpSuffix.size(), kDumpSuffix) == 0);
}

const std::string& BreakpadUtils::getCaBundlePath(void) {
    return mCaBundlePath;
}

const std::string BreakpadUtils::getIniFilePath(const std::string& crashdumpFilePath) {
    std::string inipath (crashdumpFilePath);
    inipath.append(".ini");
    return inipath;
}

bool BreakpadUtils::dumpIniFile(const std::string& crashdumpFilePath) {
    IniFile ini (getIniFilePath(crashdumpFilePath));
    ini.setString(kNameKey,kName);
    ini.setString(kVersionKey,kVersion);
    ini.setString(kReleaseNameKey,VERSION_STRING);
    ini.setString(kBuildNumberKey,BUILD_STRING);
    System::Times time_s = System::get()->getProcessTimes();
    ini.setInt64(kSystemTimeKey,time_s.systemMs);
    ini.setInt64(kUserTimeKey,time_s.userMs);
    return ini.write();
}

// Callback to get the response data from server.
size_t BreakpadUtils::WriteCallback(void *ptr, size_t size,
                            size_t nmemb, void *userp) {
  if (!userp)
    return 0;

  std::string *response = reinterpret_cast<std::string *>(userp);
  size_t real_size = size * nmemb;
  response->append(reinterpret_cast<char *>(ptr), real_size);
  return real_size;
}

bool BreakpadUtils::uploadCrashDump(const std::string& crashdumpFilePath) {
    IniFile ini (getIniFilePath(crashdumpFilePath));
    if (!ini.read()) {
        E("Can't read ini file %s\n", ini.getBackingFile().c_str());
        return false;
    }

    I("\nSending Crash %s to %s\n", crashdumpFilePath.c_str(), kCrashURL);
    curl_init(BreakpadUtils::get()->getCaBundlePath().c_str());
    struct curl_httppost *formpost_ = NULL;
    struct curl_httppost *lastptr_ = NULL;
    std::string http_response_data;
    CURLcode curlRes;


    CURL* const curl = curl_easy_default_init();
    if (!curl) {
        E("Curl instantiation failed\n");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, kCrashURL);

    for (const auto& key : ini) {
        curl_formadd(&formpost_, &lastptr_,
                 CURLFORM_COPYNAME, key.c_str(),
                 CURLFORM_COPYCONTENTS, ini.getString(key,"N/A").c_str(),
                 CURLFORM_END);
    }

    curl_formadd(&formpost_, &lastptr_,
                 CURLFORM_COPYNAME,"upload_file_crashdump",
                 CURLFORM_FILE, crashdumpFilePath.c_str(),
                 CURLFORM_END);


    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost_);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
          reinterpret_cast<void *>(&http_response_data));

    curlRes = curl_easy_perform(curl);

    bool success = true;

    if (curlRes != CURLE_OK) {
        success = false;
        E("curl_easy_perform() failed with code %d (%s)", curlRes,
        curl_easy_strerror(curlRes));
    }

    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200) {
            E("Got HTTP response code %ld", http_response);
            success = false;
        }
    } else if (curlRes == CURLE_UNKNOWN_OPTION) {
        E("Can not get a valid response code: not supported");
        success = false;
    } else {
        E("Unexpected error while checking http response: %s",
                 curl_easy_strerror(curlRes));
        success = false;
    }

    if (success) {
        I("Crash Send complete\n");
        I("Crash Report ID: %s\n",http_response_data.c_str());
    }

    curl_formfree(formpost_);
    curl_easy_cleanup(curl);
    curl_cleanup();
    return success;
}

void BreakpadUtils::cleanupCrashDump(const std::string& crashdumpFilePath) {
    remove(crashdumpFilePath.c_str());
    remove(getIniFilePath(crashdumpFilePath).c_str());
}

#if CRASHSERVER == PROD
const char* BreakpadUtils::kCrashURL = "https://clients2.google.com/cr/report";
#elif CRASHSERVER == STAGING
const char* BreakpadUtils::kCrashURL = "https://clients2.google.com/cr/staging_report";
#endif


const char* BreakpadUtils::kNameKey = "prod";
const char* BreakpadUtils::kVersionKey = "ver";
const char* BreakpadUtils::kReleaseNameKey = "releasename";
const char* BreakpadUtils::kBuildNumberKey = "buildnumber";
const char* BreakpadUtils::kSystemTimeKey = "systemtime";
const char* BreakpadUtils::kUserTimeKey = "usertime";
const char* BreakpadUtils::kName= "AndroidEmulator";
const char* BreakpadUtils::kVersion= PRODUCT_VERSION;


const char* BreakpadUtils::kCrashSubDir = "breakpad";
#ifdef _WIN32
const char* BreakpadUtils::kEmulatorBinary = "emulator.exe";
#else
const char* BreakpadUtils::kEmulatorBinary = "emulator";
#endif
const char* BreakpadUtils::kEmulatorCrashArg = "-process-crash";
const std::string BreakpadUtils::kDumpSuffix = ".dmp";


LazyInstance<BreakpadUtils> gBreakpadUtils = LAZY_INSTANCE_INIT;

BreakpadUtils* BreakpadUtils::get() {
    return gBreakpadUtils.ptr();
}
