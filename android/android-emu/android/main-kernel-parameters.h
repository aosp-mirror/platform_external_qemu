// Copyright 2016 The Android Open Source Project
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

#include "android/cmdline-option.h"
#include "android/utils/compiler.h"
#include "host-common/opengl/emugl_config.h"

#include <stdint.h>
#include <string>
#include <vector>

ANDROID_BEGIN_HEADER

typedef struct mem_map {
  uint64_t start;
  uint64_t size;
} mem_map;

// Return a string containing the kernel parameter.
// |opts| corresponds to the command-line options after they have been
// processed by emulator_parseCommonCommandLineOptions().
// |targetArch| is the target architecture. (e.g. 'arm64')
// |apiLevel| is the AVD's API level.
// |kernelSerialPrefix| is the guest tty device prefix (e.g. 'ttyS')
// |imageKernelParameters| the system image provided kernel command line, optional
// |avdKernelParameters| are the optional extra kernel parameters stored
// in the AVD's kernel.parameters hardware property, if any. They will
// be appended to the result.
// |verifiedBootParameters| is a vector of verified boot kernel parameters to
// add.  Passing NULL is OK.  Passing an empty vector is OK.
// |ramoops| The memory range that will be used by the ramoops module.
// |isCros| is true to indicate that it's a Chrome OS image.
std::string emulator_getKernelParameters(const AndroidOptions* opts,
                                         const char* targetArch,
                                         int apiLevel,
                                         const char* kernelSerialPrefix,
                                         const char* imageKernelParameters,
                                         const char* avdKernelParameters,
                                         const char* kernelPath,
                                         const std::vector<std::string>* verifiedBootParameters,
                                         uint64_t glFramebufferSizeBytes,
                                         mem_map ramoops,
                                         bool isCros,
                                         std::vector<std::string> userspaceBootProps);

ANDROID_END_HEADER
