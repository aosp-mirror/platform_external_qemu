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
    if (android_foldable_hinge_configured()) {
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
    } else if (android_foldable_rollable_configured()) {
        for (uint32_t i = 0; i < mAnglesToPostures.size(); i++) {
            if (mAnglesToPostures[i].angles[0].left >
                mState.currentRolledPercent[0]) {
                break;
            }
            if (mAnglesToPostures[i].angles[0].left <=
                        mState.currentRolledPercent[0] &&
                mAnglesToPostures[i].angles[0].right >=
                        mState.currentRolledPercent[0]) {
                entries.push_back(i);
            }
        }
        for (const auto i : entries) {
            bool found = true;
            for (uint32_t j = 1; j < mState.config.numRolls; j++) {
                if (mAnglesToPostures[i].angles[j].left >
                            mState.currentRolledPercent[j] ||
                    mAnglesToPostures[i].angles[j].right <
                            mState.currentRolledPercent[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return mAnglesToPostures[i].posture;
            }
        }
    }
    return POSTURE_UNKNOWN;
}

static bool compareAnglesToPosture(AnglesToPosture v1, AnglesToPosture v2) {
    return (v1.angles[0].left < v2.angles[0].left);
}

void FoldableModel::initFoldableHinge() {
    if (!android_hw->hw_sensor_hinge) {
        mState.config.numHinges = 0;
        return;
    }

    if (!android_foldable_hinge_configured()) {
        // Create a default hinge with 180 degree.
        // Also help back compatible with legacy foldable config which does not
        // include hinge configs
        mState.config.numHinges = 1;
        mState.config.hingeParams[0].defaultDegrees = 180.0f;
        mState.currentHingeDegrees[0] = 180.0f;
        mState.currentPosture = POSTURE_OPENED;
        mState.config.foldAtPosture = POSTURE_CLOSED;
        mState.config.supportedFoldablePostures[POSTURE_CLOSED] = true;
        mState.config.supportedFoldablePostures[POSTURE_OPENED] = true;
        mState.config.supportedFoldablePostures[POSTURE_HALF_OPENED] = false;
        mState.config.supportedFoldablePostures[POSTURE_FLIPPED] = false;
        mState.config.supportedFoldablePostures[POSTURE_TENT] = false;
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
            (enum FoldablePostures)android_hw
                    ->hw_sensor_hinge_fold_to_displayRegion_0_1_at_posture;
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

    struct FoldableConfig& defaultConfig = mState.config;
    defaultConfig.type = type;
    defaultConfig.hingesSubType = subType;
    defaultConfig.foldAtPosture = foldAtPosture;
    defaultConfig.numHinges = numHinge;

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
                    switch (type) {
                        case ANDROID_FOLDABLE_HORIZONTAL_SPLIT:
                            defaultConfig.hingeParams[i] = {
                                    .x = 0,
                                    .y = (int)(std::stof(area[0]) *
                                               android_hw->hw_lcd_height /
                                               100.0),
                                    .width = android_hw->hw_lcd_width,
                                    .height = std::stoi(area[1]),
                                    .displayId = 0,  // TODO: put 0 for now
                                    .minDegrees = std::stof(angles[0]),
                                    .maxDegrees = std::stof(angles[1]),
                                    .defaultDegrees = std::stof(
                                            hingeAngleDefaultTokens[i]),
                            };
                            break;
                        case ANDROID_FOLDABLE_VERTICAL_SPLIT:
                            defaultConfig.hingeParams[i] = {
                                    .x = (int)(std::stof(area[0]) *
                                               android_hw->hw_lcd_width /
                                               100.0),
                                    .y = 0,
                                    .width = std::stoi(area[1]),
                                    .height = android_hw->hw_lcd_height,
                                    .displayId = 0,  // TODO: put 0 for now
                                    .minDegrees = std::stof(angles[0]),
                                    .maxDegrees = std::stof(angles[1]),
                                    .defaultDegrees = std::stof(
                                            hingeAngleDefaultTokens[i]),
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
                            .defaultDegrees =
                                    std::stof(hingeAngleDefaultTokens[i]),
                    };
                }
            }
        }
    }

    for (unsigned int i = 0; i < mState.config.numHinges; ++i) {
        mState.currentHingeDegrees[i] =
                mState.config.hingeParams[i].defaultDegrees;
    }
}

