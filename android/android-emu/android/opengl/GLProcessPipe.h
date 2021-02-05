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

#include <functional>

namespace android {
namespace opengl {

// registerGLProcessPipeService() registers a "GLProcessPipe" pipe service that is
// used to detect GL process exits.
void registerGLProcessPipeService();

void forEachProcessPipeId(std::function<void(uint64_t)>);
void forEachProcessPipeIdRunAndErase(std::function<void(uint64_t)>);

}  // namespace opengl
}  // namespace android
