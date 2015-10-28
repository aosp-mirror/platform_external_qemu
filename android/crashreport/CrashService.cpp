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

#include "android/base/files/IniFile.h"
#include "android/base/String.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#endif
#include "android/curl-support.h"
#if CONFIG_QT
#include "android/crashreport/ui/ConfirmDialog.h"
#endif
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#if defined(__linux__)
#include "client/linux/crash_generation/crash_generation_server.h"
#elif defined(__APPLE__)
#include "client/mac/crash_generation/crash_generation_server.h"
#elif defined(_WIN32)
#include "client/windows/crash_generation/crash_generation_server.h"
#else
#error Breakpad not supported on this platform
#endif
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/process_state.h"
#include "processor/stackwalk_common.h"

#if CONFIG_QT
#include "android/qt/qt_path.h"
#include <QApplication>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef ANDROID_SDK_TOOLS_REVISION
#define VERSION_STRING STRINGIFY(ANDROID_SDK_TOOLS_REVISION) ".0"
#else
#define VERSION_STRING "standalone"
#endif

#ifdef ANDROID_BUILD_NUMBER
#define BUILD_STRING STRINGIFY(ANDROID_BUILD_NUMBER)
#else
#define BUILD_STRING "0"
#endif

#define PRODUCT_VERSION VERSION_STRING "-" BUILD_STRING

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

const char* kNameKey = "prod";
const char* kVersionKey = "ver";
const char* kName = "AndroidEmulator";
const char* kVersion = PRODUCT_VERSION;

const char* kMessageBoxTitle = "Android Emulator";
const char* kMessageBoxMessage =
        "<p>Android Emulator closed unexpectedly.</p>"
        "<p>Do you want to send a crash report about the problem?</p>";
const char* kMessageBoxMessageDetail =
        "An error report containing the information shown below "
        "will be sent to Google's Android team to help identify "
        "and fix the problem. "
        "<a href=\"https://www.google.com/policies/privacy/\">Privacy "
        "Policy</a>.";

// Callback to get the response data from server.
size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    if (!userp)
        return 0;

    std::string* response = reinterpret_cast<std::string*>(userp);
    size_t real_size = size * nmemb;
    response->append(reinterpret_cast<char*>(ptr), real_size);
    return real_size;
}

bool uploadCrash(const std::string& crashdumpFilePath, std::string& reportID) {
    curl_init(::android::crashreport::CrashSystem::get()
                      ->getCaBundlePath()
                      .c_str());
    struct curl_httppost* formpost_ = NULL;
    struct curl_httppost* lastptr_ = NULL;
    std::string http_response_data;
    CURLcode curlRes;

    CURL* const curl = curl_easy_default_init();
    if (!curl) {
        E("Curl instantiation failed\n");
        return false;
    }

    curl_easy_setopt(
            curl, CURLOPT_URL,
            ::android::crashreport::CrashSystem::get()->getCrashURL().c_str());

    curl_formadd(&formpost_, &lastptr_, CURLFORM_COPYNAME, kNameKey,
                 CURLFORM_COPYCONTENTS, kName, CURLFORM_END);

    curl_formadd(&formpost_, &lastptr_, CURLFORM_COPYNAME, kVersionKey,
                 CURLFORM_COPYCONTENTS, kVersion, CURLFORM_END);

    curl_formadd(&formpost_, &lastptr_, CURLFORM_COPYNAME,
                 "upload_file_crashdump", CURLFORM_FILE,
                 crashdumpFilePath.c_str(), CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost_);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     reinterpret_cast<void*>(&http_response_data));

    curlRes = curl_easy_perform(curl);

    bool success = true;

    if (curlRes != CURLE_OK) {
        success = false;
        E("curl_easy_perform() failed with code %d (%s)", curlRes,
          curl_easy_strerror(curlRes));
    }

    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200) {
            E("Got HTTP response code %ld", http_response);
            success = false;
        }
    } else if (curlRes == CURLE_UNKNOWN_OPTION) {
        E("Can not get a valid response code: not supported");
        success = false;
    } else {
        E("Unexpected error while checking http response: %s",
          curl_easy_strerror(curlRes));
        success = false;
    }

    if (success) {
        I("Crash Send complete\n");
        I("Crash Report ID: %s\n", http_response_data.c_str());
        reportID.assign(http_response_data.c_str());
    }

    curl_formfree(formpost_);
    curl_easy_cleanup(curl);
    curl_cleanup();
    return success;
}

void cleanupCrash(const std::string& crashdumpFilePath) {
    remove(crashdumpFilePath.c_str());
}

