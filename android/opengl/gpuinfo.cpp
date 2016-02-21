// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/system/System.h"
#include "android/opengl/gpuinfo.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using android::base::RunOptions;
using android::base::System;

char* getcharptr(std::string& s) {
    return &s[0];
}
    
#ifdef _WIN32
void GpuInfo::convert(std::string& str, wchar_t* input, int len) {
    wcstombs(getcharptr(str), input, len);
}
#else
void GpuInfo::convert(std::string& str, char* input, int len) {
    strncpy(getcharptr(str), input, len);
}
#endif

void GpuInfo::setMake(INPUT_CHAR_TYPE* val_str, int len) {
    make.resize(len); convert(make, val_str, len);
}

void GpuInfo::setModel(INPUT_CHAR_TYPE* val_str, int len) {
    model.resize(len); convert(model, val_str, len);
}

void GpuInfo::setVersion(INPUT_CHAR_TYPE* val_str, int len) {
    version.resize(len); convert(version, val_str, len);
}

void GpuInfo::setDeviceId(INPUT_CHAR_TYPE* val_str, int len) {
    device_id.resize(len); convert(device_id, val_str, len);
}

void GpuInfo::setRevisionId(INPUT_CHAR_TYPE* val_str, int len) {
    revision_id.resize(len); convert(revision_id, val_str, len);
}

void GpuInfo::setRenderer(INPUT_CHAR_TYPE* val_str, int len) {
    renderer.resize(len); convert(renderer, val_str, len);
}

void GpuInfo::setCurrent() {
    current_gpu = true;
}

void GpuInfo::addDll(INPUT_CHAR_TYPE* dll_str, int len) {
    std::string dll;
    dll.resize(len);
    convert(dll, dll_str, len);
    dlls.push_back(dll);
}

void GpuInfo::addDll(INPUT_CHAR_TYPE* dll_str) {
#ifdef _WIN32
    addDll(dll_str, wcslen(dll_str));
#else
    addDll(dll_str, strlen(dll_str));
#endif
}

void GpuInfoList::addGpu() { infos.push_back(GpuInfo()); }
GpuInfo& GpuInfoList::currGpu() { return infos.back(); }

void GpuInfoList::dump() {
    fprintf(stderr, "-=GPU INFO=-\n");
    for (int i = 0; i < infos.size(); i++) {
        fprintf(stderr, "GPU #%d {\n", i);
        fprintf(stderr, "\tMake:[%s]\n", infos[i].make.c_str());
        fprintf(stderr, "\tModel:[%s]\n", infos[i].model.c_str());
        fprintf(stderr, "\tDevice ID:[%s]\n", infos[i].device_id.c_str());
        fprintf(stderr, "\tRevision ID:[%s]\n", infos[i].revision_id.c_str());
        fprintf(stderr, "\tVersion:[%s]\n", infos[i].version.c_str());
        fprintf(stderr, "\tRenderer:[%s]\n", infos[i].renderer.c_str());
        fprintf(stderr, "\tActive?:[%d]\n", infos[i].current_gpu);
        fprintf(stderr, "\tDLLS:\n");
        for (int j = 0; j < infos[i].dlls.size(); j++) {
            fprintf(stderr, "\t\t[%s]\n", infos[i].dlls[j].c_str());
        }
        fprintf(stderr, "}\n");
    }
}

