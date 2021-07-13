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

ANDROID_BEGIN_HEADER

std::vector<std::pair<std::string, std::string>>
getUserspaceBootProperties(const AndroidOptions* opts,
                           const char* targetArch,
                           const char* serialno,
                           bool isQemu2,
                           uint32_t lcd_width,
                           uint32_t lcd_height,
                           uint32_t lcd_vsync,
                           AndroidGlesEmulationMode glesMode,
                           int bootPropOpenglesVersion,
                           int vm_heapSize,
                           int apiLevel,
                           const char* kernelSerialPrefix,
                           const std::vector<std::string>* verifiedBootParameters,
                           const char* gltransport,
                           uint32_t gltransport_drawFlushInterval,
                           const char* displaySettingsXml,
                           const char* avdName);

ANDROID_END_HEADER
