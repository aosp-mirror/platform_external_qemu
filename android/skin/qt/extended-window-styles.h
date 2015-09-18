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

// This gives the order of the tabbed panes on the extended window.
// This must correspond to the ordering that is set from within
// QtDesigner and written to extended.ui.
enum { PANE_IDX_LOCATION = 0,
       PANE_IDX_CELLULAR,
       PANE_IDX_BATTERY,
       PANE_IDX_CAMERA,
       PANE_IDX_TELEPHONE,
       PANE_IDX_VIRT_SENSORS,
       PANE_IDX_HW_SENSORS,
       PANE_IDX_DPAD,
       PANE_IDX_MESSAGE,
       PANE_IDX_FINGER,
       PANE_IDX_SD,
       PANE_IDX_RECORD_SCR,
       PANE_IDX_KBD_SHORTS,
       PANE_IDX_SETTINGS };

// Style sheets
//
// We have two styles: one for the light-colored theme and one for
// the dark-colored theme.
//
// Each theme indicates the colors that are to be used for the foreground
// (mostly text) and the background.
//
// Even within a theme, not all widgets use the same colors. The style
// sheet accomodates this by associating colors based on "properties"
// that can be assigned to a widget. (Properties are listed in the .ui
// file.)
//
// So far, we have four sets of colors for each theme:
//   "MajorTab":       The area of the tab buttons on the left of the window
//   "MajorTabTitle":  Section titles separating the tab buttons
//   "Title":          Section titles in the main part of the window
//   <Normal>:         Text in the main part of the window
//
// The first three sets are identified by the value of their "ColorGroup"
// property. The last set is identified by not having a "ColorGroup" property.

// These are the colors used in the two themes

#define LIGHT_BKG_COLOR         "#f9f9f9"  // Main page background
#define LIGHT_MAJOR_TAB_COLOR   "#91a4ad"  // Text of major tabs
#define LIGHT_MAJOR_TITLE_COLOR "#617d8a"  // Text of major tab separators
#define LIGHT_TAB_BKG_COLOR     "#ffffff"  // Background of major tabs
#define LIGHT_TEXT_COLOR        "#212121"  // Main page text
#define LIGHT_TITLE_COLOR       "#757575"  // Main page titles
#define LIGHT_TOOL_COLOR        "#00bea4"  // Checkboxes, sliders, etc.

#define DARK_BKG_COLOR          "#273238"
#define DARK_MAJOR_TAB_COLOR    "#bdc0c3"
#define DARK_MAJOR_TITLE_COLOR  "#e5e6e7"
#define DARK_TAB_BKG_COLOR      "#394249"
#define DARK_TEXT_COLOR         "#eeeeee"
#define DARK_TITLE_COLOR        "#ffffff"
#define DARK_TOOL_COLOR         "#00bea4"

// These are the styles. One for each theme.

#define QT_STYLE_LIGHT_THEME \
    "*[ColorGroup=\"MajorTab\"]      { color:" LIGHT_MAJOR_TAB_COLOR "}" \
    "*[ColorGroup=\"MajorTabTitle\"] { color:" LIGHT_MAJOR_TITLE_COLOR ";" \
                                      "background-color:" LIGHT_TAB_BKG_COLOR "}" \
    "*[ColorGroup=\"Title\"]         { color:" LIGHT_TITLE_COLOR "}" \
    "*                               { color:" LIGHT_TEXT_COLOR ";" \
                                      "background-color: "LIGHT_BKG_COLOR "}" \
    "QTextEdit, QPlainTextEdit, QTreeView {" \
            "border: 1px solid " LIGHT_TEXT_COLOR "} "\
    "QCheckBox::indicator:checked {" \
            "image: url(:/dark/check_yes); " \
            "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked {" \
            "image: url(:/dark/check_no); " \
            "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QSlider::sub-page:horizontal {background: " LIGHT_TOOL_COLOR "}" \
    "QSlider::handle:horizontal {" \
            "background:" LIGHT_TOOL_COLOR ";" \
            "border: 1px solid " LIGHT_TOOL_COLOR "; border-radius: 7px }"

#define QT_STYLE_DARK_THEME \
    "*[ColorGroup=\"MajorTab\"]      { color:" DARK_MAJOR_TAB_COLOR "}" \
    "*[ColorGroup=\"MajorTabTitle\"] { color:" DARK_MAJOR_TITLE_COLOR ";" \
                                      "background-color:" DARK_TAB_BKG_COLOR "}" \
    "*[ColorGroup=\"Title\"]         { color:" DARK_TITLE_COLOR "}" \
    "*                               { color:" DARK_TEXT_COLOR ";" \
                                      "background-color: "DARK_BKG_COLOR "}" \
    "QTextEdit, QPlainTextEdit, QTreeView {" \
            "border: 1px solid " DARK_TEXT_COLOR "} "\
    "QCheckBox::indicator:checked {" \
            "image: url(:/dark/check_yes); " \
            "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked {" \
            "image: url(:/dark/check_no); " \
            "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QSlider::sub-page:horizontal {background: " DARK_TOOL_COLOR "}" \
    "QSlider::handle:horizontal {" \
            "background:" DARK_TOOL_COLOR ";" \
            "border: 1px solid " DARK_TOOL_COLOR "; border-radius: 7px }"

#endif // ANDROID_SKIN_QT_EXTENDED_WINDOW_STYLES_H