std::string getCrashDetails(const char* crashPath) {
    std::string details;
    google_breakpad::BasicSourceLineResolver resolver;
    google_breakpad::MinidumpProcessor minidump_processor(NULL, &resolver);

    // Process the minidump.
    google_breakpad::Minidump dump(crashPath);
    if (!dump.Read()) {
        E("Minidump %s could not be read", dump.path().c_str());
        return details;
    }
    google_breakpad::ProcessState process_state;
    if (minidump_processor.Process(&dump, &process_state) !=
        google_breakpad::PROCESS_OK) {
        E("MinidumpProcessor::Process failed");
        return details;
    }

    // Capture printf's from PrintProcessState to file
    FILE* fp;
    std::string crashDetailsPath(crashPath);
    crashDetailsPath += ".details";

    fp = fopen(crashDetailsPath.c_str(), "w");
    if (fp == NULL) {
        E("Couldn't open file %s for writing\n", crashDetailsPath.c_str());
        return details;
    }
    google_breakpad::SetPrintStream(fp);
    google_breakpad::PrintProcessState(process_state, true, &resolver);
    fclose(fp);

    fp = fopen(crashDetailsPath.c_str(), "rb");
    if (fp == NULL) {
        E("Couldn't open file %s for reading\n", crashDetailsPath.c_str());
        return details;
    }
    fseek(fp, 0, SEEK_END);
    details.resize(std::ftell(fp));
    rewind(fp);
    fread(&details[0], 1, details.size(), fp);
    fclose(fp);

    // Cleanup
    remove(crashDetailsPath.c_str());
    return details;
}

#if CONFIG_QT
bool displayConfirmDialog(const char* crashPath) {
    static const char kIconFile[] = "emulator_icon_128.png";
    size_t icon_size;
    QPixmap icon;

    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    std::string details = getCrashDetails(crashPath);
    ConfirmDialog msgBox(NULL, icon, kMessageBoxTitle, kMessageBoxMessage,
                         kMessageBoxMessageDetail,
                         getCrashDetails(crashPath).c_str());
    msgBox.show();
    int ret = msgBox.exec();
    switch (ret) {
        case ConfirmDialog::Accepted:
            return true;
        default:
            break;
    }
    return false;
}

void InitQt(int argc, char** argv) {
    Q_INIT_RESOURCE(resources);

    // Give Qt the fonts from our resource file
    QFontDatabase fontDb;
    int fontId = fontDb.addApplicationFont(":/lib/fonts/Roboto");
    if (fontId < 0) {
        D("Count not load font resource: \":/lib/fonts/Roboto");
    }
    fontId = fontDb.addApplicationFont(":/lib/fonts/Roboto-Bold");
    if (fontId < 0) {
        D("Count not load font resource: \":/lib/fonts/Roboto-Bold");
    }
    fontId = fontDb.addApplicationFont(":/lib/fonts/Roboto-Medium");
    if (fontId < 0) {
        D("Count not load font resource: \":/lib/fonts/Roboto-Medium");
    }
}
#else  // Not QT, default to no confirmation
bool displayConfirmDialog(const char* crashPath) {
    return false;
}
#endif

struct DumpRequestContext {
    std::string file_path;
};

struct ServerState {
    bool waiting;
    int connected;
};

#if defined(__linux__)
void OnClientDumpRequest(void* context,
                         const google_breakpad::ClientInfo* client_info,
                         const std::string* file_path) {
    D("Client Requesting dump %s\n", file_path->c_str());
    static_cast<DumpRequestContext*>(context)
            ->file_path.assign(file_path->c_str());
}

void OnClientExit(void* context,
                  const google_breakpad::ClientInfo* client_info) {
    D("Client exiting\n");
    ServerState* serverstate = static_cast<ServerState*>(context);
    if (serverstate->connected > 0) {
        serverstate->connected -= 1;
    }
    if (serverstate->connected == 0) {
        serverstate->waiting = false;
    }
}

#elif defined(_WIN32)

void OnClientConnect(void* context,
                     const google_breakpad::ClientInfo* client_info) {
    D("Client connected, pid = %d\n", client_info->pid());
    static_cast<ServerState*>(context)->connected += 1;
}

void OnClientDumpRequest(void* context,
                         const google_breakpad::ClientInfo* client_info,
                         const std::wstring* file_path) {
    ::android::base::String file_path_string =
            ::android::base::Win32UnicodeString::convertToUtf8(
                    file_path->c_str());
    D("Client Requesting dump %s\n", file_path_string.c_str());
    static_cast<DumpRequestContext*>(context)
            ->file_path.assign(file_path_string.c_str());
}

void OnClientExit(void* context,
                  const google_breakpad::ClientInfo* client_info) {
    D("Client exiting\n");
    ServerState* serverstate = static_cast<ServerState*>(context);
    if (serverstate->connected > 0) {
        serverstate->connected -= 1;
    }
    if (serverstate->connected == 0) {
        serverstate->waiting = false;
    }
}

