// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/StringView.h"

#include <fstream>
#include <string>
#include <vector>

namespace android {
namespace base {

class FileTail {
public:
    FileTail(android::base::StringView filePath);

    std::string get();
    std::vector<std::string> get(unsigned numLines);
    bool good() const { return !mErrorSeen; }

    static const unsigned kSeekChunkSize;

private:
    const std::string mFilePath;
    std::ifstream mFile;
    bool mErrorSeen = false;

    DISALLOW_COPY_AND_ASSIGN(FileTail);
};

}  // namespace base
}  // namespace android
