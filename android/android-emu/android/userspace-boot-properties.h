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

#include <string>
#include <vector>
#include "android/cmdline-option.h"
#include "android/opengl/emugl_config.h"
#include "android/avd/hw-config.h"

ANDROID_BEGIN_HEADER

std::vector<std::pair<std::string, std::string>>
getUserspaceBootProperties(const AndroidOptions* opts,
                           const char* targetArch,
                           const char* serialno,
                           bool isQemu2,
                           AndroidGlesEmulationMode glesMode,
                           int bootPropOpenglesVersion,
                           int apiLevel,
                           const char* kernelSerialPrefix,
                           const std::vector<std::string>* verifiedBootParameters,
                           const AndroidHwConfig* hw);

ANDROID_END_HEADER
