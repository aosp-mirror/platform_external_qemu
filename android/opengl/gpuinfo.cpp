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

void gpuinfo_init(GpuInfoList* gpulist) {
    gpulist->infos = new GpuInfo*[MAX_GPUS];
    memset(gpulist->infos, 0, MAX_GPUS);
    gpulist->count = 0;
}

void gpuinfo_add_gpu(GpuInfoList* gpulist) {
    gpulist->count++;
    assert(gpulist->count <= MAX_GPUS);

    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;

    gpuinfos[curr_gpu] = (GpuInfo*)malloc(sizeof(GpuInfo));
    gpuinfos[curr_gpu]->make = (char*)malloc(FIELD_LEN);
    gpuinfos[curr_gpu]->model = (char*)malloc(FIELD_LEN);
    gpuinfos[curr_gpu]->device_id = (char*)malloc(FIELD_LEN);
    gpuinfos[curr_gpu]->revision_id = (char*)malloc(FIELD_LEN);
    gpuinfos[curr_gpu]->version = (char*)malloc(FIELD_LEN);
    gpuinfos[curr_gpu]->renderer = (char*)malloc(FIELD_LEN);
    gpuinfos[curr_gpu]->current_gpu = false;
    gpuinfos[curr_gpu]->dlls = NULL;
    gpuinfos[curr_gpu]->num_dlls = 0;

    memset(gpuinfos[curr_gpu]->make, 0, FIELD_LEN);
    memset(gpuinfos[curr_gpu]->model, 0, FIELD_LEN);
    memset(gpuinfos[curr_gpu]->device_id, 0, FIELD_LEN);
    memset(gpuinfos[curr_gpu]->revision_id, 0, FIELD_LEN);
    memset(gpuinfos[curr_gpu]->version, 0, FIELD_LEN);
    memset(gpuinfos[curr_gpu]->renderer, 0, FIELD_LEN);
}

#ifdef _WIN32
void gpuinfo_extract_val(wchar_t* val_str, int howmuch, wchar_t* out) {
    wcsncpy(out, val_str, howmuch + 1);
    out[howmuch] = L'\0';
}

void gpuinfo_extract_val_bytes(wchar_t* val_str, int len, char* out) {
    wcstombs(out, val_str, len + 1);
    out[len] = '\0';
}

void gpuinfo_set_make(GpuInfoList* gpulist, wchar_t* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val_bytes(val_str, len, gpuinfos[curr_gpu]->make);
}

void gpuinfo_set_model(GpuInfoList* gpulist, wchar_t* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val_bytes(val_str, len, gpuinfos[curr_gpu]->model);
}

void gpuinfo_set_version(GpuInfoList* gpulist, wchar_t* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val_bytes(val_str, len, gpuinfos[curr_gpu]->version);
}

void gpuinfo_init_dlls(GpuInfoList* gpulist) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfos[curr_gpu]->dlls = (char**)malloc(sizeof(char*) * FIELD_LEN);
    memset(gpuinfos[curr_gpu]->dlls, 0, FIELD_LEN);
}

void gpuinfo_add_dll(GpuInfoList* gpulist, wchar_t* dll_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;

    gpuinfos[curr_gpu]->num_dlls++;
    int dll_index = gpuinfos[curr_gpu]->num_dlls - 1;
    gpuinfos[curr_gpu]->dlls[dll_index] = (char*)malloc(len + 1);
    gpuinfo_extract_val_bytes(dll_str, len, gpuinfos[curr_gpu]->dlls[dll_index]);
}

void gpuinfo_add_constdll(GpuInfoList* gpulist, wchar_t* dll_str) {
    gpuinfo_add_dll(gpulist, dll_str, wcslen(dll_str));
}

#else

void gpuinfo_extract_val(char* val_str, int howmuch, char* out) {
    strncpy(out, val_str, howmuch + 1);
    out[howmuch] = '\0';
}

