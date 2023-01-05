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
#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

#include "android/featurecontrol/features_c.h"

// Call this function first to initialize the feature control.
void feature_initialize();

// Get the access rules given by |name| if they exist, otherwise returns NULL
bool feature_is_enabled(Feature feature);
bool feature_is_enabled_by_guest(Feature feature);
void feature_set_enabled_override(Feature feature, bool isEnabled);
void feature_reset_enabled_to_default(Feature feature);

// Set the feature if it is not user-overriden.
void feature_set_if_not_overridden(Feature feature, bool enable);

// Set the feature if it is not user-overriden or disabled from the guest.
void feature_set_if_not_overridden_or_guest_disabled(Feature feature, bool enable);

// Runs applyCachedServerFeaturePatterns then
// asyncUpdateServerFeaturePatterns. See FeatureControl.h
// for more info. To be called only once on startup.
void feature_update_from_server();

ANDROID_END_HEADER
