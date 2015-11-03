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
enum ExtendedWindowPane {
    PANE_IDX_LOCATION = 0,
    PANE_IDX_CELLULAR,
    PANE_IDX_BATTERY,
    PANE_IDX_TELEPHONE,
    PANE_IDX_VIRT_SENSORS,
    PANE_IDX_DPAD,
    PANE_IDX_FINGER,
    PANE_IDX_HELP,
    PANE_IDX_SETTINGS,
};

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
// So far, we have these special styles for each theme:
//   "MajorTab":       The area of the tab buttons on the left of the window
//   "MajorTabTitle":  Section titles separating the tab buttons
//   "Title":          Section titles in the main part of the window
//   "Tool":           Buttons whose text is the same color as a checkbox
//   "EditableValue":    Text that can be edited
//   "SliderLabel":      The label on a slider
//   "SmsBox":           The one item that has a border on all four sides
//   "GradientDivider":  The vertical line to the right of the main tabs
//   <Normal>:         Text in the main part of the window
//
// These are identified by the value of their "ColorGroup" or "class"
// property.

// These are the colors used in the two themes

#define LIGHT_BOX_COLOR            "#e0e0e0"  // Boundary around SMS text area
#define LIGHT_BKG_COLOR            "#f0f0f0"  // Main page background
#define LIGHT_DISABLED_TOOL_COLOR  "#baeae4"  // Grayed-out button text
#define LIGHT_DIVIDER_COLOR        "#e0e0e0"  // Line between items
#define LIGHT_EDIT_COLOR           "#e0e0e0"  // Line under editable fields
#define LIGHT_LARGE_DIVIDER_COLOR  "#ebebeb"  // Start of large divider's gradient
#define LIGHT_MAJOR_TAB_COLOR      "#91a4ad"  // Text of major tabs
#define LIGHT_MAJOR_TITLE_COLOR    "#617d8a"  // Text of major tab separators
#define LIGHT_PATH                 "light"    // Icon directory under images/
#define LIGHT_SCROLL_BKG_COLOR     "f6f6f6"   // Background of scroll bar
#define LIGHT_SCROLL_HANDLE_COLOR  "d9d9d9"   // Handle of scroller
#define LIGHT_TAB_BKG_COLOR        "#ffffff"  // Background of major tabs
#define LIGHT_TAB_SELECTED_COLOR   "#fafafa"  // Background of the selected major tab
#define LIGHT_TABLE_BOTTOM_COLOR   "#e0e0e0"
#define LIGHT_TEXT_COLOR           "#212121"  // Main page text
#define LIGHT_TITLE_COLOR          "#757575"  // Main page titles
#define LIGHT_TOOL_COLOR           "#00bea4"  // Checkboxes, sliders, etc.


#define DARK_BOX_COLOR             "#414a50"
#define DARK_BKG_COLOR             "#273238"
#define DARK_DISABLED_TOOL_COLOR   "#1b5c58"
#define DARK_DIVIDER_COLOR         "#e0e0e0"
#define DARK_EDIT_COLOR            "#808080"
#define DARK_LARGE_DIVIDER_COLOR   "#1f282d"
#define DARK_MAJOR_TAB_COLOR       "#bdc0c3"
#define DARK_MAJOR_TITLE_COLOR     "#e5e6e7"
#define DARK_PATH                  "dark"
#define DARK_SCROLL_BKG_COLOR      "#333b43"
#define DARK_SCROLL_HANDLE_COLOR   "#1d272c"
#define DARK_TAB_BKG_COLOR         "#394249"
#define DARK_TAB_SELECTED_COLOR    "#313c42"
#define DARK_TABLE_BOTTOM_COLOR    "#1d272c"
#define DARK_TEXT_COLOR            "#eeeeee"
#define DARK_TITLE_COLOR           "#bec1c3"
#define DARK_TOOL_COLOR            "#00bea4"

////////////////////////////////////////////////////////////
//
// This is the style, based on either the LIGHT or DARK theme
// and the display pixel density

