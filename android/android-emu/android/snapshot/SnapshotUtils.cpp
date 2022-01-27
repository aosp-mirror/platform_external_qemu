
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

#include "android/android-emu/android/snapshot/SnapshotUtils.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "android/android.h"
#include "android/avd/info.h"
#include "android/base/Stopwatch.h"
#include "android/base/Uuid.h"  // for Uuid
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/GzipStreambuf.h"
#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/TarStream.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/console.h"
#include "android/crashreport/CrashReporter.h"
#include "android/globals.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshot.h"
#include "android/snapshot/interface.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

namespace android {
namespace emulation {
namespace control {

static constexpr uint32_t k64KB = 64 * 1024;

static void logError(std::string errorMessage,
                     void* opaque,
                     LineConsumerCallback errConsumer) {
    LOG(WARNING) << errorMessage;
    errConsumer(opaque, errorMessage.c_str(), errorMessage.length());
}

bool pullSnapshot(const char* snapshotName,
                  std::streambuf* output,
                  const char* outputDirectory,
                  bool selfContained,
                  FileFormat format,
                  bool deleteAfterPull,
                  void* opaque,
                  LineConsumerCallback errConsumer) {
    auto snapshot = snapshot::Snapshot::getSnapshotById(snapshotName);

    if (!snapshot) {
        // Nope, the snapshot doesn't exist.
        logError(std::string("Could not find ") + snapshotName, opaque,
                 errConsumer);
        return false;
    }

    base::Stopwatch sw;
    std::string tmpdir;
    auto tmpdir_deleter =
            base::makeCustomScopedPtr(&tmpdir, [](std::string* tmpdir) {
                // Best effort to cleanup the mess.
                if (*tmpdir != "") {
                    path_delete_dir(tmpdir->c_str());
                }
            });
    if (format != FileFormat::DIRECTORY) {
        tmpdir = pj(base::System::get()->getTempDir(), snapshot->name());
        android_mkdir(tmpdir.data(), 0700);
    }
    const char* targetDir = outputDirectory;

    crashreport::CrashReporter::get()->hangDetector().pause(true);
    // Exports all qcow2 images..
    bool succeed;
    // Put everything in main thread, to avoid calling export during
    // snapshot operations.
    android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
            [&snapshot, targetDir, tmpdir, &succeed, &sw, format, opaque,
             &errConsumer, output, deleteAfterPull] {
                if (!succeed) {
                    return;
                }
                LOG(VERBOSE)
                        << "Exported snapshot in " << sw.restartUs() << " us";
                // iniFile_saveToFile returns 0 on succeed.
                succeed = !iniFile_saveToFile(
                        avdInfo_getConfigIni(android_avdInfo),
                        base::PathUtils::join(snapshot->dataDir(),
                                              CORE_CONFIG_INI)
                                .c_str());
                if (!succeed) {
                    logError("Failed to save snapshot meta data", opaque,
                             errConsumer);
                    return;
                }

                std::string exportdIniPath = base::PathUtils::join(
                        snapshot->dataDir(), "exported.ini");
                base::IniFile exportedIni(exportdIniPath);
                exportedIni.setString("avdId",
                                      avdInfo_getName(android_avdInfo));
                exportedIni.setString("snasphotName", snapshot->name());
                exportedIni.setString("target",
                                      avdInfo_getTarget(android_avdInfo));
                succeed = exportedIni.write();
                if (!succeed) {
                    logError("Failed to save snapshot meta data", opaque,
                             errConsumer);
                    return;
                }

                if (deleteAfterPull) {
                    base::PathUtils::move(snapshot->dataDir(), targetDir);
                } else {
                    path_copy_dir(targetDir, snapshot->dataDir().data());
                }

                succeed = getConsoleAgents()->vm->snapshotExport(
                        snapshot->name().data(),
                        format == FileFormat::DIRECTORY ? targetDir
                                                        : tmpdir.c_str(),
                        opaque, errConsumer);

                if (format == FileFormat::DIRECTORY) {
                    if (deleteAfterPull) {
                        androidSnapshot_delete(snapshot->name().data());
                    }
                    return;
                }
                // Stream the targetDir out as a tar.gz..

                std::unique_ptr<std::ostream> stream;
                std::ostream* streamPtr = nullptr;
                if (format == TARGZ) {
                    stream = std::make_unique<base::GzipOutputStream>(output);
                    streamPtr = stream.get();
                } else {
                    stream = std::make_unique<std::ostream>(output);
                    streamPtr = stream.get();
                }

                // Use of  a 64 KB  buffer gives good performance (see
                // performance tests.)
                base::TarWriter tw(targetDir, *streamPtr, k64KB);
                tw.addDirectory(".");
                if (tw.fail()) {
                    logError(tw.error_msg(), opaque, errConsumer);
                    succeed = false;
                    return;
                }
                LOG(VERBOSE)
                        << "Completed writing in " << sw.restartUs() << " us";
                // Now add in the metadata.
                auto entries = base::System::get()->scanDirEntries(
                        snapshot->dataDir(), true);
                for (const auto& fname : entries) {
                    if (!base::System::get()->pathIsFile(fname)) {
                        continue;
                    }
                    struct stat sb;
                    android::base::StringView name;
                    char buf[k64KB];
                    base::PathUtils::split(fname, nullptr, &name);

                    // Use of  a 64 KB  buffer gives good performance (see
                    // performance tests.)
                    std::ifstream ifs(
                            fname, std::ios_base::in | std::ios_base::binary);
                    ifs.rdbuf()->pubsetbuf(buf, sizeof(buf));
                    LOG(VERBOSE) << "Zipping " << name;
                    if (android_stat(fname.c_str(), &sb) != 0 ||
                        !tw.addFileEntryFromStream(ifs, name, sb)) {
                        logError("Unable to tar " + fname, opaque, errConsumer);
                        succeed = false;
                        break;
                    }
                }
                LOG(VERBOSE) << "Wrote metadata in " << sw.restartUs() << " us";

                tw.close();
                if (tw.fail()) {
                    logError(tw.error_msg(), opaque, errConsumer);
                }
                if (deleteAfterPull) {
                    androidSnapshot_delete(snapshot->name().data());
                }
            });

