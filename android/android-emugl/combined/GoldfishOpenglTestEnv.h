// Copyright (C) 2018 The Android Open Source Project
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

#include <vector>
#include "android/emulation/testing/TemporaryCommandLineOptions.h"

namespace android {
namespace base {

class Stream;
class MemStream;

} // namespace base

namespace snapshot {

class TextureSaver;
class TextureLoader;

} // namespace snapshot

} // namespace android

class GoldfishOpenglTestEnv {
public:
    static std::vector<const char*> getTransportsToTest();

    GoldfishOpenglTestEnv();
    ~GoldfishOpenglTestEnv();

    static GoldfishOpenglTestEnv* get();

    void clear();

    void saveSnapshot(android::base::Stream* stream);
    void loadSnapshot(android::base::Stream* stream);

    android::base::MemStream* snapshotStream = nullptr;
private:
    AndroidOptions mTestEnvCmdLineOptions{};
    TemporaryCommandLineOptions mTcl{&mTestEnvCmdLineOptions};
};
