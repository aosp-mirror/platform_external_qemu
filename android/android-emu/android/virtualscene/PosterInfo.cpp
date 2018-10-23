// Copyright (C) 2018 The Android Open Source Project
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

#include "android/virtualscene/PosterInfo.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/physics/GlmHelpers.h"
#include "android/utils/debug.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <fstream>
#include <vector>

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

namespace android {
namespace virtualscene {

std::vector<PosterInfo> parsePostersFile(
        const char* filename) {
    const std::string resourcesDir =
            PathUtils::addTrailingDirSeparator(PathUtils::join(
                    System::get()->getLauncherDirectory(), "resources"));
    const std::string filePath = PathUtils::join(
            System::get()->getLauncherDirectory(), "resources", filename);

    std::ifstream in(filePath);
    if (!in) {
        W("%s: Could not find file '%s'", __FUNCTION__, filename);
        return {};
    }

    std::vector<PosterInfo> results;
    PosterInfo poster;

    std::string str;

    for (in >> str; !in.eof(); in >> str) {
        if (str.empty()) {
            continue;
        }

        if (str == "poster") {
            // New poster entry, specified with a string name.
            if (!poster.name.empty()) {
                // Store existing poster.
                D("%s: Loaded poster %s at (%f, %f, %f)", __FUNCTION__,
                  poster.name.c_str(), poster.position.x, poster.position.y,
                  poster.position.z);
                results.push_back(poster);
            }

            poster = PosterInfo();
            in >> poster.name;
            if (!in) {
                W("%s: Invalid name.", __FUNCTION__);
                return {};
            }

        } else if (str == "position") {
            // Poster center position.
            // Specified with three floating point numbers, separated by
            // whitespace.

            in >> poster.position.x >> poster.position.y >> poster.position.z;
            if (!in) {
                W("%s: Invalid position.", __FUNCTION__);
                return {};
            }
        } else if (str == "rotation") {
            // Poster rotation.
            // Specified with three floating point numbers, separated by
            // whitespace.  This represents euler angle rotation in degrees, and
            // it is applied in XYZ order.

            glm::vec3 eulerRotation;
            in >> eulerRotation.x >> eulerRotation.y >> eulerRotation.z;
            if (!in) {
                W("%s: Invalid rotation.", __FUNCTION__);
                return {};
            }

            poster.rotation = fromEulerAnglesXYZ(glm::radians(eulerRotation));
        } else if (str == "size") {
            // Poster center position.
            // Specified with two floating point numbers, separated by
            // whitespace.

            in >> poster.size.x >> poster.size.y;
            if (!in) {
                W("%s: Invalid size.", __FUNCTION__);
                return {};
            }
        } else if (str == "default") {
            // Poster default filename.
            // Specified with a string parameter.

            in >> poster.defaultFilename;
            if (!in) {
                W("%s: Invalid default filename.", __FUNCTION__);
                return {};
            }
        } else {
            W("%s: Invalid input %s", __FUNCTION__, str.c_str());
            return {};
        }
    }

    if (poster.name.empty()) {
        E("%s: Posters file did not contain any entries.", __FUNCTION__);
        return {};
    }

    // Store last poster.
    D("%s: Loaded poster %s at (%f, %f, %f)", __FUNCTION__, poster.name.c_str(),
      poster.position.x, poster.position.y, poster.position.z);
    results.push_back(poster);

    return results;
}

}  // namespace virtualscene
}  // namespace android