void FoldableModel::initFoldableRoll() {
    if (!android_hw->hw_sensor_roll) {
        mState.config.numRolls = 0;
        return;
    }

    struct FoldableConfig& config = mState.config;
    // type
    enum FoldableDisplayType type =
            (enum FoldableDisplayType)android_hw->hw_sensor_hinge_type;
    if (type < 0 || type >= ANDROID_FOLDABLE_TYPE_MAX) {
        type = ANDROID_FOLDABLE_HORIZONTAL_ROLL;
        W("Incorrect rollable type %d, default to horizontal roll\n",
          android_hw->hw_sensor_hinge_type);
    }
    config.type = type;

    // number
    int numRolls = android_hw->hw_sensor_roll_count;
    if (numRolls < 0 || numRolls > ANDROID_FOLDABLE_MAX_ROLLS) {
        numRolls = 0;
        W("Incorrect roll count %d, default to 0\n",
          android_hw->hw_sensor_roll_count);
    }
    config.numRolls = numRolls;

    // resize at postures
    config.resizeAtPosture[0] =
            (enum FoldablePostures)android_hw
                    ->hw_sensor_roll_resize_to_displayRegion_0_1_at_posture;
    config.resizeAtPosture[1] =
            (enum FoldablePostures)android_hw
                    ->hw_sensor_roll_resize_to_displayRegion_0_2_at_posture;
    config.resizeAtPosture[2] =
            (enum FoldablePostures)android_hw
                    ->hw_sensor_roll_resize_to_displayRegion_0_3_at_posture;

    // hinge angle ranges and defaults
    std::string rollRanges(android_hw->hw_sensor_roll_ranges);
    std::string rollDefaults(android_hw->hw_sensor_roll_defaults);
    std::string rollRadius(android_hw->hw_sensor_roll_radius);
    std::string rollDirection(android_hw->hw_sensor_roll_direction);
    std::vector<std::string> rollRangeTokens, rollDefaultTokens,
            rollRadiusTokens, rollDirectionTokens;
    android::base::splitTokens(rollRanges, &rollRangeTokens, ",");
    android::base::splitTokens(rollDefaults, &rollDefaultTokens, ",");
    android::base::splitTokens(rollRadius, &rollRadiusTokens, ",");
    android::base::splitTokens(rollDirection, &rollDirectionTokens, ",");
    if (rollRangeTokens.size() != numRolls ||
        rollDefaultTokens.size() != numRolls ||
        rollRadiusTokens.size() != numRolls ||
        rollDirectionTokens.size() != numRolls) {
        E("Incorrect rollable configs for ranges %s, defaults %s, "
          "radius %s, or directions %s\n",
          rollRanges.c_str(), rollDefaults.c_str(), rollRadius.c_str(),
          rollDirection.c_str());
    } else {
        for (int i = 0; i < numRolls; i++) {
            std::vector<std::string> range;
            android::base::splitTokens(rollRangeTokens[i], &range, "-");
            if (range.size() != 2) {
                E("Incorrect rollable angle range %s \n",
                  rollRangeTokens[i].c_str());
            } else {
                config.rollableParams[i] = {
                        .rollRadiusAsDisplayPercent =
                                std::stof(rollRadiusTokens[i]),
                        .displayId = 0,  // TODO: put 0 for now
                        .minRolledPercent = std::stof(range[0]),
                        .maxRolledPercent = std::stof(range[1]),
                        .defaultRolledPercent = std::stof(rollDefaultTokens[i]),
                        .direction = std::stoi(rollDirectionTokens[i]),
                };
            }
        }
    }
    for (unsigned int i = 0; i < mState.config.numRolls; ++i) {
        mState.currentRolledPercent[i] =
                mState.config.rollableParams[i].defaultRolledPercent;
    }
}

