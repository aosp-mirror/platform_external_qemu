#include "android/crashreport/BreakpadUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/path.h"
#include "android/utils/debug.h"


#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

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

StringVector& BreakpadUtils::getCrashCmdLine(void) {
    return mCrashCmdLine;
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

const char* BreakpadUtils::kCrashSubDir = "breakpad";
#ifdef _WIN32
const char* BreakpadUtils::kEmulatorBinary = "emulator.exe";
#else
const char* BreakpadUtils::kEmulatorBinary = "emulator";
#endif
const char* BreakpadUtils::kEmulatorCrashArg = "-processcrash";
const std::string BreakpadUtils::kDumpSuffix = ".dmp";


LazyInstance<BreakpadUtils> gBreakpadUtils = LAZY_INSTANCE_INIT;

BreakpadUtils* BreakpadUtils::get() {
    return gBreakpadUtils.ptr();
}
