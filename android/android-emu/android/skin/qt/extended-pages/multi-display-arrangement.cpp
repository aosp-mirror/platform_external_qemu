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

static const QColor kBackGroundColor = QColor(32, 39, 43);
static const QColor kMultiDisplayColor = QColor(41, 50, 55);
static const QColor kMultiDisplayBorderColor = QColor(123, 126, 131);
static const QColor kHighlightMultiDisplayBorderColor = QColor(80, 135, 236);
static const QColor kTextColor = QColor(168, 172, 174);
static const QColor kHighlightTextColor = QColor(245, 245, 245);
static const int kBorderX = 10;
static const int kBorderY = 50;
static const int kTextHeight = 80;

MultiDisplayArrangement::MultiDisplayArrangement(QWidget* parent)
    : QWidget(parent) {
}

void MultiDisplayArrangement::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.fillRect(this->rect(), kBackGroundColor);

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
        QPen pen(kMultiDisplayBorderColor, 1);
        if (iter.first == mHighLightDisplayId) {
            pen.setColor(kHighlightMultiDisplayBorderColor);
        }
        p.setPen(pen);
        p.fillPath(path, kMultiDisplayColor);
        p.drawPath(path);

        pen.setColor(kHighlightTextColor);
        p.setPen(pen);
        p.drawText(rect, Qt::AlignCenter, tr(iter.second.name.c_str()));

        if (iter.first == 0) {
            p.drawText(rect, Qt::AlignTop, tr("Default"));
        } else {
            std::string title = "Display " + std::to_string(iter.first);
            p.drawText(rect, Qt::AlignTop, tr(title.c_str()));
        }
    }

    QPen penText(kTextColor, 1);
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

