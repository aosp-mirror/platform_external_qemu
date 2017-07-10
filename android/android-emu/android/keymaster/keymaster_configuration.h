/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_KEYMASTER_CONFIGURATION_H_
#define SYSTEM_KEYMASTER_KEYMASTER_CONFIGURATION_H_

#include <string>

#include <stdint.h>

#include <hardware/keymaster2.h>
#include <hardware/keymaster_defs.h>

namespace keymaster {

/**
 * Retrieves OS version information from system build properties and configures the provided
 * keymaster device.
 */
keymaster_error_t ConfigureDevice(keymaster2_device_t* dev);

/**
 * Parses OS version string, returning in integer form. For example, "6.1.2" will be returned as
 * 60102.  Ignores any non-numeric suffix, and allows short build numbers, e.g. "6" -> 60000 and
 * "6.1" -> 60100. Returns 0 if the string doesn't contain a numeric version number.
 */
uint32_t GetOsVersion(const char* version_string);

/**
 * Parses OS patch level string, returning year and month in integer form. For example, "2016-03-25"
 * will be returned as 201603. Returns 0 if the string doesn't contain a date in the form
 * YYYY-MM-DD.
 */
uint32_t GetOsPatchlevel(const char* patchlevel_string);

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEYMASTER_CONFIGURATION_H_
