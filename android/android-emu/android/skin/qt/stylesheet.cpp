//Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/stylesheet.h"

#include <ctype.h>                             // for isalpha
#include <qfont.h>                             // for QFont::PercentageSpacing
#include <qhash.h>                             // for QHash (ptr only), QHas...
#include <qiodevice.h>                         // for QIODevice (ptr only)
#include <qloggingcategory.h>                  // for qCWarning
#include <qstring.h>                           // for QString (ptr only)
#include <QApplication>                        // for QApplication
#include <QFile>                               // for QFile
#include <QFont>                               // for QFont
#include <QHash>                               // for QHash
#include <QIODevice>                           // for QIODevice
#include <QString>                             // for QString
#include <QTextStream>                         // for QTextStream
#include <string>                              // for basic_string, string
#include <vector>                              // for vector

#include "aemu/base/memory/LazyInstance.h"  // for LazyInstance
#include "android/cmdline-option.h"            // for android_cmdLineOptions
#include "android/skin/qt/logging-category.h"  // for emu

class QTextStream;

namespace Ui {

Q_LOGGING_CATEGORY(emu, "android.emu")

const char TABLE_BOTTOM_COLOR_VAR[] = "TABLE_BOTTOM_COLOR";
const char THEME_PATH_VAR[] = "PATH";
const char THEME_TEXT_COLOR[] = "TEXT_COLOR";
const char MAJOR_TAB_COLOR_VAR[] = "MAJOR_TAB_COLOR";
const char MAJOR_TAB_SELECTED_COLOR_VAR[] = "MAJOR_TAB_SELECTED_COLOR";
const char TAB_BKG_COLOR_VAR[] = "TAB_BKG_COLOR";
const char TAB_SELECTED_COLOR_VAR[] = "TAB_SELECTED_COLOR";
const char TABLE_SELECTED_VAR[] = "TABLE_SELECTED";
const char MACRO_BKG_COLOR_VAR[] = "MACRO_BKG_COLOR";
const char HORIZ_DIVIDER_COLOR_VAR[] = "HORIZ_DIVIDER_COLOR";
// As of now low density font stylesheet is exactly the same.
// static QString loDensityFontStylesheet;

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
//   "Overlay"         The overlay widget on the recording screen
//   <Normal>:         Text in the main part of the window
//
// These are identified by the value of their "ColorGroup" or "class"
// property.

// MacOS has some very different font sizes in points, so our default 8/10pt
// look really tiny. Let's load the system sizes first before falling back
// to defaults.
static const char kFontMediumName[] = "FONT_MEDIUM";
static const char kFontLargeName[] = "FONT_LARGE";
static const char kFontLargerName[] = "FONT_LARGER";
static const char kFontHugeName[] = "FONT_HUGE";

struct FontSizeMapLoader {
    FontSizeMapLoader() {
// Windows takes care of the font sizes, and if we try loading them from the
// system here the result is actually worse compared to hardcoded 8/10pt.
#ifndef _WIN32
        QFont font; // Default ctor populates all values from the system.
        // Set letter spacing
        font.setLetterSpacing(QFont::PercentageSpacing, 105);
        QApplication::setFont(font);
        if (font.pointSizeF() > 0) {
            const auto largeSize = font.pointSizeF();
            const auto medSize = largeSize * 85.0 / 100;
            fontMap = {
                {kFontMediumName, QString("%1pt").arg(medSize) },
                {kFontLargeName, QString("%1pt").arg(largeSize) }
            };
        } else if (font.pixelSize() > 0) {
            const auto largeSize = font.pixelSize();
            const auto medSize = largeSize * 85.0 / 100;
            fontMap = {
                {kFontMediumName, QString("%1px").arg(medSize) },
                {kFontLargeName, QString("%1px").arg(largeSize) }
            };
        } else
#endif // !_WIN32
        {
            fontMap = {
                {kFontMediumName, "8pt"},
                {kFontLargeName, "10pt"},
            };
        }

        fontMap[kFontLargerName] = "11pt";
        fontMap[kFontHugeName] = "15pt";
    }

