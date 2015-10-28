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

#include "android/crashreport/CrashSystem.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#endif

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define PROD 1
#define STAGING 2

namespace android {
namespace crashreport {

namespace {

using namespace android::base;

#if CRASHUPLOAD == PROD
const char kCrashURL[] = "https://clients2.google.com/cr/report";
#else
const char kCrashURL[] = "https://clients2.google.com/cr/staging_report";
#endif

#if defined(_WIN32)
const char kPipeName[] = "\\\\.\\pipe\\AndroidEmulatorCrashService";
#elif defined(__APPLE__)
const char kPipeName[] = "com.google.AndroidEmulator.CrashService";
#endif

const char kCrashSubDir[] = "breakpad";

class HostCrashSystem : public CrashSystem {
public:
    HostCrashSystem()
        : mCaBundlePath(),
          mEmulatorPath(),
          mCrashServicePath(),
          mCrashDir(),
          mCrashURL() {
        mCrashDir.assign(::android::ConfigDirs::getUserDirectory().c_str());
        mCrashDir += System::kDirSeparator;
        mCrashDir += kCrashSubDir;
        mCaBundlePath.assign(System::get()->getLauncherDirectory().c_str());
        mCaBundlePath += System::kDirSeparator;
        mCaBundlePath += "lib";
        mCaBundlePath += System::kDirSeparator;
        mCaBundlePath += "ca-bundle.pem";

        mCrashServicePath.assign(
                ::android::base::System::get()->getLauncherDirectory().c_str());
        mCrashServicePath += System::kDirSeparator;
        mCrashServicePath += "emulator";
        if (::android::base::System::get()->getHostBitness() == 64) {
            mCrashServicePath += "64";
        }
        mCrashServicePath += "-crash-service";
#ifdef _WIN32
        mCrashServicePath += ".exe";
#endif

        mCrashURL.assign(kCrashURL);
    }

    virtual ~HostCrashSystem() {}

    virtual const std::string& getCrashDirectory(void) override {
        return mCrashDir;
    }

#ifdef _WIN32
    virtual const std::wstring getWCrashDirectory(void) override {
        ::android::base::Win32UnicodeString crashdir_win32(getCrashDirectory().c_str());
        std::wstring crashdir_wstr(crashdir_win32.c_str());
        return crashdir_wstr;
    }
#endif

    virtual const std::string& getCaBundlePath(void) override {
        return mCaBundlePath;
    }

    virtual const std::string& getCrashServicePath(void) override {
        return mCrashServicePath;
    }

    virtual const std::string& getCrashURL(void) override { return mCrashURL; }

    virtual const StringVector getCrashServiceCmdLine(
            const std::string& pipe,
            const std::string& proc) override {
        StringVector cmdline;
        cmdline.append(::android::base::StringView(mCrashServicePath.c_str()));
        cmdline.append(::android::base::StringView("-pipe"));
        cmdline.append(::android::base::StringView(pipe.c_str()));
        cmdline.append(::android::base::StringView("-ppid"));
        cmdline.append(::android::base::StringView(proc.c_str()));
        return cmdline;
    }

    virtual const std::string getPipeName(int ident) override {
        std::string pipename;

#if defined(_WIN32) || defined(__APPLE__)
        pipename.append(kPipeName);
        pipename.append(".");
#endif
        pipename.append(std::to_string(ident));

        return pipename;
    }

    virtual bool validatePaths(void) override {
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
        if (!System::get()->pathIsFile(mCrashServicePath.c_str())) {
            E("Couldn't find crash service executable %s\n",
              mCrashServicePath.c_str());
            valid = false;
        }
        return valid;
    }

private:
    std::string mCaBundlePath;
    std::string mEmulatorPath;
    std::string mCrashServicePath;
    std::string mCrashDir;
    std::string mCrashURL;
};

LazyInstance<HostCrashSystem> sCrashSystem = LAZY_INSTANCE_INIT;
CrashSystem* sCrashSystemForTesting = NULL;

}  // namespace

CrashSystem* CrashSystem::get(void) {
    CrashSystem* result = sCrashSystemForTesting;
    if (!result) {
        result = sCrashSystem.ptr();
    }
    return result;
}

// static
bool CrashSystem::isDump(const std::string& path) {
    static const char kDumpSuffix[] = ".dmp";
    static const int kDumpSuffixSize = sizeof(kDumpSuffix) - 1;
    return System::get()->pathIsFile(path.c_str()) &&
           path.size() >= kDumpSuffixSize &&
           (path.compare(path.size() - kDumpSuffixSize, kDumpSuffixSize,
                         kDumpSuffix) == 0);
}

// static
CrashSystem* CrashSystem::setForTesting(CrashSystem* crashsystem) {
    CrashSystem* result = sCrashSystemForTesting;
    sCrashSystemForTesting = crashsystem;
    return result;
}

}  // namespace crashreport
}  // namespace android
