// Copyright (C) 2021 The Android Open Source Project
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
#include "emulator/avd/FakeAvd.h"

#include <grpcpp/grpcpp.h>  // for Status
#include <fstream>          // for operator<<
#include <memory>           // for unique_ptr
#include <type_traits>      // for add_con...
#include <unordered_map>    // for unorder...
#include <vector>           // for vector

#include "android/base/Log.h"                                 // for LogStre...
#include "android/base/StringFormat.h"                        // for StringF...
#include "android/base/StringView.h"                          // for CStrWra...
#include "android/base/files/IniFile.h"                       // for IniFile
#include "android/base/files/PathUtils.h"                     // for pj, Pat...
#include "android/base/system/System.h"                       // for System
#include "android/emulation/ConfigDirs.h"                     // for ConfigDirs
#include "android/emulation/control/EmulatorAdvertisement.h"  // for Emulato...
#include "android/utils/file_io.h"                            // for android...
#include "android/utils/path.h"                               // for path_mk...
#include "android/emulation/control/utils/EmulatorGrcpClient.h"                  // for Emulato...
#include "emulator_controller.grpc.pb.h"                      // for Emulato...
#include "emulator_controller.pb.h"                           // for Emulato...
#include "google/protobuf/empty.pb.h"                         // for Empty

#ifdef _WIN32
#include <windows.h>

#undef ERROR
#include <errno.h>
#include <stdio.h>
#endif

#include <dirent.h>    // for closedir
#include <stdlib.h>    // for NULL
#include <string.h>    // for strcmp
#include <sys/stat.h>  // for stat

using android::base::PathUtils;
using android::base::System;
using android::emulation::control::EmulatorGrpcClient;
using ::google::protobuf::Empty;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

std::string FakeAvd::sPIDFILE("fake_avd_pid");
std::string FakeAvd::sFAKE_PREFIX("remote_");

FakeAvd::FakeAvd(EmulatorGrpcClient* fromRemoteAvd) : mClient(fromRemoteAvd){};

FakeAvd::~FakeAvd() {
    remove();
};

// Recursively delete a directory and its contents.
// This will only delete directories below the avd root directory.
void FakeAvd::deleteRecursive(const std::string& path) {
    if (PathUtils::relativeTo(ConfigDirs::getAvdRootDirectory(), path) ==
        path) {
        LOG(WARNING) << "Not removing path " << path << ", it is not under "
                     << ConfigDirs::getAvdRootDirectory();
        return;
    };

    // First remove any files in the dir
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return;
    }

    dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        std::string entry_path =
                android::base::StringFormat("%s/%s", path, entry->d_name);
#ifdef _WIN32
        struct _stati64 stats;
        android_lstat(entry_path.c_str(),
                      reinterpret_cast<struct stat*>(&stats));
#else
        struct stat stats;
        android_lstat(entry_path.c_str(), &stats);
#endif

        if (S_ISDIR(stats.st_mode)) {
            deleteRecursive(entry_path);
        } else {
            android_unlink(entry_path.c_str());
        }
    }
    closedir(dir);
    android_rmdir(path.c_str());
}

