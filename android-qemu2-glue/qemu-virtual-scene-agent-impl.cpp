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

#include "android/emulation/control/virtual_scene_agent.h"

#include "android/virtualscene/VirtualSceneManager.h"

using android::virtualscene::VirtualSceneManager;

static const QAndroidVirtualSceneAgent sQAndroidVirtualSceneAgent = {
    VirtualSceneManager::setInitialPoster,
    VirtualSceneManager::loadPoster,
    VirtualSceneManager::enumeratePosters,
    VirtualSceneManager::setPosterScale,
    VirtualSceneManager::setAnimationState,
    VirtualSceneManager::getAnimationState
};

extern "C" const QAndroidVirtualSceneAgent* const gQAndroidVirtualSceneAgent =
        &sQAndroidVirtualSceneAgent;
