/* Copyright (C) 2016 The Android Open Source Project
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

#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/emulation/control/test_runner.h"

static void listTests(char* buf, unsigned int* sz) {
    android_test_runner_list_tests(buf, sz);
}

static void getResultLocation(char* buf, unsigned int* sz) {
    android_test_runner_get_result_location(buf, sz);
}

static void addTest(const char* test_id, android_test_function_t func) {
    android_test_runner_add_test(test_id, func);
}

static bool runTest(const char* test_id) {
    return android_test_runner_run_test(test_id);
}

static const QAndroidTestAgent sQAndroidTestAgent = {
    .listTests = listTests,
    .getResultLocation = getResultLocation,
    .addTest = addTest,
    .runTest = runTest,
};

const QAndroidTestAgent* const gQAndroidTestAgent = &sQAndroidTestAgent;
