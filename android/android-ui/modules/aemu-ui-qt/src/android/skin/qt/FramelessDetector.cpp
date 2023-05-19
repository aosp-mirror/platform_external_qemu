#include "android/skin/qt/FramelessDetector.h"

#include "android/avd/info.h"  // for avdInfo_getAvdFlavor
#include "android/avd/util.h"  // for AVD_WEAR
#include "android/console.h"   // for getConsoleAgents()->settings->avdInfo()

// TODO: Other things might come up, put them here
bool FramelessDetector::isFramelessOk() {
    bool isWear =
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_WEAR;

    bool ok = !isWear;

    return ok;
}
