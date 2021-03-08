// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <string>

namespace android {
namespace icebox {
// Start tracking pid for exceptions. Save a snapshot under
// snapshot_name if unexpected error happens.
extern bool track(int pid,
                  const std::string snapshot_name,
                  int max_snapshot_number);
extern bool run_async(const std::string cmd);
extern bool track_async(int pid,
                        const std::string snapshot_name,
                        int max_snapshot_number);
extern void set_jdwp_port(int adb_port);
}  // namespace icebox
}  // namespace android