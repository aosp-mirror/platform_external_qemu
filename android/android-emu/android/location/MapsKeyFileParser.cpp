// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/location/MapsKeyFileParser.h"

#include <fstream>
#include <sstream>
#include <string>

namespace android {
namespace location {

std::string parseMapsKeyFromFile(const base::StringView& file) {
    // File is in the following format:
    //
    // xx xx xx xx ...
    //
    // where xx is a two-digit hex value.
    std::ifstream fin;
    std::string apikey;

    fin.open(file, std::ifstream::in);
    if (!fin.good()) {
        return "";
    }

    for (;;) {
        int hexval;
        std::string str;
        fin >> str;
        if (str.empty()) {
            break;
        }
        std::istringstream(str) >> std::hex >> hexval;
        apikey.push_back((char)hexval);
    }

    return apikey.c_str();
}

}  // namespace location
}  // namespace android

