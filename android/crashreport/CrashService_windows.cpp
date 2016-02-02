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

#include "android/crashreport/CrashService_windows.h"

#include "android/base/String.h"
#include "android/base/system/System.h"

#include "android/base/system/Win32UnicodeString.h"
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"

#include <sys/types.h>
#include <unistd.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define CMD_BUF_SIZE 1024
#define HWINFO_CMD "dxdiag /dontskip /whql:off /64bit /t"

namespace android {

using ::android::base::String;
using ::android::base::System;

namespace crashreport {

HostCrashService::~HostCrashService() {
    stopCrashServer();
    cleanupHWInfo();
}

void HostCrashService::OnClientConnect(
        void* context,
        const google_breakpad::ClientInfo* client_info) {
    D("Client connected, pid = %d\n", client_info->pid());
    static_cast<CrashService::ServerState*>(context)->connected += 1;
}

void HostCrashService::OnClientDumpRequest(
        void* context,
        const google_breakpad::ClientInfo* client_info,
        const std::wstring* file_path) {
    if (static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.empty()) {
        ::android::base::String file_path_string =
                ::android::base::Win32UnicodeString::convertToUtf8(
                        file_path->c_str());
        D("Client Requesting dump %s\n", file_path_string.c_str());
        static_cast<CrashService::DumpRequestContext*>(context)
                ->file_path.assign(file_path_string.c_str());
    }
}

void HostCrashService::OnClientExit(
        void* context,
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

bool HostCrashService::startCrashServer(const std::string& pipe) {
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

bool HostCrashService::stopCrashServer() {
    if (mCrashServer) {
        mCrashServer.reset();
        return true;
    } else {
        return false;
    }
}

bool HostCrashService::setClient(int clientpid) {
    mClientProcess.reset(OpenProcess(SYNCHRONIZE, FALSE, clientpid));
    return mClientProcess.get() != nullptr;
}

bool HostCrashService::isClientAlive() {
    if (!mClientProcess) {
        return false;
    }
    if (WaitForSingleObject(mClientProcess.get(), 0) != WAIT_TIMEOUT) {
        return false;
    } else {
        return true;
    }
}

bool HostCrashService::getHWInfo() {
    mHWTmpFilePath.clear();
    System* sys = System::get();
    String tmp_dir = sys->getTempDir();

    char tmp_filename_buffer[CMD_BUF_SIZE] = {};
    DWORD temp_file_ret =
            GetTempFileName(tmp_dir.c_str(), "emu", 0, tmp_filename_buffer);

    if (!temp_file_ret) {
        E("Error: Can't create temporary file! error code=%d", GetLastError());
        return false;
    }

    String tmp_filename(tmp_filename_buffer);
    String syscmd(HWINFO_CMD);
    syscmd += tmp_filename;
    system(syscmd.c_str());

    mHWTmpFilePath = tmp_filename.c_str();
    return true;
}

void HostCrashService::cleanupHWInfo() {
    if (mHWTmpFilePath.empty()) {
        return;
    }

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

}  // namespace crashreport
}  // namespace android
