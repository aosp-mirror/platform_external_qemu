// Copyright (C) 2015 The Android Open Source Project
 
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.


#include "android/skin/qt/qt-ui-commands.h"
#include <QApplication>
#include <map>

bool parseQtUICommand(const QString& string, QtUICommand* command) {
    using CmdPair = std::pair<QString, QtUICommand>;
#define NAME_TO_CMD(x) {#x, QtUICommand::x}
    static const CmdPair name_to_cmd[] = {
        NAME_TO_CMD(SHOW_PANE_LOCATION),
        NAME_TO_CMD(SHOW_PANE_CELLULAR),
        NAME_TO_CMD(SHOW_PANE_BATTERY),
        NAME_TO_CMD(SHOW_PANE_PHONE),
        NAME_TO_CMD(SHOW_PANE_VIRTSENSORS),
        NAME_TO_CMD(SHOW_PANE_DPAD),
        NAME_TO_CMD(SHOW_PANE_SETTINGS),
        NAME_TO_CMD(TAKE_SCREENSHOT),
        NAME_TO_CMD(ENTER_ZOOM),
        NAME_TO_CMD(GRAB_KEYBOARD),
        NAME_TO_CMD(VOLUME_UP),
        NAME_TO_CMD(VOLUME_DOWN),
        NAME_TO_CMD(POWER),
        NAME_TO_CMD(MENU),
        NAME_TO_CMD(HOME),
        NAME_TO_CMD(BACK),
        NAME_TO_CMD(RECENTS),
        NAME_TO_CMD(UNGRAB_KEYBOARD)
    };
#undef NAME_TO_CMD
    auto it = std::find_if(std::begin(name_to_cmd),
                           std::end(name_to_cmd),
                           [&string] (const CmdPair& value) {
                               return value.first == string;
                           });
    bool result = (it != std::end(name_to_cmd));
    if (result) {
        *command = it->second;
    }
    return result;
}


QString getQtUICommandDescription(QtUICommand command) {
    using CmdPair = std::pair<QtUICommand, QString>;
#define CMD_TO_DESC(x, y) {QtUICommand::x, qApp->translate("QtUICommand", y)}
    static const CmdPair cmd_to_desc[] = {
        CMD_TO_DESC(SHOW_PANE_LOCATION, "Location"),
        CMD_TO_DESC(SHOW_PANE_CELLULAR, "Cellular"),
        CMD_TO_DESC(SHOW_PANE_BATTERY, "Battery"),
        CMD_TO_DESC(SHOW_PANE_PHONE, "Phone"),
        CMD_TO_DESC(SHOW_PANE_VIRTSENSORS, "Virtual sensors"),
        CMD_TO_DESC(SHOW_PANE_DPAD, "D-Pad"),
        CMD_TO_DESC(SHOW_PANE_SETTINGS, "Settings"),
        CMD_TO_DESC(TAKE_SCREENSHOT, "Take screenshot"),
        CMD_TO_DESC(ENTER_ZOOM, "Enter zoom mode"),
        CMD_TO_DESC(GRAB_KEYBOARD, "Let Android grab the keyboard input"),
        CMD_TO_DESC(VOLUME_UP, "Volume up"),
        CMD_TO_DESC(VOLUME_DOWN, "Volume down"),
        CMD_TO_DESC(POWER, "Power"),
        CMD_TO_DESC(HOME, "Home"),
        CMD_TO_DESC(BACK, "Back"),
        CMD_TO_DESC(MENU, "Menu"),
        CMD_TO_DESC(RECENTS, "Recents"),
        CMD_TO_DESC(UNGRAB_KEYBOARD, "Stop grabbing keyboard input")
    };
#undef CMD_TO_DESC

    QString result;
    auto it = std::find_if(std::begin(cmd_to_desc),
                        std::end(cmd_to_desc),
                        [command](const CmdPair& value) {
                            return value.first == command;
                        });
    if (it != std::end(cmd_to_desc)) {
        result = it->second;
    }

    return result;
}
