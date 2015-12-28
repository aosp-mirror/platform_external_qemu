// Copyright (C) 2015 The Android Open Source Project
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

#include "ui_dpad-page.h"
#include <QPushButton>
#include <QWidget>
#include <memory>

struct QAndroidUserEventAgent;
class DPadPage : public QWidget
{
    Q_OBJECT

public:
    explicit DPadPage(QWidget *parent = 0);
    void setUserEventsAgent(const QAndroidUserEventAgent* agent);

private slots:
#define ON_PRESS_RELEASE(button) \
    void on_dpad_ ## button ## Button_pressed(); \
    void on_dpad_ ## button ## Button_released(); \

    ON_PRESS_RELEASE(back);
    ON_PRESS_RELEASE(down);
    ON_PRESS_RELEASE(forward);
    ON_PRESS_RELEASE(left);
    ON_PRESS_RELEASE(play);
    ON_PRESS_RELEASE(right);
    ON_PRESS_RELEASE(select);
    ON_PRESS_RELEASE(up);
#undef ON_PRESS_RELEASE

private:
    void dpad_setPressed(QPushButton* button);
    void dpad_setReleased(QPushButton* button);

private:
    std::unique_ptr<Ui::DPadPage> mUi;
    const QAndroidUserEventAgent* mUserEventsAgent;
};
