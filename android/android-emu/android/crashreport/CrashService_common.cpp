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

#ifdef _WIN32
// To avoid a compiler warning that asks to include this before <windows.h>
// which is included by <curl/curl.h>. This must appear before other includes.
#include <winsock2.h>
#endif

#include "android/crashreport/CrashService.h"

#include "android/crashreport/CrashReporter.h"

#ifdef _WIN32
#include "android/crashreport/CrashService_windows.h"
#elif defined(__APPLE__)
#include "android/crashreport/CrashService_darwin.h"
#elif defined(__linux__)
#include "android/crashreport/CrashService_linux.h"
#else
#error "Unsupported platform"
#endif

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashSystem.h"
#include "android/curl-support.h"
#include "android/utils/debug.h"

#include <curl/curl.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define WAIT_INTERVAL_MS 100

using android::base::PathUtils;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

const char kNameKey[] = "prod";
const char kVersionKey[] = "ver";
const char kName[] = "AndroidEmulator";
const char kCommentsKey[] = "comments";

// Callback to get the response data from server.
static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    if (!userp)
        return 0;

    auto& response = *static_cast<std::string*>(userp);
    size_t real_size = size * nmemb;
    response.append(static_cast<char*>(ptr), real_size);
    return real_size;
}

namespace android {
namespace crashreport {

const char* const CrashService::kHwInfoName = "hw_info.txt";
const char* const CrashService::kMemInfoName = "mem_info.txt";

CrashService::CrashService(const std::string& version,
                           const std::string& build,
                           const char* dataDir)
    : mDumpRequestContext(),
      mServerState(),
      mDataDirectory(dataDir ? dataDir : ""),
      mVersion(version),
      mBuild(build),
      mVersionId(),
      mDumpFile(),
      mReportId(),
      mComments(),
      mDidCrashOnExit(false) {
    mVersionId = version;
    mVersionId += "-";
    mVersionId += build;
};

CrashService::~CrashService() {
}

void CrashService::setDumpFile(const std::string& dumpfile) {
    mDumpFile = dumpfile;
}

std::string CrashService::getDumpFile() const {
    return mDumpFile;
}

bool CrashService::validDumpFile() const {
    return false;
}

std::string CrashService::getReport() {
    return "";
}

std::string CrashService::getReportId() const {
    return mReportId;
}

const std::string& CrashService::getCustomDumpMessage() const {
    return mCustomDumpMessage;
}

const std::string& CrashService::getCrashOnExitMessage() const {
    return mCrashOnExitMessage;
}

bool CrashService::didCrashOnExit() const {
    return mDidCrashOnExit;
}

void CrashService::addReportValue(const std::string& key,
                                  const std::string& value) {
}

void CrashService::addReportFile(const std::string& key,
                                 const std::string& path) {
}

void CrashService::addUserComments(const std::string& comments) {
}

bool CrashService::uploadCrash() {
    return false;
}

UserSuggestions CrashService::getSuggestions() {
    return UserSuggestions(nullptr);
}

bool CrashService::processCrash() {
    return false;
}

bool CrashService::collectSysInfo() {
    return false;
}

std::unique_ptr<CrashService> CrashService::makeCrashService(
        const std::string& version,
        const std::string& build,
        const char* dataDir) {

    return std::unique_ptr<CrashService>(
            new HostCrashService(version, build, dataDir));
}

std::string CrashService::readFile(StringView path) {
    return "";
}

std::string CrashService::getSysInfo() {
    return "";
}

void CrashService::initCrashServer() {
    mServerState.waiting = true;
    mServerState.connected = 0;
}

const std::string& CrashService::getDataDirectory() const {
    return mDataDirectory;
}

int64_t CrashService::waitForDumpFile(int clientpid, int timeout) {
    return 0;
}

bool CrashService::setClient(int clientpid) {
    return false;
}

void CrashService::collectProcessList()
{
}

void CrashService::retrieveDumpMessage() {
}

void CrashService::collectDataFiles() {
}

// This is a list of all substrings that definitely belong to some graphics
// drivers.
// NB: all names have to be lower-case - we transform the string from the stack
// trace to lowe case before doing the comparisons

static const char* const gfx_drivers_lcase[] = {
    // NVIDIA
    "nvcpl", // Control Panel
    "nvshell", // Desktop Explorer
    "nvinit", // initializer
    "nv4_disp", // Windows 2000 (!) display driver
    "nvcod", // CoInstaller
    "nvcuda", // Cuda
    "nvopencl", // OpenCL
    "nvcuvid", // Video decode
    "nvogl", // OpenGL (modern)
    "ig4icd", // OpenGL (icd?)
    "geforcegldriver", // OpenGL (old)
    "nvd3d", // Direct3D
    "nvwgf2", // D3D10
    "nvdx", // D3D shim drivers
    "nvml", // management library
    "nvfbc", // front buffer capture library
    "nvapi", // NVAPI Library
    "libnvidia", // NVIDIA Linux

    // ATI
    "atioglxx", // OpenGL
    "atig6txx", // OpenGL
    "r600", // Radeon r600 series
    "aticfx", // DX11
    "atiumd", // D3D
    "atidxx", // "TMM Com Clone Control Module" (???)
    "atimpenc", // video encoder
    "atigl", // another ATI OpenGL driver

    // Intel
    "i915", // Intel i915 gpu
    "igd", // 'igd' series of Intel GPU's
    "igl", // ?
    "igfx", // ?
    "ig75icd", // Intel icd
    "intelocl", // Intel OpenCL

    // Others
    "libgl.", // Low-level Linux OpenGL
    "opengl32.dll", // Low-level Windows OpenGL
};

static bool containsGfxPattern(const std::string& str) {
    for (const char* pattern : gfx_drivers_lcase) {
        if (str.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

UserSuggestions::UserSuggestions(int* process_state) {
}

}  // namespace crashreport
}  // namespace android