bool FakeAvd::create() {
    if (!retrieveRemoteProperties()) {
        return false;
    }

    auto avdDir = mRemoteProperties["avd.dir"];
    std::string name = sFAKE_PREFIX + mClient->address() + "_" + mAvdId;

    // Write out the ini file.
    std::string ini =
            android::base::pj(ConfigDirs::getAvdRootDirectory(), name + ".ini");
    std::ofstream iniFile(PathUtils::asUnicodePath(ini).c_str());
    iniFile << "avd.ini.encoding=UTF-8" << std::endl;
    iniFile << "path=" << avdDir << std::endl;
    iniFile << "path.rel=" << android::base::pj("avd", name + ".avd")
            << std::endl;
    iniFile << "target=android-29" << std::endl;

    if (iniFile.bad()) {
        return false;
    }

    path_mkdir_if_needed(avdDir.c_str(), 0755);

    // Write out pidfile, used to garbage collect leftover fakes.
    auto pidFilename = android::base::pj(avdDir, sPIDFILE);
    std::ofstream pidFile(PathUtils::asUnicodePath(pidFilename).c_str());
    pidFile << "pid=" << System::get()->getCurrentProcessId() << std::endl;
    pidFile << "ini=" << ini << std::endl;
    if (pidFile.bad()) {
        return false;
    }

    // Write out config.ini & hardware-qemu.ini
    auto hwIniFile = android::base::pj(avdDir, "hardware-qemu.ini");
    auto cfgIniFile = android::base::pj(avdDir, "config.ini");

    android::base::IniFile hwIni(hwIniFile);
    android::base::IniFile cfgIni(cfgIniFile);
    for (const auto& [key, value] : mRemoteProperties) {
        if (!value.empty()) {
            hwIni.setString(key, value);
            cfgIni.setString(key, value);
        }
    }
    bool successfulWrite = hwIni.write() && cfgIni.write();
    if (!successfulWrite) {
        LOG(ERROR) << "Unable to write fake configuration files.";
    }
    return successfulWrite;
}

bool FakeAvd::retrieveRemoteProperties() {
    auto stub = mClient->stub<android::emulation::control::EmulatorController>();
    auto ctx = mClient->newContext();
    Empty empty;
    EmulatorStatus response;
    Status status = stub->getStatus(ctx.get(), Empty(), &response);
    if (!status.ok()) {
        LOG(ERROR) << "Unable to retrieve status " << status.error_message();
        return false;
    }

    for (const Entry& entry : response.hardwareconfig().entry()) {
        mRemoteProperties[entry.key()] = entry.value();
    }

    mAvdId = mRemoteProperties["avd.id"];
    std::string name = sFAKE_PREFIX + mClient->address() + "_" + mAvdId;

    auto avdDir = android::base::pj(ConfigDirs::getAvdRootDirectory(), name);
    mRemoteProperties["avd.dir"] = avdDir;
    mRemoteProperties["avd.id"] = name;
    mRemoteProperties["avd.name"] = name;

    return true;
}

void FakeAvd::remove() {
    LOG(INFO) << "Cleaning up dirs";
    if (mRemoteProperties.count("avd.id") != 0) {
        std::string name = mRemoteProperties["avd.id"];
        std::string ini = android::base::pj(ConfigDirs::getAvdRootDirectory(),
                                            name + ".ini");

        System::get()->deleteFile(ini);
    }

    if (mRemoteProperties.count("avd.dir") != 0) {
        deleteRecursive(mRemoteProperties["avd.dir"]);
    }
}

void FakeAvd::garbageCollect() {
    auto avds = System::get()->scanDirEntries(ConfigDirs::getAvdRootDirectory(),
                                              true);

    for (const auto& dirname : avds) {
        std::string fname = android::base::pj(dirname, sPIDFILE);
        if (System::get()->pathIsFile(fname)) {
            // Oh oh, a pidfile.. Check if this process has a discovery file.
            android::base::IniFile pidFile(fname);
            if (!pidFile.read() || !pidFile.hasKey("pid") ||
                !pidFile.hasKey("ini")) {
                LOG(WARNING)
                        << "Unable to read pidFile, or missing keys: " << fname;
                continue;
            }
            if (!EmulatorAdvertisement::exists(pidFile.getInt("pid", 0))) {
                // Pid is no longer alive.. Let's clean it up..
                auto iniPath = pidFile.getString("ini", "");
                android::base::IniFile iniFile(iniPath);
                if (!iniFile.read()) {
                    LOG(WARNING) << "Unable to read iniFile: " << iniPath;
                    continue;
                }
                std::string avdPath = iniFile.getString("path", "");
                LOG(INFO) << "Removing: " << iniPath << " and " << avdPath;
                System::get()->deleteFile(iniPath);
                deleteRecursive(avdPath);
            }
        }
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android