    QHash<QString, QString> fontMap;
};

static android::base::LazyInstance<FontSizeMapLoader> sFontSizeMapLoader = {};

// Encapsulates parsing a stylesheet template and generating a stylesheet
// from template.
// Stylesheet templates are arbitrary strings which may contain references
// to variables. A reference to a variable looks like this: %%variable_name%%
// The variable name is allowed to contain alphabetic characters.
class StylesheetTemplate {
    enum class TemplateBlockType { RawText, UnboundVariable };

    struct TemplateBlock {
        TemplateBlockType type;
        QString text;
    };

    // Helper function. Looks at the next character in the stream and discards
    // it if it is equal to a given character. Returns true if the next
    // character in the stream has been discarded, false if not.
    static bool peekAndDiscard(char discard_char, QIODevice* stream) {
        char c;
        if (stream->peek(&c, 1) > 0) {
            return c == discard_char ? stream->getChar(nullptr) : false;
        } else {
            return false;
        }
    }

public:
    // Loads the template from a given location and parses it.
    explicit StylesheetTemplate(const QString& location) : mOk(true) {
        QFile source_file(location);
        if (!source_file.open(QIODevice::ReadOnly)) {
            qCWarning(emu, "StylesheetTemplate: could not open input file %s",
                     location.toStdString().c_str());
            mOk = false;
            return;
        }

        TemplateBlockType current_block_type = TemplateBlockType::RawText;
        char current_char;
        QString buf;
        while (mOk && source_file.getChar(&current_char)) {
            switch(current_block_type) {
            case TemplateBlockType::RawText:
                if (current_char == '%') {
                    if (peekAndDiscard('%', &source_file)) {
                        addBlock(current_block_type, buf);
                        current_block_type = TemplateBlockType::UnboundVariable;
                        buf.clear();
                    }
                } else {
                    buf.append(current_char);
                }
                break;

            case TemplateBlockType::UnboundVariable:
                if (isalpha(current_char) || current_char == '_') {
                    buf.append(current_char);
                } else {
                    if (peekAndDiscard('%', &source_file)) {
                        addBlock(current_block_type, buf);
                        current_block_type = TemplateBlockType::RawText;
                        buf.clear();
                    } else {
                        qCWarning(emu, "StylesheetTemplate: Bad variable name %s",
                                 buf.toStdString().c_str());
                        buf.clear();
                        current_block_type = TemplateBlockType::RawText;
                        mOk = false;
                    }
                }
                break;
            }
        }

        // Handle the last remaining block.
        switch (current_block_type) {
        case TemplateBlockType::RawText:
            // flush.
            addBlock(current_block_type, buf);
            break;
        case TemplateBlockType::UnboundVariable:
            // This shouldn't happen for valid inputs.
            qCWarning(emu, "StylesheetTemplate: Unterminated variable name %s",
                     buf.toStdString().c_str());
            mOk = false;
            break;
        }
    }

    // Outputs a stylesheet to the given |stream| with all the references to
    // variables substituted by values looked up from |arguments|.
    // Returns true if the operation is successful, false otherwise.
    bool render(const QHash<QString, QString>& arguments, QTextStream* stream) const {
        for (const auto& block : mBlocks) {
            switch(block.type) {
            case TemplateBlockType::RawText:
                (*stream) << block.text;
                break;
            case TemplateBlockType::UnboundVariable:
                auto value_iterator = arguments.find(block.text);
                if (value_iterator != arguments.end()) {
                    (*stream) << value_iterator.value();
                } else {
                    qCWarning(emu, "StylesheetTemplate: variable %s unbound",
                             block.text.toStdString().c_str());
                    return false;
                }
                break;
            }
        }
        return true;
    }

    // Returns true if the template has been parsed successfully.
    bool isOk() const {
        return mOk;
    }

private:
    // Helper function - appends a new element to the list of blocks.
    void addBlock(TemplateBlockType type, const QString& str) {
        mBlocks.emplace_back();
        mBlocks.back().type = type;
        mBlocks.back().text = str;
    }

