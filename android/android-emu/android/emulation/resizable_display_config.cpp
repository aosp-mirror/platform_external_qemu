/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/emulation/resizable_display_config.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/misc/StringUtils.h"
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbInterface
#include "android/featurecontrol/feature_control.h"
#include "android/globals.h"
#include "android/opengles.h"

#include <map>

namespace android {
namespace emulation {

class ResizableConfig {
public:
    ResizableConfig() {
        std::string configStr(android_hw->hw_resizable_configs);
        std::vector<std::string> entrys;
        android::base::splitTokens(configStr, &entrys, ",");
        if (entrys.size() != PRESET_SIZE_MAX) {
            LOG(ERROR) << "Failed to parse resizable config " << configStr;
            return;
        }
        for (auto entry : entrys) {
            std::vector<std::string> tokens;
            android::base::splitTokens(entry, &tokens, "-");
            if (tokens.size() != 5) {
                LOG(ERROR) << "Failed to parse resizable config entry "
                           << entry;
                mConfigs.clear();
                return;
            }
            int id = std::stoi(tokens[1]);
            if (id < 0 || id >= PRESET_SIZE_MAX) {
                LOG(ERROR) << "Failed to parse resizable config entry, "
                              "incorrect index "
                           << tokens[1];
                mConfigs.clear();
                return;
            }
            int width = std::stoi(tokens[2]);
            int height = std::stoi(tokens[3]);
            int dpi = std::stoi(tokens[4]);
            mConfigs[static_cast<PresetEmulatorSizeType>(id)] =
                    PresetEmulatorSizeInfo{
                            static_cast<PresetEmulatorSizeType>(id), width,
                            height, dpi};
            if (width == android_hw->hw_lcd_width &&
                height == android_hw->hw_lcd_height &&
                dpi == android_hw->hw_lcd_density) {
                mActiveConfigId = static_cast<PresetEmulatorSizeType>(id);
            }
        }
    }

    void init() {
        for (auto config : mConfigs) {
            android_setOpenglesDisplayConfigs(
                    (int)config.first, config.second.width,
                    config.second.height, config.second.dpi, config.second.dpi);
        }
        android_setOpenglesDisplayActiveConfig(mActiveConfigId);
        updateAndroidDisplayConfigPath(mActiveConfigId);
    }

    void updateAndroidDisplayConfigPath(enum PresetEmulatorSizeType configId) {
        auto adbInterface = emulation::AdbInterface::getGlobal();
        if (!adbInterface) {
            LOG(ERROR) << "Adb interface unavailable";
            return;
        }

        if (shouldApplyLargeDisplaySetting(configId)) {
            adbInterface->enqueueCommand(
                    {"shell",
                     "cmd window set-ignore-orientation-request true"});
        } else {
            adbInterface->enqueueCommand(
                    {"shell",
                     "cmd window set-ignore-orientation-request false"});
        }
    }

    bool getResizableConfig(PresetEmulatorSizeType id,
                            struct PresetEmulatorSizeInfo* info) {
        if (mConfigs.find(id) == mConfigs.end()) {
            LOG(ERROR) << "Failed to find resizable config for " << id;
            return false;
        }
        *info = mConfigs[id];  // structure copy
        return true;
    }

    PresetEmulatorSizeType getResizableActiveConfigId() {
        return mActiveConfigId;
    }

    void setResizableActiveConfigId(PresetEmulatorSizeType configId) {
        if (mActiveConfigId == configId) {
            return;
        }
        mActiveConfigId = configId;

        android_setOpenglesDisplayActiveConfig(configId);

        auto adbInterface = emulation::AdbInterface::getGlobal();
        if (!adbInterface) {
            LOG(ERROR) << "Adb interface unavailable";
            return;
        }

        // SurfaceFlinger index the configId in reverse order.
        int sfConfigId = PRESET_SIZE_MAX - 1 - configId;
        // Tell SurfaceFlinger to change display mode to sConfigId.
        std::string cmd = "su 0 service call SurfaceFlinger 1035 i32 " +
                          std::to_string(sfConfigId);
        adbInterface->enqueueCommand({"shell", cmd});
        // window manager dpi
        cmd = "wm density " + std::to_string(mConfigs[configId].dpi);
        adbInterface->enqueueCommand({"shell", cmd});

        // tablet setting
        updateAndroidDisplayConfigPath(configId);
    }

    bool shouldApplyLargeDisplaySetting(enum PresetEmulatorSizeType id) {
        if (id == PRESET_SIZE_UNFOLDED || id == PRESET_SIZE_TABLET ||
            id == PRESET_SIZE_DESKTOP) {
            return true;
        }
        return false;
    }

private:
    std::map<PresetEmulatorSizeType, PresetEmulatorSizeInfo> mConfigs;
    PresetEmulatorSizeType mActiveConfigId = PRESET_SIZE_MAX;
};

static android::base::LazyInstance<ResizableConfig> sResizableConfig =
        LAZY_INSTANCE_INIT;

}  // namespace emulation
}  // namespace android

void resizableInit() {
    android::emulation::sResizableConfig->init();
}

bool getResizableConfig(enum PresetEmulatorSizeType id,
                        struct PresetEmulatorSizeInfo* info) {
    return android::emulation::sResizableConfig->getResizableConfig(id, info);
}

enum PresetEmulatorSizeType getResizableActiveConfigId() {
    return android::emulation::sResizableConfig->getResizableActiveConfigId();
}

void setResizableActiveConfigId(enum PresetEmulatorSizeType id) {
    android::emulation::sResizableConfig->setResizableActiveConfigId(id);
}

void updateAndroidDisplayConfigPath(enum PresetEmulatorSizeType id) {
    android::emulation::sResizableConfig->updateAndroidDisplayConfigPath(id);
}

bool resizableEnabled() {
    return android_hw->hw_device_name &&
           !strcmp(android_hw->hw_device_name, "resizable") &&
           feature_is_enabled(kFeature_HWCMultiConfigs);
}
