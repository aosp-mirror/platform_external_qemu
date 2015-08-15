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

#ifndef SKIN_QT_TOOLWINDOW_H
#define SKIN_QT_TOOLWINDOW_H

#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QToolButton>

namespace Ui {
    class ToolControls;
}

class EmulatorQtWindow;

typedef void(EmulatorQtWindow::*EmulatorQtWindowSlot)();

class ToolWindow : public QFrame
{
    Q_OBJECT

public:
    explicit ToolWindow(EmulatorQtWindow *emulatorWindow);
    void show();

private:
    QToolButton *addButton(QGridLayout *layout, int row, int col, const char *iconPath, QString tip, EmulatorQtWindowSlot slot);
    void setExpandedState(bool expanded);

    QWidget *button_area;
    EmulatorQtWindow *emulator_window;
    bool expanded;
    QList<QToolButton*> expanded_buttons;
    QBoxLayout *top_layout;

    Ui::ToolControls  *toolsUi;

private slots:
    void on_close_button_clicked();
    void on_power_button_clicked();
    void on_volume_up_button_clicked();
    void on_volume_down_button_clicked();
    void on_rotate_CW_button_clicked();
    void on_rotate_CCW_button_clicked();
    void on_zoom_button_clicked();
    void on_fullscreen_button_clicked();
    void on_more_button_clicked();

};

typedef void(ToolWindow::*ToolWindowSlot)();

#endif // SKIN_QT_TOOLWINDOW_H