// Actual blacklist starts here. 
// Most entries imported from Chrome blacklist.
static const BlacklistEntry sGpuBlacklist[] = {

    // Make | Model | DeviceID | RevisionID | DriverVersion | Renderer
    
    {NULL, NULL, "0x7249", NULL, NULL, NULL}, // ATI Radeon X1900 on Mac
    {"8086", NULL, NULL, NULL, NULL, "Mesa"}, // Linux, Intel, Mesa
    {"8086", NULL, NULL, NULL, NULL, "mesa"}, // Linux, Intel, Mesa

    {"8086", NULL, "27ae", NULL, NULL, NULL}, // Intel 945 Chipset
    {"1002", NULL, NULL, NULL, NULL, NULL}, // Linux, ATI

    {NULL, NULL, "0x9583", NULL, NULL, NULL}, // ATI Radeon HD2600 on Mac
    {NULL, NULL, "0x94c8", NULL, NULL, NULL}, // ATI Radeon HD2400 on Mac

    {"NVIDIA (0x10de)", NULL, "0x0324", NULL, NULL, NULL}, // NVIDIA GeForce FX Go5200 (Mac)
    {"NVIDIA", "NVIDIA GeForce FX Go5200", NULL, NULL, NULL, NULL}, // NVIDIA GeForce FX Go5200 (Win)
    {"10de", NULL, "0324", NULL, NULL, NULL}, // NVIDIA GeForce FX Go5200 (Linux)

    {"10de", NULL, "029e", NULL, NULL, NULL}, // NVIDIA Quadro FX 1500 (Linux)

    {"NVIDIA (0x10de)", NULL, "0x0393", NULL, NULL, NULL}, // NVIDIA GeForce 7300 GT (Mac)
};

static const int sBlacklistSize = sizeof(sGpuBlacklist) / sizeof(BlacklistEntry);

// If any blacklist entry matches any gpu, return true.
bool gpuinfo_query_blacklist(GpuInfoList* gpulist) {
    gpulist->dump();
    for (int i = 0; i < gpulist->infos.size(); i++) {
        const char* bl_make = NULL;
        const char* bl_model = NULL;
        const char* bl_device_id = NULL;
        const char* bl_revision_id = NULL;
        const char* bl_version = NULL;
        for (int j = 0; j < sBlacklistSize; j++) {
            bl_make = sGpuBlacklist[j].make;
            bl_model = sGpuBlacklist[j].model;
            bl_device_id = sGpuBlacklist[j].device_id;
            bl_revision_id = sGpuBlacklist[j].revision_id;
            bl_version = sGpuBlacklist[j].version;

            if (bl_make && strcmp(bl_make, gpulist->infos[i].make.c_str())) continue;
            if (bl_model && strcmp(bl_model, gpulist->infos[i].model.c_str())) continue;
            if (bl_device_id && strcmp(bl_device_id, gpulist->infos[i].device_id.c_str())) continue;
            if (bl_revision_id && strcmp(bl_revision_id, gpulist->infos[i].revision_id.c_str())) continue;
            if (bl_version && strcmp(bl_version, gpulist->infos[i].version.c_str())) continue;

            fprintf(stderr, "There may be a problem with your GPU drivers."
                            " hw.gpu.mode='auto' will now select"
                            " software rendering.\n");
            return true;
        }
    }
    return false;
}

