// Copyright (C) 2015 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QApplication>
#include <QString>

enum class QtUICommand {
    SHOW_PANE_LOCATION,
    SHOW_PANE_CELLULAR,
    SHOW_PANE_BATTERY,
    SHOW_PANE_PHONE,
    SHOW_PANE_MICROPHONE,
    SHOW_PANE_VIRTSENSORS,
    SHOW_PANE_DPAD,
    SHOW_PANE_FINGER,
    SHOW_PANE_SETTINGS,
    SHOW_PANE_HELP,
    SHOW_MULTITOUCH,
    TAKE_SCREENSHOT,
    ENTER_ZOOM,
    ZOOM_IN,
    ZOOM_OUT,
    PAN_UP,
    PAN_DOWN,
    PAN_LEFT,
    PAN_RIGHT,
    VOLUME_UP,
    VOLUME_DOWN,
    POWER,
    MENU,
    HOME,
    BACK,
    OVERVIEW,
    ROTATE_RIGHT,
    ROTATE_LEFT,
    TOGGLE_TRACKBALL,
};

#define NAME_TO_CMD(x) {#x, QtUICommand::x}
static const std::pair<QString, QtUICommand> NameToQtUICmd[] = {
        NAME_TO_CMD(SHOW_PANE_LOCATION),
        NAME_TO_CMD(SHOW_PANE_CELLULAR),
        NAME_TO_CMD(SHOW_PANE_BATTERY),
        NAME_TO_CMD(SHOW_PANE_PHONE),
        NAME_TO_CMD(SHOW_PANE_MICROPHONE),
        NAME_TO_CMD(SHOW_PANE_VIRTSENSORS),
        NAME_TO_CMD(SHOW_PANE_DPAD),
        NAME_TO_CMD(SHOW_PANE_FINGER),
        NAME_TO_CMD(SHOW_PANE_SETTINGS),
        NAME_TO_CMD(SHOW_PANE_HELP),
        NAME_TO_CMD(SHOW_MULTITOUCH),
        NAME_TO_CMD(ROTATE_LEFT),
        NAME_TO_CMD(ROTATE_RIGHT),
        NAME_TO_CMD(TAKE_SCREENSHOT),
        NAME_TO_CMD(ENTER_ZOOM),
        NAME_TO_CMD(ZOOM_IN),
        NAME_TO_CMD(ZOOM_OUT),
        NAME_TO_CMD(PAN_UP),
        NAME_TO_CMD(PAN_DOWN),
        NAME_TO_CMD(PAN_LEFT),
        NAME_TO_CMD(PAN_RIGHT),
        NAME_TO_CMD(VOLUME_UP),
        NAME_TO_CMD(VOLUME_DOWN),
        NAME_TO_CMD(POWER),
        NAME_TO_CMD(MENU),
        NAME_TO_CMD(HOME),
        NAME_TO_CMD(BACK),
        NAME_TO_CMD(OVERVIEW),
};
#undef NAME_TO_CMD

#define CMD_TO_DESC(x, y) {QtUICommand::x, qApp->translate("QtUICommand", y)}
static const std::pair<QtUICommand, QString> QtUICmdToDesc[] = {
        CMD_TO_DESC(SHOW_PANE_LOCATION, "Location"),
        CMD_TO_DESC(SHOW_PANE_CELLULAR, "Cellular"),
        CMD_TO_DESC(SHOW_PANE_BATTERY, "Battery"),
        CMD_TO_DESC(SHOW_PANE_PHONE, "Phone"),
        CMD_TO_DESC(SHOW_PANE_MICROPHONE, "Microphone"),
        CMD_TO_DESC(SHOW_PANE_VIRTSENSORS, "Virtual sensors"),
        CMD_TO_DESC(SHOW_PANE_DPAD, "D-Pad"),
        CMD_TO_DESC(SHOW_PANE_FINGER, "Fingerprint"),
        CMD_TO_DESC(SHOW_PANE_SETTINGS, "Settings"),
        CMD_TO_DESC(SHOW_PANE_HELP, "Help"),
        CMD_TO_DESC(SHOW_MULTITOUCH,
                    "Multitouch (left click to pinch/zoom, right click to "
                    "vertical swipe)"),
        CMD_TO_DESC(ROTATE_LEFT, "Rotate left"),
        CMD_TO_DESC(ROTATE_RIGHT, "Rotate right"),
        CMD_TO_DESC(TAKE_SCREENSHOT, "Take screenshot"),
        CMD_TO_DESC(ENTER_ZOOM, "Enter zoom mode"),
        CMD_TO_DESC(ZOOM_IN,
                    "Zoom in (when in zoom mode), or scale up (when not)"),
        CMD_TO_DESC(ZOOM_OUT,
                    "Zoom out (when in zoom mode), or scale down (when not)"),
        CMD_TO_DESC(PAN_UP, "Pan up (when in zoom mode)"),
        CMD_TO_DESC(PAN_DOWN, "Pan down (when in zoom mode)"),
        CMD_TO_DESC(PAN_LEFT, "Pan left (when in zoom mode)"),
        CMD_TO_DESC(PAN_RIGHT, "Pan right (when in zoom mode)"),
        CMD_TO_DESC(VOLUME_UP, "Volume up"),
        CMD_TO_DESC(VOLUME_DOWN, "Volume down"), CMD_TO_DESC(POWER, "Power"),
        CMD_TO_DESC(HOME, "Home"), CMD_TO_DESC(BACK, "Back"),
        CMD_TO_DESC(MENU, "Menu"), CMD_TO_DESC(OVERVIEW, "Overview"),
};
#undef CMD_TO_DESC

bool parseQtUICommand(const QString& string, QtUICommand* command);
QString getQtUICommandDescription(QtUICommand command);
