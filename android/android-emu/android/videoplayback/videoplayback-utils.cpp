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

#include "android/videoplayback/videoplayback-utils.h"

#include "android/base/Log.h"

#include <regex>
#include <string>
#include <vector>

namespace android {
namespace videoplayback {

int parsePPMHeader(PPMInfo* info, const unsigned char* data, int dataLen) {
    std::regex whitespaceRegex("\\s+");
    std::regex_token_iterator<const char*> first((const char*)data,
                                                 (const char*)data + dataLen,
                                                 whitespaceRegex, -1),
            last;
    std::vector<std::string> tokens(first, last);
    if (tokens.size() != 5) {
        LOG(ERROR) << "Frame is not a PPM image: Wrong number of tokens. "
                   << "Expected 5 but got " << tokens.size();
        return -1;
    }

    if (tokens[0].compare("P6") != 0) {
      LOG(ERROR) << "Frame is not a PPM image: Expected magic number is P6 "
                 << "but got " << tokens[0].c_str();
      return -1;
    }
    info->width = std::stoi(tokens[1]);
    info->height = std::stoi(tokens[2]);
    info->maxColor = std::stoi(tokens[3]);

    return dataLen - tokens[4].size();
}

}  // namespace videoplayback
}  // namespace android
