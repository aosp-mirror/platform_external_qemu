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
#include <string>
#include <string.h>
#include <vector>

ANDROID_BEGIN_HEADER

#define FIELD_LEN 2048
#define MAX_GPUS 12

struct BlacklistEntry {
    const char* make;
    const char* model;
    const char* device_id;
    const char* revision_id;
    const char* version;
    const char* renderer;
};

class GpuInfo {
public:
    GpuInfo() { }

    bool current_gpu;

    void setMake(char* val_str, int len);
    void setModel(char* val_str, int len);
    void setVersion(char* val_str, int len);
    void setDeviceId(char* val_str, int len);
    void setRevisionId(char* val_str, int len);
    void setRenderer(char* val_str, int len);
    void setCurrent();
    void addDll(char* dll_str, int len);
    void addDll(char* dll_str);

    std::string make;
    std::string model;
    std::string device_id;
    std::string revision_id;
    std::string version;
    std::string renderer;

    std::vector<std::string> dlls;
private:
    void convert(std::string& str, char* input, int len);
};

class GpuInfoList {
public:
    GpuInfoList() { }
    
    void addGpu();
    GpuInfo& currGpu();
    void dump();

    std::vector<GpuInfo> infos;
};


void gpuinfo_dump(GpuInfoList* gpulist);
bool gpuinfo_query_blacklist(GpuInfoList* gpulist);

char* load_gpu_info();
void parse_gpu_info_list(char* contents, GpuInfoList* gpulist);

ANDROID_END_HEADER
