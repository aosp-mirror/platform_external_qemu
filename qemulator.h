#ifndef QEMULATOR_H
#define QEMULATOR_H

#include "android/config.h"
#include "android/skin/file.h"
#include "android/skin/image.h"
#include "android/skin/keyboard.h"
#include "android/skin/rect.h"
#include "android/skin/trackball.h"
#include "android/skin/window.h"
#include "android/cmdline-option.h"

typedef struct {
    AConfig*       aconfig;
    SkinFile*      layout_file;
    SkinLayout*    layout;
    SkinKeyboard*  keyboard;
    SkinWindow*    window;
    int            win_x;
    int            win_y;
    int            show_trackball;
    SkinTrackBall* trackball;
    int            lcd_brightness;
    SkinImage*     onion;
    SkinRotation   onion_rotation;
    int            onion_alpha;

    AndroidOptions  opts[1];  /* copy of options */
} QEmulator;

extern QEmulator qemulator[];

#endif
