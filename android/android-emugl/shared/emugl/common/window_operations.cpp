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

namespace {

QAndroidEmulatorWindowAgent g_window_operations;

}  // namespace

void emugl::set_emugl_window_operations(const QAndroidEmulatorWindowAgent &window_operations)
{
    g_window_operations = window_operations;
}

const QAndroidEmulatorWindowAgent &emugl::get_emugl_window_operations()
{
    return g_window_operations;
}
