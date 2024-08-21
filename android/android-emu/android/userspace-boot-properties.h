// Copyright 2021 The Android Open Source Project
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

#include <string>                         // for string
#include <utility>                        // for pair
#include <vector>                         // for vector

#include "host-common/hw-config.h"        // for AndroidHwConfig
#include "android/cmdline-option.h"       // for AndroidOptions
#include "host-common/opengl/emugl_config.h"  // for AndroidGlesEmulationMode
#include "android/utils/compiler.h"       // for ANDROID_BEGIN_HEADER, ANDRO...

ANDROID_BEGIN_HEADER

std::vector<std::pair<std::string, std::string>>
getUserspaceBootProperties(const AndroidOptions* opts,
                           const char* targetArch,
                           const char* serialno,
                           AndroidGlesEmulationMode glesMode,
                           int bootPropOpenglesVersion,
                           int apiLevel,
                           const char* kernelSerialPrefix,
                           const std::vector<std::string>* verifiedBootParameters,
                           const AndroidHwConfig* hw);

ANDROID_END_HEADER
