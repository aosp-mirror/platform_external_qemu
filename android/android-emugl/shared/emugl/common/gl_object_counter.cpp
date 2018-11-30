/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "emugl/common/gl_object_counter.h"

static void default_emugl_object_counter(int type) {}

namespace emugl {

emugl_counter_t emugl_object_count_inc = default_emugl_object_counter;
emugl_counter_t emugl_object_count_dec = default_emugl_object_counter;

void set_emugl_gl_object_counter(emugl_counter_struct counter) {
    emugl_object_count_inc = counter.inc;
    emugl_object_count_dec = counter.dec;
}

}