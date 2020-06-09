/* Copyright (C) 2020 The Android Open Source Project
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

#include "android/telephony/MeterService.h"

#include "android/avd/util.h"
#include "android/base/files/PathUtils.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/system.h"

#include <libxml/parser.h>

#include <mutex>
#include <string>
#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

namespace android {
namespace emulation {

using AdbCommands = std::vector<std::vector<std::string>>;

static AdbCommands s_adb_commands;

namespace {

xmlNodePtr findXmlNode(xmlNodePtr startNode, std::string match) {
    for (xmlNodePtr current = startNode; current; current = current->next) {
        if (!strcmp((const char*)current->name, match.c_str())) {
            return current;
        }
    }

    return nullptr;
}

std::vector<std::string> parseAdbCmd(xmlNodePtr parent) {
    std::vector<std::string> result;
    xmlNodePtr adb = findXmlNode(parent->children, "adb");
    if (adb == nullptr) {
        W("cannot find adb node");
        return result;
    }

    xmlNodePtr arraynode = findXmlNode(adb->children, "string-array");
    if (arraynode == nullptr) {
        W("cannot find string-array");
        return result;
    }

    result.push_back("shell");
    for (xmlNodePtr current = arraynode->children; current;
         current = current->next) {
        if (!strcmp((const char*)current->name, "item")) {
            xmlChar* propvalue = xmlGetProp(current, (const xmlChar*)"value");
            result.push_back(std::string((const char*)(propvalue)));
        }
    }
    return result;
}

void parseRadioConfigForAdbCommands(AdbCommands& cmds) {
    // read the data/misc/emulator/config/radioconfig.xml file
    // to figure out how to turn on/off meterness

    char* avdPath(path_dirname(android_hw->disk_dataPartition_path));
    std::string xmlfile = android::base::PathUtils::join(
            avdPath, "data", "misc", "emulator", "config", "radioconfig.xml");

    AFREE(avdPath);

    D("trying to open %s xml file to config meterness commands",
      xmlfile.c_str());

    xmlDocPtr doc = xmlReadFile(xmlfile.c_str(), nullptr, 0);

    if (doc == nullptr) {
        W("cannot open file %s", xmlfile.c_str());
        return;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    // find mobile_data node
    xmlNodePtr mobile_data = findXmlNode(root, "mobile_data");
    if (mobile_data == nullptr) {
        W("cannot find mobile_data");
        return;
    }
    // find meterness node
    xmlNodePtr meterness = findXmlNode(mobile_data->children, "meterness");
    if (meterness == nullptr) {
        W("cannot find meterness");
        return;
    }

    // find enable node
    xmlNodePtr enable = findXmlNode(meterness->children, "enable");
    if (enable == nullptr) {
        W("canot find enable");
        return;
    }
    std::vector<std::string> cmdEnable = parseAdbCmd(enable);

    // find disable node
    xmlNodePtr disable = findXmlNode(meterness->children, "disable");
    if (disable == nullptr) {
        W("canot find disable");
        return;
    }
    std::vector<std::string> cmdDisable = parseAdbCmd(disable);

    if (cmdEnable.empty() || cmdDisable.empty()) {
        W("empty disable and empty enable");
        return;
    }

    s_adb_commands.resize(2);
    enum cmdName { CMD_DISABLE = 0, CMD_ENABLE = 1 };
    s_adb_commands[CMD_DISABLE] = cmdDisable;
    s_adb_commands[CMD_ENABLE] = cmdEnable;
}

}  // namespace

bool InitMeterService() {
    static std::once_flag once_flag;
    static bool initialized;

    std::call_once(once_flag, []() {
        {
            parseRadioConfigForAdbCommands(s_adb_commands);
            initialized = (s_adb_commands.size() == 2);
        }
    });

    return initialized;
}

}  // namespace emulation
}  // namespace android

int set_mobile_data_meterness(int on) {
    if (on < 0 || on > 1) {
        W("on value is invalid %d", on);
        return -1;
    }

    if (!android::emulation::InitMeterService()) {
        W("not initialzied");
        return -1;
    }

    auto mAdb = android::emulation::AdbInterface::getGlobal();
    if (!mAdb) {
        W("find no adb interface");
        return -1;
    }

    int myresult = -1;

    for (size_t i = 0; i < android::emulation::s_adb_commands[on].size(); ++i) {
        D("cmd %s", android::emulation::s_adb_commands[on][i].c_str());
    }

    mAdb->runAdbCommand(
            android::emulation::s_adb_commands[on],
            [&](const android::emulation::OptionalAdbCommandResult& result) {
                if (!result) {
                    myresult = -1;
                } else if (result->exit_code) {
                    myresult = -2;
                } else {
                    myresult = 0;
                }
            },
            android::base::System::kInfinite);

    return myresult;
}