    crashreport::CrashReporter::get()->hangDetector().pause(false);
    return succeed;
}

bool pushSnapshot(const char* snapshotName,
                  std::istream* input,
                  const char* inputDirectory,
                  FileFormat format,
                  void* opaque,
                  LineConsumerCallback errConsumer) {
    // Create a temporary directory for the snapshot..
    std::string id = base::Uuid::generate().toString();
    std::string finalDest = snapshot::getSnapshotDir(snapshotName);
    if (base::System::get()->pathExists(finalDest) &&
        path_delete_dir(finalDest.c_str()) != 0) {
        logError(std::string("Failed to delete: ") + finalDest, opaque,
                 errConsumer);
        return false;
    }

    if (format == DIRECTORY) {
        if (0 != path_copy_dir(finalDest.c_str(), inputDirectory)) {
            logError(std::string("Failed to copy: ") + inputDirectory +
                             " --> " + finalDest,
                     opaque, errConsumer);
            return false;
        }
    } else {
        std::string tmpSnap = snapshot::getSnapshotDir(id.c_str());

        const auto tmpdir_deleter = base::makeCustomScopedPtr(
                &tmpSnap,
                [](std::string* tmpSnap) {  // Best effort to cleanup the mess.
                    path_delete_dir(tmpSnap->c_str());
                });

        std::unique_ptr<base::GzipInputStream> gzipInputStream;
        std::istream* actualInputStream;

        if (format == TARGZ) {
            gzipInputStream = std::make_unique<base::GzipInputStream>(*input);
            actualInputStream = gzipInputStream.get();
        } else {
            actualInputStream = input;
        }

        base::TarReader tr(tmpSnap, *actualInputStream);
        for (auto entry = tr.first(); tr.good(); entry = tr.next(entry)) {
            tr.extract(entry);
        }

        if (tr.fail()) {
            logError(tr.error_msg(), opaque, errConsumer);
            return false;
        }

        if (!android::base::PathUtils::move(tmpSnap.c_str(),
                                            finalDest.c_str())) {
            logError(std::string("Failed to rename: ") + tmpSnap + " --> " +
                             finalDest,
                     opaque, errConsumer);
            return false;
        }
    }

    // Okay, now we have to fix up (i.e. import) the snapshot
    auto snapshot = android::snapshot::Snapshot::getSnapshotById(snapshotName);
    if (!snapshot) {
        logError("Snasphot incompatible", opaque, errConsumer);
        return false;
    }

    if (!snapshot->fixImport()) {
        logError("Failed to fix import", opaque, errConsumer);
        // Best effort to cleanup mess.
        path_delete_dir(finalDest.c_str());
        return false;
    }

    return true;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
