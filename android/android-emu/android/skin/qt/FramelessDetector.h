#pragma once

// Detects whether or not it is OK to enable frameless
// AVD, based on various things such as whether or not the
// existing skin will work with frameless.
class FramelessDetector {
public:
    static bool isFramelessOk();
};
