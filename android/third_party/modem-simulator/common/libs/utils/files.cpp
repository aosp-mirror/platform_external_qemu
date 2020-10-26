/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "files.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

#include <array>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace cuttlefish {

bool FileExists(const std::string& path) {
    return android::base::System::get()->pathExists(path);
}

bool FileHasContent(const std::string& path) {
    android::base::System::FileSize fileSize;
    android::base::System::get()->pathFileSize(path, &fileSize);
    return (fileSize > 0);
}

std::string AbsolutePath(const std::string& path) {
    char* abspath = path_get_absolute(path.c_str());
    if (abspath == nullptr) {
        return std::string{};
    }
    std::string spath(abspath);
    free(abspath);
    abspath = nullptr;
    return spath;
}

std::string ReadFile(const std::string& file) {
    std::string contents;
    std::ifstream in(android::base::PathUtils::asUnicodePath(file).c_str(),
                     std::ios::in | std::ios::binary);
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return (contents);
}

}  // namespace cuttlefish