    std::vector<TemplateBlock> mBlocks;
    bool mOk;
};

// There are four theme supported in total: dark, light, android studio dark
// and android studio light. The latter two themes are only used when emulator
// is running in embedded mode to match with Android studio.

struct StylesheetValues {
    QString darkStylesheet;
    QString lightStylesheet;
    QString studioDarkStylesheet;
    QString studioLightStylesheet;
    QString studioContrastStylesheet;
    QString hiDensityFontStylesheet;

    // These are the colors used in the themes
    QHash<QString, QString> lightValues = {
        {"BOX_COLOR",                       "#e0e0e0"},  // Boundary around SMS text area
        {"BKG_COLOR",                       "#f0f0f0"},  // Main page background
        {"DISABLED_BKG_COLOR","rgba(240,240,240,60%)"},  // Main page background (disabled)
        {"BKG_COLOR_OVERLAY", "rgba(236,236,236,175)"},  // Overlay background
        {"BUTTON_BKG_COLOR",                "#F9F9F9"},  // Background of push buttons
        {"BUTTON_BKG_PRESSED_COLOR",        "#e6e6e6"},  // Background of push buttons
        {"BUTTON_COLOR",                    "#757575"},  // Text in push buttons
        {"BUTTON_BORDER",                   "0px solid #FFFFFF"}, //Raise button border
        {"DISABLED_BUTTON_BORDER",        "0px solid #FFFFFFs"}, //Raise button border pressed
        {"DISABLED_BUTTON_COLOR",           "#bbbbbb"},  // Text in disabled push buttons
        {"DISABLED_PULLDOWN_COLOR",         "#c0c0c0"},  // Text in disabled combo box
        {"DISABLED_TOOL_COLOR",             "#baeae4"},  // Grayed-out tool text
        {"DIVIDER_COLOR",                   "#e0e0e0"},  // Line between items
        {"EDIT_COLOR",                      "#e0e0e0"},  // Line under editable fields
        {"INSTRUCTION_COLOR",               "#91a4ad"},  // Large instruction string
        {"LARGE_DIVIDER_COLOR",    "rgba(0,0,0,2.1%)"},  // Start of large divider's gradient
        {MAJOR_TAB_COLOR_VAR,               "#91a4ad"},  // Text of major tabs
        {MAJOR_TAB_SELECTED_COLOR_VAR,      "#91a4ad"},  // Text of selected major tabs 
       {"MAJOR_TITLE_COLOR",               "#617d8a"},  // Text of major tab separators
        {"SCROLL_BKG_COLOR",                "#f6f6f6"},  // Background of scroll bar
        {"SCROLL_HANDLE_COLOR",             "#d9d9d9"},  // Handle of scroller
        {"SNAPSHOT_INFO_BKG",               "#f9f9f9"},  // Background of snapshot description text
        {TAB_BKG_COLOR_VAR,                 "#FFFFFF"},  // Background of major tabs
        {TAB_SELECTED_COLOR_VAR,            "#f5f5f5"},  // Background of the selected major tab
        {HORIZ_DIVIDER_COLOR_VAR,           "#f5f5f5"},  // Horizontal divider in the left Navigation.
        {"TAB_DARKENED_COLOR",              "#e6e6e6"},
        {"TABLE_BOTTOM_COLOR",              "#e0e0e0"},
        {"TABLE_SELECTED",                  "#72a4fb"},  // Background of selected table row
        {"TEXT_COLOR",                      "#212121"},  // Main page text
        {"INACTIVE_TEXT_COLOR",  "rgba(33,33,33,50%)"},
        {"TITLE_COLOR",                     "#757575"},  // Main page titles
        {"DISABLED_TITLE_COLOR", "rgba(117,117,117,60%)"},  // Main page titles (disabled color)
        {"TOOL_COLOR",                      "#00bea4"},  // Checkboxes, sliders, etc.
        {"TOOL_ON_COLOR",                   "#d6d6d6"},  // Main toolbar button ON
        {"TREE_WIDGET_BKG",                 "#ffffff"},  // List of Snapshots
        {"TREE_WIDGET_BORDER",              "#e3e3e3"},
        {"LIST_WIDGET_BORDER",      "rgba(0,0,0,11%)"},
        {MACRO_BKG_COLOR_VAR,       "rgba(234,67,53,88%)"},
        {"LINK_COLOR",                      "#0288D1"},  // Highlighted link
        {"PREVIEW_IMAGE_BKG",               "#e3e3e3"},  // Behind snapshot preview image
        {"RAISED_COLORED_BKG_COLOR",        "#459388"},  // Colored raised button background color.
        {"RAISED_COLORED_PRESSED_COLOR",    "#5faca0"},  // Colored raised button pressed color.
        {"RAISED_COLORED_COLOR",            "#ffffff"},  // Colored raised button text color.
        {"DROP_TARGET_BKG_COLOR", "rgba(0,190,64,10%)"},  // Drop target background color.
        {THEME_PATH_VAR,                      "light"},  // Icon directory under images/
        {"VHAL_PROPERTY_BKG",               "#f2f2f2"},  // Vhal property background color.
        {"TEXT_INPUT_BKG",                  "#FFFFFF"},  // Background for text input
    };

