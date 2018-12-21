/* Copyright (C) 2015 The Android Open Source Project
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

#include "android/skin/qt/emulator-no-qt-no-window.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/async/ThreadLooper.h"
#include "android/emulation/control/vm_operations.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/metrics/metrics.h"
#include "android/test/checkboot.h"
#include "android/utils/filelock.h"

#define DEBUG 0

#if DEBUG
#define D(...) qDebug(__VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

#define derror(msg) do { fprintf(stderr, (msg)); } while (0)

static android::base::LazyInstance<EmulatorNoQtNoWindow::Ptr> sNoWindowInstance =
        LAZY_INSTANCE_INIT;

/******************************************************************************/

void EmulatorNoQtNoWindow::create() {
    sNoWindowInstance.get() = Ptr(new EmulatorNoQtNoWindow());
}

EmulatorNoQtNoWindow::EmulatorNoQtNoWindow()
    : mLooper(android::base::Looper::create()),
      mAdbInterface(android::emulation::AdbInterface::create(mLooper)) {
    android::base::ThreadLooper::setLooper(mLooper, true);
    android_metrics_start_adb_liveness_checker(mAdbInterface.get());
    if (android_hw->test_quitAfterBootTimeOut > 0) {
        android_test_start_boot_complete_timer(android_hw->test_quitAfterBootTimeOut);
    }
}

EmulatorNoQtNoWindow::Ptr EmulatorNoQtNoWindow::getInstancePtr() {
    return sNoWindowInstance.get();
}

EmulatorNoQtNoWindow* EmulatorNoQtNoWindow::getInstance() {
    return getInstancePtr().get();
}

static std::function<void()> sQemuMainLoop;

#ifdef _WIN32
    static DWORD WINAPI threadWrapper(void* ignored)
#else
    static void*        threadWrapper(void* ignored)
#endif
    {
        sQemuMainLoop();
        return 0;
    }

void EmulatorNoQtNoWindow::startThread(std::function<void()> looperFunction) {

    sQemuMainLoop = looperFunction;

    // Spawn a thread to run the QEMU main loop
#if defined(__WIN32) || defined(_MSC_VER)
    DWORD  threadId;
    HANDLE threadHandle = CreateThread(nullptr,        // Default security attributes
                                       0,              // Default stack size
                                       threadWrapper,
                                       nullptr,        // Argument to the thread
                                       0,              // Default creation flags
                                       &threadId);
    if (threadHandle == nullptr) {
        fprintf(stderr, "%s %s: CreateThread() failed!\n", __FILE__, __FUNCTION__);
    }
#else
    pthread_t eventThread;
    int createFailed = pthread_create(&eventThread, nullptr, threadWrapper, nullptr);

    if (createFailed) {
        fprintf(stderr, "%s %s: pthread_create() failed!\n", __FILE__, __FUNCTION__);
    }
#endif
}

extern "C" void qemu_system_shutdown_request(QemuShutdownCause cause);

void EmulatorNoQtNoWindow::requestClose() {
    // We don't want to restore to a state where the
    // framework is shut down by 'adb reboot -p',
    // so skip saving when we're doing a reboot.
    const bool fastSnapshotV1 =
        android::featurecontrol::isEnabled(android::featurecontrol::FastSnapshotV1);
    if (fastSnapshotV1) {
        // Tell the system that we are in saving; create a file lock.
        if (!filelock_create(
                    avdInfo_getSnapshotLockFilePath(android_avdInfo))) {
            derror("unable to lock snapshot save on exit!\n");
        }
    }

    if (fastSnapshotV1 || savevm_on_exit) {
        qemu_system_shutdown_request(QEMU_SHUTDOWN_CAUSE_HOST_UI);
    } else {
        mAdbInterface->runAdbCommand(
                {"shell", "reboot", "-p"},
                [](const android::emulation::OptionalAdbCommandResult&) {
                    qemu_system_shutdown_request(QEMU_SHUTDOWN_CAUSE_HOST_UI);
                },
                5000, false);
    }
}
