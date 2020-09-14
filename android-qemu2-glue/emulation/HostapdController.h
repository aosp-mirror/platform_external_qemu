/* Copyright 2020 The Android Open Source Project
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
#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "android/base/Compiler.h"
#include "android/base/synchronization/Event.h"
#include "android/base/sockets/ScopedSocket.h"

namespace android {
namespace qemu2 {
class HostapdController {
public:
	static HostapdController* getInstance();
	bool initAndRun(bool verbose);
	void setDriverSocket(android::base::ScopedSocket sock);
	bool addSsid(std::string ssid, std::string passphrase);
	bool blockOnSsid(std::string ssid, bool blocked);
	bool isBlocked(std::string ssid);
	void terminate(bool wait);
    struct AccessPoint {
        AccessPoint() : blocked(false) {}
        std::string ifName;
        std::string ssid;
        std::string password;
        bool blocked;
    };

    enum class DebugLevel {
		MSG_EXCESSIVE, MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR
	};
private:
	HostapdController();
	static void enterMainLoop();
	std::string writeConfig();
	std::string mTemplateConf;
	std::unordered_map<std::string, AccessPoint> mAccessPoints;
	std::unordered_set<std::string> mUsedInterfaces;
	int mLowestInterfaceNumber;
	bool mRunning;
	std::unique_ptr<android::base::Event> mEvent;
	android::base::ScopedSocket mSock;
};
} // namespace qemu2
} // namespace android
