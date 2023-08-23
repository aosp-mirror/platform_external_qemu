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
#include "multi-display-arrangement-grpc.h"

#include <qnamespace.h>
#include <qpainter.h>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QIncompatibleFlag>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <algorithm>
#include <cstdint>

#include "android/settings-agent.h"
#include "android/skin/qt/extended-pages/common.h"

class QPaintEvent;
class QWidget;

static const int kBorderX = 10;
static const int kBorderY = 50;
static const int kTextHeight = 80;

const MultiDisplayArrangementGrpc::ThemeColor
        MultiDisplayArrangementGrpc::kLightThemeColor = {
                QColor(255, 255, 255), QColor(240, 240, 240),
                QColor(193, 193, 193), QColor(0, 190, 164),
                QColor(107, 108, 110), QColor(32, 33, 36),
                QColor(83, 83, 86),
};
const MultiDisplayArrangementGrpc::ThemeColor
        MultiDisplayArrangementGrpc::kDarkThemeColor = {
                QColor(32, 39, 43),    QColor(41, 50, 55),
                QColor(123, 126, 131), QColor(0, 190, 164),
                QColor(182, 185, 187), QColor(245, 245, 245),
                QColor(168, 172, 174),
};

MultiDisplayArrangementGrpc::MultiDisplayArrangementGrpc(QWidget* parent)
    : QWidget(parent) {}

void MultiDisplayArrangementGrpc::paintDisplays(QPainter& p, bool highLightBorder) {
    int w, h;
    const ThemeColor* t;
    if (getSelectedTheme() == SETTINGS_THEME_LIGHT) {
        t = &kLightThemeColor;
    } else {
        t = &kDarkThemeColor;
    }
    getCombinedDisplaySize(&w, &h);
    float wRatio = ((float)width() - 2 * kBorderX) / (float)w;
    float hRatio = ((float)height() - 2 * kBorderY - kTextHeight) / (float)h;
    float ratio = (wRatio < hRatio) ? wRatio : hRatio;

    for (const auto& iter : mLayout) {
        if (highLightBorder) {
            if (iter.first != mHighLightDisplayId) {
                continue;
            }
        } else {
            if (iter.first == mHighLightDisplayId) {
                continue;
            }
        }

        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        QRectF rect = QRectF(
                (float)iter.second.x * ratio + kBorderX,
                (float)(h - iter.second.y - iter.second.h) * ratio + kBorderY,
                (float)iter.second.w * ratio, (float)iter.second.h * ratio);
        path.addRoundedRect(rect, 5, 5);
        QPen pen(t->MDBorder, 1);
        if (highLightBorder) {
            pen.setColor(t->MDBorderHighLight);
        }
        p.setPen(pen);
        p.fillPath(path, t->MD);
        p.drawPath(path);
        pen.setColor(t->MDName);
        QFont font = p.font();
        int fontSize = font.pointSize();
        font.setPointSize(fontSize * 1.5);
        p.setFont(font);
        p.setPen(pen);
        QFontMetrics metrix(font);
        QString clippedTitle = metrix.elidedText(tr(iter.second.name.c_str()),
                                                 Qt::ElideRight, rect.width());
        p.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, clippedTitle);
        pen.setColor(t->MDTitle);
        p.setPen(pen);
        font.setPointSize(fontSize);
        p.setFont(font);
        rect.setX(rect.x() + 5);
        rect.setY(rect.y() + 5);
        if (iter.first == 0) {
            p.drawText(rect, Qt::AlignTop, tr("Default"));
        } else {
            std::string title = "Display " + std::to_string(iter.first);
            p.drawText(rect, Qt::AlignTop, tr(title.c_str()));
        }
    }
}

void MultiDisplayArrangementGrpc::paintEvent(QPaintEvent* e) {
    const ThemeColor* t;
    if (getSelectedTheme() == SETTINGS_THEME_LIGHT) {
        t = &kLightThemeColor;
    } else {
        t = &kDarkThemeColor;
    }

    QPainter p(this);
    p.fillRect(this->rect(), t->backGround);

    paintDisplays(p, false);
    paintDisplays(p, true);

    QPen penText(t->text, 1);
    p.setPen(penText);
    p.drawText(QRectF(0, height() - kTextHeight, width(), kTextHeight),
               Qt::AlignCenter,
               tr("This is just a simulation. You need to apply\n"
                  "changes to see new displays on the Emulator\n"
                  "window."));
}

void MultiDisplayArrangementGrpc::setHighLightDisplay(int id) {
    mHighLightDisplayId = id;
    repaint();
}

void MultiDisplayArrangementGrpc::setLayout(
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& size,
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& offset,
        std::unordered_map<uint32_t, std::string>& name) {
    mLayout.clear();
    for (const auto& iter : size) {
        mLayout[iter.first] = {offset[iter.first].first,   // pos_x
                               offset[iter.first].second,  // pos_y
                               size[iter.first].first,     // width
                               size[iter.first].second,
                               name[iter.first]};  // height
    }
    repaint();
}

void MultiDisplayArrangementGrpc::getCombinedDisplaySize(int* w, int* h) {
    uint32_t total_h = 0;
    uint32_t total_w = 0;
    for (const auto& iter : mLayout) {
        total_h = std::max(total_h, iter.second.h + iter.second.y);
        total_w = std::max(total_w, iter.second.w + iter.second.x);
    }
    if (h)
        *h = (int)total_h;
    if (w)
        *w = (int)total_w;
}
