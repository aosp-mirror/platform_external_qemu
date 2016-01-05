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

#include "android/base/String.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include "android/crashreport/CrashService.h"

#include "android/base/system/Win32UnicodeString.h"
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "client/windows/crash_generation/crash_generation_server.h"

#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <sstream>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define CMD_BUF_SIZE 1024
#define HWINFO_CMD "dxdiag /dontskip /whql:off /64bit /t"

namespace android {

using ::android::base::String;
using ::android::base::PathUtils;
using ::android::base::System;

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
                                                         pipe.length());
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
        return mClientProcess != nullptr;
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

    virtual std::string getHWInfo() {
        std::ostringstream err_stream;

        mHWTmpFilePath.clear();
        System* sys = System::get();
        String tmp_dir = sys->getTempDir();

        char tmp_filename_buffer[CMD_BUF_SIZE] = {};
        DWORD temp_file_ret =
                GetTempFileName(tmp_dir.c_str(), "emu", 0, tmp_filename_buffer);

        if (!temp_file_ret) {
            err_stream << "Error: Can't create temporary file! error code="
                       << GetLastError() << std::endl;
            return err_stream.str();
        }

        String tmp_filename(tmp_filename_buffer);
        mHWTmpFilePath = tmp_filename.c_str();
        String syscmd(HWINFO_CMD);
        syscmd += tmp_filename;
        system(syscmd.c_str());

        FILE* hwinfo_fh = fopen(tmp_filename.c_str(), "r");

        if (!hwinfo_fh) {
            err_stream << "Error: Can't open temp file " << tmp_filename.c_str()
                       << " for reading. error code=" << GetLastError()
                       << std::endl;
            return err_stream.str();
        }

        fseek(hwinfo_fh, 0, SEEK_END);
        size_t fsize = ftell(hwinfo_fh);
        fseek(hwinfo_fh, 0, SEEK_SET);

        std::string out(fsize, '\0');
        fread(&out[0], fsize, 1, hwinfo_fh);
        fclose(hwinfo_fh);

        return out;
    }

    virtual void cleanupHWInfo() {
        DWORD del_ret = DeleteFile(mHWTmpFilePath.c_str());

        if (!del_ret) {
            // wait 100 ms
            Sleep(100);
            del_ret = DeleteFile(mHWTmpFilePath.c_str());
            if (!del_ret) {
                E("Error: Failed to delete temp file at %s. error code = %lu",
                  mHWTmpFilePath.c_str(), GetLastError());
            }
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