void gpuinfo_set_make(GpuInfoList* gpulist, char* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val(val_str, len, gpuinfos[curr_gpu]->make);
}

void gpuinfo_set_model(GpuInfoList* gpulist, char* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val(val_str, len, gpuinfos[curr_gpu]->model);
}

void gpuinfo_set_version(GpuInfoList* gpulist, char* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val(val_str, len, gpuinfos[curr_gpu]->version);
}

void gpuinfo_set_renderer(GpuInfoList* gpulist, char* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val(val_str, len, gpuinfos[curr_gpu]->renderer);
}

void gpuinfo_set_device_id(GpuInfoList* gpulist, char* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val(val_str, len, gpuinfos[curr_gpu]->device_id);
}

void gpuinfo_set_revision_id(GpuInfoList* gpulist, char* val_str, int len) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfo_extract_val(val_str, len, gpuinfos[curr_gpu]->revision_id);
}

void gpuinfo_set_current(GpuInfoList* gpulist) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    gpuinfos[curr_gpu]->current_gpu = true;
}


#endif

char* gpuinfo_get_make(GpuInfoList* gpulist) {
    GpuInfo** gpuinfos = gpulist->infos;
    int curr_gpu = gpulist->count - 1;
    return gpuinfos[curr_gpu]->make;
}

