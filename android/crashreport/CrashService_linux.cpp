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

#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "client/linux/crash_generation/crash_generation_server.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define CMD_BUF_SIZE 1024
#define HWINFO_CMD "lshw"

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
            const ::google_breakpad::ClientInfo* client_info,
            const ::std::string* file_path) {
        D("Client Requesting dump %s\n", file_path->c_str());
        static_cast<CrashService::DumpRequestContext*>(context)->file_path =
                *file_path;
    }

    static void OnClientExit(void* context,
                             const ::google_breakpad::ClientInfo* client_info) {
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
        const int listen_fd = std::stoi(pipe);
        if (mCrashServer) {
            return false;
        }

        initCrashServer();

        mCrashServer.reset(new ::google_breakpad::CrashGenerationServer(
                listen_fd, OnClientDumpRequest, &mDumpRequestContext,
                OnClientExit, &mServerState, true,
                &CrashSystem::get()->getCrashDirectory()));

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

    bool isClientAlive() override {
        if (!mClientPID) {
            return 0;
        }
        // waitpid for child clients
        waitpid(mClientPID, nullptr, WNOHANG);
        // kill with 0 signal will return non 0 if process does not exist
        if (kill(mClientPID, 0) != 0) {
            return false;
        } else {
            return true;
        }
    }

    virtual std::string getHWInfo() const {

        char tmp_dir[CMD_BUF_SIZE] = {};
        char tmp_filename[CMD_BUF_SIZE] = {};
        char syscmd[CMD_BUF_SIZE] = {};
        char errstr[CMD_BUF_SIZE] = {};

        if (getenv("TMPDIR")) {
            snprintf(tmp_dir, CMD_BUF_SIZE, "%s", getenv("TMPDIR"));
        } else {
            if (P_tmpdir) {
                snprintf(tmp_dir, CMD_BUF_SIZE, "%s", P_tmpdir);
            } else {
                snprintf(tmp_dir, CMD_BUF_SIZE, "/tmp/");
            }
        }

        // check for slash at the end. add if missing
        if (tmp_dir[strlen(tmp_dir) - 1] != '/') {
            strcat(tmp_dir, "/");
        }

        snprintf(tmp_filename, CMD_BUF_SIZE, "%s%s", tmp_dir, "android_emulator_crash_report_XXXXXX");

        int tmpfd = mkstemp(tmp_filename);
        if (tmpfd == -1) {
            snprintf(errstr, CMD_BUF_SIZE, "Error: Can't create temporary file! errno=%d\n", errno);
            return std::string(errstr);
        }

        close(tmpfd);
        snprintf(syscmd, CMD_BUF_SIZE, HWINFO_CMD " > %s", tmp_filename);
        system(syscmd);

        FILE* hwinfo_fh = fopen(tmp_filename, "r");

        if (!hwinfo_fh) {
            snprintf(errstr, CMD_BUF_SIZE, "Error: Can't open temp file %s for reading. errno=%d\n", tmp_filename, errno);
            return std::string(errstr);
        }

        fseek(hwinfo_fh, 0, SEEK_END);
        size_t fsize = ftell(hwinfo_fh);
        fseek(hwinfo_fh, 0, SEEK_SET);

        char* out = (char*)malloc(fsize);
        fread(out, fsize, 1, hwinfo_fh);
        fclose(hwinfo_fh);

        int rm_ret = remove(tmp_filename);

        if (rm_ret == -1) {
            snprintf(errstr, CMD_BUF_SIZE, "Error: Failed to delete temp file at %s ; errno=%d\n", tmp_filename, errno);
        }

        std::string str_res(out);
        free(out);
        return str_res + std::string(errstr);
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
