// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/MemoryWatch.h"

hva2gpa_t hva2gpa_call = 0;
gpa2hva_t gpa2hva_call = 0;

void set_address_translation_funcs(
        hva2gpa_t hva2gpa,
        gpa2hva_t gpa2hva) {
    hva2gpa_call = hva2gpa;
    gpa2hva_call = gpa2hva;
}
