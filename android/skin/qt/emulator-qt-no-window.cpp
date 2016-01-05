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


#include <QObject>
#include <QThread>
#include <QtCore>
#include <QCoreApplication>

#include "android/base/memory/LazyInstance.h"
#include "android/skin/qt/emulator-qt-no-window.h"

#define  DEBUG  0

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

using namespace android::base;

static LazyInstance<EmulatorQtNoWindow::Ptr> sNoWindowInstance = LAZY_INSTANCE_INIT;

/******************************************************************************/

Chore::Chore(std::function<void ()> f) : fptr(f)
{
}

Chore::~Chore()
{
}

void Chore::complete()
{
    fptr();
    emit finished();
}

/******************************************************************************/

void EmulatorQtNoWindow::create()
{
    sNoWindowInstance.get() = Ptr(new EmulatorQtNoWindow());
}

EmulatorQtNoWindow::EmulatorQtNoWindow(QObject *parent)
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
}

void EmulatorQtNoWindow::startThread(std::function<void ()> f)
{
    auto thread = new QThread();
    auto task = new Chore(f);

    // pass task object to thread and start task when thread starts
    task->moveToThread(thread);
    connect(thread, SIGNAL(started()),  task,  SLOT(complete()));
    // when the task is finished, signal the thread to quit
    connect(task, SIGNAL(finished()), thread, SLOT(quit()));
    // queue up task object for deletion when the thread is finished
    connect(thread, SIGNAL(finished()), task,  SLOT(deleteLater()));
    // queue up thread object for deletion when the thread is finished
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    // when the thread is finished, quit this GUI-less window too
    connect(thread, SIGNAL(finished()), getInstance(), SLOT(slot_finished()));

    thread->start();
}

void EmulatorQtNoWindow::slot_clearInstance()
{
    sNoWindowInstance.get().reset();
}

void EmulatorQtNoWindow::slot_finished()
{
    D("Closing GUI-less window, quiting application");
    QCoreApplication::instance()->quit();
}
