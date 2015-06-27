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

#ifndef TOOLWINDOW_H
#define TOOLWINDOW_H

#include <QFrame>

namespace Ui {
    class ToolWindow;
}

class EmulatorWindow;
typedef void(EmulatorWindow::*EmulatorWindowSlot)();

class ToolWindow : public QFrame
{
    Q_OBJECT

public:
    explicit ToolWindow(EmulatorWindow *emulatorWindow);

private:
    void addButton(QLayout *layout, const char *iconPath, EmulatorWindowSlot slot);
    
    EmulatorWindow *emulator_window;
};

#endif // TOOLWINDOW_H
