// Copyright 2018 The Android Open Source Project
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
#include <cutils/properties.h>

#include "AndroidHostCommon.h"

#include <string.h>

extern "C" {

EXPORT int property_get(const char* key, char* value, const char* default_value) {
    const char ro_kernel_qemu_gles_prop[] = "1";
    const char qemu_sf_lcd_density[] = "420";
    int len = 0;

    if (!strcmp(key, "ro.kernel.qemu.gles")) {
        len = strlen(ro_kernel_qemu_gles_prop);
        memcpy(value, ro_kernel_qemu_gles_prop, len + 1);
    } else if (!strcmp(key, "qemu.sf.lcd_density")) {
        len = strlen(qemu_sf_lcd_density);
        memcpy(value, qemu_sf_lcd_density, len + 1);
    } else if (default_value) {
        len = strlen(qemu_sf_lcd_density);
        memcpy(value, default_value, len + 1);
    }

    return len;
}

} // extern "C"