    QHash<QString, QString> darkValues = {
        {"BOX_COLOR",                    "#414a50"},
        {"BKG_COLOR",                    "#273238"},
        {"DISABLED_BKG_COLOR","rgba(39,50,56,60%)"},
        {"BKG_COLOR_OVERLAY", "rgba(35,46,52,175)"},
        {"BUTTON_BKG_COLOR",             "#37474f"},
        {"BUTTON_BKG_PRESSED_COLOR",     "#20292e"},  // Background of push buttons
        {"BUTTON_COLOR",                 "#bec1c3"},
        {"BUTTON_BORDER",                "0px solid #FFFFFF"}, //Raise button border
        {"DISABLED_BUTTON_BORDER",        "0px solid #FFFFFF"}, //Raise button border pressed
        {"DISABLED_BUTTON_COLOR",        "#5f6162"},
        {"DISABLED_PULLDOWN_COLOR",      "#808080"},
        {"DISABLED_TOOL_COLOR",          "#1b5c58"},
        {"DIVIDER_COLOR",                "#e0e0e0"},
        {"EDIT_COLOR",                   "#808080"},
        {"INSTRUCTION_COLOR",            "#ffffff"},
        {"LARGE_DIVIDER_COLOR",  "rgba(0,0,0,20%)"},
        {MAJOR_TAB_COLOR_VAR,            "#bdc0c3"},
        {MAJOR_TAB_SELECTED_COLOR_VAR,      "#bdc0c3"}, 
       {"MAJOR_TITLE_COLOR",            "#e5e6e7"},
        {"SCROLL_BKG_COLOR",             "#333b43"},
        {"SCROLL_HANDLE_COLOR",          "#1d272c"},
        {"SNAPSHOT_INFO_BKG",            "#273238"},
        {TAB_BKG_COLOR_VAR,              "#394249"},
        {TAB_SELECTED_COLOR_VAR,         "#313c42"},
        {HORIZ_DIVIDER_COLOR_VAR,        "#313c42"},
        {"TAB_DARKENED_COLOR",           "#20292e"},
        {"TABLE_BOTTOM_COLOR",           "#1d272c"},
        {"TABLE_SELECTED",               "#4286f5"},
        {"TEXT_COLOR",                   "#eeeeee"},
        {"INACTIVE_TEXT_COLOR", "rgba(238,238,238,50%)"},
        {"TITLE_COLOR",                  "#bec1c3"},
        {"DISABLED_TITLE_COLOR", "rgba(206,209,211,60%)"},
        {"TOOL_COLOR",                   "#00bea4"},
        {"TOOL_ON_COLOR",                "#586670"},
        {"TREE_WIDGET_BKG",              "#394249"},
        {"TREE_WIDGET_BORDER",           "#494949"},
        {"LIST_WIDGET_BORDER", "rgba(255,255,255,22%)"},
        {MACRO_BKG_COLOR_VAR, "rgba(234,67,53,55%)"},
        {"LINK_COLOR",                   "#29B6F6"},
        {"PREVIEW_IMAGE_BKG",            "#343d43"},
        {"RAISED_COLORED_BKG_COLOR",     "#459388"},
        {"RAISED_COLORED_PRESSED_COLOR", "#5faca0"},
        {"RAISED_COLORED_COLOR",         "#ffffff"},
        {"DROP_TARGET_BKG_COLOR", "rgba(0,190,64,7%)"},
        {THEME_PATH_VAR,                    "dark"},
        {"VHAL_PROPERTY_BKG",            "#394249"},
        {"TEXT_INPUT_BKG",               "#394249"},  // Background for text input
 };

