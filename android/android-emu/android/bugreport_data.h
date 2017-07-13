#pragma once
#include "android/utils/compiler.h"

#include <string>

ANDROID_BEGIN_HEADER
struct BugreportData {
    std::string androidVer;
    std::string avdDetails;
    std::string cpuModel;
    std::string deviceName;
    std::string emulatorVer;
    std::string hypervisorVer;
    std::string hostOsName;
    std::string reproSteps;
    std::string sdkToolsVer;

    std::string adbLogcatOutput;
    std::string saveLocation;
    std::string adbBugreportFilePath;
    std::string screenshotFilePath;
    std::string bugreportFolderPath;

    bool adbBugreportSucceed;
    bool adbLogactSucceed;
    bool screenshotSucceed;
    bool bugreportSavedSucceed;
};
ANDROID_END_HEADER