#elif defined(__APPLE__)

void OnClientDumpRequest(void* context,
                         const google_breakpad::ClientInfo& client_info,
                         const std::string& file_path) {
    D("Client Requesting dump %s\n", file_path.c_str());
    static_cast<DumpRequestContext*>(context)
            ->file_path.assign(file_path.c_str());
}

void OnClientExit(void* context,
                  const google_breakpad::ClientInfo& client_info) {
    D("Client exiting\n");
    ServerState* serverstate = static_cast<ServerState*>(context);
    if (serverstate->connected > 0) {
        serverstate->connected -= 1;
    }
    if (serverstate->connected == 0) {
        serverstate->waiting = false;
    }
}
#endif

/* Main routine */
int main(int argc, char** argv) {
    google_breakpad::CrashGenerationServer* server;

    DumpRequestContext dumpcontext;
    ServerState serverstate;

    const char* service_arg = NULL;
    const char* dump_file = NULL;
    int nn;
    int ppid = 0;
    for (nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];
        if (!strcmp(opt, "-pipe")) {
            if (nn + 1 < argc) {
                nn++;
                service_arg = argv[nn];
            }
        } else if (!strcmp(opt, "-dumpfile")) {
            if (nn + 1 < argc) {
                nn++;
                dump_file = argv[nn];
            }
        } else if (!strcmp(opt, "-ppid")) {
            if (nn + 1 < argc) {
                nn++;
                ppid = atoi(argv[nn]);
            }
        }
    }

    if (dump_file) {
        if (::android::base::System::get()->pathIsFile(dump_file)) {
            dumpcontext.file_path = dump_file;
        }
    } else if (service_arg && ppid) {
        serverstate.waiting = true;
        serverstate.connected = 0;
#if defined(__linux__)
        const int listen_fd = atoi(service_arg);
        server = new google_breakpad::CrashGenerationServer(
                listen_fd, OnClientDumpRequest, &dumpcontext, OnClientExit,
                &serverstate, true, &::android::crashreport::CrashSystem::get()
                                             ->getCrashDirectory());
#elif defined(_WIN32)
        const char* pipe_path = service_arg;
        ::android::base::Win32UnicodeString pipe_unicode(pipe_path,
                                                         strlen(pipe_path));
        ::std::wstring pipe_string(pipe_unicode.data());
        server = new google_breakpad::CrashGenerationServer(
                pipe_string, NULL, OnClientConnect, &serverstate,
                OnClientDumpRequest, &dumpcontext, OnClientExit, &serverstate,
                NULL, NULL, true, &::android::crashreport::CrashSystem::get()
                                           ->getWCrashDirectory());
#elif defined(__APPLE__)
        const char* mach_port_name = service_arg;
        server = new google_breakpad::CrashGenerationServer(
                mach_port_name, NULL, NULL, &OnClientDumpRequest, &dumpcontext,
                &OnClientExit, &serverstate, true,
                ::android::crashreport::CrashSystem::get()
                        ->getCrashDirectory());
#endif
        if (!server->Start()) {
            E("Server failure\n");
        }
        while (true) {
            if (!serverstate.waiting && serverstate.connected == 0) {
                break;
            }
#if defined(__linux__) || defined(__APPLE__)
            if (ppid && getppid() != ppid) {
                E("CrashService parent died unexpectedly\n");
                return 1;
            }
#elif defined(_WIN32)
            printf("Checking on %d\n", ppid);
            if (WaitForSingleObject(reinterpret_cast<HANDLE>(ppid), 0) !=
                WAIT_TIMEOUT) {
                E("CrashService parent died unexpectedly\n");
                return 1;
            }
#endif
            sleep_ms(1000);
        }
#ifndef _WIN32
        server->Stop();
#endif
    } else {
        E("Must supply a dump path\n");
    }
    // Check if crash_path exists and is valid
    std::string crashPath = dumpcontext.file_path;

    if (!::android::crashreport::CrashSystem::get()->isDump(
                crashPath.c_str())) {
        E("CrashPath '%s' is invalid\n", crashPath.c_str());
        return 1;
    }

#if CONFIG_QT
    QApplication app(argc, argv);

    InitQt(argc, argv);
#endif

    // Check confirmation before processing crash
    if (displayConfirmDialog(crashPath.c_str())) {
        std::string reportID;
        if (!uploadCrash(crashPath, reportID)) {
            exit(1);
        }
        D("Crash Report %s submitted\n", reportID.c_str());
    }
    cleanupCrash(crashPath);

    return 0;
}