    QHash<QString, QString> studioLightValues = {
        {"BOX_COLOR",                       "#d9d9d9"},  // Boundary around SMS text area
        {"BKG_COLOR",                       "#F2F2F2"},  // Main page background
        {"DISABLED_BKG_COLOR","rgba(240,240,240,60%)"},  // Main page background (disabled)
        {"BKG_COLOR_OVERLAY", "rgba(236,236,236,175)"},  // Overlay background
        {"BUTTON_BKG_COLOR",                "#FFFFFF"},  // Background of push buttons
        {"BUTTON_BKG_PRESSED_COLOR",        "#CFCFCF"},  // Background of push buttons
        {"BUTTON_COLOR",                    "#1D1D1D"},  // Text in push buttons
        {"BUTTON_BORDER",                   "1px solid #C4C4C4"}, //Raise button border
        {"DISABLED_BUTTON_BORDER",        "1px solid #C4C4C4"}, //Raise button border pressed
       {"DISABLED_BUTTON_COLOR",           "#8C8C8C"},  // Text in disabled push buttons
        {"DISABLED_BUTTON_BKG_COLOR",       "#F2F2F2"},
        {"DISABLED_PULLDOWN_COLOR",         "#8C8C8C"},  // Text in disabled combo box
        {"DISABLED_TOOL_COLOR",             "#8C8C8C"},  // Grayed-out tool text
        {"DIVIDER_COLOR",                   "#D1D1D1"},  // Line between items
        {"EDIT_COLOR",                      "#C4C4C4"},  // Line under editable fields
        {"INSTRUCTION_COLOR",               "#808080"},  // Large instruction string
        {"LARGE_DIVIDER_COLOR",             "#D1D1D1"},  // Start of large divider's gradient
        {MAJOR_TAB_COLOR_VAR,               "#1D1D1D"},  // Text of major tabs
        {MAJOR_TAB_SELECTED_COLOR_VAR,      "#FFFFFF"},  // Text of selected major tabs 
        {"MAJOR_TITLE_COLOR",               "#FFFFFF"},  // Text of major tab separators
        {"SCROLL_BKG_COLOR",                "rgba(0,0,0,0%)"},  // Background of scroll bar
        {"SCROLL_HANDLE_COLOR",             "#999999"},  // Handle of scroller
        {"SNAPSHOT_INFO_BKG",               "#FFFFFF"},  // Background of snapshot description text
        {TAB_BKG_COLOR_VAR,                 "#E6EBF0"},  // Background of major tabs
        {TAB_SELECTED_COLOR_VAR,            "#2675BF"},  // Background of the selected major tab
        {HORIZ_DIVIDER_COLOR_VAR,           "#D1D1D1"},  // Horizontal divider in the left Navigation.
        {"TAB_DARKENED_COLOR",              "#1E67CE"},
        {"TABLE_BOTTOM_COLOR",              "#D1D1D1"},
        {"TABLE_SELECTED",                  "#87AFDA"},  // Background of selected table row
        {"TEXT_COLOR",                      "#1D1D1D"},  // Main page text
        {"INACTIVE_TEXT_COLOR",             "#8C8C8C"},
        {"TITLE_COLOR",                     "#1D1D1D"},  // Main page titles
        {"DISABLED_TITLE_COLOR",            "#8C8C8C"},  // Main page titles (disabled color)
        {"TOOL_COLOR",                      "#4083C9"},  // Checkboxes, sliders, etc.
        {"TOOL_ON_COLOR",                   "#CFCfCF"},  // Main toolbar button ON
        {"TREE_WIDGET_BKG",                 "#ffffff"},  // List of Snapshots
        {"TREE_WIDGET_BORDER",              "#D1D1D1"},
        {"LIST_WIDGET_BORDER",              "#D1D1D1"},
        {MACRO_BKG_COLOR_VAR,       "rgba(224,85,85, 70%)"},
        {"LINK_COLOR",                      "#2470B3"},  // Highlighted link
        {"PREVIEW_IMAGE_BKG",               "#F2F2F2"},  // Behind snapshot preview image
        {"RAISED_COLORED_BKG_COLOR",        "#87AFDA"},  // Colored raised button background color.
        {"RAISED_COLORED_PRESSED_COLOR",    "#87AFDA"},  // Colored raised button pressed color.
        {"RAISED_COLORED_COLOR",            "#ffffff"},  // Colored raised button text color.
        {"DROP_TARGET_BKG_COLOR", "rgba(98,181,67,60%)"},  // Drop target background color.
        {THEME_PATH_VAR,                    "studio-light"},  // Icon directory under images/
        {"VHAL_PROPERTY_BKG",               "#f2f2f2"},  // Vhal property background color.
        {"TEXT_INPUT_BKG",                  "#FFFFFF"},  // Background for text input
    };

