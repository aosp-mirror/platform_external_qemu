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

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ANDROID_BEGIN_HEADER

#define FIELD_LEN 2048
#define MAX_GPUS 12

struct BlacklistEntry {
    const char* make;
    const char* model;
    const char* device_id;
    const char* revision_id;
    const char* version;
};

struct GpuInfo {
    char* make;
    char* model;
    char* device_id;
    char* revision_id;
    char* version;
    bool current_gpu;
    char** dlls;
    int num_dlls;
};

struct GpuInfoList {
    GpuInfo** infos;
    int count;
};

void gpuinfo_init(GpuInfoList* gpulist);
void gpuinfo_add_gpu(GpuInfoList* gpulist);

#ifdef _WIN32
void gpuinfo_extract_val(wchar_t* val_str, int howmuch, wchar_t* out);
void gpuinfo_extract_val_bytes(wchar_t* val_str, int len, char* out);
void gpuinfo_set_make(GpuInfoList* gpulist, wchar_t* val_str, int len);
void gpuinfo_set_model(GpuInfoList* gpulist, wchar_t* val_str, int len);
void gpuinfo_set_version(GpuInfoList* gpulist, wchar_t* val_str, int len);
void gpuinfo_init_dlls(GpuInfoList* gpulist);
void gpuinfo_add_dll(GpuInfoList* gpulist, wchar_t* dll_str, int len);
void gpuinfo_add_dll(GpuInfoList* gpulist, wchar_t* dll_str);
#else
void gpuinfo_extract_val(char* val_str, int howmuch, char* out);
void gpuinfo_set_make(GpuInfoList* gpulist, char* val_str, int len);
void gpuinfo_set_model(GpuInfoList* gpulist, char* val_str, int len);
void gpuinfo_set_version(GpuInfoList* gpulist, char* val_str, int len);
void gpuinfo_set_device_id(GpuInfoList* gpulist, char* val_str, int len);
void gpuinfo_set_revision_id(GpuInfoList* gpulist, char* val_str, int len);
void gpuinfo_set_current(GpuInfoList* gpulist);
#endif

char* gpuinfo_get_make(GpuInfoList* gpulist);
void gpuinfo_dump(GpuInfoList* gpulist);
bool gpuinfo_query_blacklist(GpuInfoList* gpulist);

ANDROID_END_HEADER
