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

#include "android/skin/qt/extended-pages/common.h"
#include "android/base/Log.h"

#include <QGraphicsOpacityEffect>
#include <QPainter>

static const QColor kBackGroundColorDark = QColor(32, 39, 43);
static const QColor kBackGroundColorLight = QColor(255, 255, 255);
static const QColor kMultiDisplayColorDark = QColor(41, 50, 55);
static const QColor kMultiDisplayColorLight= QColor(240, 240, 240);
static const QColor kMultiDisplayBorderColorDark = QColor(123, 126, 131);
static const QColor kMultiDisplayBorderColorLight = QColor(193, 193, 193);
static const QColor kHighlightMultiDisplayBorderColor= QColor(80, 135, 236);
static const QColor kTextColorDark = QColor(168, 172, 174);
static const QColor kTextColorLight = QColor(83, 83, 86);
static const QColor kMultiDisplayNameTextColorDark = QColor(245, 245, 245);
static const QColor kMultiDisplayNameTextColorLight = QColor(32, 33, 36);
static const QColor kMultiDisplayTitleTextColorDark = QColor(182, 185, 187);
static const QColor kMultiDisplayTitleTextColorLight = QColor(107, 108, 110);
static const int kBorderX = 10;
static const int kBorderY = 50;
static const int kTextHeight = 80;

struct ThemeColor {
    QColor backGround;
    QColor MD;
    QColor MDBorder;
    QColor MDBorderHighLight;
    QColor MDTitle;
    QColor MDName;
    QColor text;
};
static const ThemeColor kLightThemeColor = {
    QColor(255, 255, 255),
    QColor(240, 240, 240),
    QColor(193, 193, 193),
    QColor(80, 135, 236),
    QColor(107, 108, 110),
    QColor(32, 33, 36),
    QColor(83, 83, 86),
};
static const ThemeColor kDarkThemeColor = {
    QColor(32, 39, 43),
    QColor(41, 50, 55),
    QColor(123, 126, 131),
    QColor(80, 135, 236),
    QColor(182, 185, 187),
    QColor(245, 245, 245),
    QColor(168, 172, 174),
};

MultiDisplayArrangement::MultiDisplayArrangement(QWidget* parent)
    : QWidget(parent) {
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
    int w, h;
    getCombinedDisplaySize(&w, &h);
    float wRatio = ((float) width() - 2 * kBorderX) / (float) w;
    float hRatio = ((float) height() - 2 * kBorderY - kTextHeight) / (float) h;
    float ratio = (wRatio < hRatio) ? wRatio : hRatio;
    for (const auto& iter : mLayout) {
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        QRectF rect = QRectF((float)iter.second.x * ratio + kBorderX,
                             (float)(h - iter.second.y - iter.second.h) * ratio + kBorderY,
                             (float)iter.second.w * ratio,
                             (float)iter.second.h * ratio);
        path.addRoundedRect(rect, 5, 5);
        QPen pen(t->MDBorder, 1);
        if (iter.first == mHighLightDisplayId) {
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
        p.drawText(rect, Qt::AlignCenter, tr(iter.second.name.c_str()));

        pen.setColor(t->MDTitle);
        p.setPen(pen);
        font.setPointSize(fontSize);
        p.setFont(font);
        if (iter.first == 0) {
            p.drawText(rect, Qt::AlignTop, tr("Default"));
        } else {
            std::string title = "Display " + std::to_string(iter.first);
            p.drawText(rect, Qt::AlignTop, tr(title.c_str()));
        }
    }

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