void gpuinfo_dump(GpuInfoList* gpulist) {
    GpuInfo** gpuinfos = gpulist->infos;
    fprintf(stderr, "-=GPU INFO=-\n");
    for (int i = 0; i < gpulist->count; i++) {
        fprintf(stderr, "GPU #%d {\n", i);
        fprintf(stderr, "\tMake:[%s]\n", gpuinfos[i]->make);
        fprintf(stderr, "\tModel:[%s]\n", gpuinfos[i]->model);
        fprintf(stderr, "\tDevice ID:[%s]\n", gpuinfos[i]->device_id);
        fprintf(stderr, "\tRevision ID:[%s]\n", gpuinfos[i]->revision_id);
        fprintf(stderr, "\tVersion:[%s]\n", gpuinfos[i]->version);
        fprintf(stderr, "\tRenderer:[%s]\n", gpuinfos[i]->renderer);
        fprintf(stderr, "\tActive?:[%d]\n", gpuinfos[i]->current_gpu);
        fprintf(stderr, "\tDLLS:\n");
        for (int j = 0; j < gpuinfos[i]->num_dlls; j++) {
            fprintf(stderr, "\t\t[%s]\n", gpuinfos[i]->dlls[j]);
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

    // Test entries
    {"10de", NULL, "13ba", NULL, NULL, NULL} // NVIDIA Quadro K2200
};

#define BLACKLIST_SIZE 12

// If any blacklist entry matches any gpu, return true.
bool gpuinfo_query_blacklist(GpuInfoList* gpulist) {
    GpuInfo** gpuinfos = gpulist->infos;
    for (int i = 0; i < gpulist->count; i++) {
        const char* bl_make = NULL;
        const char* bl_model = NULL;
        const char* bl_device_id = NULL;
        const char* bl_revision_id = NULL;
        const char* bl_version = NULL;
        for (int j = 0; j < BLACKLIST_SIZE; j++) {
            bl_make = sGpuBlacklist[j].make;
            bl_model = sGpuBlacklist[j].model;
            bl_device_id = sGpuBlacklist[j].device_id;
            bl_revision_id = sGpuBlacklist[j].revision_id;
            bl_version = sGpuBlacklist[j].version;

            if (bl_make && strcmp(bl_make, gpuinfos[i]->make)) continue;
            if (bl_model && strcmp(bl_model, gpuinfos[i]->model)) continue;
            if (bl_device_id && strcmp(bl_device_id, gpuinfos[i]->device_id)) continue;
            if (bl_revision_id && strcmp(bl_revision_id, gpuinfos[i]->revision_id)) continue;
            if (bl_version && strcmp(bl_version, gpuinfos[i]->version)) continue;

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
        Sleep(500);
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
                gpuinfo_add_gpu(gpulist);
                gpuinfo_set_model(gpulist, val, val_strlen);
            } else if (strstr(key, "Vendor")) {
                gpuinfo_set_make(gpulist, val, val_strlen);
            } else if (strstr(key, "Device ID")) {
                gpuinfo_set_device_id(gpulist, val, val_strlen);
            } else if (strstr(key, "Revision ID")) {
                gpuinfo_set_revision_id(gpulist, val, val_strlen);
            } else if (strstr(key, "Display Type")) {
                gpuinfo_set_current(gpulist);
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
        // kvsep_loc = strstr(p, ": ");
        // if (kvsep_loc && kvsep_loc < line_loc) {
            int key_strlen = line_loc - p;
            strncpy(key, p, key_strlen + 1);
            key[key_strlen] = '\0';

            // int val_strlen = line_loc - (kvsep_loc + 2);
            // strncpy(val, kvsep_loc + 2, val_strlen + 1);
            // val[val_strlen] = '\0';

            if (!lookfor && strstr(key, "VGA")) {
                lookfor = true;
                gpuinfo_add_gpu(gpulist);
            } else if (lookfor && strstr(key, "Vendor")) {
                parse_last_hexbrackets(key, val, &val_strlen);
                gpuinfo_set_make(gpulist, val, val_strlen);
            } else if (lookfor && strstr(key, "Device")) {
                parse_last_hexbrackets(key, val, &val_strlen);
                gpuinfo_set_device_id(gpulist, val, val_strlen);
                lookfor = false;
            } else if (strstr(key, "OpenGL version string")) {
                parse_renderer_part(key, val, &val_strlen);
                gpuinfo_set_renderer(gpulist, val, val_strlen);
            } else {

            }
        // }
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
                gpuinfo_add_gpu(gpulist);
                gpuinfo_set_make(gpulist, val, val_strlen);
            } else if (wcsstr(key, L"Caption")) {
                gpuinfo_set_model(gpulist, val, val_strlen);
            } else if (wcsstr(key, L"DriverVersion")) {
                gpuinfo_set_version(gpulist, val, val_strlen);
            } else if (wcsstr(key, L"InstalledDisplayDrivers")) {
                if (val_strlen == 0) {
                    continue;
                }
                gpuinfo_init_dlls(gpulist);

                wchar_t* v_p = val;
                wchar_t* val_end = v_p + val_strlen;
                wchar_t* dll_sep_loc = wcsstr(v_p, L",");
                wchar_t* dll_pos = v_p;
                wchar_t* dll_end = dll_sep_loc ? dll_sep_loc : val_end;
                wchar_t curr_dll[FIELD_LEN] = {};
                int curr_dll_len = dll_end - dll_pos;

                gpuinfo_add_dll(gpulist, dll_pos, curr_dll_len);

                while (dll_sep_loc) {

                    v_p = dll_sep_loc + 1;
                    dll_sep_loc = wcsstr(v_p, L",");
                    dll_pos = v_p;

                    dll_end = dll_sep_loc ? dll_sep_loc : val_end;
                    curr_dll_len = dll_end - dll_pos;

                    gpuinfo_add_dll(gpulist, dll_pos, curr_dll_len);
                }

                char* curr_make = gpuinfo_get_make(gpulist);
                if (strstr(curr_make, "NVIDIA")) {
                    gpuinfo_add_constdll(gpulist, L"nvoglv32.dll");
                    gpuinfo_add_constdll(gpulist, L"nvoglv64.dll");
                } else if (strstr(curr_make, "Advanced Micro Devices, Inc.")) {
                    gpuinfo_add_constdll(gpulist, L"atioglxx.dll");
                    gpuinfo_add_constdll(gpulist, L"atig6txx.dll");
                }
            }
        }

        num_lines++;
        p = line_end_pos + 2;
        line_end_pos = wcsstr(p, L"\r\n");
    }
#endif
}