#define QT_STYLE(THEME) \
    "*[ColorGroup=\"MajorTab\"]      { color:" THEME##_MAJOR_TAB_COLOR ";" \
                                      "padding: 0px 0px 0px 16px }" \
    "*[ColorGroup=\"MajorTabTitle\"] { color:" THEME##_MAJOR_TITLE_COLOR ";" \
            "background-color:" THEME##_TAB_BKG_COLOR "}" \
    "*[ColorGroup=\"Title\"]         { color:" THEME##_TITLE_COLOR "}" \
    "*::disabled[ColorGroup=\"Tool\"]{ color:" THEME##_DISABLED_TOOL_COLOR "}" \
    "*[ColorGroup=\"Tool\"]          { color:" THEME##_TOOL_COLOR "}" \
    "*[class=\"EditableValue\"]  { color:" THEME##_TEXT_COLOR "}" \
    "*[ColorGroup=\"SliderLabel\"]  { color:" THEME##_DIVIDER_COLOR " }" \
    "*[ColorGroup=\"SmsBox\"] { border: 1px solid " THEME##_BOX_COLOR "}" \
    "*[ColorGroup=\"GradientDivider\"] { background:" \
            "qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0," \
                            "stop: 0 " THEME##_LARGE_DIVIDER_COLOR \
                            ", stop: 1 " THEME##_BKG_COLOR ");" \
            "border: 0px }" \
    "*                               { color:" THEME##_TEXT_COLOR ";" \
                                      "background-color: " THEME##_BKG_COLOR "}" \
    "QGroupBox::title { margin-top: 4px; margin-left: 0px }" \
    "QPlainTextEdit:disabled, QTextEdit:disabled, QLineEdit:disabled {" \
            "border: 0px}" \
    "QPlainTextEdit, QTextEdit, QLineEdit {" \
            "border-top: 0px; "\
            "border-bottom: 1px solid " THEME##_EDIT_COLOR "}" \
    "QCheckBox::indicator:checked {" \
            "image: url(:/" THEME##_PATH "/check_yes) }" \
    "QCheckBox::indicator:unchecked {" \
            "image: url(:/" THEME##_PATH "/check_no) }" \
    "QRadioButton::indicator:unchecked {" \
            "image: url(:/" THEME##_PATH "/radio_off) }" \
    "QRadioButton::indicator:checked {" \
            "image: url(:/" THEME##_PATH "/radio_on) }" \
    "QSlider::groove {border: 1px ;height: 2px}" \
    "QSlider::handle {background: solid " THEME##_TOOL_COLOR ";" \
        "border: 1px solid " THEME##_TOOL_COLOR ";width: 12px;" \
        "margin: -6px 0; border-radius: 7px}" \
    "QSlider::sub-page:horizontal {background:" THEME##_TOOL_COLOR \
        "; border-radius: 1px}" \
    "QSlider::add-page:horizontal {background:" THEME##_BOX_COLOR \
        "; border-radius: 1px}" \
    "QTableWidget { gridline-color:" THEME##_TAB_BKG_COLOR ";" \
        "background:" THEME##_TAB_BKG_COLOR "; border: 0px }" \
    "QTableView::item { padding-left: 16; padding-right: 16; color: " THEME##_TEXT_COLOR"; " \
        "border: 0; border-bottom: 1 solid " THEME##_TABLE_BOTTOM_COLOR "}" \
    "QTableView::item:focus{}" \
    "QHeaderView::section { color:" THEME##_TITLE_COLOR "; background:" \
        THEME##_TAB_BKG_COLOR "; border: 0px; padding: 16;"\
        " border-bottom: 1 solid " THEME##_DIVIDER_COLOR "}" \
    "QScrollBar::vertical { background:" THEME##_SCROLL_BKG_COLOR \
        "; width: 8px; margin 21px 0 21 px 0 }" \
    "QScrollBar::handle:vertical { background:" THEME##_SCROLL_HANDLE_COLOR \
        "; min-height: 25px }" \
    "QScrollBar::add-line:vertical { border: none; background: none }" \
    "QScrollBar::sub-line:vertical { border: none; background: none }" \
    "QComboBox { border-top: 0px; border-bottom: 1px solid " \
        THEME##_BOX_COLOR "; " \
        "selection-background-color: " THEME##_TAB_BKG_COLOR "}" \
    "QComboBox::drop-down { border-left-style: solid; }" \
    "QComboBox::down-arrow { image:url(:/" THEME##_PATH "/drop_down); " \
        "border: none; width: 24px; height: 24px; }" \
    "QTabWidget::pane {border: none;}" \
    "QTabWidget::tab-bar {left: 0px;}" \
    "QTabBar::tab {background:" THEME##_BKG_COLOR "; padding-bottom:8px;" \
    "width: 150px; text-align: left}" \
    "QTabBar::tab:selected {color:" THEME##_TEXT_COLOR ";" \
        "border-bottom: 2px solid " THEME##_TOOL_COLOR "}" \
    "QTabBar::tab:!selected{color:" THEME##_TITLE_COLOR ";" \
        "border-bottom: 2px solid " THEME##_BKG_COLOR "}" \
    "QTabBar::tab:disabled {color: transparent;" \
        "border-bottom: 2px solid transparent}"

////////////////////////////////////////////////////////////
//
// Font sizes for use with hi-density displays (like Mac's Retina)
#define HI_FONT_MEDIUM  "12px"
#define HI_FONT_LARGE   "14px"

// Font sizes for use with regular-density displays
#define LO_FONT_MEDIUM  "10px"
#define LO_FONT_LARGE   "12px"

#define QT_FONTS(DENSITY) \
    "QGroupBox { font-weight: regular; font-size: " DENSITY##_FONT_MEDIUM "}" \
    "QComboBox, QLineEdit, QPlainTextEdit, QPushButton, QRadioButton " \
            "{ font-weight: regular; font-size: " DENSITY##_FONT_LARGE "}"
