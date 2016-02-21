// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/opengl/emugl_config.h"

#include "android/base/files/PathUtils.h"
#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/Uuid.h"
#include "android/opengl/EmuglBackendList.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 1

#if DEBUG
#define D(...)  printf(__VA_ARGS__)
#else
#define D(...)  ((void)0)
#endif


using android::base::PathUtils;
using android::base::RunOptions;
using android::base::String;
using android::base::StringFormat;
using android::base::StringVector;
using android::base::System;
using android::base::Uuid;
using android::opengl::EmuglBackendList;

static EmuglBackendList* sBackendList = NULL;

static void resetBackendList(int bitness) {
    delete sBackendList;
    sBackendList = new EmuglBackendList(
            System::get()->getLauncherDirectory().c_str(), bitness);
}

static bool stringVectorContains(const StringVector& list,
                                 const char* value) {
    for (size_t n = 0; n < list.size(); ++n) {
        if (!strcmp(list[n].c_str(), value)) {
            return true;
        }
    }
    return false;
}

struct GpuInfo {
    char* make;
    char* model;
    char* device_id;
    char* revision_id;
    char* version;
    bool current_gpu;
};

static bool hostGPUBlacklisted() {

#define FIELD_LEN 2048
    fprintf(stderr, "test if gpu is blacklisted\n");
    fprintf(stderr, "get a temp file going\n");
    auto& sys = *System::get();

    std::string tmp_dir(sys.getTempDir().c_str());
    fprintf(stderr, "tmp dir=%s\n", tmp_dir.c_str());
    fprintf(stderr, "tmp dir strlen=%d\n", strlen(tmp_dir.c_str()));

    // Get temporary file path
#ifndef _WIN32 // Link error when using Uuid here for some reason
    // PathUtils::join and Uuid:: based files no good on OS X at this point
    std::string sep("");
    if (tmp_dir.back() != '/') {
        sep += "/";
    }
    std::string temp_filename_pattern = "gpuinfo_XXXXXX";
    std::string temp_file_path_template =
        (tmp_dir + sep + temp_filename_pattern);

    int tmpfd = mkstemp((char*)temp_file_path_template.data());

    if (tmpfd == -1) {
        fprintf(stderr, "Error: Can't create temporary file!"
                        "errno=%d\n", errno);
        return false;
    }

    const char* temp_file_path = temp_file_path_template.c_str();

#else
    char tmp_filename_buffer[FIELD_LEN] = {};
    DWORD temp_file_ret =
            GetTempFileName(tmp_dir.c_str(), "gpu", 0, tmp_filename_buffer);

    if (!temp_file_ret) {
        fprintf(stderr, "Error: can't create temporary file!"
                        "error code=%d\n", GetLastError());
        return false;
    }

    const char* temp_file_path = PathUtils::join(
            tmp_dir,
            std::string(tmp_filename_buffer)).c_str();
#endif

    fprintf(stderr, "tmp path=%s\n",
            temp_file_path);

#ifdef __APPLE__
    char command[FIELD_LEN] = {};
    snprintf(command, sizeof(command),
            "system_profiler SPDisplaysDataType > %s",
            temp_file_path_template.c_str());
    fprintf(stderr, "command=%s\n", command);
    fprintf(stderr, "Hi! I'm on Apple!\n");
    fprintf(stderr, "cmd... .. errno=%d\n", errno);
    system(command);

    // Read the file.
    
    fprintf(stderr, "fopen .. errno=%d\n", errno);
    FILE* fh = fopen(temp_file_path_template.c_str(), "r");
    if (!fh) {
        fprintf(stderr, "fopen fail. errno=%d\n", errno);
        D("%s: cannot fopen gpu info file."
          "default to not blacklisted.\n", __FUNCTION__);
        return false;
    }

    int fseek_ret;
    fseek_ret = fseek(fh, 0, SEEK_END);
    if (fseek_ret == -1) {
        fclose(fh);
        D("%s: cannot fseek gpu info file."
          "default to not blacklisted.\n", __FUNCTION__);
        return false;
    }

    uint64_t fsize = ftell(fh);
    fseek_ret = fseek(fh, 0, SEEK_SET);
    if (fseek_ret == -1) {
        fclose(fh);
        D("%s: cannot rewind gpu info file."
          "default to not blacklisted.\n", __FUNCTION__);
        return false;
    } else {
        fprintf(stderr, "file size = %" PRIu64 "bytes\n", fsize);
    }

    char* contents = new char[fsize];
    uint64_t read_bytes = fread(contents, fsize, 1, fh);
    if (!read_bytes) {
        fprintf(stderr, "cannot fread() the file handle. fail.\n");
        fclose(fh);
        return 1;
    }
    fclose(fh);


    // Parse file contents.

    char* p = contents;
    char* line_loc = strstr(p, "\n");
    char* kvsep_loc;
#define FIELD_SIZE 2048
#define MAX_GPUS 12
    char key[FIELD_SIZE] = {};
    char val[FIELD_SIZE] = {};

    GpuInfo** gpuinfos = new GpuInfo*[MAX_GPUS];
    memset(gpuinfos, 0, 12);

    int curr_gpu = 0;
    int num_lines = 0;
    while (line_loc) {
        kvsep_loc = strstr(p, ": ");
        if (kvsep_loc && kvsep_loc < line_loc) {
            int key_strlen = kvsep_loc - p;
            strncpy(key, p, key_strlen + 1);
            key[key_strlen] = '\0';

            int val_strlen = line_loc - (kvsep_loc + 2);
            strncpy(val, kvsep_loc + 2, val_strlen + 1);
            val[val_strlen] = '\0';

            fprintf(stderr, "key=[%s] val=[%s]\n", key, val);

            if (strstr(key, "Chipset Model")) {
                fprintf(stderr, "NEW GPU FOUND (GPU #%d)\n", curr_gpu);
                if (gpuinfos[curr_gpu] != NULL) {
                    curr_gpu++;
                }
                if (curr_gpu > MAX_GPUS - 1) {
                    // We currently only support detecting systems
                    // with <= MAX_GPUS GPU's.
                    curr_gpu--;
                    break;
                }
                gpuinfos[curr_gpu] = (GpuInfo*)malloc(sizeof(GpuInfo));
                gpuinfos[curr_gpu]->make = (char*)malloc(FIELD_SIZE);
                gpuinfos[curr_gpu]->model = (char*)malloc(FIELD_SIZE);
                gpuinfos[curr_gpu]->device_id = (char*)malloc(FIELD_SIZE);
                gpuinfos[curr_gpu]->revision_id = (char*)malloc(FIELD_SIZE);
                gpuinfos[curr_gpu]->version = (char*)malloc(FIELD_SIZE);
                gpuinfos[curr_gpu]->current_gpu = false;

                memset(gpuinfos[curr_gpu]->make, 0, FIELD_SIZE);
                memset(gpuinfos[curr_gpu]->model, 0, FIELD_SIZE);
                memset(gpuinfos[curr_gpu]->device_id, 0, FIELD_SIZE);
                memset(gpuinfos[curr_gpu]->revision_id, 0, FIELD_SIZE);
                memset(gpuinfos[curr_gpu]->version, 0, FIELD_SIZE);

                fprintf(stderr, "(found model)\n");
                strcpy(gpuinfos[curr_gpu]->model, val);
            } else if (strstr(key, "Vendor")) {
                fprintf(stderr, "(found make)\n");
                strcpy(gpuinfos[curr_gpu]->make, val);
            } else if (strstr(key, "Device ID")) {
                fprintf(stderr, "(found deviceid)\n");
                strcpy(gpuinfos[curr_gpu]->device_id, val);
            } else if (strstr(key, "Revision ID")) {
                strcpy(gpuinfos[curr_gpu]->revision_id, val);
            } else if (strstr(key, "Display Type")) {
                fprintf(stderr, "(found that this is the gpu rendering the things)\n");
                gpuinfos[curr_gpu]->current_gpu = true;
            } else {
                fprintf(stderr, "(ignore)\n");
            }
        }
        num_lines++;
        fprintf(stderr, "num_lines=%d\n", num_lines);
        p = line_loc + 1;
        line_loc = strstr(p, "\n");
    }

    fprintf(stderr, "-=GPU INFO=-\n");
    for (int i = 0; i < curr_gpu + 1; i++) {
        fprintf(stderr, "GPU #%d {\n", i);
        fprintf(stderr, "\tMake:[%s]\n", gpuinfos[i]->make);
        fprintf(stderr, "\tModel:[%s]\n", gpuinfos[i]->model);
        fprintf(stderr, "\tDevice ID:[%s]\n", gpuinfos[i]->device_id);
        fprintf(stderr, "\tRevision ID:[%s]\n", gpuinfos[i]->revision_id);
        fprintf(stderr, "\tVersion:[%s]\n", gpuinfos[i]->version);
        fprintf(stderr, "\tActive?:[%d]\n", gpuinfos[i]->current_gpu);
        fprintf(stderr, "}\n");
    }

#endif

#ifdef _WIN32
    char temp_path_arg[FIELD_LEN] = {};
    snprintf(temp_path_arg, sizeof(temp_path_arg),
            "/OUTPUT:%s", temp_file_path);
    fprintf(stderr, "temp_path_arg=%s\n", temp_path_arg);
    fprintf(stderr, "os=Windows\n");
    auto exitCode = sys.runCommand({"wmic", temp_path_arg, "path", "Win32_VideoController", "get", "/value"}, RunOptions::WaitForCompletion | RunOptions::ShowOutput);
    FILE* fh = fopen(temp_file_path, "r");
    if (!fh) {
        D("%s: cannot open gpuinfo.txt. defaulting to not blacklisted.\n", __FUNCTION__);
        return false;
    }

    int fseek_ret;
    fseek_ret = fseek(fh, 0, SEEK_END);
    if (fseek_ret == -1) {
        D("%s: invalid gpuinfo.txt file. defaulting to not blacklisted.", __FUNCTION__);
        fclose(fh);
        return false;
    }

    uint64_t fsize = ftell(fh); // This is never big.
    fseek_ret = fseek(fh, 0, SEEK_SET); // Rewind the fh
    if (fseek_ret == -1) {
        D("%s: failed to fseek to beginning of file. defaulting to not blacklisted.\n", __FUNCTION__);
        fclose(fh);
        return false;
    } else {
        fprintf(stderr, "fsize=%llu\n", fsize);
    }

    char* res = new char[fsize];
    uint64_t read_bytes = fread(res, fsize, 1, fh);
    if (!read_bytes) {
        D("%s: error reading file %s\n", __FUNCTION__, "gpuinfo.txt");
        fclose(fh);
        return false;
    }

    fclose(fh);

    fprintf(stderr, "TCHAR width=%d\n", sizeof(TCHAR));

    wchar_t* gpuinfo = (wchar_t*)res;
    wchar_t* test = L"Hello World";
    if (wcsstr(test, L"Hello")) { D("%s: hello works\n", __FUNCTION__); }
    if (wcsstr(test, L"World")) { D("%s: world works\n", __FUNCTION__); }

    wchar_t* p = gpuinfo;
    wchar_t* line_end_pos;
    wchar_t* equals_pos;
    wchar_t* val_pos;
    wchar_t curr_key[FIELD_LEN] = {};
    wchar_t curr_val[FIELD_LEN] = {};

    line_end_pos = wcsstr(p, L"\r\n");
    int num_lines = 0;
    while (line_end_pos) {
        equals_pos = wcsstr(p, L"=");
        if (equals_pos && equals_pos < line_end_pos) {
            int key_strlen = equals_pos - p;
            wcsncpy(curr_key, p, key_strlen + 1);
            curr_key[key_strlen] = L'\0';

            val_pos = equals_pos + 1;
            int val_strlen = line_end_pos - val_pos;
            wcsncpy(curr_val, val_pos, val_strlen + 1);
            curr_val[val_strlen] = L'\0';

            wprintf(L"KEY=[%ls] VAL=[%ls]\n", curr_key, curr_val);
        }

        num_lines++;
        p = line_end_pos;
        p += 2;
        D("%s: num_lines=%d\n", __FUNCTION__, num_lines);
        line_end_pos = wcsstr(p, L"\r\n");
    }
#endif
    return false;
}

