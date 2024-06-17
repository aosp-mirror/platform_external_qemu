
// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/skin/event.h"

#ifdef __cplusplus
extern "C" {
#endif

// Formats a SkinEvent into a string, note that you will have to free the
// returned string. This is mainly used for debugging.
char* format_skin_event(SkinEvent* event);

#ifdef __cplusplus
}
#endif