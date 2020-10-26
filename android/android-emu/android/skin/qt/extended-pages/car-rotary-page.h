// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <qobjectdefs.h>                                    // for Q_OBJECT
#include <QTime>                                            // for QTime
#include <QTimer>                                           // for QTimer
#include <QWidget>                                          // for QWidget
#include <memory>                                           // for unique_ptr
#include <string>                                           // for string

#include "android/emulation/control/adb/AdbInterface.h"     // for AdbInterface
#include "ui_car-rotary-page.h"                             // for CarRotaryPage

class EmulatorQtWindow;
class QEvent;
class QObject;
class QPushButton;
class QWidget;

class CarRotaryPage : public QWidget
{
    Q_OBJECT

public:
    explicit CarRotaryPage(QWidget *parent = 0);
    void setAdbInterface(android::emulation::AdbInterface* adb);
    void setEmulatorWindow(EmulatorQtWindow* eW);

private:
    void toggleButtonPressed(QPushButton* button,
                             const std::string& cmd,
                             const bool useActionArg,
                             const std::string& label);
    void toggleButtonReleased(QPushButton* button,
                              const std::string& cmd,
                              const bool useActionArg,
                              const std::string& label);
    void toggleIconTheme(QPushButton* button, const bool pressed);
    void executeLastPushButtonCmd();
    void executeCmd(const std::string& cmd, const std::string& arg);
    void remaskButtons();
    bool eventFilter(QObject*, QEvent*) override;
    void checkRotaryControllerServiceTimer();
    void checkRotaryControllerService();
    void startSupportsActionTimer();
    void determineWhetherInjectKeySupportsActionArg();
    bool isBootCompleted();

    std::unique_ptr<Ui::CarRotaryPage> mUi;
    EmulatorQtWindow* mEmulatorWindow;
    android::emulation::AdbInterface* mAdb;
    QTime mAdbExecuteTime;
    QTimer mLongPressTimer;
    QTimer mCheckTimer;
    QTimer mSupportsActionTimer;
    std::string mLastPushButtonCmd;
    bool mAdbExecuteIsActive;
    bool mIsBootCompleted;
    bool mSupportsActionArg;
};
