// Copyright 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <qobjectdefs.h>
#include <stdint.h>
#include <QColor>
#include <QString>
#include <QWidget>
#include <string>
#include <unordered_map>
#include <utility>

class QObject;
class QPaintEvent;
class QPainter;
class QWidget;

class MultiDisplayArrangementGrpc : public QWidget {
    Q_OBJECT

public:
    explicit MultiDisplayArrangementGrpc(QWidget* parent = 0);
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
