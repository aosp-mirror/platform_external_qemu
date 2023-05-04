// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <qobjectdefs.h>  // for Q_OBJECT
#include <stdint.h>       // for uint32_t
#include <QColor>         // for QColor
#include <QString>        // for QString
#include <QWidget>        // for QWidget
#include <string>         // for basic_string, string
#include <unordered_map>  // for unordered_map
#include <utility>        // for pair

class QObject;
class QPaintEvent;
class QPainter;
class QWidget;

class MultiDisplayArrangement : public QWidget {
    Q_OBJECT

public:
    explicit MultiDisplayArrangement(QWidget* parent = 0);
    virtual void paintEvent(QPaintEvent* e) override;
    void setLayout(std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& size,
                   std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& offset,
                   std::unordered_map<uint32_t, std::string>& name);
    void setHighLightDisplay(int id);
private:
    struct Rectangle {
        uint32_t x;
        uint32_t y;
        uint32_t w;
        uint32_t h;
        std::string name;
    };
    struct ThemeColor {
        QColor backGround;
        QColor MD;
        QColor MDBorder;
        QColor MDBorderHighLight;
        QColor MDTitle;
        QColor MDName;
        QColor text;
    };
    static const ThemeColor kLightThemeColor;
    static const ThemeColor kDarkThemeColor;
    std::unordered_map<uint32_t, struct Rectangle> mLayout;
    void getCombinedDisplaySize(int* w, int* h);
    void paintDisplays(QPainter& p, bool highLightBorder);
    int mHighLightDisplayId = -1;
};
