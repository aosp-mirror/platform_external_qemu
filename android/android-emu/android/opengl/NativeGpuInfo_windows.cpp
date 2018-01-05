// Copyright 2017 The Android Open Source Project
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

#include "android/opengl/NativeGpuInfo.h"

#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/Uuid.h"
#include "android/base/containers/SmallVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"

#include "android/utils/path.h"

#include <windows.h>
#include <d3d9.h>

#include <ctype.h>

#include <algorithm>
#include <string>

using android::base::makeCustomScopedPtr;
using android::base::PathUtils;
using android::base::RunOptions;
using android::base::SmallFixedVector;
using android::base::startsWith;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::Uuid;
using android::base::Win32UnicodeString;

static std::string& toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static void parse_windows_gpu_ids(const std::string& val,
                                  GpuInfoList* gpulist) {
    std::string result;
    size_t key_start = 0;
    size_t key_end = 0;

    key_start = val.find("VEN_", key_start);
    if (key_start == std::string::npos) {
        return;
    }
    key_end = val.find("&", key_start);
    if (key_end == std::string::npos) {
        return;
    }
    result = val.substr(key_start + 4, key_end - key_start - 4);
    gpulist->currGpu().make = std::move(toLower(result));

    key_start = val.find("DEV_", key_start);
    if (key_start == std::string::npos) {
        return;
    }
    key_end = val.find("&", key_start);
    if (key_end == std::string::npos) {
        return;
    }
    result = val.substr(key_start + 4, key_end - key_start - 4);
    gpulist->currGpu().device_id = std::move(toLower(result));
}

static void add_predefined_gpu_dlls(GpuInfo* gpu) {
    const std::string& currMake = gpu->make;
    if (currMake == "NVIDIA" || startsWith(gpu->model, "NVIDIA")) {
        gpu->addDll("nvoglv32.dll");
        gpu->addDll("nvoglv64.dll");
    } else if (currMake == "Advanced Micro Devices, Inc." ||
               startsWith(gpu->model, "Advanced Micro Devices, Inc.")) {
        gpu->addDll("atioglxx.dll");
        gpu->addDll("atig6txx.dll");
    }
}

static void parse_windows_gpu_dlls(int line_loc,
                                   int val_pos,
                                   const std::string& contents,
                                   GpuInfoList* gpulist) {
    if (line_loc - val_pos != 0) {
        const std::string& dll_str =
                contents.substr(val_pos, line_loc - val_pos);

        size_t vp = 0;
        size_t dll_sep_loc = dll_str.find(",", vp);
        size_t dll_end = (dll_sep_loc != std::string::npos)
                                 ? dll_sep_loc
                                 : dll_str.size() - vp;
        gpulist->currGpu().addDll(dll_str.substr(vp, dll_end - vp));

        while (dll_sep_loc != std::string::npos) {
            vp = dll_sep_loc + 1;
            dll_sep_loc = dll_str.find(",", vp);
            dll_end = (dll_sep_loc != std::string::npos) ? dll_sep_loc
                                                         : dll_str.size() - vp;
            gpulist->currGpu().addDll(dll_str.substr(vp, dll_end - vp));
        }
    }

    add_predefined_gpu_dlls(&gpulist->currGpu());
}