bool emuglConfig_init(EmuglConfig* config,
                      bool gpu_enabled,
                      const char* gpu_mode,
                      const char* gpu_option,
                      int bitness,
                      bool no_window) {
    fprintf(stderr, "%s: call\n", __FUNCTION__);
    // zero all fields first.
    memset(config, 0, sizeof(*config));

    // The value of '-gpu <mode>' overrides the hardware properties,
    // except if <mode> is 'auto'.
    if (gpu_option) {
        if (!strcmp(gpu_option, "on") || !strcmp(gpu_option, "enable")) {
            gpu_enabled = true;
            if (!gpu_mode || !strcmp(gpu_mode, "auto")) {
                gpu_mode = "host";
            }
        } else if (!strcmp(gpu_option, "off") ||
                   !strcmp(gpu_option, "disable")) {
            gpu_enabled = false;
        } else if (!strcmp(gpu_option, "auto")) {
            // Nothing to do
        } else {
            gpu_enabled = true;
            gpu_mode = gpu_option;
        }
    }

    if (!gpu_enabled) {
        config->enabled = false;
        snprintf(config->status, sizeof(config->status),
                 "GPU emulation is disabled");
        return true;
    }

    if (!bitness) {
        bitness = System::get()->getProgramBitness();
    }
    config->bitness = bitness;
    resetBackendList(bitness);

    // Check that the GPU mode is a valid value. 'auto' means determine
    // the best mode depending on the environment. Its purpose is to
    // enable 'mesa' mode automatically when NX or Chrome Remote Desktop
    // is detected.
    if (!strcmp(gpu_mode, "auto") || !strcmp(gpu_option, "auto")) {
        // The default will be 'host' unless:
        // 1. NX or Chrome Remote Desktop is detected, or |no_window| is true.
        // 2. Failing #1, the user's host GPU is on the blacklist.
        
        String sessionType;
        if (System::get()->isRemoteSession(&sessionType)) {
            D("%s: %s session detected\n", __FUNCTION__, sessionType.c_str());
            if (!sBackendList->contains("mesa")) {
                config->enabled = false;
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled under %s without Mesa",
                        sessionType.c_str());
                return true;
            }
            D("%s: 'mesa' mode auto-selected\n", __FUNCTION__);
            gpu_mode = "mesa";
        } else if (no_window) {
            if (stringVectorContains(sBackendList->names(), "mesa")) {
                D("%s: Headless (-no-window) mode, using Mesa backend\n",
                  __FUNCTION__);
                gpu_mode = "mesa";
            } else {
                D("%s: Headless (-no-window) mode without Mesa, forcing '-gpu off'\n",
                  __FUNCTION__);
                config->enabled = false;
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled (-no-window without Mesa)");
                return true;
            }
        } else if (hostGPUBlacklisted()) {
            D("%s: GPU ON BLACKLIST\n", __FUNCTION__);
        } else {
            D("%s: 'host' mode auto-selected\n", __FUNCTION__);
            gpu_mode = "host";
        }
    }

    // 'host' is a special value corresponding to the default translation
    // to desktop GL, 'guest' does not use host-side emulation,
    // anything else must be checked against existing host-side backends.
    if (strcmp(gpu_mode, "host") != 0 && strcmp(gpu_mode, "guest") != 0) {
        const StringVector& backends = sBackendList->names();
        if (!stringVectorContains(backends, gpu_mode)) {
            String error = StringFormat(
                "Invalid GPU mode '%s', use one of: on off host guest", gpu_mode);
            for (size_t n = 0; n < backends.size(); ++n) {
                error += " ";
                error += backends[n];
            }
            config->enabled = false;
            snprintf(config->status, sizeof(config->status), "%s",
                     error.c_str());
            return false;
        }
    }
    config->enabled = true;
    snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
    snprintf(config->status, sizeof(config->status),
             "GPU emulation enabled using '%s' mode", gpu_mode);
    return true;
}