    QHash<QString, QString> studioDarkValues = {
        {"BOX_COLOR",                    "#4A4A4A"},
        {"BKG_COLOR",                    "#3C3F41"},
        {"DISABLED_BKG_COLOR","rgba(39,50,56,60%)"},
        {"BKG_COLOR_OVERLAY", "rgba(35,46,52,175)"},
        {"BUTTON_BKG_COLOR",             "#4C5052"},
        {"BUTTON_BKG_PRESSED_COLOR",     "#5C6164"},  // Background of push buttons
        {"BUTTON_COLOR",                 "#BBBBBB"},
        {"BUTTON_BORDER",                "1px solid #5E6060"}, //Raise button border
        {"DISABLED_BUTTON_BORDER",        "1px solid #5E6060"}, //Raise button border pressed
        {"DISABLED_BUTTON_COLOR",        "#777777"},
        {"DISABLED_BUTTON_BKG_COLOR",    "#3C3F41"},
        {"DISABLED_PULLDOWN_COLOR",      "#777777"},
        {"DISABLED_TOOL_COLOR",          "#777777"},
        {"DIVIDER_COLOR",                "#4A4A4A"},
        {"EDIT_COLOR",                   "#4A4A4A"},
        {"INSTRUCTION_COLOR",            "#878787"},
        {"LARGE_DIVIDER_COLOR",  "rgba(0,0,0,20%)"},
        {MAJOR_TAB_COLOR_VAR,            "#BBBBBB"},
        {MAJOR_TAB_SELECTED_COLOR_VAR,   "#FFFFFF"},
       {"MAJOR_TITLE_COLOR",            "#FFFFFF"},
        {"SCROLL_BKG_COLOR",             "#2B2B2B"},
        {"SCROLL_HANDLE_COLOR",          "rgba(133,133,133,50%)"},
        {"SNAPSHOT_INFO_BKG",            "#273238"},
        {TAB_BKG_COLOR_VAR,              "#3E434C"},
        {TAB_SELECTED_COLOR_VAR,         "#1E67CE"},
        {HORIZ_DIVIDER_COLOR_VAR,        "#323232"},
        {"TAB_DARKENED_COLOR",           "#3C3F41"},
        {"TABLE_BOTTOM_COLOR",           "#BBBBBB"},
        {"TABLE_SELECTED",               "#4A4A4A"},
        {"TEXT_COLOR",                   "#BBBBBB"},
        {"INACTIVE_TEXT_COLOR",          "#777777"},
        {"TITLE_COLOR",                  "#BBBBBB"},
        {"DISABLED_TITLE_COLOR",         "#777777"},
        {"TOOL_COLOR",                   "#4A88C7"},
        {"TOOL_ON_COLOR",                "#5C6164"},
        {"TREE_WIDGET_BKG",              "#45494A"},
        {"TREE_WIDGET_BORDER",           "#4A4A4A"},
        {"LIST_WIDGET_BORDER",           "4A4A4A"},
        {MACRO_BKG_COLOR_VAR, "rgba(234,67,53,55%)"},
        {"LINK_COLOR",                   "#589DF6"},
        {"PREVIEW_IMAGE_BKG",            "#3C3F41"},
        {"RAISED_COLORED_BKG_COLOR",     "#87AFDA"},
        {"RAISED_COLORED_PRESSED_COLOR", "#87AFDA"},
        {"RAISED_COLORED_COLOR",         "#ffffff"},
        {"DROP_TARGET_BKG_COLOR", "rgba(98,181,67,60%)"},
        {THEME_PATH_VAR,                 "studio-dark"},
        {"VHAL_PROPERTY_BKG",            "#3C3F41"},
        {"TEXT_INPUT_BKG",               "#4C5052"},  // Background for text input
    };

