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
#include <QGridLayout>
#include <QHash>
#include <QToolButton>

namespace Ui {
    class ToolWindow;
}

class EmulatorQtWindow;
class TitleBarWidget;

typedef void(EmulatorQtWindow::*EmulatorQtWindowSlot)();

class ToolWindow : public QFrame
{
    Q_OBJECT

public:
    explicit ToolWindow(EmulatorQtWindow *emulatorWindow);
    void show();

public slots:
    void slot_toggleExpand();

private:
    QToolButton *addButton(QGridLayout *layout, int row, int col, const char *iconPath, EmulatorQtWindowSlot slot);
    void setExpandedState(bool expanded);

    QWidget *button_area;
    EmulatorQtWindow *emulator_qt_window;
    bool expanded;
    QList<QToolButton*> expanded_buttons;
    TitleBarWidget *title_bar;
    QBoxLayout *top_layout;
};

typedef void(ToolWindow::*ToolWindowSlot)();

class TitleBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBarWidget(ToolWindow *window);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    void setExpandedState(bool state);

private:
    QToolButton *addButton(QBoxLayout *layout, const char *iconPath, ToolWindowSlot slot);

    QIcon collapsed_icon;
    QPoint drag_offset;
    QIcon expanded_icon;
    QToolButton *expand_button;
    ToolWindow *tool_window;
};
#endif // TOOLWINDOW_H
