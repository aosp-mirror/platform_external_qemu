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
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/hw-events.h"
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

const UiEmuAgent* EmulatorNoQtNoWindow::sUiEmuAgent = nullptr;

EmulatorNoQtNoWindow::EmulatorNoQtNoWindow()
    : mLooper(android::base::Looper::create()),
      mAdbInterface(android::emulation::AdbInterface::createGlobal(mLooper)) {
    android::base::ThreadLooper::setLooper(mLooper, true);
    android_metrics_start_adb_liveness_checker(mAdbInterface);
    if (android_hw->test_quitAfterBootTimeOut > 0) {
        android_test_start_boot_complete_timer(android_hw->test_quitAfterBootTimeOut);
    }
    mAdbInterface->enqueueCommand(
        { "shell", "settings", "put", "system",
          "screen_off_timeout", "2147483647" });
    mAdbInterface->enqueueCommand(
        { "shell", "logcat", "-G", "2M" });
}

EmulatorNoQtNoWindow::Ptr EmulatorNoQtNoWindow::getInstancePtr() {
    return sNoWindowInstance.get();
}

EmulatorNoQtNoWindow* EmulatorNoQtNoWindow::getInstance() {
    return getInstancePtr().get();
}

static std::function<void()> sQemuMainLoop;

#if defined(__WIN32) || defined(_MSC_VER)

static DWORD  threadId;
static HANDLE threadHandle;
static DWORD WINAPI threadWrapper(void* ignored)
{
    sQemuMainLoop();
    return 0;
}

#else

static pthread_t eventThread;
static void*        threadWrapper(void* ignored)
{
    sQemuMainLoop();
    return 0;
}

#endif

void EmulatorNoQtNoWindow::startThread(std::function<void()> looperFunction) {

    sQemuMainLoop = looperFunction;

    // Spawn a thread to run the QEMU main loop
#if defined(__WIN32) || defined(_MSC_VER)
    threadHandle = CreateThread(nullptr,        // Default security attributes
                                0,              // Default stack size
                                threadWrapper,
                                nullptr,        // Argument to the thread
                                0,              // Default creation flags
                                &threadId);
    if (threadHandle == nullptr) {
        fprintf(stderr, "%s %s: CreateThread() failed!\n", __FILE__, __FUNCTION__);
    }
#else
    int createFailed = pthread_create(&eventThread, nullptr, threadWrapper, nullptr);

    if (createFailed) {
        fprintf(stderr, "%s %s: pthread_create() failed!\n", __FILE__, __FUNCTION__);
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
    int xOffset = 0;
    int yOffset = 0;
//  TODO: support none-0 offset
//    int xOffset = android_hw->hw_displayRegion_0_1_xOffset;
//    int yOffset = android_hw->hw_displayRegion_0_1_yOffset;
    int width   = android_hw->hw_displayRegion_0_1_width;
    int height  = android_hw->hw_displayRegion_0_1_height;
    char foldedArea[64];
    sprintf(foldedArea, "folded-area %d,%d,%d,%d",
            xOffset,
            yOffset,
            xOffset + width,
            yOffset + height);
    mAdbInterface->enqueueCommand(
            {"shell", "wm", foldedArea},
            [](const android::emulation::OptionalAdbCommandResult& result) {
                if (result && result->exit_code == 0) {
                    D("'fold-area' command succeeded\n");
                }});
}

bool EmulatorNoQtNoWindow::notSupoortFold() {
    int xOffset = android_hw->hw_displayRegion_0_1_xOffset;
    int yOffset = android_hw->hw_displayRegion_0_1_yOffset;
    int width   = android_hw->hw_displayRegion_0_1_width;
    int height  = android_hw->hw_displayRegion_0_1_height;

    if (xOffset < 0 || xOffset > 9999 ||
        yOffset < 0 || yOffset > 9999 ||
        width   < 1 || width   > 9999 ||
        height  < 1 || height  > 9999 ||
        //TODO: need 29
        avdInfo_getApiLevel(android_avdInfo) < 28) {
        return true;
    }
    return false;
}

void EmulatorNoQtNoWindow::forwardGenericEventToEmulator(int type, int code, int value) {
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

//static
void EmulatorNoQtNoWindow::earlyInitialization(const UiEmuAgent* agent) {
    sUiEmuAgent = agent;
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