   QHash<QString, QString> studioContrastValues = {
        {"BOX_COLOR",                    "#E6E6E6"},
        {"BKG_COLOR",                    "#000000"},
        {"DISABLED_BKG_COLOR","rgba(224,134,31,60%)"},
        {"BKG_COLOR_OVERLAY", "rgba(35,46,52,175)"},
        {"BUTTON_BKG_COLOR",             "#000000"},
        {"BUTTON_BKG_PRESSED_COLOR",     "#1aebff"},  // Background of push buttons
        {"BUTTON_COLOR",                 "#FFFFFF"},
        {"BUTTON_BORDER",                "1px solid #FFFFFF"}, //Raise button border
        {"DISABLED_BUTTON_BORDER",        "1px solid #E0861F"}, //Raise button border pressed
        {"DISABLED_BUTTON_COLOR",        "#E0861F"},
        {"DISABLED_BUTTON_BKG_COLOR",    "#33210C"},
        {"DISABLED_PULLDOWN_COLOR",      "#E0861F"},
        {"DISABLED_TOOL_COLOR",          "#E0861F"},
        {"DIVIDER_COLOR",                "#E6E6E6"},
        {"EDIT_COLOR",                   "#E6E6E6"},
        {"INSTRUCTION_COLOR",            "#E0861F"},
        {"LARGE_DIVIDER_COLOR",  "E6E6E6"},
        {MAJOR_TAB_COLOR_VAR,            "#FFFFFF"},
        {MAJOR_TAB_SELECTED_COLOR_VAR,   "#FFFFFF"},
       {"MAJOR_TITLE_COLOR",            "#FFFFFF"},
        {"SCROLL_BKG_COLOR",             "#000000"},
        {"SCROLL_HANDLE_COLOR",          "#B3B3B3"},
        {"SNAPSHOT_INFO_BKG",            "#000000"},
        {TAB_BKG_COLOR_VAR,              "#000000"},
        {TAB_SELECTED_COLOR_VAR,         "#3333FF"},
        {HORIZ_DIVIDER_COLOR_VAR,        "#323232"},
        {"TAB_DARKENED_COLOR",           "#3333FF"},
        {"TABLE_BOTTOM_COLOR",           "#FFFFFF"},
        {"TABLE_SELECTED",               "#3333FF"},
        {"TEXT_COLOR",                   "#FFFFFF"},
        {"INACTIVE_TEXT_COLOR",          "#E0861F"},
        {"TITLE_COLOR",                  "#FFFFFF"},
        {"DISABLED_TITLE_COLOR",         "#E0861F"},
        {"TOOL_COLOR",                   "#1AEBFF"},
        {"TOOL_ON_COLOR",                "#000000"},
        {"TREE_WIDGET_BKG",              "#000000"},
        {"TREE_WIDGET_BORDER",           "#B3B3B3"},
        {"LIST_WIDGET_BORDER",           "B3B3B3"},
        {MACRO_BKG_COLOR_VAR, "000000"},
        {"LINK_COLOR",                   "#D2F53C"},
        {"PREVIEW_IMAGE_BKG",            "#000000"},
        {"RAISED_COLORED_BKG_COLOR",     "#87AFDA"},
        {"RAISED_COLORED_PRESSED_COLOR", "#87AFDA"},
        {"RAISED_COLORED_COLOR",         "#ffffff"},
        {"DROP_TARGET_BKG_COLOR", "#B3B3B3"},
        {THEME_PATH_VAR,                 "studio-contrast"},
        {"VHAL_PROPERTY_BKG",            "#000000"},
        {"TEXT_INPUT_BKG",               "#000000"},  // Background for text input
    };
    
