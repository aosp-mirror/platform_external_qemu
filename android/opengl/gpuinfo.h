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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

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
    GpuInfo() {}

    bool current_gpu;

    void addDll(std::string dll_str);

    std::string make;
    std::string model;
    std::string device_id;
    std::string revision_id;
    std::string version;
    std::string renderer;

    std::vector<std::string> dlls;
};

class GpuInfoList {
public:
    GpuInfoList() {}
    void addGpu();
    GpuInfo& currGpu();
    void dump();

    std::vector<GpuInfo> infos;
};

void gpuinfo_dump(GpuInfoList* gpulist);
bool gpuinfo_query_blacklist(GpuInfoList* gpulist);

std::string load_gpu_info();
void parse_gpu_info_list(const std::string& contents, GpuInfoList* gpulist);
