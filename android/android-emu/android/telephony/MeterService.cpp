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

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("%s:%s:%d| " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

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

static void cleanupXmlDoc(xmlDoc *doc)
{
    xmlFreeDoc(doc);
    xmlCleanupParser();
    DD("clean up xml doc");
}

struct MyXmlCleaner {
    xmlDoc *mDoc = nullptr;
    explicit MyXmlCleaner(xmlDoc *doc) : mDoc(doc) {}
    ~MyXmlCleaner() {
        if (mDoc) {
            cleanupXmlDoc(mDoc);
            mDoc = nullptr;
        }
    }
};

void parseRadioConfigForAdbCommands(AdbCommands& cmds) {
    // read the data/misc/emulator/config/radioconfig.xml file
    // to figure out how to turn on/off meterness

    char* sysPath(path_dirname(
            android_hw->disk_systemPartition_initPath &&
                            android_hw->disk_systemPartition_initPath[0]
                    ? android_hw->disk_systemPartition_initPath
                    : android_hw->disk_systemPartition_path));
    std::string xmlfile = android::base::PathUtils::join(
            sysPath, "data", "misc", "emulator", "config", "radioconfig.xml");

    AFREE(sysPath);

    if (!path_exists(xmlfile.c_str())) {
        DD("file %s does not exists", xmlfile.c_str());
        return;
    }

    DD("trying to open %s xml file to config meterness commands",
      xmlfile.c_str());

    xmlDocPtr doc = xmlReadFile(xmlfile.c_str(), nullptr, 0);
    MyXmlCleaner myXmlCleaner(doc);

    if (doc == nullptr) {
        DD("cannot open file %s", xmlfile.c_str());
        return;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    // find mobile_data node
    xmlNodePtr mobile_data = findXmlNode(root, "mobile_data");
    if (mobile_data == nullptr) {
        DD("cannot find mobile_data");
        return;
    }
    // find meterness node
    xmlNodePtr meterness = findXmlNode(mobile_data->children, "meterness");
    if (meterness == nullptr) {
        DD("cannot find meterness");
        return;
    }

    // find enable node
    xmlNodePtr enable = findXmlNode(meterness->children, "enable");
    if (enable == nullptr) {
        DD("canot find enable");
        return;
    }
    std::vector<std::string> cmdEnable = parseAdbCmd(enable);

    // find disable node
    xmlNodePtr disable = findXmlNode(meterness->children, "disable");
    if (disable == nullptr) {
        DD("canot find disable");
        return;
    }
    std::vector<std::string> cmdDisable = parseAdbCmd(disable);

    if (cmdEnable.empty() || cmdDisable.empty()) {
        DD("empty disable and empty enable");
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
        DD("on value is invalid %d", on);
        return -1;
    }

    if (!android::emulation::InitMeterService()) {
        DD("not initialized");
        return -1;
    }

    auto mAdb = android::emulation::AdbInterface::getGlobal();
    if (!mAdb) {
        DD("find no adb interface");
        return -1;
    }

    int myresult = -1;

    for (size_t i = 0; i < android::emulation::s_adb_commands[on].size(); ++i) {
        DD("cmd %s", android::emulation::s_adb_commands[on][i].c_str());
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
