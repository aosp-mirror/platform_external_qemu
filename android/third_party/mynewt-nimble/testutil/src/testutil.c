/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "os/mynewt.h"
#include "testutil/testutil.h"
#include "testutil_priv.h"
#include "nimble/nimble_npl.h"
#include "nimble/os_types.h"
#include "sysinit/sysinit.h"
#include "aemu/base/logging/CLog.h"

#define OS_MAIN_STACK_SIZE 1024
#define OS_MAIN_TASK_PRIO 0

/* The test task runs at a lower priority (greater number) than the default
 * task.  This allows the test task to assume events get processed as soon as
 * they are initiated.  The test code can then immediately assert the expected
 * result of event processing.
 */
#define TU_TEST_TASK_PRIO   (MYNEWT_VAL(OS_MAIN_TASK_PRIO) + 1)
#define TU_TEST_STACK_SIZE  1024

struct tu_config tu_config;

int tu_any_failed;

struct ts_testsuite_list *ts_suites;

bool
equal_within_tolerance(int left, int right, int tolerance)
{
    return abs(left - right) <= abs(tolerance);
}
void
tu_set_pass_cb(tu_case_report_fn_t *cb, void *cb_arg)
{
    tu_config.pass_cb = cb;
    tu_config.pass_arg = cb_arg;
}

void
tu_set_fail_cb(tu_case_report_fn_t *cb, void *cb_arg)
{
    tu_config.fail_cb = cb;
    tu_config.fail_arg = cb_arg;
}

#if MYNEWT_VAL(SELFTEST)
static void
tu_pass_cb_self(const char *msg, void *arg)
{
    dinfo("        ------------- [pass] %s/%s %s", tu_config.ts_suite_name, tu_case_name, msg);
    fflush(stdout);
}

static void
tu_fail_cb_self(const char *msg, void *arg)
{
    dinfo("        ------------- [FAIL] %s/%s %s", tu_config.ts_suite_name, tu_case_name, msg);
    fflush(stdout);
}
#endif

void
tu_init(void)
{
    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

#if MYNEWT_VAL(SELFTEST)
    // os_init(NULL);
    tu_set_pass_cb(tu_pass_cb_self, NULL);
    tu_set_fail_cb(tu_fail_cb_self, NULL);
#endif
}

void
tu_arch_restart(void)
{
#if MYNEWT_VAL(SELFTEST)
    // os_arch_os_stop();
    tu_case_abort();
#else
    hal_system_reset();
#endif
}

void
tu_restart(void)
{
    tu_case_write_pass_auto();
    tu_arch_restart();
}

static void*
tu_dflt_task_handler(void *arg)
{
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
}

static void
tu_create_dflt_task(void)
{
    static os_stack_t stack[OS_STACK_ALIGN(OS_MAIN_STACK_SIZE)];
    static struct os_task task;

    os_task_init(&task, "tu_dflt_task", tu_dflt_task_handler, NULL,
                 OS_MAIN_TASK_PRIO, OS_WAIT_FOREVER, stack,
                 OS_STACK_ALIGN(OS_MAIN_STACK_SIZE));
}

static void
*tu_test_task_handler(void *arg)
{
    os_task_func_t fn;

    fn = (os_task_func_t)arg;
    fn(NULL);

    tu_restart();
    return NULL;
}

/**
 * Creates the "test task."  For test cases running in the OS, this is the task
 * that contains the actual test logic.
 */
static void
tu_create_test_task(const char *task_name, os_task_func_t task_handler)
{
    static os_stack_t stack[OS_STACK_ALIGN(TU_TEST_STACK_SIZE)];
    static struct os_task task;

    os_task_init(&task, task_name, tu_test_task_handler, task_handler,
                 TU_TEST_TASK_PRIO, OS_WAIT_FOREVER, stack,
                 OS_STACK_ALIGN(TU_TEST_STACK_SIZE));

    os_start();
}

/**
 * Creates the default task, creates the test task to run a test case in, and
 * starts the OS.
 */
void
tu_start_os(const char *test_task_name, os_task_func_t test_task_handler)
{
    tu_create_dflt_task();
    tu_create_test_task(test_task_name, test_task_handler);

    os_start();
}