char* load_gpu_info() {
    auto& sys = *System::get();

    std::string tmp_dir(sys.getTempDir().c_str());

    // Get temporary file path
#ifndef _WIN32
    std::string sep("");
    if (tmp_dir.back() != '/') {
        sep += "/";
    }
    std::string temp_filename_pattern = "gpuinfo_XXXXXX";
    std::string temp_file_path_template =
        (tmp_dir + sep + temp_filename_pattern);

    int tmpfd = mkstemp((char*)temp_file_path_template.data());

    if (tmpfd == -1) {
        return NULL;
    }

    const char* temp_file_path = temp_file_path_template.c_str();

#else
    char tmp_filename_buffer[FIELD_LEN] = {};
    DWORD temp_file_ret =
            GetTempFileName(tmp_dir.c_str(), "gpu", 0, tmp_filename_buffer);

    if (!temp_file_ret) {
        return NULL;
    }

    const char* temp_file_path = tmp_filename_buffer;

#endif

    // Execute the command to get GPU info.
#ifdef __APPLE__
    char command[FIELD_LEN] = {};
    snprintf(command, sizeof(command),
            "system_profiler SPDisplaysDataType > %s",
            temp_file_path);
    system(command);
#elif !defined(_WIN32)
    char command[FIELD_LEN] = {};

    snprintf(command, sizeof(command),
            "lspci -mvnn > %s",
            temp_file_path);
    system(command);
    snprintf(command, sizeof(command),
            "glxinfo >> %s",
            temp_file_path);
    system(command);
#else
    char temp_path_arg[FIELD_LEN] = {};
    snprintf(temp_path_arg, sizeof(temp_path_arg),
            "/OUTPUT:%s", temp_file_path);
    sys.runCommand({"wmic", temp_path_arg, "path", "Win32_VideoController", "get", "/value"}, RunOptions::WaitForCompletion);
#endif

    // Read the file
    FILE* fh = fopen(temp_file_path, "r");
    if (!fh) {
#ifdef _WIN32
        // try again
        Sleep(100);
        fh = fopen(temp_file_path, "r");
#endif
        if (!fh) {
            return NULL;
        }
    }
    
    int fseek_ret;
    fseek_ret = fseek(fh, 0, SEEK_END);
    if (fseek_ret == -1) {
        fclose(fh);
        return NULL;
    }

    uint64_t fsize = ftell(fh);
    fseek_ret = fseek(fh, 0, SEEK_SET);
    if (fseek_ret == -1) {
        fclose(fh);
        return NULL;
    }

    char* contents = new char[fsize];
    uint64_t read_bytes = fread(contents, fsize, 1, fh);
    if (!read_bytes) {
        fclose(fh);
        return NULL;
    }
    fclose(fh);


#ifndef _WIN32
    remove(temp_file_path);
#else
    DWORD del_ret = DeleteFile(temp_file_path);
    if (!del_ret) {
        Sleep(100);
        del_ret = DeleteFile(temp_file_path);
    }
#endif

    return contents;
}

void parse_last_hexbrackets(char* str, char* out, int* out_len) {
    int n = strlen(str);
    int i = n;
    int j = n;
    while (str[i] != ']') { i--; }
    while (str[j] != '[') { j--; }

    int pos = j + 1;
    *out_len = i - pos;
    strncpy(out, str + pos, *out_len + 1);
    out[*out_len] = '\0';
}

void parse_renderer_part(char* str, char* out, int* out_len) {
    int n = strlen(str);
    int sp1 = n;
    int sp2 = n;
    while (str[sp1] != ' ') { sp1--; }
    while (sp2 == sp1 || str[sp2] != ' ') { sp2--; }


    int pos = sp2 + 1;
    *out_len = sp1 - pos;
    strncpy(out, str + pos, *out_len + 1);
    out[*out_len] = '\0';
}


