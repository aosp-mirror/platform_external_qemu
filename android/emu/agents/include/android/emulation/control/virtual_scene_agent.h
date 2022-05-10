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

ANDROID_BEGIN_HEADER

// Callback called to enumerate posters and their current values in the scene.
//
// |posterName| - Name of the poster position, such as "wall" or "table".
// |minWidth| - The poster position's minimum width, in meters.
// |maxWidth| - The poster position's maximum width, in meters.
// |filename| - Path to an image file, either PNG or JPEG, or nullptr if none
//              is set.
// |scale| - The poster scale, a value between 0 and 1.
typedef void (*EnumeratePostersCallback)(void* context,
                                         const char* posterName,
                                         float minWidth,
                                         float maxWidth,
                                         const char* filename,
                                         float scale);

typedef struct QAndroidVirtualSceneAgent {
    // Set the initial poster of the scene, loaded from persisted settings.
    // Command line flags take precedence, so if the -virtualscene-poster flag
    // has been specified for the posterName this call will not replace it.
    //
    // |posterName| - Name of the poster position, such as "wall" or "table".
    // |filename| - Path to an image file, either PNG or JPEG, or nullptr
    //              to set to default.
    void (*setInitialPoster)(const char* posterName, const char* filename);

    // Load a poster into the scene from a file.
    //
    // |posterName| - Name of the poster position, such as "wall" or "table".
    // |filename| - Path to an image file, either PNG or JPEG, or nullptr
    //              to set to default.
    //
    // Returns true if the poster name was valid and the image will be loaded.
    bool (*loadPoster)(const char* posterName, const char* filename);

    // Enumerate posters in the scene and their current values.  Synchronously
    // calls the callback for each poster in the scene.
    //
    // |context| - Context object, passed into callback.
    // |callback| - Callback to be invoked for each poster.
    void (*enumeratePosters)(void* context, EnumeratePostersCallback callback);

    // Set the scale of a poster.
    //
    // |posterName| - Name of the poster position, such as "wall" or "table".
    // |scale| - Poster scale, a value between 0 and 1. The value will be
    //          clamped between the poster's minimum size and 1.
    void (*setPosterScale)(const char* posterName, float scale);

    // Enable/Disable the TV animation.
    //
    // |state| - State of the TV animation. True to enable TV animation, false
    // to disable.
    void (*setAnimationState)(bool state);

    // Get the TV animation state.
    //
    // Returns true for enabled, false for disabled.
    bool (*getAnimationState)();
} QAndroidVirtualSceneAgent;

ANDROID_END_HEADER
