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

#pragma once

#include <QObject>
#include <QThread>

#include "android/skin/winsys.h"

#include <memory>

class NoWindowMainLoopThread : public QThread
{
    Q_OBJECT
public:
    NoWindowMainLoopThread(StartFunction f, int argc, char **argv) : start_function(f), argc(argc), argv(argv) {}
    void run() Q_DECL_OVERRIDE { if (start_function) start_function(argc, argv); }
private:
    StartFunction start_function;
    int argc;
    char **argv;
};

class EmulatorQtNoWindow final : public QObject
{
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<EmulatorQtNoWindow>;

private:
    explicit EmulatorQtNoWindow(QObject *parent = 0);

    NoWindowMainLoopThread *mMainLoopThread;

public:
    virtual ~EmulatorQtNoWindow();

    static void create();
    static EmulatorQtNoWindow* getInstance();
    static Ptr getInstancePtr();

    void startThread(StartFunction f, int argc, char **argv);

private slots:
    void slot_clearInstance();
    void slot_finished();
};