static void load_gpu_registry_info(const wchar_t* keyName, GpuInfo* gpu) {
    HKEY hkey;
    if (::RegOpenKeyW(HKEY_LOCAL_MACHINE, keyName, &hkey) != ERROR_SUCCESS) {
        return;
    }

    auto keyCloser = makeCustomScopedPtr(hkey, ::RegCloseKey);

    SmallFixedVector<wchar_t, 256> name;
    SmallFixedVector<BYTE, 1024> value;
    for (int i = 0;; ++i) {
        name.resize_noinit(name.capacity());
        value.resize_noinit(value.capacity());
        DWORD nameLen = name.size();
        DWORD valueLen = value.size();
        DWORD type;
        auto res = RegEnumValueW(hkey, i, name.data(), &nameLen, nullptr, &type,
                                 value.data(), &valueLen);
        if (res == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (res == ERROR_MORE_DATA) {
            if (type != REG_SZ && type != REG_MULTI_SZ) {
                // we don't care about other types for now, so let's not even
                // try
                continue;
            }
            name.resize_noinit(nameLen + 1);
            value.resize_noinit(valueLen + 1);
            nameLen = name.size();
            valueLen = value.size();
            res = ::RegEnumValueW(hkey, i, name.data(), &nameLen, nullptr,
                                  &type, value.data(), &valueLen);
            if (res != ERROR_SUCCESS) {
                break;
            }
        }
        if (res != ERROR_SUCCESS) {
            break;  // well, what can we do here?
        }

        name[nameLen] = L'\0';

        if (type == REG_SZ && wcscmp(name.data(), L"DriverVersion") == 0) {
            const auto strVal = (wchar_t*)value.data();
            const auto strLen = valueLen / sizeof(wchar_t);
            strVal[strLen] = L'\0';
            gpu->version = Win32UnicodeString::convertToUtf8(strVal, strLen);
        } else if (type == REG_MULTI_SZ &&
                   (wcscmp(name.data(), L"UserModeDriverName") == 0 ||
                    wcscmp(name.data(), L"UserModeDriverNameWoW") == 0)) {
            const auto strVal = (wchar_t*)value.data();
            const auto strLen = valueLen / sizeof(wchar_t);
            strVal[strLen] = L'\0';
            // Iterate over the '0'-delimited list of strings,
            // stopping at double '0' (AKA empty string after the
            // delimiter).
            for (const wchar_t* ptr = strVal;;) {
                auto len = wcslen(ptr);
                if (!len) {
                    break;
                }
                gpu->dlls.emplace_back(
                        Win32UnicodeString::convertToUtf8(ptr, len));
                ptr += len + 1;
            }
        }
    }
}

static const int kGPUInfoQueryTimeoutMs = 5000;

static std::string load_gpu_info_wmic() {
    auto guid = Uuid::generateFast().toString();
    // WMIC doesn't allow one to have any unquoted '-' characters in file name,
    // so let's get rid of them.
    guid.erase(std::remove(guid.begin(), guid.end(), '-'), guid.end());
    auto tempName = PathUtils::join(System::get()->getTempDir(),
                                    StringFormat("gpuinfo_%s.txt", guid));

    auto deleteTempFile = makeCustomScopedPtr(
            &tempName,
            [](const std::string* name) { path_delete_file(name->c_str()); });
    if (!System::get()->runCommand(
                {"wmic", StringFormat("/OUTPUT:%s", tempName), "path",
                 "Win32_VideoController", "get", "/value"},
                RunOptions::WaitForCompletion | RunOptions::TerminateOnTimeout,
                kGPUInfoQueryTimeoutMs)) {
        return {};
    }
    auto res = android::readFileIntoString(tempName);
    return res ? Win32UnicodeString::convertToUtf8(
                         (const wchar_t*)res->c_str(),
                         res->size() / sizeof(wchar_t))
               : std::string{};
}

void parse_gpu_info_list_windows(const std::string& contents,
                                 GpuInfoList* gpulist) {
    size_t line_loc = contents.find("\r\n");
    if (line_loc == std::string::npos) {
        line_loc = contents.size();
    }
    size_t p = 0;
    size_t equals_pos = 0;
    size_t val_pos = 0;
    std::string key;
    std::string val;

    // Windows: We use `wmic path Win32_VideoController get /value`
    // to get a reasonably detailed list of '<key>=<val>'
    // pairs. From these, we can get the make/model
    // of the GPU, the driver version, and all DLLs involved.
    while (line_loc != std::string::npos) {
        equals_pos = contents.find("=", p);
        if ((equals_pos != std::string::npos) && (equals_pos < line_loc)) {
            key = contents.substr(p, equals_pos - p);
            val_pos = equals_pos + 1;
            val = contents.substr(val_pos, line_loc - val_pos);

            if (key.find("AdapterCompatibility") != std::string::npos) {
                gpulist->addGpu();
                gpulist->currGpu().os = "W";
                // 'make' will be overwritten in parsing 'PNPDeviceID'
                // later. Set it here because we need it in paring
                // 'InstalledDisplayDrivers' which comes before
                // 'PNPDeviceID'.
                gpulist->currGpu().make = val;
            } else if (key.find("Caption") != std::string::npos) {
                gpulist->currGpu().model = val;
            } else if (key.find("PNPDeviceID") != std::string::npos) {
                parse_windows_gpu_ids(val, gpulist);
            } else if (key.find("DriverVersion") != std::string::npos) {
                gpulist->currGpu().version = val;
            } else if (key.find("InstalledDisplayDrivers") !=
                       std::string::npos) {
                parse_windows_gpu_dlls(line_loc, val_pos, contents, gpulist);
            }
        }
        if (line_loc == contents.size()) {
            break;
        }
        p = line_loc + 2;
        line_loc = contents.find("\r\n", p);
        if (line_loc == std::string::npos) {
            line_loc = contents.size();
        }
    }
}

static bool queryGpuInfoD3D(GpuInfoList* gpus) {
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    UINT numAdapters = pD3D->GetAdapterCount();

    char vendoridBuf[16] = {};
    char deviceidBuf[16] = {};

    // MAX_DEVICE_IDENTIFIER_STRING can be pretty big,
    // don't allocate on stack.
    std::vector<char> descriptionBuf(MAX_DEVICE_IDENTIFIER_STRING + 1, '\0');

    if (numAdapters == 0) return false;

    // The adapter that is equal to D3DADAPTER_DEFAULT is the primary display adapter.
    // D3DADAPTER_DEFAULT is currently defined to be 0, btw---but this is more future proof
    for (UINT i = 0; i < numAdapters; i++) {
        if (i == D3DADAPTER_DEFAULT) {
            gpus->addGpu();
            GpuInfo& gpu = gpus->currGpu();
            gpu.os = "W";

            D3DADAPTER_IDENTIFIER9 id;
            pD3D->GetAdapterIdentifier(0, 0, &id);
            snprintf(vendoridBuf, sizeof(vendoridBuf), "%04x", (unsigned int)id.VendorId);
            snprintf(deviceidBuf, sizeof(deviceidBuf), "%04x", (unsigned int)id.DeviceId);
            snprintf(&descriptionBuf[0], MAX_DEVICE_IDENTIFIER_STRING, "%s", id.Description);
            gpu.make = vendoridBuf;
            gpu.device_id = deviceidBuf;
            gpu.model = &descriptionBuf[0];
            return true;
        }
    }

    return false;
}

void getGpuInfoListNative(GpuInfoList* gpus) {
    if (queryGpuInfoD3D(gpus)) return;

    DISPLAY_DEVICEW device = { sizeof(device) };

    for (int i = 0; EnumDisplayDevicesW(nullptr, i, &device, 0); ++i) {
        gpus->addGpu();
        GpuInfo& gpu = gpus->currGpu();
        gpu.os = "W";
        gpu.current_gpu =
                (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
        gpu.model = Win32UnicodeString::convertToUtf8(device.DeviceString);
        parse_windows_gpu_ids(
                Win32UnicodeString::convertToUtf8(device.DeviceID), gpus);

        // Now try inspecting the registry directly; |device|.DeviceKey can be a
        // path to the GPU information key.
        static constexpr StringView prefix = "\\Registry\\Machine\\";
        if (startsWith(Win32UnicodeString::convertToUtf8(device.DeviceKey),
                       prefix)) {
            load_gpu_registry_info(device.DeviceKey + prefix.size(), &gpu);
        }
        add_predefined_gpu_dlls(&gpu);
    }

    if (gpus->infos.empty()) {
        // Everything failed - fall back to the good^Wbad old WMIC command.
        auto gpuInfoWmic = load_gpu_info_wmic();
        parse_gpu_info_list_windows(gpuInfoWmic, gpus);
    }
}
