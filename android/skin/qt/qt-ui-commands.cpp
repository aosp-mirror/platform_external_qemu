#include "android/skin/qt/qt-ui-commands.h"
#include <QApplication>
#include <map>

bool parseQtUICommand(const QString& string, QtUICommand* command) {
#define NAME_TO_CMD(x) {#x, QtUICommand::x}
    static const std::map<QString, QtUICommand> name_to_cmd = {
        NAME_TO_CMD(SHOW_PANE_LOCATION),
        NAME_TO_CMD(SHOW_PANE_CELLULAR),
        NAME_TO_CMD(SHOW_PANE_BATTERY),
        NAME_TO_CMD(SHOW_PANE_PHONE),
        NAME_TO_CMD(SHOW_PANE_VIRTSENSORS),
        NAME_TO_CMD(SHOW_PANE_DPAD),
        NAME_TO_CMD(SHOW_PANE_SETTINGS),
        NAME_TO_CMD(TAKE_SCREENSHOT),
        NAME_TO_CMD(ENTER_ZOOM)
    };
#undef NAME_TO_CMD
    auto it = name_to_cmd.find(string);
    bool result = (it != name_to_cmd.end());
    if (result) {
        *command = it->second;
    }
    return result;
}


QString getQtUICommandDescription(QtUICommand command) {
#define CMD_TO_DESC(x, y) {x, qApp->translate("QtUICommand", y)}
    static const std::map<QtUICommand, QString> cmd_to_desc = {
        CMD_TO_DESC(QtUICommand::SHOW_PANE_LOCATION, "Location"),
        CMD_TO_DESC(QtUICommand::SHOW_PANE_CELLULAR, "Cellular"),
        CMD_TO_DESC(QtUICommand::SHOW_PANE_BATTERY, "Battery"),
        CMD_TO_DESC(QtUICommand::SHOW_PANE_PHONE, "Phone"),
        CMD_TO_DESC(QtUICommand::SHOW_PANE_VIRTSENSORS, "Virtual Sensors"),
        CMD_TO_DESC(QtUICommand::SHOW_PANE_DPAD, "D-Pad"),
        CMD_TO_DESC(QtUICommand::SHOW_PANE_SETTINGS, "Settings"),
        CMD_TO_DESC(QtUICommand::TAKE_SCREENSHOT, "Take Screenshot"),
        CMD_TO_DESC(QtUICommand::ENTER_ZOOM, "Enter Zoom Mode")
    };
#undef CMD_TO_DESC

    QString result;
    auto it = cmd_to_desc.find(command);
    if (it != cmd_to_desc.end()) {
        result = it->second;
    }

    return result;
}
