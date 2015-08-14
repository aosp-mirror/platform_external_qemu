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

#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/tool-window.h"
#include "ui_tools.h"

static ToolWindow *twInstance = NULL;

ToolWindow::ToolWindow(EmulatorQtWindow *window) :
    QFrame(window),
    emulator_window(window),
    extendedWindow(NULL),
    uiEmuAgent(NULL),
    toolsUi(new Ui::ToolControls),
    scale(.4) // TODO: add specific UI for scaling
{
    Q_INIT_RESOURCE(resources);

    twInstance = this;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    toolsUi->setupUi(this);
    // Make this more narrow than QtDesigner likes
    this->resize(54, this->height());
}

void ToolWindow::show()
{
    dockMainWindow();
    QFrame::show();
    setFixedSize(size());
}

void ToolWindow::dockMainWindow()
{
    move(emulator_window->geometry().right() + 10, emulator_window->geometry().top() + 10);
}

void ToolWindow::on_close_button_clicked()
{
    emulator_window->simulateQuit();
}
void ToolWindow::on_power_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F7, 0);
}
void ToolWindow::on_volume_up_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F5, kKeyModLCtrl);
}
void ToolWindow::on_volume_down_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F6, kKeyModLCtrl);
}
void ToolWindow::on_rotate_CW_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F12, kKeyModLCtrl);
}
void ToolWindow::on_rotate_CCW_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F11, kKeyModLCtrl);
}
void ToolWindow::on_zoom_button_clicked()
{
    // TODO
    // Need either a scale factor in [0.1 .. 3.0] or a 'DPI' value
    // If DPI_value is specified, scale = (DPI_value / device_dpi)
    // See android_emulator_set_window_scale() in emulator-window.c

    // How should this be done?
    //   o A modal dialog tied to the button
    //   o A pane on the extended window
    //   o Implicit from resizing the device window itself
    // Obviously the last is the best

    scale = 1.0 - scale;
    emulator_window->simulateSetScale(scale);
}
void ToolWindow::on_fullscreen_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F9, 0);
}

void ToolWindow::on_more_button_clicked()
{
    // Show the tabbed pane
    if (extendedWindow) {
        // It already exists. Don't create another.
        // (But raise it in case it's hidden.)
        extendedWindow->raise();
        return;
    }
    extendedWindow = new ExtendedWindow(emulator_window, this, uiEmuAgent);
    extendedWindow->show();
    // completeInitialization() must be called AFTER show()
    extendedWindow->completeInitialization();
}

extern "C" void setUiEmuAgent(const UiEmuAgent *agentPtr) {
    if (twInstance) {
        twInstance->setToolEmuAgent(agentPtr);
    }
}
