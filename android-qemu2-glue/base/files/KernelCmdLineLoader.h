// Copyright 2024 The Android Open Source Project
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
#include <string>
#include "android/avd/avd-info.h"

namespace android {
namespace qemu {

/**
 * @brief Loads the kernel command line from the specified file path.
 *
 * This function attempts to open the file at the given path, reads its contents into a string,
 * and trims any trailing ASCII whitespace. If the file cannot be opened or the path is null,
 * an empty string is returned.
 *
 * @param kernelCmdLinePath The path to the file containing the kernel command line.
 * @return The kernel command line as a string, or an empty string if the file cannot be opened or the path is null.
 */
std::string loadKernelCmdLine(const char* kernelCmdLinePath);

/**
 * @brief Loads the kernel command line for the given AVD.
 *
 * This function attempts to open the file at the given path, reads its contents into a string,
 * and trims any trailing ASCII whitespace. If the file cannot be opened or the path is null,
 * an empty string is returned.
 *
 * @param kernelCmdLinePath The path to the file containing the kernel command line.
 * @return The kernel command line as a string, or an empty string if the file cannot be opened or the path is null.
 */
std::string loadKernelCmdLineFromAvd(const AvdInfo* avd);
}  // namespace qemu
}  // namespace android