void FoldableModel::initPostures() {
    bool isHinge;
    if (android_foldable_hinge_configured()) {
        isHinge = true;
    } else if (android_foldable_rollable_configured()) {
        isHinge = false;
    } else {
        mState.currentPosture = POSTURE_UNKNOWN;
        return;
    }
    for (int i = 0; i < POSTURE_MAX; ++i) {
        mState.config.supportedFoldablePostures[i] = false;
    }

    std::string postureList(android_hw->hw_sensor_posture_list);
    std::string postureValues(
            isHinge ? android_hw->hw_sensor_hinge_angles_posture_definitions
                    : android_hw->hw_sensor_roll_percentages_posture_definitions);
    std::vector<std::string> postureListTokens, postureValuesTokens;
    android::base::splitTokens(postureList, &postureListTokens, ",");
    android::base::splitTokens(postureValues, &postureValuesTokens, ",");
    if (postureList.empty() || postureValues.empty() ||
        postureListTokens.size() != postureValuesTokens.size()) {
        E("Incorrect posture list %s or posture mapping %s\n",
          postureList.c_str(), postureValues.c_str());
    } else {
        std::vector<std::string> valuesToken;
        for (int i = 0; i < postureListTokens.size(); i++) {
            android::base::splitTokens(postureValuesTokens[i], &valuesToken,
                                       "&");
            if (isHinge && valuesToken.size() != mState.config.numHinges ||
                !isHinge && valuesToken.size() != mState.config.numRolls) {
                E("Incorrect posture mapping %s\n",
                  postureValuesTokens[i].c_str());
            } else {
                std::vector<std::string> values;
                struct AnglesToPosture valuesToPosture;
                for (int j = 0; j < valuesToken.size(); j++) {
                    android::base::splitTokens(valuesToken[j], &values, "-");
                    if (values.size() != 2) {
                        E("Incorrect posture mapping %s\n",
                          valuesToken[j].c_str());
                    } else {
                        valuesToPosture.angles[j].left = std::stof(values[0]);
                        valuesToPosture.angles[j].right = std::stof(values[1]);
                    }
                }
                enum FoldablePostures parsedPosture =
                        (FoldablePostures)std::stoi(postureListTokens[i]);
                mState.config.supportedFoldablePostures[parsedPosture] = true;

                valuesToPosture.posture = parsedPosture;
                mAnglesToPostures.push_back(valuesToPosture);
                std::sort(mAnglesToPostures.begin(), mAnglesToPostures.end(),
                          compareAnglesToPosture);
            }
        }
        mState.currentPosture = calculatePosture();
    }
}

FoldableModel::FoldableModel() {
    initFoldableHinge();
    initFoldableRoll();
    initPostures();
}

static const float kFloatValueEpsilon = 0.001f;
void FoldableModel::setHingeAngle(uint32_t hingeIndex,
                                  float degrees,
                                  PhysicalInterpolation mode,
                                  std::recursive_mutex& mutex) {
    VLOG(foldable) << "setHingeAngle index " << hingeIndex << " degrees " << degrees;
    std::unique_lock<std::recursive_mutex> lock(mutex);
    if (hingeIndex >= ANDROID_FOLDABLE_MAX_HINGES)
        return;
    if (hingeIndex >= mState.config.numHinges)
        return;
    if (abs(mState.currentHingeDegrees[hingeIndex] - degrees) <
        kFloatValueEpsilon) {
        return;
    }

    mState.currentHingeDegrees[hingeIndex] = degrees;
    enum FoldablePostures p = calculatePosture();
    if (mState.currentPosture != p) {
        mState.currentPosture = p;
        if (!android_hw_sensors_is_loading_snapshot()) {
            // during snapshot load, setPosture will handle the following
            lock.unlock();
            sendPostureToSystem(p);
            updateFoldablePostureIndicator();
        }
    }
}

void FoldableModel::setPosture(float posture, PhysicalInterpolation mode,
                               std::recursive_mutex& mutex) {
    std::unique_lock<std::recursive_mutex> lock(mutex);
    enum FoldablePostures p = (enum FoldablePostures)posture;
    VLOG(foldable) << "setPosture " << posture << " current posture " << mState.currentPosture;

    // E.g., if initial config is folded, and snapshot also foled.
    // Need force sending this status to UI, and adjust the host window size
    if (android_hw_sensors_is_loading_snapshot() &&
        (android_foldable_hinge_configured() ||
         android_foldable_rollable_configured() ||
         android_foldable_any_folded_area_configured())) {
        mState.currentPosture = p;
        lock.unlock();
        sendPostureToSystem(p);
        updateFoldablePostureIndicator();
        return;
    }

    if (mState.currentPosture == p) {
        return;
    }
    mState.currentPosture = p;
    // Update the hinge angles
    if (android_foldable_hinge_configured()) {
        for (const auto i : mAnglesToPostures) {
            if (i.posture == mState.currentPosture) {
                for (uint32_t j = 0; j < mState.config.numHinges; j++) {
                    mState.currentHingeDegrees[j] =
                            (i.angles[j].left + i.angles[j].right) / 2.0f;
                }
            }
        }
    }
    // Update rollable
    if (android_foldable_rollable_configured()) {
            for (const auto i : mAnglesToPostures) {
            if (i.posture == mState.currentPosture) {
                for (uint32_t j = 0; j < mState.config.numRolls; j++) {
                    mState.currentRolledPercent[j] =
                            (i.angles[j].left + i.angles[j].right) / 2.0f;
                }
            }
        }
    }

    lock.unlock();
    sendPostureToSystem(p);
    updateFoldablePostureIndicator();
}

