#include "FramelessDetector.h"

#include "android/globals.h"

// TODO(lfy): Other things might come up, put them here
bool FramelessDetector::isFramelessOk() {
    bool isWear =
        avdInfo_getAvdFlavor(android_avdInfo) == AVD_WEAR;

    bool ok = !isWear;

    return ok;
}
