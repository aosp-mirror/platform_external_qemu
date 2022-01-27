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

#include "android/emulation/control/callbacks.h"

#include <fstream>

namespace android {
namespace emulation {
namespace control {

enum FileFormat {
    TAR,
    TARGZ,
    DIRECTORY,
};

// Exports a snapshot.
// Args:
//   snapshotName: snapshot ID.
//   output: output streambuf, usually points to a file, or gRPC buffer. Only
//           used if format is not DIRECTORY.
//   outputDirectory: output directory used when format is DIRECTORY.
//   selfContained: TODO(b/196863626) set to true to export a self-contained
//                  snapshot which wraps up an SDK downloader and setup
//                  script.
//   format: TARGZ (compressed) or TAR (not compressed).
//   opaque: opaque object for errConsumer.
//   errConsumer: consumes error messages.
bool pullSnapshot(const char* snapshotName,
                  std::streambuf* output,
                  const char* outputDirectory,
                  bool selfContained,
                  FileFormat format,
                  bool deleteSnapshotAfterPull,
                  void* opaque,
                  LineConsumerCallback errConsumer);

bool pushSnapshot(const char* snapshotName,
                  std::istream* input,
                  const char* inputDirectory,
                  FileFormat format,
                  void* opaque,
                  LineConsumerCallback errConsumer);

}  // namespace control
}  // namespace emulation
}  // namespace android