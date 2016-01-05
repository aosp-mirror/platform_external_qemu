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

#include <memory>

/******************************************************************************/

class Chore : public QObject
{
    Q_OBJECT

public:
    Chore(std::function<void ()> f);
    ~Chore();

public slots:
    void complete();

signals:
    void finished();

private:
    std::function<void ()> fptr;
};

/******************************************************************************/

class EmulatorQtNoWindow final : public QObject
{
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<EmulatorQtNoWindow>;

    virtual ~EmulatorQtNoWindow();

    static void create();
    static EmulatorQtNoWindow* getInstance();
    static Ptr getInstancePtr();

    void startThread(std::function<void ()> f);

private:
    explicit EmulatorQtNoWindow(QObject *parent = 0);

private slots:
    void slot_clearInstance();
    void slot_finished();
};
