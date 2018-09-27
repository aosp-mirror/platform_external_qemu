// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

#include <string>
#include <vector>

namespace android {
namespace verifiedboot {

enum class Status {
    OK = 0,  // No Errors

    CouldNotOpenFile = 1,    // Could not open config file
    EmptyParameter = 2,      // An empty Parameter was provided
    IllegalCharacter = 3,    // A Parameter contained an illegal character
    MissingDMParam = 4,      // No dm-verity paramters were provided
    MissingVersion = 5,      // Version information is missing
    ParseError = 6,          // Errors parsing the proto file
    UnsupportedVersion = 7,  // Version is unsupported
};

// Reads verified boot parameters from the given pathname
// and formats them into |params|.
//
// It is legal for |pathname| to be NULL.  In that case,
// Status::CouldNotOpenFile is returned.
//
// If any of the following issues occur, the vector is not populated and
// the function returns an error Status:
//   - File is not found or can not be read.
//   - Parsing the file fails.
//   - The file's version is unsupported.
//   - Insufficient paramater counts.
//   - Illegal characters within parameters.
//
// In each case above, more information about the issue will be written to the
// debug logs.
Status getParametersFromFile(const char* pathname,
                             std::vector<std::string>* params);

// Reads parameters from the provided |textproto|.  This function is used
// to provide the text proto directly, such as in unit testing.
// Outside of the data source, operation is identical to getParameters()
// described above.
Status getParameters(const std::string& textproto,
                     std::vector<std::string>* params);

}  // namespace verifiedboot
}  // namespace android
