// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/crashreport/CrashService.h"

#include "android/base/system/Win32UnicodeString.h"
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "client/windows/crash_generation/crash_generation_server.h"

#include <sys/types.h>
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

    virtual ~HostCrashService() {
        stopCrashServer();
        if (mClientProcess) {
            CloseHandle(mClientProcess);
        }
    }

    static void OnClientConnect(
            void* context,
            const google_breakpad::ClientInfo* client_info) {
        D("Client connected, pid = %d\n", client_info->pid());
        static_cast<CrashService::ServerState*>(context)->connected += 1;
    }

    static void OnClientDumpRequest(
            void* context,
            const google_breakpad::ClientInfo* client_info,
            const std::wstring* file_path) {
        ::android::base::String file_path_string =
                ::android::base::Win32UnicodeString::convertToUtf8(
                        file_path->c_str());
        D("Client Requesting dump %s\n", file_path_string.c_str());
        static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.assign(file_path_string.c_str());
    }

    static void OnClientExit(void* context,
                             const google_breakpad::ClientInfo* client_info) {
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

    bool startCrashServer(const std::string& pipe) override {
        if (mCrashServer) {
            return false;
        }
        initCrashServer();

        ::android::base::Win32UnicodeString pipe_unicode(pipe.c_str(),
                                                         strlen(pipe.c_str()));
        ::std::wstring pipe_string(pipe_unicode.data());
        ::android::base::Win32UnicodeString crashdir_unicode(
                ::android::crashreport::CrashSystem::get()
                        ->getCrashDirectory()
                        .c_str());
        std::wstring crashdir_wstr(crashdir_unicode.c_str());

        mCrashServer.reset(new ::google_breakpad::CrashGenerationServer(
                pipe_string, nullptr, OnClientConnect, &mServerState,
                OnClientDumpRequest, &mDumpRequestContext, OnClientExit,
                &mServerState, nullptr, nullptr, true, &crashdir_wstr));

        return mCrashServer->Start();
    }

    bool stopCrashServer() override {
        if (mCrashServer) {
            mCrashServer.reset();
            return true;
        } else {
            return false;
        }
    }

    bool setClient(int clientpid) override {
        if (mClientProcess) {
            CloseHandle(mClientProcess);
        }
        mClientProcess = OpenProcess(SYNCHRONIZE, FALSE, clientpid);
        return mClientProcess == nullptr;
    }

    bool isClientAlive() override {
        if (!mClientProcess) {
            return false;
        }
        if (WaitForSingleObject(mClientProcess, 0) != WAIT_TIMEOUT) {
            return false;
        } else {
            return true;
        }
    }

private:
    std::unique_ptr<::google_breakpad::CrashGenerationServer> mCrashServer;
    HANDLE mClientProcess = nullptr;
};

}  // namespace anonymous

CrashService* CrashService::makeCrashService(const std::string& version,
                                             const std::string& build) {
    return new HostCrashService(version, build);
}

}  // namespace crashreport
}  // namespace android
