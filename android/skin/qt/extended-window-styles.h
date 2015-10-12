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

#pragma once

// This file contains the colors and other styles that determine
// how the extended window appears

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
// So far, we have five sets of colors for each theme:
//   "MajorTab":       The area of the tab buttons on the left of the window
//   "MajorTabTitle":  Section titles separating the tab buttons
//   "Title":          Section titles in the main part of the window
//   "Tool":           Buttons whose text is the same color as a checkbox
//   <Normal>:         Text in the main part of the window
//
// The first four sets are identified by the value of their "ColorGroup"
// property. The last set is identified by not having a "ColorGroup" property.

// These are the colors used in the two themes

#define LIGHT_BKG_COLOR            "#f9f9f9"  // Main page background
#define LIGHT_DISABLED_TOOL_COLOR  "#baeae4"  // Grayed-out button text
#define LIGHT_DIVIDER_COLOR        "#e0e0e0"  // Line between items
#define LIGHT_MAJOR_TAB_COLOR      "#91a4ad"  // Text of major tabs
#define LIGHT_MAJOR_TITLE_COLOR    "#617d8a"  // Text of major tab separators
#define LIGHT_PATH                 "light"    // Icon directory under images/
#define LIGHT_TAB_BKG_COLOR        "#ffffff"  // Background of major tabs
#define LIGHT_TEXT_COLOR           "#212121"  // Main page text
#define LIGHT_TITLE_COLOR          "#757575"  // Main page titles
#define LIGHT_TOOL_COLOR           "#00bea4"  // Checkboxes, sliders, etc.

#define DARK_BKG_COLOR             "#273238"
#define DARK_DISABLED_TOOL_COLOR   "#1b5c58"
#define DARK_DIVIDER_COLOR         "#e0e0e0"
#define DARK_MAJOR_TAB_COLOR       "#bdc0c3"
#define DARK_MAJOR_TITLE_COLOR     "#e5e6e7"
#define DARK_PATH                  "dark"
#define DARK_TAB_BKG_COLOR         "#394249"
#define DARK_TEXT_COLOR            "#eeeeee"
#define DARK_TITLE_COLOR           "#ffffff"
#define DARK_TOOL_COLOR            "#00bea4"

// This is the style, based on either the LIGHT or DARK theme
#define QT_STYLE(THEME) \
    "*[ColorGroup=\"MajorTab\"]      { color:" THEME##_MAJOR_TAB_COLOR ";" \
                                      "padding: 0px 0px 0px 16px }" \
    "*[ColorGroup=\"MajorTabTitle\"] { color:" THEME##_MAJOR_TITLE_COLOR ";" \
            "background-color:" THEME##_TAB_BKG_COLOR "}" \
    "*[ColorGroup=\"Title\"]         { color:" THEME##_TITLE_COLOR "}" \
    "*::disabled[ColorGroup=\"Tool\"]{ color:" THEME##_DISABLED_TOOL_COLOR "}" \
    "*[ColorGroup=\"Tool\"]          { color:" THEME##_TOOL_COLOR "}" \
    "*[class=\"EditableValue\"]  { color:" THEME##_TEXT_COLOR "; font-size: 11pt }" \
    "*[ColorGroup=\"SliderLabel\"]  { color:" THEME##_DIVIDER_COLOR "; font-size: 9pt }" \
    "*                               { color:" THEME##_TEXT_COLOR ";" \
                                      "background-color: " THEME##_BKG_COLOR "}" \
    "QPlainTextEdit, QTextEdit, QLineEdit, QTreeView {" \
            "border-top: 0px; "\
            "border-bottom: 2px solid " THEME##_DIVIDER_COLOR "}" \
    "QCheckBox::indicator:checked {" \
            "image: url(:/" THEME##_PATH "/check_yes); " \
            "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked {" \
            "image: url(:/" THEME##_PATH "/check_no); " \
            "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QSlider::groove {border: 1px ;height: 2px}" \
    "QSlider::handle {background: solid " THEME##_TOOL_COLOR ";" \
        "border: 1px solid " THEME##_TOOL_COLOR ";width: 12px;" \
        "margin: -6px 0; border-radius: 7px}" \
    "QSlider::sub-page:horizontal {background:" THEME##_TOOL_COLOR \
        "; border-radius: 1px}" \
    "QSlider::add-page:horizontal {background:" THEME##_DIVIDER_COLOR \
        "; border-radius: 1px}" \
    "QTableWidget { gridline-color:" THEME##_TAB_BKG_COLOR ";" \
        "background:" THEME##_TAB_BKG_COLOR "; border: 0px }" \
    "QTableView::item { border-top: 1px solid " THEME##_DIVIDER_COLOR "}" \
    "QHeaderView::section { color:" THEME##_TITLE_COLOR "; background:" \
        THEME##_TAB_BKG_COLOR "; border: 0px }" \
    "QScrollBar::vertical { background:" THEME##_TAB_BKG_COLOR \
        "; margin 21px 0 21 px 0 }" \
    "QScrollBar::handle:vertical { background:" THEME##_DIVIDER_COLOR \
        "; min-height: 25px }" \
    "QScrollBar::add-line:vertical { border: none; background: none }" \
    "QScrollBar::sub-line:vertical { border: none; background: none }" \
    "QComboBox { border-top: 0px; border-bottom: 2px solid " \
        THEME##_DIVIDER_COLOR "}" \
    "QComboBox::drop-down { border-left-style: solid; }" \
    "QComboBox::down-arrow { image:url(:/" THEME##_PATH "/drop_down); " \
        "border: none; width: 24px; height: 24px; }" \
    "QTabWidget::pane {border: none;}" \
    "QTabWidget::tab-bar {left: 0px;}" \
    "QTabBar::tab {background:" THEME##_BKG_COLOR "; padding-bottom:8px; font-size: 11pt; width: 150px; text-align: left}" \
    "QTabBar::tab:selected {color:" THEME##_TEXT_COLOR ";" \
        "border-bottom: 4px solid " THEME##_TOOL_COLOR "}" \
    "QTabBar::tab:!selected{color:" THEME##_TITLE_COLOR ";" \
        "border-bottom: 4px solid " THEME##_BKG_COLOR "}" \
    "QTabBar::tab:disabled {color: transparent;" \
        "border-bottom: 4px solid transparent}"
