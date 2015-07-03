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

#include <QPushButton>

#include "android/skin/qt/emulator-window.h"
#include "android/skin/qt/tool-window.h"
#include "ui_tool-window.h"


ToolWindow::ToolWindow(EmulatorWindow *window) :
    QFrame(window),
    emulator_window(window)
{
    Q_INIT_RESOURCE(resources);

    setWindowFlags(Qt::Tool);
    QGridLayout *layout = new QGridLayout(this);
    setLayout(layout);
    setStyleSheet(QString("QWidget { background: #2c3239 }"));
    addButton(layout, ":/images/ic_power_settings_new_48px.svg", &EmulatorWindow::slot_power);
    addButton(layout, ":/images/ic_volume_up_48px.svg", &EmulatorWindow::slot_volumeUp);
    addButton(layout, ":/images/ic_volume_down_48px.svg", &EmulatorWindow::slot_volumeDown);
    addButton(layout, ":/images/ic_stay_current_portrait_48px.svg", &EmulatorWindow::slot_rotate);
    addButton(layout, ":/images/ic_zoom_in_24px.svg", &EmulatorWindow::slot_zoom);
    addButton(layout, ":/images/ic_fullscreen_48px.svg", &EmulatorWindow::slot_fullscreen);
}

void ToolWindow::addButton(QLayout *layout, const char *iconPath, EmulatorWindowSlot slot)
{
    QToolButton *button = new QToolButton();
    button->setAutoRaise(true);
    button->setIcon(QIcon(iconPath));
    QObject::connect(button, &QPushButton::clicked, emulator_window, slot);
    layout->addWidget(button);
}
