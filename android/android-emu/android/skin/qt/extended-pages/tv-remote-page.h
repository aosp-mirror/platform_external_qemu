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

#include <qobjectdefs.h>           // for Q_OBJECT
#include <QString>                 // for QString
#include <QWidget>                 // for QWidget
#include <memory>                  // for unique_ptr

#include "android/skin/event.h"    // for SkinEventType
#include "android/skin/keycode.h"  // for SkinKeyCode
#include "ui_tv-remote-page.h"     // for TvRemotePage

class EmulatorQtWindow;
class QEvent;
class QObject;
class QPushButton;
class QWidget;

class TvRemotePage : public QWidget
{
    Q_OBJECT

public:
    explicit TvRemotePage(QWidget *parent = 0);
    void setEmulatorWindow(EmulatorQtWindow* eW);

private:
    void toggleButtonEvent(QPushButton* button,
                             const SkinKeyCode key_code,
                             const SkinEventType event_type);
private:
    void remaskButtons();
    bool eventFilter(QObject*, QEvent*) override;

    std::unique_ptr<Ui::TvRemotePage> mUi;
    EmulatorQtWindow* mEmulatorWindow;
};
