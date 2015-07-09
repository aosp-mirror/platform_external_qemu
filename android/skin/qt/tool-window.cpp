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
    
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    top_layout = new QBoxLayout(QBoxLayout::TopToBottom);
    top_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(top_layout);
    setStyleSheet(QString("* { background: #2c3239 }"));
    title_bar = new TitleBarWidget(this);
    top_layout->addWidget(title_bar);
    
    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(10, 0, 10, 10);
    layout->setAlignment(Qt::AlignHCenter);
    int col = 0;
    int row = 0;
    addButton(layout, row++, col, ":/images/ic_power_settings_new_48px.svg", &EmulatorWindow::slot_power);
    addButton(layout, row++, col, ":/images/ic_volume_up_48px.svg", &EmulatorWindow::slot_volumeUp);
    addButton(layout, row++, col, ":/images/ic_volume_down_48px.svg", &EmulatorWindow::slot_volumeDown);
    addButton(layout, row++, col, ":/images/ic_stay_current_portrait_48px.svg", &EmulatorWindow::slot_rotate);
    addButton(layout, row++, col, ":/images/ic_zoom_in_24px.svg", &EmulatorWindow::slot_zoom);
    addButton(layout, row++, col, ":/images/ic_fullscreen_48px.svg", &EmulatorWindow::slot_fullscreen);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_camera_enhance_48px.svg", &EmulatorWindow::slot_screenshot);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_hangout_video_48px.svg", &EmulatorWindow::slot_screenrecord);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_arrow_back_48px.svg", &EmulatorWindow::slot_back);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_panorama_fish_eye_48px.svg", &EmulatorWindow::slot_home);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_crop_square_48px.svg", &EmulatorWindow::slot_recents);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_menu_48px.svg", &EmulatorWindow::slot_menu);
    col++;
    row = 0;
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_mic_48px.svg", &EmulatorWindow::slot_voice);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_sd_card_48px.svg", &EmulatorWindow::slot_sdcard);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_location_on_48px.svg", &EmulatorWindow::slot_gps);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_signal_cellular_4_bar_48px.svg", &EmulatorWindow::slot_cellular);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_battery_std_48px.svg", &EmulatorWindow::slot_battery);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_photo_camera_48px.svg", &EmulatorWindow::slot_camera);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_call_48px.svg", &EmulatorWindow::slot_phone);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_filter_tilt_shift_48px.svg", &EmulatorWindow::slot_sensors);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_keyboard_arrow_left_48px.svg", &EmulatorWindow::slot_left);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_keyboard_arrow_down_48px.svg", &EmulatorWindow::slot_down);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_keyboard_arrow_up_48px.svg", &EmulatorWindow::slot_up);
    expanded_buttons << addButton(layout, row++, col, ":/images/ic_keyboard_arrow_right_48px.svg", &EmulatorWindow::slot_right);
    button_area = new QWidget();
    button_area->setLayout(layout);

    top_layout->addWidget(button_area);
    setExpandedState(false);
}

QToolButton *ToolWindow::addButton(QGridLayout *layout, int row, int col, const char *iconPath, EmulatorWindowSlot slot)
{
    QToolButton *button = new QToolButton();
    button->setAutoRaise(true);
    button->setIcon(QIcon(iconPath));
    QObject::connect(button, &QPushButton::clicked, emulator_window, slot);
    layout->addWidget(button, row, col);
    layout->setAlignment(button, Qt::AlignHCenter);
    return button;
}

void ToolWindow::setExpandedState(bool exp)
{
    for (int i = 0; i < expanded_buttons.size(); i++) {
        if (exp) {
            expanded_buttons.at(i)->show();
        } else {
            expanded_buttons.at(i)->hide();
        }
    }
    title_bar->setExpandedState(exp);
    expanded = exp;
}

void ToolWindow::show()
{
    move(emulator_window->geometry().right() + 10, emulator_window->geometry().top() + 10);
    QFrame::show();
    setFixedSize(size());
    title_bar->setFixedSize(title_bar->size());
}

void ToolWindow::slot_toggleExpand()
{
    setUpdatesEnabled(false);
    top_layout->setEnabled(false);
    setExpandedState(!expanded);
    setUpdatesEnabled(true);
    top_layout->setEnabled(true);
    button_area->updateGeometry();
    top_layout->activate();
    setFixedSize(minimumSize());
}

TitleBarWidget::TitleBarWidget(ToolWindow *window) :
    QWidget(window), collapsed_icon(":/images/ic_unfold_more_48px.svg"), expanded_icon(":/images/ic_unfold_less_48px.svg"), tool_window(window)
{
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->setContentsMargins(4, 4, 20, 4);
    layout->setAlignment(Qt::AlignLeft);
    setLayout(layout);
    addButton(layout, ":/images/ic_close_48px.svg", (ToolWindowSlot)&ToolWindow::close);
    layout->addSpacing(3);
    expand_button = addButton(layout, NULL, (ToolWindowSlot)&ToolWindow::slot_toggleExpand);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QToolButton *TitleBarWidget::addButton(QBoxLayout *layout, const char *iconPath, ToolWindowSlot slot)
{
    QToolButton *button = new QToolButton();
    button->setAutoRaise(true);
    if (iconPath) button->setIcon(QIcon(iconPath));
    button->setMaximumSize(QSize(15, 15));
    button->setMinimumSize(button->maximumSize());
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QObject::connect(button, &QPushButton::clicked, tool_window, slot);
    layout->addWidget(button);
    layout->setAlignment(button, Qt::AlignLeft);
    return button;
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event)
{
    tool_window->move(event->globalPos() - drag_offset);
}

void TitleBarWidget::mousePressEvent(QMouseEvent *event)
{
    drag_offset = event->globalPos() - tool_window->pos();
}

void TitleBarWidget::setExpandedState(bool state)
{
    expand_button->setIcon(state ? expanded_icon : collapsed_icon);
}
