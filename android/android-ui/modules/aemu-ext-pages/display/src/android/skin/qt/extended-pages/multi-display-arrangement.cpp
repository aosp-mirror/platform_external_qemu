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

#include "multi-display-arrangement.h"

#include <qnamespace.h>                             // for operator|, AlignC...
#include <qpainter.h>                               // for QPainter (ptr only)
#include <QBrush>                                   // for QBrush
#include <QFont>                                    // for QFont
#include <QFontMetrics>                             // for QFontMetrics
#include <QIncompatibleFlag>                        // for QIncompatibleFlag
#include <QPainter>                                 // for QPainter
#include <QPainterPath>                             // for QPainterPath
#include <QPen>                                     // for QPen
#include <QRectF>                                   // for QRectF
#include <algorithm>                                // for max
#include <cstdint>                                  // for uint32_t

#include "android/settings-agent.h"                 // for SETTINGS_THEME_LIGHT
#include "android/skin/qt/extended-pages/common.h"  // for getSelectedTheme

class QPaintEvent;
class QWidget;

static const int kBorderX = 10;
static const int kBorderY = 50;
static const int kTextHeight = 80;

const MultiDisplayArrangement::ThemeColor MultiDisplayArrangement::kLightThemeColor = {
    QColor(255, 255, 255),
    QColor(240, 240, 240),
    QColor(193, 193, 193),
    QColor(0, 190, 164),
    QColor(107, 108, 110),
    QColor(32, 33, 36),
    QColor(83, 83, 86),
};
const MultiDisplayArrangement::ThemeColor MultiDisplayArrangement::kDarkThemeColor = {
    QColor(32, 39, 43),
    QColor(41, 50, 55),
    QColor(123, 126, 131),
    QColor(0, 190, 164),
    QColor(182, 185, 187),
    QColor(245, 245, 245),
    QColor(168, 172, 174),
};

MultiDisplayArrangement::MultiDisplayArrangement(QWidget* parent)
    : QWidget(parent) {
}

void MultiDisplayArrangement::paintDisplays(QPainter& p,
                                            bool highLightBorder){
    int w, h;
    const ThemeColor* t;
    if (getSelectedTheme() == SETTINGS_THEME_LIGHT) {
        t = &kLightThemeColor;
    } else {
        t = &kDarkThemeColor;
    }
    getCombinedDisplaySize(&w, &h);
    float wRatio = ((float) width() - 2 * kBorderX) / (float) w;
    float hRatio = ((float) height() - 2 * kBorderY - kTextHeight) / (float) h;
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
        QRectF rect = QRectF((float)iter.second.x * ratio + kBorderX,
                             (float)(h - iter.second.y - iter.second.h) * ratio + kBorderY,
                             (float)iter.second.w * ratio,
                             (float)iter.second.h * ratio);
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
        font.setPointSize(fontSize* 1.5);
        p.setFont(font);
        p.setPen(pen);
        QFontMetrics metrix(font);
        QString clippedTitle =
                metrix.elidedText(tr(iter.second.name.c_str()), Qt::ElideRight, rect.width());
        p.drawText(rect, Qt::AlignCenter|Qt::TextWordWrap, clippedTitle);
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

void MultiDisplayArrangement::paintEvent(QPaintEvent* e) {
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
    p.drawText(QRectF(0, height() - kTextHeight, width(), kTextHeight), Qt::AlignCenter,
               tr("This is just a simulation. You need to apply\n"
                  "changes to see new displays on the Emulator\n"
                  "window."));
}

void MultiDisplayArrangement::setHighLightDisplay(int id) {
    mHighLightDisplayId = id;
    repaint();
}

void MultiDisplayArrangement::setLayout(
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& size,
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& offset,
    std::unordered_map<uint32_t, std::string>& name) {
    mLayout.clear();
    for (const auto& iter : size) {
        mLayout[iter.first] = {offset[iter.first].first,  //pos_x
                               offset[iter.first].second, //pos_y
                               size[iter.first].first,    //width
                               size[iter.first].second,
                               name[iter.first]};  //height
    }
    repaint();
}

void MultiDisplayArrangement::getCombinedDisplaySize(int* w, int* h) {
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

