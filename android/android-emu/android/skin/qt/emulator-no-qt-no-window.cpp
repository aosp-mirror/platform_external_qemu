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

#include <stdint.h>  // for uint32_t
#include <stdio.h>   // for fprintf
#include <string.h>  // for memcpy
#ifdef CONFIG_POSIX
#include <pthread.h>  // for pthread_c...
#endif
#include <string>  // for string

#include "android/avd/info.h"                               // for avdInfo_r...
#include "aemu/base/async/Looper.h"                      // for Looper
#include "aemu/base/async/ThreadLooper.h"                // for ThreadLooper
#include "aemu/base/memory/LazyInstance.h"               // for LazyInstance
#include "android/console.h"                                // for getConsol...
#include "host-common/crash-handler.h"              // for crashhand...
#include "android/emulation/control/adb/AdbInterface.h"     // for OptionalA...
#include "host-common/multi_display_agent.h"  // for QAndroidM...
#include "android/emulation/control/user_event_agent.h"     // for QAndroidU...
#include "host-common/vm_operations.h"        // for QEMU_SHUT...
#include "host-common/FeatureControl.h"          // for isEnabled
#include "host-common/Features.h"                // for FastSnaps.
#include "android/hw-events.h"                              // for EV_SW
#include "android/metrics/DependentMetrics.h"               // for android_m...
#include "android/test/checkboot.h"                         // for android_t...
#include "android/utils/debug.h"                            // for derror
#include "android/utils/filelock.h"                         // for filelock_...

#define DEBUG 0

#if DEBUG
#define D(...) dprint(__VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

static android::base::LazyInstance<EmulatorNoQtNoWindow::Ptr>
        sNoWindowInstance = LAZY_INSTANCE_INIT;

/******************************************************************************/

void EmulatorNoQtNoWindow::create() {
    sNoWindowInstance.get() = Ptr(new EmulatorNoQtNoWindow());
}

const UiEmuAgent* EmulatorNoQtNoWindow::sUiEmuAgent = nullptr;

EmulatorNoQtNoWindow::EmulatorNoQtNoWindow()
    : mLooper(android::base::Looper::create()),
      mAdbInterface(android::emulation::AdbInterface::createGlobalOwnThread()) {
    android::base::ThreadLooper::setLooper(mLooper, true);
    if (getConsoleAgents()->settings->hw()->test_quitAfterBootTimeOut > 0) {
        android_test_start_boot_complete_timer(
                getConsoleAgents()->settings->hw()->test_quitAfterBootTimeOut);
    }
}

EmulatorNoQtNoWindow::Ptr EmulatorNoQtNoWindow::getInstancePtr() {
    return sNoWindowInstance.get();
}

EmulatorNoQtNoWindow* EmulatorNoQtNoWindow::getInstance() {
    return getInstancePtr().get();
}

static std::function<void()> sQemuMainLoop;

#if defined(__WIN32) || defined(_MSC_VER)

static DWORD threadId;
static HANDLE threadHandle;
static DWORD WINAPI threadWrapper(void* ignored) {
    sQemuMainLoop();
    return 0;
}

#else

static pthread_t eventThread;
static void* threadWrapper(void* ignored) {
    sQemuMainLoop();
    return 0;
}

#endif

void EmulatorNoQtNoWindow::startThread(std::function<void()> looperFunction) {
    sQemuMainLoop = looperFunction;

    // Spawn a thread to run the QEMU main loop
#if defined(__WIN32) || defined(_MSC_VER)
    threadHandle = CreateThread(nullptr,  // Default security attributes
                                0,        // Default stack size
                                threadWrapper,
                                nullptr,  // Argument to the thread
                                0,        // Default creation flags
                                &threadId);
    if (threadHandle == nullptr) {
        derror("%s: CreateThread() failed!", __FUNCTION__);
    }
#else
    int createFailed =
            pthread_create(&eventThread, nullptr, threadWrapper, nullptr);

    if (createFailed) {
        derror("%s: pthread_create() failed!", __FUNCTION__);
    }
#endif
}

void EmulatorNoQtNoWindow::waitThread() {
#if defined(__WIN32) || defined(_MSC_VER)
    WaitForSingleObject(threadHandle, INFINITE);
#else
    pthread_join(eventThread, nullptr);
#endif
}

void EmulatorNoQtNoWindow::fold() {
    sendFoldedArea();
    forwardGenericEventToEmulator(EV_SW, SW_LID, true);
    forwardGenericEventToEmulator(EV_SYN, 0, 0);
    mIsFolded = true;
}

void EmulatorNoQtNoWindow::unFold() {
    forwardGenericEventToEmulator(EV_SW, SW_LID, false);
    forwardGenericEventToEmulator(EV_SYN, 0, 0);
    mIsFolded = false;
}

bool EmulatorNoQtNoWindow::isFolded() const {
    return mIsFolded;
}

