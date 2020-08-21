/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/physics/FoldableModel.h"

#include "android/base/misc/StringUtils.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/globals.h"

#define D(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define E(...) derror(__VA_ARGS__)

namespace android {
namespace physics {

enum FoldablePostures FoldableModel::calculatePosture() {
    std::vector<uint32_t> entries;
    for (uint32_t i = 0; i < mAnglesToPostures.size(); i++) {
        if (mAnglesToPostures[i].angles[0].left >
            mState.currentHingeDegrees[0]) {
            break;
        }
        if (mAnglesToPostures[i].angles[0].left <=
                    mState.currentHingeDegrees[0] &&
            mAnglesToPostures[i].angles[0].right >=
                    mState.currentHingeDegrees[0]) {
            entries.push_back(i);
        }
    }
    for (const auto i : entries) {
        bool found = true;
        for (uint32_t j = 1; j < mState.config.numHinges; j++) {
            if (mAnglesToPostures[i].angles[j].left >
                        mState.currentHingeDegrees[j] ||
                mAnglesToPostures[i].angles[j].right <
                        mState.currentHingeDegrees[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return mAnglesToPostures[i].posture;
        }
    }
    return POSTURE_UNKNOWN;
}

static bool compareAnglesToPosture(AnglesToPosture v1, AnglesToPosture v2) {
    return (v1.angles[0].left < v2.angles[0].left);
}

FoldableModel::FoldableModel() {
    if (android_hw->hw_sensor_hinge == false) {
        mState.config.numHinges = 0;
        mState.currentPosture = POSTURE_UNKNOWN;
        return;
    }
    if (!android_foldable_hinge_configured()) {
        // create a default hinge with 180 degree
        mState.config.numHinges = 1;
        mState.config.hingeParams[0].defaultDegrees = 180.0f;
        mState.currentHingeDegrees[0] = 180.0f;
        return;
    }

    // hinge type
    enum FoldableDisplayType type =
            (enum FoldableDisplayType)android_hw->hw_sensor_hinge_type;
    if (type < 0 || type >= ANDROID_FOLDABLE_TYPE_MAX) {
        type = ANDROID_FOLDABLE_HORIZONTAL_SPLIT;
        W("Incorrect hinge type %d, default to 0\n",
          android_hw->hw_sensor_hinge_type);
    }
    enum FoldableHingeSubType subType =
            (enum FoldableHingeSubType)android_hw->hw_sensor_hinge_sub_type;
    if (subType < 0 || subType >= ANDROID_FOLDABLE_HINGE_SUB_TYPE_MAX) {
        subType = ANDROID_FOLDABLE_HINGE_FOLD;
        W("Incorrect hinge sub_type %d, default to 0\n",
          android_hw->hw_sensor_hinge_sub_type);
    }

    enum FoldablePostures foldAtPosture =
            (enum FoldablePostures)android_hw->hw_sensor_hinge_fold_to_displayRegion_0_1_at_posture;
    if (foldAtPosture < 0 || foldAtPosture >= POSTURE_MAX) {
        foldAtPosture = POSTURE_CLOSED;
        W("Incorrect fold at posture %d, default to 1\n",
          android_hw->hw_sensor_hinge_fold_to_displayRegion_0_1_at_posture);
    }

     // hinge number
    int numHinge = android_hw->hw_sensor_hinge_count;
    if (numHinge < 0 || numHinge > ANDROID_FOLDABLE_MAX_HINGES) {
        numHinge = 0;
        W("Incorrect hinge count %d, default to 0\n",
          android_hw->hw_sensor_hinge_count);
    }
    struct FoldableConfig defaultConfig = {
            .type = type,
            .hingesSubType = subType,
            .foldAtPosture = foldAtPosture,
            .numHinges = numHinge,
    };
    // hinge angle ranges and defaults
    std::string hingeAngleRanges(android_hw->hw_sensor_hinge_ranges);
    std::string hingeAngleDefaults(android_hw->hw_sensor_hinge_defaults);
    std::string hingeAreas(android_hw->hw_sensor_hinge_areas);
    std::vector<std::string> hingeAngleRangeTokens, hingeAngleDefaultTokens,
            hingeAreaTokens;
    android::base::splitTokens(hingeAngleRanges, &hingeAngleRangeTokens, ",");
    android::base::splitTokens(hingeAngleDefaults, &hingeAngleDefaultTokens,
                               ",");
    android::base::splitTokens(hingeAreas, &hingeAreaTokens, ",");
    if (hingeAngleRangeTokens.size() != numHinge ||
        hingeAngleDefaultTokens.size() != numHinge ||
        hingeAreaTokens.size() != numHinge) {
        E("Incorrect hinge angle configs for ranges %s, defaults %s, or "
          "areas %s\n",
          hingeAngleRanges.c_str(), hingeAngleDefaults.c_str(),
          hingeAreas.c_str());
    } else {
        for (int i = 0; i < numHinge; i++) {
            std::vector<std::string> angles, area;
            android::base::splitTokens(hingeAngleRangeTokens[i], &angles, "-");
            android::base::splitTokens(hingeAreaTokens[i], &area, "-");
            if (angles.size() != 2 || (area.size() != 2 && area.size() != 4)) {
                E("Incorrect hinge angle range %s or area %s\n",
                  hingeAngleRangeTokens[i].c_str(), hingeAreaTokens[i].c_str());
            } else {
                if (area.size() == 2) {
                    // percentage on screen and width config style
                    switch(type) {
                        case ANDROID_FOLDABLE_HORIZONTAL_SPLIT:
                            defaultConfig.hingeParams[i] = {
                                .x = 0,
                                .y = (int)(std::stof(area[0]) * android_hw->hw_lcd_height / 100.0),
                                .width = android_hw->hw_lcd_width,
                                .height = std::stoi(area[1]),
                                .displayId = 0,  // TODO: put 0 for now
                                .minDegrees = std::stof(angles[0]),
                                .maxDegrees = std::stof(angles[1]),
                                .defaultDegrees = std::stof(hingeAngleDefaultTokens[i]),
                            };
                            break;
                        case ANDROID_FOLDABLE_VERTICAL_SPLIT:
                            defaultConfig.hingeParams[i] = {
                                .x = (int)(std::stof(area[0]) * android_hw->hw_lcd_width / 100.0),
                                .y = 0,
                                .width = std::stoi(area[1]),
                                .height = android_hw->hw_lcd_height,
                                .displayId = 0,  // TODO: put 0 for now
                                .minDegrees = std::stof(angles[0]),
                                .maxDegrees = std::stof(angles[1]),
                                .defaultDegrees = std::stof(hingeAngleDefaultTokens[i]),
                            };
                            break;
                        default:
                            E("Invalid hinge type %d", type);
                    }
                } else {
                    defaultConfig.hingeParams[i] = {
                        // x, y, width, height, old configuration style
                        .x = std::stoi(area[0]),
                        .y = std::stoi(area[1]),
                        .width = std::stoi(area[2]),
                        .height = std::stoi(area[3]),
                        .displayId = 0,  // TODO: put 0 for now
                        .minDegrees = std::stof(angles[0]),
                        .maxDegrees = std::stof(angles[1]),
                        .defaultDegrees = std::stof(hingeAngleDefaultTokens[i]),
                    };
                }
            }
        }
    }
    mState.config = defaultConfig;

    // hinge angles to posture mapping
    std::string postureList(android_hw->hw_sensor_posture_list);
    std::string postureAngles(
            android_hw->hw_sensor_hinge_angles_posture_definitions);
    std::vector<std::string> postureListTokens, postureAnglesTokens;
    android::base::splitTokens(postureList, &postureListTokens, ",");
    android::base::splitTokens(postureAngles, &postureAnglesTokens, ",");
    if (postureList.empty() || postureAngles.empty() ||
        postureListTokens.size() != postureAnglesTokens.size()) {
        E("Incorrect posture_list %s or hinge_angles_posture_definitions %s\n",
          postureList.c_str(), postureAngles.c_str());
    } else {
        std::vector<std::string> anglesToken;
        for (int i = 0; i < postureListTokens.size(); i++) {
            android::base::splitTokens(postureAnglesTokens[i], &anglesToken,
                                       "&");
            if (anglesToken.size() != mState.config.numHinges) {
                E("Incorrect hinge_angles_posture_definition %s\n",
                  postureAnglesTokens[i].c_str());
            } else {
                std::vector<std::string> angles;
                struct AnglesToPosture anglesToPosture;
                for (int j = 0; j < anglesToken.size(); j++) {
                    android::base::splitTokens(anglesToken[j], &angles, "-");
                    if (angles.size() != 2) {
                        E("Incorrect hinge_angles_posture_definition %s\n",
                          anglesToken[j].c_str());
                    } else {
                        anglesToPosture.angles[j].left = std::stof(angles[0]);
                        anglesToPosture.angles[j].right = std::stof(angles[1]);
                    }
                }
                anglesToPosture.posture =
                        (FoldablePostures)std::stoi(postureListTokens[i]);
                mAnglesToPostures.push_back(anglesToPosture);
                std::sort(mAnglesToPostures.begin(), mAnglesToPostures.end(),
                          compareAnglesToPosture);
            }
        }
    }

    for (unsigned int i = 0; i < mState.config.numHinges; ++i) {
        mState.currentHingeDegrees[i] =
                mState.config.hingeParams[i].defaultDegrees;
    }
    mState.currentPosture = calculatePosture();
}

static const float kHingeAngleEpsilon = 0.001f;
void FoldableModel::setHingeAngle(uint32_t hingeIndex,
                                  float degrees,
                                  PhysicalInterpolation mode,
                                  std::recursive_mutex& mutex) {
    std::unique_lock<std::recursive_mutex> lock(mutex);
    if (hingeIndex >= ANDROID_FOLDABLE_MAX_HINGES)
        return;
    if (hingeIndex >= mState.config.numHinges)
        return;
    if (abs(mState.currentHingeDegrees[hingeIndex] - degrees) <
        kHingeAngleEpsilon) {
        return;
    }

    mState.currentHingeDegrees[hingeIndex] = degrees;
    enum FoldablePostures p = calculatePosture();
    if (mState.currentPosture != p) {
        enum FoldablePostures oldP = mState.currentPosture;
        mState.currentPosture = p;
        lock.unlock();
        sendPostureToSystem(p);
        lock.lock();
        if (mState.currentPosture == mState.config.foldAtPosture) {
            lock.unlock();
            setToolBarFold(true);
        } else if (oldP == mState.config.foldAtPosture) {
            lock.unlock();
            setToolBarFold(false);
        }
    }
}

void FoldableModel::setPosture(float posture, PhysicalInterpolation mode,
                               std::recursive_mutex& mutex) {
    enum FoldablePostures p = (enum FoldablePostures)posture;
    enum FoldablePostures oldP = mState.currentPosture;
    std::unique_lock<std::recursive_mutex> lock(mutex);

    if (mState.currentPosture == p) {
        return;
    }
    mState.currentPosture = p;
    lock.unlock();
    sendPostureToSystem(p);
    lock.lock();
    // Update the hinge angles
    for (const auto i : mAnglesToPostures) {
        if (i.posture == mState.currentPosture) {
            for (uint32_t j = 0; j < mState.config.numHinges; j++) {
                mState.currentHingeDegrees[j] =
                        (i.angles[j].left + i.angles[j].right) / 2.0f;
            }
        }
    }
    if (mState.currentPosture == mState.config.foldAtPosture) {
        lock.unlock();
        setToolBarFold(true);
    } else if (oldP == mState.config.foldAtPosture) {
        lock.unlock();
        setToolBarFold(false);
    }
}

float FoldableModel::getHingeAngle(uint32_t hingeIndex,
                                   ParameterValueType parameterValueType) const {
    if (hingeIndex >= ANDROID_FOLDABLE_MAX_HINGES)
        return 0.0f;
    if (hingeIndex >= mState.config.numHinges)
        return 0.0f;

    return parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT
                   ? mState.config.hingeParams[hingeIndex].defaultDegrees
                   : mState.currentHingeDegrees[hingeIndex];
}

float FoldableModel::getPosture(ParameterValueType parameterValueType) const {
    return parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT
                   ? (float)POSTURE_UNKNOWN
                   : (float)mState.currentPosture;
}

void FoldableModel::sendPostureToSystem(enum FoldablePostures p) {
    auto adbInterface = emulation::AdbInterface::getGlobal();
    if (!adbInterface) return;
    adbInterface->enqueueCommand({ "shell", "settings", "put",
                                    "global", "device_posture",
                                    std::to_string((int)p).c_str() });

}

void FoldableModel::setToolBarFold(bool fold) {
    const QAndroidEmulatorWindowAgent* windowAgent =
        android_hw_sensors_get_window_agent();
    if (!windowAgent) {
        return;
    }
    windowAgent->fold(fold);
}

bool FoldableModel::isFolded() {
    if (mState.currentPosture == mState.config.foldAtPosture) {
        return true;
    }
    return false;
}

bool FoldableModel::getFoldedArea(int* x, int* y, int* w, int* h) {
    if (x) {
        *x = android_hw->hw_displayRegion_0_1_xOffset;
    }
    if (y) {
        *y = android_hw->hw_displayRegion_0_1_yOffset;
    }
    if (w) {
        *w = android_hw->hw_displayRegion_0_1_width;
    }
    if (h) {
        *h = android_hw->hw_displayRegion_0_1_height;
    }
    return true;
}

}  // namespace physics
}  // namespace android
