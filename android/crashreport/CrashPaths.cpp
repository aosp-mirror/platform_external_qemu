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


#include "android/crashreport/CrashPaths.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define PROD 1
#define STAGING 2

using namespace android::base;

CrashPaths::CrashPaths() {
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

bool CrashPaths::validatePaths(void) {
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

const std::string& CrashPaths::getEmulatorPath(void) {
    return mEmulatorPath;
}

const std::string& CrashPaths::getCrashDirectory(void) {
    return mCrashDir;
}

#ifdef _WIN32
const std::wstring& CrashPaths::getWCrashDirectory(void) {
    return mWCrashDir;
}
#endif

const StringVector CrashPaths::getCrashCmdLine(String dumpPath) {
    StringVector cmdline (mCrashCmdLine);
    cmdline.append(dumpPath);
    return cmdline;
}

bool CrashPaths::isDump(const std::string &path) {
    return System::get()->pathIsFile(path.c_str()) &&
              path.size() >= kDumpSuffix.size() &&
              (path.compare(path.size() - kDumpSuffix.size(),
                          kDumpSuffix.size(), kDumpSuffix) == 0);
}

const std::string& CrashPaths::getCaBundlePath(void) {
    return mCaBundlePath;
}

#if CRASHUPLOAD == PROD
const char* CrashPaths::kCrashURL = "https://clients2.google.com/cr/report";
#else
const char* CrashPaths::kCrashURL = "https://clients2.google.com/cr/staging_report";
#endif

#ifdef _WIN32
const wchar_t* CrashPaths::kWPipeName = L"\\\\.\\pipe\\AndroidEmulatorCrashService";
const char* CrashPaths::kPipeName = "\\\\.\\pipe\\AndroidEmulatorCrashService";
#endif

const char* CrashPaths::kCrashSubDir = "breakpad";
#ifdef _WIN32
const char* CrashPaths::kEmulatorBinary = "emulator.exe";
#else
const char* CrashPaths::kEmulatorBinary = "emulator";
#endif
const char* CrashPaths::kEmulatorCrashArg = "-process-crash";
const std::string CrashPaths::kDumpSuffix = ".dmp";


LazyInstance<CrashPaths> gCrashPaths = LAZY_INSTANCE_INIT;

CrashPaths* CrashPaths::get() {
    return gCrashPaths.ptr();
}
