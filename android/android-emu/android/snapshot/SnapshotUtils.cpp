
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
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/GzipStreambuf.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/TarStream.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/console.h"
#include "android/crashreport/CrashReporter.h"
#include "android/globals.h"
#include "android/snapshot/Snapshot.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

namespace android {
namespace emulation {
namespace control {

static constexpr uint32_t k64KB = 64 * 1024;

bool pullSnapshot(const char* snapshotName,
                  std::streambuf* output,
                  bool selfContained,
                  FileFormat format,
                  void* opaque,
                  LineConsumerCallback errConsumer) {
    auto snapshot = snapshot::Snapshot::getSnapshotById(snapshotName);

    if (!snapshot) {
        // Nope, the snapshot doesn't exist.
        std::string errorMessage =
                std::string("Could not find ") + snapshotName;
        errConsumer(opaque, errorMessage.c_str(), errorMessage.length());
        return false;
    }

    base::Stopwatch sw;
    auto tmpdir = pj(base::System::get()->getTempDir(), snapshot->name());
    const auto tmpdir_deleter =
            base::makeCustomScopedPtr(&tmpdir, [](std::string* tmpdir) {
                // Best effort to cleanup the mess.
                path_delete_dir(tmpdir->c_str());
            });
    android_mkdir(tmpdir.data(), 0700);

    crashreport::CrashReporter::get()->hangDetector().pause(true);
    // Exports all qcow2 images..
    bool succeed;
    // Put everything in main thread, to avoid calling export during
    // snapshot operations.
    android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
            [&snapshot, &tmpdir, &succeed, &sw, format, opaque, &errConsumer,
             output] {
                succeed = getConsoleAgents()->vm->snapshotExport(
                        snapshot->name().data(), tmpdir.data(), opaque,
                        errConsumer);
                if (!succeed) {
                    return;
                }
                LOG(VERBOSE)
                        << "Exported snapshot in " << sw.restartUs() << " us";
                // Stream the tmpdir out as a tar.gz..

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
                base::TarWriter tw(tmpdir, *streamPtr, k64KB);
                tw.addDirectory(".");
                if (tw.fail()) {
                    errConsumer(opaque, tw.error_msg().c_str(),
                                tw.error_msg().length());
                    succeed = false;
                    return;
                }
                LOG(VERBOSE)
                        << "Completed writing in " << sw.restartUs() << " us";
                // iniFile_saveToFile returns 0 on succeed.
                succeed = !iniFile_saveToFile(
                        avdInfo_getConfigIni(android_avdInfo),
                        base::PathUtils::join(snapshot->dataDir(),
                                              CORE_CONFIG_INI)
                                .c_str());
                if (!succeed) {
                    const char* errorMessage =
                            "Failed to save snapshot meta data";
                    LOG(VERBOSE) << errorMessage;
                    errConsumer(opaque, errorMessage, strlen(errorMessage));
                    return;
                }

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
                        std::string errorMessage = "Unable to tar " + fname;
                        errConsumer(opaque, errorMessage.c_str(),
                                    errorMessage.length());
                        succeed = false;
                        break;
                    }
                }
                LOG(VERBOSE) << "Wrote metadata in " << sw.restartUs() << " us";

                tw.close();
                if (tw.fail()) {
                    errConsumer(opaque, tw.error_msg().c_str(),
                                tw.error_msg().length());
                }
            });

    crashreport::CrashReporter::get()->hangDetector().pause(false);
    return succeed;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
