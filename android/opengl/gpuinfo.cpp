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

#include "android/opengl/gpuinfo.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void gpuinfo_add_dll(GpuInfoList* gpulist, wchar_t* dll_str) {
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
    {"10de", NULL, "0x0324", NULL, NULL, NULL}, // NVIDIA GeForce FX Go5200 (Linux)

    {"10de", NULL, "0x029e", NULL, NULL, NULL}, // NVIDIA Quadro FX 1500 (Linux)

    {"NVIDIA (0x10de)", NULL, "0x0393", NULL, NULL, NULL}, // NVIDIA GeForce 7300 GT (Mac)
};

#define BLACKLIST_SIZE 12

// If any blacklist entry matches any gpu, return true.
bool gpuinfo_query_blacklist(GpuInfoList* gpulist) {
    GpuInfo** gpuinfos = gpulist->infos;
    fprintf(stderr, "-=GPU INFO=-\n");
    for (int i = 0; i < gpulist->count; i++) {
        fprintf(stderr, "GPU #%d {\n", i);
        fprintf(stderr, "\tMake:[%s]\n", gpuinfos[i]->make);
        fprintf(stderr, "\tModel:[%s]\n", gpuinfos[i]->model);
        fprintf(stderr, "\tDevice ID:[%s]\n", gpuinfos[i]->device_id);
        fprintf(stderr, "\tRevision ID:[%s]\n", gpuinfos[i]->revision_id);
        fprintf(stderr, "\tVersion:[%s]\n", gpuinfos[i]->version);
        fprintf(stderr, "\tActive?:[%d]\n", gpuinfos[i]->current_gpu);
        fprintf(stderr, "\tDLLS:\n");
        for (int j = 0; j < gpuinfos[i]->num_dlls; j++) {
            fprintf(stderr, "\t\t[%s]\n", gpuinfos[i]->dlls[j]);
        }
        fprintf(stderr, "}\n");

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
        fprintf(stderr, "BL #%d {\n", j);
        fprintf(stderr, "\tMake:[%s]\n", bl_make);
        fprintf(stderr, "\tModel:[%s]\n", bl_model);
        fprintf(stderr, "\tDevice ID:[%s]\n", bl_device_id);
        fprintf(stderr, "\tRevision ID:[%s]\n", bl_revision_id);
        fprintf(stderr, "\tVersion:[%s]\n", bl_version);

            if (bl_make && strcmp(bl_make, gpuinfos[i]->make)) continue;
            if (bl_model && strcmp(bl_model, gpuinfos[i]->model)) continue;
            if (bl_device_id && strcmp(bl_device_id, gpuinfos[i]->device_id)) continue;
            if (bl_revision_id && strcmp(bl_revision_id, gpuinfos[i]->revision_id)) continue;
            if (bl_version && strcmp(bl_version, gpuinfos[i]->version)) continue;

            fprintf(stderr, "blacklisted.\n");
            return true;
        }
    }
    fprintf(stderr, "Not blacklisted.\n");
    return false;
}