    StylesheetValues() {
        if (!initializeStylesheets()) {
            qCWarning(emu, "Failed to initialize UI stylesheets!");
        }
    }

private:
    bool initializeStylesheets() {
        StylesheetTemplate tpl(":/styles/stylesheet_template.css");
        if (!tpl.isOk()) {
            qCWarning(emu, "Failed to load stylesheet template!");
            return false;
        }

        QTextStream dark_stylesheet_stream(&darkStylesheet);
        if (!tpl.render(darkValues, &dark_stylesheet_stream)) {
            return false;
        }

        QTextStream light_stylesheet_stream(&lightStylesheet);
        if (!tpl.render(lightValues, &light_stylesheet_stream)) {
            return false;
        }

        QTextStream studio_dark_stylesheet_stream(&studioDarkStylesheet);
        if (!tpl.render(studioDarkValues, &studio_dark_stylesheet_stream)) {
            return false;
        }

        QTextStream studio_light_stylesheet_stream(&studioLightStylesheet);
        if (!tpl.render(studioLightValues, &studio_light_stylesheet_stream)) {
            return false;
        }

        QTextStream studio_contrast_stylesheet_stream(&studioContrastStylesheet);
        if (!tpl.render(studioContrastValues, &studio_contrast_stylesheet_stream)) {
            return false;
        }

        StylesheetTemplate font_tpl(":/styles/fonts_stylesheet_template.css");
        if (!font_tpl.isOk()) {
            qCWarning(emu, "Failed to load font stylesheet template!");
            return false;
        }

        QTextStream hi_font_stylesheet_stream(&hiDensityFontStylesheet);
        if (!font_tpl.render(sFontSizeMapLoader->fontMap, &hi_font_stylesheet_stream)) {
            return false;
        }

        return true;
    }
};

static android::base::LazyInstance<StylesheetValues> sStylesheetValues = {};

const QString& stylesheetForTheme(SettingsTheme theme) {
    switch (theme) {
        case SETTINGS_THEME_STUDIO_CONTRAST:
            return sStylesheetValues->studioContrastStylesheet;
        case SETTINGS_THEME_STUDIO_DARK:
            return sStylesheetValues->studioDarkStylesheet;
        case SETTINGS_THEME_STUDIO_LIGHT:
            return sStylesheetValues->studioLightStylesheet;
        case SETTINGS_THEME_DARK:
            return sStylesheetValues->darkStylesheet;
        case SETTINGS_THEME_LIGHT:
        default:
            return sStylesheetValues->lightStylesheet;
    }
}

const QString& fontStylesheet(bool hi_density) {
    return sStylesheetValues->hiDensityFontStylesheet;
}


const QHash<QString, QString>& stylesheetValues(SettingsTheme theme) {
    switch (theme) {
        case SETTINGS_THEME_STUDIO_CONTRAST:
            return sStylesheetValues->studioContrastValues;
        case SETTINGS_THEME_STUDIO_DARK:
            return sStylesheetValues->studioDarkValues;
        case SETTINGS_THEME_STUDIO_LIGHT:
            return sStylesheetValues->studioLightValues;
        case SETTINGS_THEME_DARK:
            return sStylesheetValues->darkValues;
        case SETTINGS_THEME_LIGHT:
        default:
            return sStylesheetValues->lightValues;
    }
}

const QString& stylesheetFontSize(FontSize size) {
    auto& map = sFontSizeMapLoader->fontMap;
    switch (size) {
        default:
        case FontSize::Medium: return map[kFontMediumName];
        case FontSize::Large: return map[kFontLargeName];
        case FontSize::Larger: return map[kFontLargerName];
        case FontSize::Huge: return map[kFontHugeName];
    }
}

const char* stylesheetHugeFontSize() {
    return "15pt";
}

}  // namespace Ui
