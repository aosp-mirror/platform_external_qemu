/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "android/videoplayback/VideoplaybackUtils.h"

#include "android/base/Log.h"

#include <string>
#include <vector>

const static size_t kNumMetadataEntries = 4;

namespace android {
namespace videoplayback {

base::Optional<size_t> parsePPMHeader(PPMInfo* info,
                                      const unsigned char* data,
                                      size_t dataLen) {
    std::vector<std::string> tokens;
    int numTokensParsed = 0;

    int idx = 0;
    int tokStartIdx = 0;
    int tokEndIdx = 0;
    while (numTokensParsed < kNumMetadataEntries) {
        tokStartIdx = idx;
        while (!isspace(static_cast<char>(data[idx]))) {
            idx++;
        }
        tokEndIdx = idx;
        tokens.push_back(std::string((const char*)(&data[tokStartIdx]),
                                     (const char*)(&data[tokEndIdx])));
        numTokensParsed++;

        while (isspace(static_cast<char>(data[idx]))) {
          idx++;
        }
    }

    if (idx >= dataLen) {
        LOG(ERROR) << "Frame is not a PPM image: Missing data section.";
        return base::kNullopt;
    }

    if (tokens[0].compare("P6") != 0) {
        LOG(ERROR) << "Frame is not a PPM image: Expected magic number is P6 "
                   << "but got " << tokens[0].c_str();
        return base::kNullopt;
    }
    info->width = std::stoi(tokens[1]);
    info->height = std::stoi(tokens[2]);
    info->maxColor = std::stoi(tokens[3]);

    return base::makeOptional<size_t>(idx);
}

}  // namespace videoplayback
}  // namespace android
