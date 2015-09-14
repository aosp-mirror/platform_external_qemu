/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

// This file contains the colors and other styles that determine
// how the extended window appears

#ifndef ANDROID_SKIN_QT_EXTENDED_WINDOW_STYLES_H
#define ANDROID_SKIN_QT_EXTENDED_WINDOW_STYLES_H

// Colors for the dark theme
#define DARK_FOREGROUND  "#f0f0f0"
#define DARK_BACKGROUND  "#3c4249"
#define DARK_ACTIVE      "#4c5259"  // A little lighter than the background

// Colors for the light theme
#define LIGHT_FOREGROUND "#0f0f0f"
#define LIGHT_BACKGROUND "#c3bdb6"
#define LIGHT_ACTIVE     "#b3ada6"  // A little darker than the background

// The sliders look ugly without some customization
#define SLIDER_STYLE "QSlider::sub-page:horizontal {background: #888}" \
                     "QSlider::handle:horizontal {background: #eee;" \
                               "border: 2px solid #444; border-radius: 4px }"

// The check boxes also look ugly without some customization
#define DARK_CHECKBOX_STYLE \
    "QCheckBox::indicator:checked   { image: url(:/dark/check_yes); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked { image: url(:/dark/check_no); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }"

#define LIGHT_CHECKBOX_STYLE \
    "QCheckBox::indicator:checked   { image: url(:/light/check_yes); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked { image: url(:/light/check_no); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }"

// This gives the order of the tabbed panes on the extended window.
// This must correspond to the ordering that is set from within
// QtDesigner and written to extended.ui.
enum { PANE_IDX_BATTERY = 0,
       PANE_IDX_TELEPHONE,
       PANE_IDX_MESSAGE,
       PANE_IDX_LOCATION,
       PANE_IDX_CELLULAR,
       PANE_IDX_FINGER,
       PANE_IDX_SD,
       PANE_IDX_SETTINGS };

#endif // ANDROID_SKIN_QT_EXTENDED_WINDOW_STYLES_H