void parse_gpu_info_list(char* contents, GpuInfoList* gpulist) {
#ifdef __APPLE__
    char* p = contents;
    char* line_loc = strstr(p, "\n");
    char* kvsep_loc;
    char key[FIELD_LEN] = {};
    char val[FIELD_LEN] = {};

    int num_lines = 0;

    // OS X
    while (line_loc) {
        kvsep_loc = strstr(p, ": ");
        if (kvsep_loc && kvsep_loc < line_loc) {
            int key_strlen = kvsep_loc - p;
            strncpy(key, p, key_strlen + 1);
            key[key_strlen] = '\0';

            int val_strlen = line_loc - (kvsep_loc + 2);
            strncpy(val, kvsep_loc + 2, val_strlen + 1);
            val[val_strlen] = '\0';

            if (strstr(key, "Chipset Model")) {
                gpulist->addGpu();
                gpulist->currGpu().setModel(val, val_strlen);
            } else if (strstr(key, "Vendor")) {
                gpulist->currGpu().setMake(val, val_strlen);
            } else if (strstr(key, "Device ID")) {
                gpulist->currGpu().setDeviceId(val, val_strlen);
            } else if (strstr(key, "Revision ID")) {
                gpulist->currGpu().setRevisionId(val, val_strlen);
            } else if (strstr(key, "Display Type")) {
                gpulist->currGpu().setCurrent();
            } else {
            }
        }
        num_lines++;
        p = line_loc + 1;
        line_loc = strstr(p, "\n");
    }
#elif !defined(_WIN32)
    char* p = contents;
    char* line_loc = strstr(p, "\n");
    char key[FIELD_LEN] = {};
    char val[FIELD_LEN] = {};

    int num_lines = 0;
    int val_strlen = 0;

    bool lookfor = false;

    // Linux - Only support one GPU for now.
    while (line_loc) {
        int key_strlen = line_loc - p;
        strncpy(key, p, key_strlen + 1);
        key[key_strlen] = '\0';

        if (!lookfor && strstr(key, "VGA")) {
            lookfor = true;
            gpulist->addGpu();
        } else if (lookfor && strstr(key, "Vendor")) {
            parse_last_hexbrackets(key, val, &val_strlen);
            gpulist->currGpu().setMake(val, val_strlen);
        } else if (lookfor && strstr(key, "Device")) {
            parse_last_hexbrackets(key, val, &val_strlen);
            gpulist->currGpu().setDeviceId(val, val_strlen);
            lookfor = false;
        } else if (strstr(key, "OpenGL version string")) {
            parse_renderer_part(key, val, &val_strlen);
            gpulist->currGpu().setRenderer(val, val_strlen);
        } else {

        }
        num_lines++;
        p = line_loc + 1;
        line_loc = strstr(p, "\n");
    }
#else
    wchar_t* gpuinfo = (wchar_t*)contents;
    wchar_t* p = gpuinfo;
    wchar_t* line_end_pos;
    wchar_t* equals_pos;
    wchar_t* val_pos;
    wchar_t key[FIELD_LEN] = {};
    wchar_t val[FIELD_LEN] = {};

    line_end_pos = wcsstr(p, L"\r\n");
    int num_lines = 0;
    // WINDOWS
    while (line_end_pos) {
        equals_pos = wcsstr(p, L"=");
        if (equals_pos && equals_pos < line_end_pos) {
            int key_strlen = equals_pos - p;
            wcsncpy(key, p, key_strlen + 1);
            key[key_strlen] = L'\0';

            val_pos = equals_pos + 1;
            int val_strlen = line_end_pos - val_pos;
            wcsncpy(val, val_pos, val_strlen + 1);
            val[val_strlen] = L'\0';

            if (wcsstr(key, L"AdapterCompatibility")) {
                gpulist->addGpu();
                gpulist->currGpu().setMake(val, val_strlen);
            } else if (wcsstr(key, L"Caption")) {
                gpulist->currGpu().setModel(val, val_strlen);
            } else if (wcsstr(key, L"DriverVersion")) {
                gpulist->currGpu().setVersion(val, val_strlen);
            } else if (wcsstr(key, L"InstalledDisplayDrivers")) {
                if (val_strlen == 0) {
                    continue;
                }
                wchar_t* v_p = val;
                wchar_t* val_end = v_p + val_strlen;
                wchar_t* dll_sep_loc = wcsstr(v_p, L",");
                wchar_t* dll_pos = v_p;
                wchar_t* dll_end = dll_sep_loc ? dll_sep_loc : val_end;
                int curr_dll_len = dll_end - dll_pos;

                gpulist->currGpu().addDll(dll_pos, curr_dll_len);

                while (dll_sep_loc) {

                    v_p = dll_sep_loc + 1;
                    dll_sep_loc = wcsstr(v_p, L",");
                    dll_pos = v_p;

                    dll_end = dll_sep_loc ? dll_sep_loc : val_end;
                    curr_dll_len = dll_end - dll_pos;

                    gpulist->currGpu().addDll(dll_pos, curr_dll_len);
                }

                std::string curr_make = gpulist->infos.back().make;
                if (curr_make == "NVIDIA") {
                    gpulist->currGpu().addDll(L"nvoglv32.dll");
                    gpulist->currGpu().addDll(L"nvoglv64.dll");
                } else if (curr_make == "Advanced Micro Devices, Inc.") {
                    gpulist->currGpu().addDll(L"atioglxx.dll");
                    gpulist->currGpu().addDll(L"atig6txx.dll");
                }
            }
        }

        num_lines++;
        p = line_end_pos + 2;
        line_end_pos = wcsstr(p, L"\r\n");
    }
#endif
}