void emuglConfig_setupEnv(const EmuglConfig* config) {
    System* system = System::get();

    if (!config->enabled) {
        // There is no real GPU emulation. As a special case, define
        // SDL_RENDER_DRIVER to 'software' to ensure that the
        // software SDL renderer is being used. This allows one
        // to run with '-gpu off' under NX and Chrome Remote Desktop
        // properly.
        system->envSet("SDL_RENDER_DRIVER", "software");
        return;
    }

    // $EXEC_DIR/<lib>/ is already added to the library search path by default,
    // since generic libraries are bundled there. We may need more though:
    resetBackendList(config->bitness);
    if (strcmp(config->backend, "host") != 0) {
        // If the backend is not 'host', we also need to add the
        // backend directory.
        String dir = sBackendList->getLibDirPath(config->backend);
        if (dir.size()) {
            D("Adding to the library search path: %s\n", dir.c_str());
            system->addLibrarySearchDir(dir);
        }
    }

    if (!strcmp(config->backend, "host")) {
        // Nothing more to do for the 'host' backend.
        return;
    }

    // For now, EmuGL selects its own translation libraries for
    // EGL/GLES libraries, unless the following environment
    // variables are defined:
    //    ANDROID_EGL_LIB
    //    ANDROID_GLESv1_LIB
    //    ANDROID_GLESv2_LIB
    //
    // If a backend provides one of these libraries, use it.
    String lib;
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_EGL, &lib)) {
        system->envSet("ANDROID_EGL_LIB", lib);
    }
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv1, &lib)) {
        system->envSet("ANDROID_GLESv1_LIB", lib);
    }
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv2, &lib)) {
        system->envSet("ANDROID_GLESv2_LIB", lib);
    }

    if (!strcmp(config->backend, "mesa")) {
        system->envSet("ANDROID_GL_LIB", "mesa");
        system->envSet("ANDROID_GL_SOFTWARE_RENDERER", "1");
    }
}
