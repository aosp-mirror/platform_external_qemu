/*
* Copyright (C) 2019 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "emugl/common/misc.h"
#include <cassert>

namespace {

QAndroidEmulatorWindowAgent g_window_operations;
QAndroidMultiDisplayAgent g_multi_display_operations;
static bool g_window_initialized = false;
static bool g_multi_display_initialized = false;

}  // namespace

void emugl::set_emugl_window_operations(const QAndroidEmulatorWindowAgent &operations)
{
    g_window_operations = operations;
    g_window_initialized = true;
}

const QAndroidEmulatorWindowAgent &emugl::get_emugl_window_operations()
{
    assert(g_window_initialized);
    return g_window_operations;
}

void emugl::set_emugl_multi_display_operations(const QAndroidMultiDisplayAgent &operations) {
    g_multi_display_operations = operations;
    g_multi_display_initialized = true;
}

const QAndroidMultiDisplayAgent &emugl::get_emugl_multi_display_operations() {
    assert(g_multi_display_initialized);
    return g_multi_display_operations;
}
