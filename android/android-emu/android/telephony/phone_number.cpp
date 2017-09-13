/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include <string>

#include "android/telephony/phone_number.h"

#include "android/cmdline-option.h"

#define  DEFAULT_PHONE_PREFIX "1555521"

extern "C" const char* get_phone_number_prefix() {
  static std::string phone_prefix;

  if (phone_prefix.empty()) {
    if (android_cmdLineOptions && android_cmdLineOptions->phone_number_prefix != nullptr) {
      phone_prefix = android_cmdLineOptions->phone_number_prefix;
    }
    phone_prefix = DEFAULT_PHONE_PREFIX;
  }
  return phone_prefix.c_str();
}
