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

#include <QtCore>
#include <QCoreApplication>

#include "android/base/memory/LazyInstance.h"
#include "android/skin/qt/emulator-qt-no-window.h"

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

using namespace android::base;

// Make sure it is POD here
static LazyInstance<EmulatorQtNoWindow::Ptr> sNoWindowInstance = LAZY_INSTANCE_INIT;

void EmulatorQtNoWindow::create()
{
    sNoWindowInstance.get() = Ptr(new EmulatorQtNoWindow());
}

EmulatorQtNoWindow::EmulatorQtNoWindow(QObject *parent):
    mMainLoopThread(nullptr)
{
    QObject::connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,this,&EmulatorQtNoWindow::slot_clearInstance);
}

EmulatorQtNoWindow::Ptr EmulatorQtNoWindow::getInstancePtr()
{
    return sNoWindowInstance.get();
}

EmulatorQtNoWindow* EmulatorQtNoWindow::getInstance()
{
    return getInstancePtr().get();
}

EmulatorQtNoWindow::~EmulatorQtNoWindow()
{
    delete mMainLoopThread;
}

void EmulatorQtNoWindow::startThread(StartFunction f, int argc, char **argv)
{
    if (!mMainLoopThread) {
        mMainLoopThread = new NoWindowMainLoopThread(f, argc, argv);
        QObject::connect(mMainLoopThread, &QThread::finished, this, &EmulatorQtNoWindow::slot_finished);
        mMainLoopThread->start();
    } else {
        D("mMainLoopThread already started");
    }
}

void EmulatorQtNoWindow::slot_clearInstance()
{
    sNoWindowInstance.get().reset();

    return;
}

void EmulatorQtNoWindow::slot_finished()
{
    if (mMainLoopThread != NULL && mMainLoopThread->isRunning()) {
      D("Emulator -no-window quiting while busy!");
    }

    QCoreApplication::instance()->quit();
    return;
}