void EmulatorNoQtNoWindow::sendFoldedArea() {
    if (notSupoortFold()) {
        return;
    }
    int xOffset = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_xOffset;
    int yOffset = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_yOffset;
    int width = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_width;
    int height = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_height;
    char foldedArea[64];
    sprintf(foldedArea, "folded-area %d,%d,%d,%d", xOffset, yOffset,
            xOffset + width, yOffset + height);
    mAdbInterface->enqueueCommand(
            {"shell", "wm", foldedArea},
            [](const android::emulation::OptionalAdbCommandResult& result) {
                if (result && result->exit_code == 0) {
                    D("'fold-area' command succeeded\n");
                }
            });
}

bool EmulatorNoQtNoWindow::notSupoortFold() {
    int xOffset = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_xOffset;
    int yOffset = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_yOffset;
    int width = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_width;
    int height = getConsoleAgents()->settings->hw()->hw_displayRegion_0_1_height;

    if (xOffset < 0 || xOffset > 9999 || yOffset < 0 || yOffset > 9999 ||
        width < 1 || width > 9999 || height < 1 || height > 9999 ||
        // TODO: need 29
        avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) < 28) {
        return true;
    }
    return false;
}

void EmulatorNoQtNoWindow::forwardGenericEventToEmulator(int type,
                                                         int code,
                                                         int value) {
    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = kEventGeneric;
    SkinEventGenericData& genericData = skin_event->u.generic_event;
    genericData.type = type;
    genericData.code = code;
    genericData.value = value;

    queueSkinEvent(skin_event);
}

void EmulatorNoQtNoWindow::queueSkinEvent(SkinEvent* event) {
    android::base::AutoLock lock(mSkinEventQueueLock);
    const bool firstEvent = mSkinEventQueue.empty();
    mSkinEventQueue.push(event);
    lock.unlock();

    const auto uiAgent = sUiEmuAgent;
    if (firstEvent && uiAgent && uiAgent->userEvents &&
        uiAgent->userEvents->onNewUserEvent) {
        // We know that as soon as emulator starts processing user events
        // it processes them until there are none. So we should only notify it
        // if this event is the first one.
        uiAgent->userEvents->onNewUserEvent();
    }
}

void EmulatorNoQtNoWindow::pollEvent(SkinEvent* event, bool* hasEvent) {
    android::base::AutoLock lock(mSkinEventQueueLock);
    if (mSkinEventQueue.empty()) {
        lock.unlock();
        *hasEvent = false;
    } else {
        SkinEvent* newEvent = mSkinEventQueue.front();
        mSkinEventQueue.pop();
        lock.unlock();
        *hasEvent = true;

        memcpy(event, newEvent, sizeof(SkinEvent));
        delete newEvent;
    }
}

// static
void EmulatorNoQtNoWindow::earlyInitialization(const UiEmuAgent* agent) {
    sUiEmuAgent = agent;
}

extern "C" void qemu_system_shutdown_request(QemuShutdownCause cause);

static bool getMultiDisplay(uint32_t id,
                            int32_t* x,
                            int32_t* y,
                            uint32_t* w,
                            uint32_t* h,
                            uint32_t* dpi,
                            uint32_t* flag,
                            bool* enabled) {
    const auto multiDisplay = getConsoleAgents()->multi_display;
    return (multiDisplay != nullptr &&
            multiDisplay->getMultiDisplay(id, x, y, w, h, dpi, flag, enabled));
}

void EmulatorNoQtNoWindow::saveMultidisplayToConfig() {
    int pos_x, pos_y;
    uint32_t width, height, dpi, flag;
    const int maxEntries = avdInfo_maxMultiDisplayEntries();
    for (uint32_t i = 1; i < maxEntries + 1; i++) {
        if (!getMultiDisplay(i, &pos_x, &pos_y, &width, &height, &dpi, &flag,
                             nullptr)) {
            avdInfo_replaceMultiDisplayInConfigIni(getConsoleAgents()->settings->avdInfo(), i, -1, -1,
                                                   0, 0, 0, 0);
        } else {
            avdInfo_replaceMultiDisplayInConfigIni(
                    getConsoleAgents()->settings->avdInfo(), i, pos_x, pos_y, width, height, dpi, flag);
        }
    }
}

void EmulatorNoQtNoWindow::requestClose() {
    crashhandler_exitmode(__FUNCTION__);
    saveMultidisplayToConfig();
    // We don't want to restore to a state where the
    // framework is shut down by 'adb reboot -p',
    // so skip saving when we're doing a reboot.
    const bool fastSnapshotV1 = android::featurecontrol::isEnabled(
            android::featurecontrol::FastSnapshotV1);
    if (fastSnapshotV1) {
        // Tell the system that we are in saving; create a file lock.
        auto snapshotFileLock =
                avdInfo_getSnapshotLockFilePath(getConsoleAgents()->settings->avdInfo());
        if (snapshotFileLock && !filelock_create(snapshotFileLock)) {
            derror("unable to lock snapshot save on exit!\n");
        }
    }

    android::base::System::get()->waitAndKillSelf();

    if (fastSnapshotV1) {
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