void FoldableModel::setRollable(uint32_t index,
                                float percentage,
                                PhysicalInterpolation mode,
                                std::recursive_mutex& mutex) {
    VLOG(foldable) << "setRollable index " << index << " percentage " << percentage;
    std::unique_lock<std::recursive_mutex> lock(mutex);
    if (index >= ANDROID_FOLDABLE_MAX_ROLLS ||
        index >= mState.config.numRolls) {
        return;
    }
    if (abs(mState.currentRolledPercent[index] - percentage) <
        kFloatValueEpsilon) {
        return;
    }

    mState.currentRolledPercent[index] = percentage;
    enum FoldablePostures p = calculatePosture();
    if (mState.currentPosture != p) {
        mState.currentPosture = p;
        if (!android_hw_sensors_is_loading_snapshot()) {
            // during snapshot load, setPosture will handle the following
            lock.unlock();
            updateFoldablePostureIndicator();
        }
    }
}

float FoldableModel::getHingeAngle(
        uint32_t hingeIndex,
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

float FoldableModel::getRollable(uint32_t index,
                                 ParameterValueType parameterValueType) const {
    if (index >= ANDROID_FOLDABLE_MAX_ROLLS)
        return 0.0f;
    if (index >= mState.config.numRolls)
        return 0.0f;

    return parameterValueType == PARAMETER_VALUE_TYPE_DEFAULT
                   ? mState.config.rollableParams[index].defaultRolledPercent
                   : mState.currentRolledPercent[index];
}

bool FoldableModel::isFolded() {
    if (android_foldable_rollable_configured()) {
        for (int i = 0; i < ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS; i++) {
            if (mState.config.resizeAtPosture[i] == mState.currentPosture &&
                android_foldable_folded_area_configured(i)) {
                return true;
            }
        }
        return false;
    }
    // legacy foldable, and new hinge model
    if (mState.config.foldAtPosture == mState.currentPosture &&
        android_foldable_folded_area_configured(0)) {
        return true;
    }
    return false;
}

bool FoldableModel::getFoldedArea(int* x, int* y, int* w, int* h) {
    int xOffset, yOffset, width, height;
    if (android_foldable_rollable_configured()) {
        int displayRegion = 0;
        while (displayRegion < ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS) {
            if (mState.config.resizeAtPosture[displayRegion] == mState.currentPosture &&
                android_foldable_folded_area_configured(displayRegion)) {
                break;
            }
            displayRegion++;
        }
        if (displayRegion == ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS) {
            return false;
        }
        switch (displayRegion) {
            case 0:
                xOffset = android_hw->hw_displayRegion_0_1_xOffset;
                yOffset = android_hw->hw_displayRegion_0_1_yOffset;
                width   = android_hw->hw_displayRegion_0_1_width;
                height  = android_hw->hw_displayRegion_0_1_height;
                break;
            case 1:
                xOffset = android_hw->hw_displayRegion_0_2_xOffset;
                yOffset = android_hw->hw_displayRegion_0_2_yOffset;
                width   = android_hw->hw_displayRegion_0_2_width;
                height  = android_hw->hw_displayRegion_0_2_height;
                break;
            case 2:
                xOffset = android_hw->hw_displayRegion_0_3_xOffset;
                yOffset = android_hw->hw_displayRegion_0_3_yOffset;
                width   = android_hw->hw_displayRegion_0_3_width;
                height  = android_hw->hw_displayRegion_0_3_height;
                break;
            default:
                return false;
        }
        if (x) {
            *x = xOffset;
        }
        if (y) {
            *y = yOffset;
        }
        if (w) {
            *w = width;
        }
        if (h) {
            *h = height;
        }
        return true;
    }
    // legacy fold and new hinge model
    if (mState.currentPosture == mState.config.foldAtPosture &&
        android_foldable_folded_area_configured(0)) {
        xOffset = android_hw->hw_displayRegion_0_1_xOffset;
        yOffset = android_hw->hw_displayRegion_0_1_yOffset;
        width   = android_hw->hw_displayRegion_0_1_width;
        height  = android_hw->hw_displayRegion_0_1_height;
    }
    if (x) {
        *x = xOffset;
    }
    if (y) {
        *y = yOffset;
    }
    if (w) {
        *w = width;
    }
    if (h) {
        *h = height;
    }
    return true;
}

void FoldableModel::updateFoldablePostureIndicator() {
    const QAndroidEmulatorWindowAgent* windowAgent =
        android_hw_sensors_get_window_agent();
    if (windowAgent) {
        windowAgent->updateFoldablePostureIndicator(false);
    } else {
        E("Could not update foldable posture indicator: null WindowAgent");
    }
}

}  // namespace physics
}  // namespace android
