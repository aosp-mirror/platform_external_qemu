/* Copyright (C) 2011 The Android Open Source Project
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

#include "android/crashreport/CrashService.h"
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#include "client/mac/crash_generation/crash_generation_server.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

namespace android {
namespace crashreport {

namespace {

class HostCrashService : public CrashService {
public:
    // Inherit CrashServer constructor
    using CrashService::CrashService;

    virtual ~HostCrashService() { stopCrashServer(); }

    static void OnClientDumpRequest(
            void* context,
            const google_breakpad::ClientInfo& client_info,
            const std::string& file_path) {
        D("Client Requesting dump %s\n", file_path.c_str());
        static_cast<CrashService::DumpRequestContext*>(context)->file_path =
                file_path;
    }

    static void OnClientExit(void* context,
                             const google_breakpad::ClientInfo& client_info) {
        D("Client exiting\n");
        CrashService::ServerState* serverstate =
                static_cast<CrashService::ServerState*>(context);
        if (serverstate->connected > 0) {
            serverstate->connected -= 1;
        }
        if (serverstate->connected == 0) {
            serverstate->waiting = false;
        }
    }

    bool startCrashServer(const std::string pipe) {
        if (mCrashServer) {
            return false;
        }
        initCrashServer();

        mCrashServer.reset(new ::google_breakpad::CrashGenerationServer(
                pipe.c_str(), NULL, NULL, &OnClientDumpRequest, &mDumpRequestContext,
                &OnClientExit, &mServerState, true,
                CrashSystem::get()->getCrashDirectory()));

        return mCrashServer->Start();
    }

    bool stopCrashServer() {
        if (mCrashServer) {
            mCrashServer.reset();
            return true;
        } else {
            return false;
        }
    }

    bool isClientAlive(const int clientpid) {
        waitpid(clientpid, NULL, WNOHANG);
        if (clientpid && kill(clientpid, 0) != 0) {
            return false;
        } else {
            return true;
        }
    }

private:
    std::unique_ptr<::google_breakpad::CrashGenerationServer> mCrashServer;
};

}  // namespace anonymous

CrashService* CrashService::makeCrashService(const std::string& version,
                                             const std::string& build) {
    return new HostCrashService(version, build);
}

}  // namespace crashreport
}  // namespace android
