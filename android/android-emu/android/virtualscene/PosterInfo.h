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

#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/utils/compiler.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace android {
namespace virtualscene {

static constexpr float kPosterMinimumSizeMeters = 0.2f;

struct PosterInfo {
    std::string name;
    glm::vec2 size = glm::vec2(1.0f, 1.0f);
    glm::vec3 position = glm::vec3();
    glm::quat rotation = glm::quat();
    std::string defaultFilename;
};

// Parse a .posters file and return the information about all the posters
// in the file.
//
// |filename| - Filename to load from.
std::vector<PosterInfo> parsePostersFile(const char* filename);

}  // namespace virtualscene
}  // namespace android
