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

#include <QGraphicsOpacityEffect>
#include <QPainter>

static const QColor kBackGroundColor = QColor(32, 39, 43);
static const QColor kMultiDisplayColor = QColor(41, 50, 55);
static const QColor kMultiDisplayBorderColor = QColor(123, 126, 131);
static const QColor kHighlightMultiDisplayBorderColor = QColor(80, 135, 236);
static const QColor kTextColor = QColor(168, 172, 174);
static const QColor kHighlightTextColor = QColor(245, 245, 245);

MultiDisplayArrangement::MultiDisplayArrangement(QWidget* parent)
    : QWidget(parent) {
}

void MultiDisplayArrangement::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.fillRect(this->rect(), kBackGroundColor);

    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    QRectF rect = QRectF(10, 10, 100, 50);
    path.addRoundedRect(rect, 5, 5);
    QPen pen(kMultiDisplayBorderColor, 1);
    p.setPen(pen);
    p.fillPath(path, kMultiDisplayColor);
    p.drawPath(path);

    pen.setColor(kHighlightTextColor);
    p.setPen(pen);
    p.drawText(rect, Qt::AlignCenter, tr("middle one"));

    pen.setColor(kTextColor);
    p.setPen(pen);
    p.drawText(QRectF(10, 10, 40, 16), Qt::AlignCenter, tr("default"));
    p.drawText(QRectF(0, height() - 80, width(), 80), Qt::AlignCenter,
               tr("This is just a simulation. You need to apply\n"
                  "changes to see new displays on the Emulator\n"
                  "window."));
}
